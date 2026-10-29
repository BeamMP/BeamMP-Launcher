/*
 Copyright (C) 2026 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/

#include "Jobs/VehicleFiles.h"
#include "Logger.h"

#include "Jobs/ZipReader.h"

#include <algorithm>
#include <fstream>
#include <map>
#include <mutex>
#include <nlohmann/json.hpp>
#include <set>
#include <sstream>

namespace fs = std::filesystem;

namespace Jobs {

namespace {
    // UTF-8 string -> path (std::filesystem::u8path is deprecated in C++20)
    fs::path Utf8Path(const std::string& s) { return fs::path(std::u8string(s.begin(), s.end())); }
}

std::string NormalizeGamePath(std::string p) {
    std::replace(p.begin(), p.end(), '\\', '/');
    std::transform(p.begin(), p.end(), p.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    if (p.empty() || p[0] != '/')
        p.insert(p.begin(), '/');
    return p;
}

namespace {
    struct Source {
        fs::path path; // zip file, or the folder that maps to the game root
        bool isZip;
    };
    struct Entry {
        size_t source;
        std::string inner; // name inside the zip, or the path relative to the source folder
        uint64_t size;
        int64_t time;
        // zip entries only
        uint64_t compressedSize = 0, offset = 0;
        uint16_t method = 0;
    };

    int64_t FileTime(const fs::path& p) {
        std::error_code ec;
        auto t = fs::last_write_time(p, ec);
        return ec ? 0 : int64_t(t.time_since_epoch().count());
    }

    std::string PathString(const fs::path& p) {
        auto u8 = p.u8string();
        return std::string(u8.begin(), u8.end());
    }

    // mods switched off in the game's mod manager (mods/db.json), by their "fullpath" ("/mods/repo/x.zip")
    std::set<std::string> InactiveMods(const fs::path& userDir) {
        std::set<std::string> out;
        std::ifstream f(userDir / "mods" / "db.json", std::ios::binary);
        if (!f)
            return out;
        std::stringstream ss;
        ss << f.rdbuf();
        auto db = nlohmann::json::parse(ss.str(), nullptr, false);
        if (!db.is_object() || !db.contains("mods") || !db["mods"].is_object())
            return out;
        for (auto& [name, mod] : db["mods"].items()) {
            if (mod.is_object() && mod.value("active", true) == false && mod.contains("fullpath") && mod["fullpath"].is_string())
                out.insert(NormalizeGamePath(mod["fullpath"].get<std::string>()));
        }
        return out;
    }

    // the zips and folders the game mounts, lowest priority first
    std::vector<Source> FindSources(const fs::path& gameDir, const fs::path& userDir) {
        std::vector<Source> out;
        std::error_code ec;
        auto sorted = [](std::vector<fs::path> v) { std::sort(v.begin(), v.end()); return v; };

        std::vector<fs::path> content;
        for (auto& e : fs::directory_iterator(gameDir / "content" / "vehicles", ec))
            if (e.is_regular_file() && e.path().extension() == ".zip")
                content.push_back(e.path());
        for (auto& p : sorted(content))
            out.push_back({ p, true });

        auto inactive = InactiveMods(userDir);
        std::vector<fs::path> mods;
        const auto modsDir = userDir / "mods";
        for (auto it = fs::recursive_directory_iterator(modsDir, fs::directory_options::skip_permission_denied, ec);
             it != fs::recursive_directory_iterator(); it.increment(ec)) {
            if (ec)
                break;
            if (it->is_directory() && it->path().filename() == "unpacked") {
                it.disable_recursion_pending();
                continue;
            }
            if (!it->is_regular_file() || it->path().extension() != ".zip")
                continue;
            auto rel = fs::relative(it->path(), userDir, ec);
            if (!ec && inactive.count(NormalizeGamePath(PathString(rel))))
                continue;
            mods.push_back(it->path());
        }
        for (auto& p : sorted(mods))
            out.push_back({ p, true });

        std::vector<fs::path> unpacked;
        for (auto& e : fs::directory_iterator(modsDir / "unpacked", ec))
            if (e.is_directory())
                unpacked.push_back(e.path());
        for (auto& p : sorted(unpacked)) {
            auto rel = fs::relative(p, userDir, ec);
            if (!ec && inactive.count(NormalizeGamePath(PathString(rel))))
                continue;
            out.push_back({ p, false });
        }

        out.push_back({ userDir, false }); // loose files in the user folder win
        return out;
    }

    std::string SourcesSignature(const std::vector<Source>& sources) {
        std::string sig;
        std::error_code ec;
        for (auto& s : sources) {
            sig += PathString(s.path);
            if (s.isZip) {
                sig += ":" + std::to_string(fs::file_size(s.path, ec)) + ":" + std::to_string(FileTime(s.path));
            } else {
                uint64_t n = 0, total = 0;
                int64_t newest = 0;
                for (auto it = fs::recursive_directory_iterator(s.path / "vehicles", fs::directory_options::skip_permission_denied, ec);
                     it != fs::recursive_directory_iterator(); it.increment(ec)) {
                    if (ec)
                        break;
                    if (it->is_regular_file()) {
                        n++;
                        total += it->file_size(ec);
                        newest = std::max(newest, FileTime(it->path()));
                    }
                }
                sig += ":" + std::to_string(n) + ":" + std::to_string(total) + ":" + std::to_string(newest);
            }
            sig += "|";
        }
        return sig;
    }
}

struct VehicleFiles::Impl {
    std::vector<Source> sources;
    std::map<std::string, Entry> files; // normalized game path -> where it comes from
    std::string sourcesSig;

    void AddZip(size_t idx) {
        ZipReader zip(sources[idx].path);
        if (!zip.IsOpen()) {
            warn("(Jobs) Could not open " + PathString(sources[idx].path));
            return;
        }
        const auto zipTime = FileTime(sources[idx].path);
        zip.ForEach([&](const ZipReader::Entry& z) {
            auto p = NormalizeGamePath(z.name);
            if (p.rfind("/vehicles/", 0) != 0)
                return;
            files[p] = Entry { idx, z.name, z.size, zipTime, z.compressedSize, z.localHeaderOffset, z.method };
        });
    }

    void AddFolder(size_t idx) {
        std::error_code ec;
        const auto& root = sources[idx].path;
        for (auto it = fs::recursive_directory_iterator(root / "vehicles", fs::directory_options::skip_permission_denied, ec);
             it != fs::recursive_directory_iterator(); it.increment(ec)) {
            if (ec)
                break;
            if (!it->is_regular_file())
                continue;
            auto rel = fs::relative(it->path(), root, ec);
            if (ec)
                continue;
            files[NormalizeGamePath(PathString(rel))] = Entry { idx, PathString(rel), uint64_t(it->file_size(ec)), FileTime(it->path()) };
        }
    }
};

VehicleFiles::VehicleFiles(fs::path gameDir, fs::path userDir)
    : mImpl(std::make_unique<Impl>())
    , mUserDir(std::move(userDir)) {
    mImpl->sources = FindSources(gameDir, mUserDir);
    mImpl->sourcesSig = SourcesSignature(mImpl->sources);
    for (size_t i = 0; i < mImpl->sources.size(); i++) {
        if (mImpl->sources[i].isZip)
            mImpl->AddZip(i);
        else
            mImpl->AddFolder(i);
    }
    debug("(Jobs) Indexed " + std::to_string(mImpl->files.size()) + " vehicle files from " + std::to_string(mImpl->sources.size()) + " sources");
}

VehicleFiles::~VehicleFiles() = default;

std::vector<std::string> VehicleFiles::List(const std::string& dir, const std::string& suffix) const {
    std::vector<std::string> out;
    auto d = NormalizeGamePath(dir);
    for (auto it = mImpl->files.lower_bound(d); it != mImpl->files.end() && it->first.compare(0, d.size(), d) == 0; ++it) {
        const auto& p = it->first;
        if (p.size() >= suffix.size() && p.compare(p.size() - suffix.size(), suffix.size(), suffix) == 0)
            out.push_back(p);
    }
    return out;
}

bool VehicleFiles::Exists(const std::string& path) const {
    return mImpl->files.count(NormalizeGamePath(path)) > 0;
}

std::optional<std::string> VehicleFiles::Read(const std::string& path) const {
    auto it = mImpl->files.find(NormalizeGamePath(path));
    if (it == mImpl->files.end())
        return std::nullopt;
    const auto& e = it->second;
    const auto& src = mImpl->sources[e.source];
    if (!src.isZip) {
        std::ifstream f(src.path / Utf8Path(e.inner), std::ios::binary);
        if (!f)
            return std::nullopt;
        std::string data(size_t(e.size), '\0');
        f.read(data.data(), std::streamsize(data.size()));
        data.resize(size_t(f.gcount()));
        return data;
    }
    // zips are opened per read (one seek to the entry, no central directory read), which keeps the worker
    // threads independent of each other
    ZipReader zip(src.path);
    return zip.Read({ e.inner, e.compressedSize, e.size, e.offset, e.method });
}

std::string VehicleFiles::Signature(const std::string& dir) const {
    uint64_t n = 0, total = 0;
    int64_t newest = 0;
    for (const char* ext : { ".jbeam", ".dae", ".json", ".pc" }) {
        for (auto& p : List(dir, ext)) {
            const auto& e = mImpl->files.at(p);
            n++;
            total += e.size;
            newest = std::max(newest, e.time);
        }
    }
    return std::to_string(n) + "|" + std::to_string(total) + "|" + std::to_string(newest);
}

size_t VehicleFiles::FileCount() const { return mImpl->files.size(); }

std::shared_ptr<VehicleFiles> VehicleFiles::Get(const fs::path& gameDir, const fs::path& userDir) {
    static std::mutex m;
    static std::shared_ptr<VehicleFiles> current;
    static fs::path curGame, curUser;
    std::lock_guard lock(m);
    if (current && curGame == gameDir && curUser == userDir
        && SourcesSignature(FindSources(gameDir, userDir)) == current->mImpl->sourcesSig)
        return current;
    current = std::make_shared<VehicleFiles>(gameDir, userDir);
    curGame = gameDir;
    curUser = userDir;
    return current;
}

}
