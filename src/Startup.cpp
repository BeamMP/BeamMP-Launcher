/*
 Copyright (C) 2024 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/


#include "zip_file.h"
#include <charconv>
#include <cstring>
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <string>
#if defined(_WIN32)
#elif defined(__linux__)
#include <unistd.h>
#elif defined (__APPLE__)
#include <unistd.h>
#include <libproc.h>
#endif // __APPLE__
#include "Http.h"
#include "Logger.h"
#include "Network/network.hpp"
#include "Options.h"
#include "Security/Init.h"
#include "Startup.h"
#include "Utils.h"
#include "hashpp.h"
#include <filesystem>
#include <fstream>
#include <thread>

extern int TraceBack;
int ProxyPort = 0;

namespace fs = std::filesystem;

struct Version {
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
    Version(uint8_t major, uint8_t minor, uint8_t patch);
    Version(const std::array<uint8_t, 3>& v);
};

std::array<uint8_t, 3> VersionStrToInts(const std::string& str) {
    std::array<uint8_t, 3> Version;
    std::stringstream ss(str);
    for (uint8_t& i : Version) {
        std::string Part;
        std::getline(ss, Part, '.');
        std::from_chars(&*Part.begin(), &*Part.begin() + Part.size(), i);
    }
    return Version;
}

bool IsOutdated(const Version& Current, const Version& Newest) {
    if (Newest.major > Current.major) {
        return true;
    } else if (Newest.major == Current.major && Newest.minor > Current.minor) {
        return true;
    } else if (Newest.major == Current.major && Newest.minor == Current.minor && Newest.patch > Current.patch) {
        return true;
    } else {
        return false;
    }
}

Version::Version(uint8_t major, uint8_t minor, uint8_t patch)
    : major(major)
    , minor(minor)
    , patch(patch) { }

Version::Version(const std::array<uint8_t, 3>& v)
    : Version(v[0], v[1], v[2]) {
}

beammp_fs_string GetEN() {
#if defined(_WIN32)
    return L"BeamMP-Launcher.exe";
#elif defined(__linux__)
    return "BeamMP-Launcher";
#endif
}

std::string GetVer() {
    return "2.5";
}
std::string GetPatch() {
    return ".1";
}

beammp_fs_string GetEP(const beammp_fs_char* P) {
    static beammp_fs_string Ret = [&]() {
        beammp_fs_string path(P);
        return path.substr(0, path.find_last_of(beammp_wide("\\/")) + 1);
    }();
    return Ret;
}

fs::path GetBP(const beammp_fs_char* P) {
    fs::path fspath = {};
#if defined(_WIN32)
    beammp_fs_char path[256];
    GetModuleFileNameW(nullptr, path, sizeof(path));
    fspath = path;
#elif defined(__linux__)
    fspath = fs::canonical("/proc/self/exe");
#elif defined(__APPLE__)
    pid_t pid = getpid();
    char path[PROC_PIDPATHINFO_MAXSIZE];
    // While this is fine for a raw executable,
    // an application bundle is read-only and these files
    // should instead be placed in Application Support.
    proc_pidpath(pid, path, sizeof(path));
    fspath = std::string(path);
 #else
    fspath = beammp_fs_string(P);
#endif
    fspath = fs::weakly_canonical(fspath.string() + "/..");
#if defined(_WIN32)
    return fspath.wstring();
#else
    return fspath.string();
#endif
}

#if defined(_WIN32)
void ReLaunch() {
    std::wstring Arg;
    for (int c = 2; c <= options.argc; c++) {
        Arg += Utils::ToWString(options.argv[c - 1]);
        Arg += L" ";
    }
    info("Relaunch!");
    system("cls");
    ShellExecuteW(nullptr, L"runas", (GetBP() / GetEN()).c_str(), Arg.c_str(), nullptr, SW_SHOWNORMAL);
    ShowWindow(GetConsoleWindow(), 0);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    exit(1);
}
void URelaunch() {
    std::wstring Arg;
    for (int c = 2; c <= options.argc; c++) {
        Arg += Utils::ToWString(options.argv[c - 1]);
        Arg += L" ";
    }
    ShellExecuteW(nullptr, L"open", (GetBP() / GetEN()).c_str(), Arg.c_str(), nullptr, SW_SHOWNORMAL);
    ShowWindow(GetConsoleWindow(), 0);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    exit(1);
}
#elif defined(__linux__)
void ReLaunch() {
    std::string Arg;
    for (int c = 2; c <= options.argc; c++) {
        Arg += options.argv[c - 1];
        Arg += " ";
    }
    info("Relaunch!");
    system("clear");
    int ret = execv((GetBP() / GetEN()).c_str(), const_cast<char**>(options.argv));
    if (ret < 0) {
        error(std::string("execv() failed with: ") + strerror(errno) + ". Failed to relaunch");
        exit(1);
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    exit(1);
}
void URelaunch() {
    int ret = execv((GetBP() / GetEN()).c_str(), const_cast<char**>(options.argv));
    if (ret < 0) {
        error(std::string("execv() failed with: ") + strerror(errno) + ". Failed to relaunch");
        exit(1);
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    exit(1);
}
#endif

void CheckName() {
#if defined(_WIN32)
    std::wstring DN = GetEN(), CDir = Utils::ToWString(options.executable_name), FN = CDir.substr(CDir.find_last_of('\\') + 1);
#elif defined(__linux__)
    std::string DN = GetEN(), CDir = options.executable_name, FN = CDir.substr(CDir.find_last_of('/') + 1);
#endif
    if (FN != DN) {
        if (fs::exists(DN))
            fs::remove(DN.c_str());
        if (fs::exists(DN))
            ReLaunch();
        fs::rename(FN.c_str(), DN.c_str());
        URelaunch();
    }
}

void CheckForUpdates(const std::string& CV) {
    std::string LatestHash = HTTP::Get("https://backend.beammp.com/sha/launcher?branch=" + Branch + "&pk=" + PublicKey);
    std::string LatestVersion = HTTP::Get(
        "https://backend.beammp.com/version/launcher?branch=" + Branch + "&pk=" + PublicKey);

    transform(LatestHash.begin(), LatestHash.end(), LatestHash.begin(), ::tolower);
    beammp_fs_string BP(GetBP() / GetEN()), Back(GetBP() / beammp_wide("BeamMP-Launcher.back"));

    std::string FileHash = Utils::GetSha256HashReallyFastFile(BP);

    if (FileHash != LatestHash && IsOutdated(Version(VersionStrToInts(GetVer() + GetPatch())), Version(VersionStrToInts(LatestVersion)))) {
        if (!options.no_update) {
            info("Launcher update " + LatestVersion + " found!");
#if defined(__linux__)
            error("Auto update is NOT implemented for the Linux version. Please update manually ASAP as updates contain security patches.");
#else
            info("Downloading Launcher update " + LatestHash);
            HTTP::Download(
                "https://backend.beammp.com/builds/launcher?download=true"
                "&pk="
                    + PublicKey + "&branch=" + Branch,
                beammp_wide("new_") + BP, LatestHash);
            std::error_code ec;
            fs::remove(Back, ec);
            if (ec == std::errc::permission_denied) {
                error("Failed to remove old backup file: " + ec.message() + ". Using alternative name.");
                fs::rename(BP, Back + beammp_wide(".") + Utils::ToWString(FileHash.substr(0, 8)));
            } else {
                fs::rename(BP, Back);
            }
            fs::rename(beammp_wide("new_") + BP, BP);
            URelaunch();
#endif
        } else {
            warn("Launcher update was found, but not updating because --no-update or --dev was specified.");
        }
    } else
        info("Launcher version is up to date. Latest version: " + LatestVersion);
    TraceBack++;
}


#ifdef _WIN32
void LinuxPatch() {
    HKEY hKey = nullptr;
    LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, R"(Software\Wine)", 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS || getenv("USER") == nullptr)
        return;
    RegCloseKey(hKey);
    info("Wine/Proton Detected! If you are on windows delete HKEY_CURRENT_USER\\Software\\Wine in regedit");
    info("Applying patches...");

    result = RegCreateKey(HKEY_CURRENT_USER, R"(Software\Valve\Steam\Apps\284160)", &hKey);

    if (result != ERROR_SUCCESS) {
        fatal(R"(failed to create HKEY_CURRENT_USER\Software\Valve\Steam\Apps\284160)");
        return;
    }

    result = RegSetValueEx(hKey, "Name", 0, REG_SZ, (BYTE*)"BeamNG.drive", 12);

    if (result != ERROR_SUCCESS) {
        fatal(R"(failed to create the value "Name" under HKEY_CURRENT_USER\Software\Valve\Steam\Apps\284160)");
        return;
    }
    RegCloseKey(hKey);

    info("Patched!");
}
#endif

#if defined(_WIN32)

void InitLauncher() {
    SetConsoleTitleA(("BeamMP Launcher v" + std::string(GetVer()) + GetPatch()).c_str());
    debug("Launcher Version : " + GetVer() + GetPatch());
    CheckName();
    LinuxPatch();
    CheckLocalKey();
    CheckForUpdates(std::string(GetVer()) + GetPatch());
}
#elif defined(__linux__)

void InitLauncher() {
    info("BeamMP Launcher v" + GetVer() + GetPatch());
    CheckName();
    CheckLocalKey();
    CheckForUpdates(std::string(GetVer()) + GetPatch());
}
#endif

size_t DirCount(const fs::path& path) {
    return (size_t)std::distance(fs::directory_iterator { path }, fs::directory_iterator {});
}

void CheckMP(const beammp_fs_string& Path) {
    if (!fs::exists(Path))
        return;
    size_t c = DirCount(fs::path(Path));
    try {
        for (auto& p : fs::directory_iterator(Path)) {
            if (p.exists() && !p.is_directory()) {
                std::string Name = p.path().filename().string();
                for (char& Ch : Name)
                    Ch = char(tolower(Ch));
                if (Name != "beammp.zip")
                    fs::remove(p.path());
            }
        }
    } catch (...) {
        fatal("We were unable to clean the multiplayer mods folder! Is the game still running or do you have something open in that folder?");
    }
}

void EnableMP() {
    beammp_fs_string File(GetGamePath() / beammp_wide("mods/db.json"));
    if (!fs::exists(File))
        return;
    auto Size = fs::file_size(File);
    if (Size < 2)
        return;
    std::ifstream db(File);
    if (db.is_open()) {
        std::string Data(Size, 0);
        db.read(&Data[0], Size);
        db.close();
        nlohmann::json d = nlohmann::json::parse(Data, nullptr, false);
        if (Data.at(0) != '{' || d.is_discarded()) {
            // error("Failed to parse " + File); //TODO illegal formatting
            return;
        }
        if (d.contains("mods") && d["mods"].contains("multiplayerbeammp")) {
            d["mods"]["multiplayerbeammp"]["active"] = true;
            std::ofstream ofs(File);
            if (ofs.is_open()) {
                ofs << d.dump();
                ofs.close();
            } else {
                error(beammp_wide("Failed to write ") + File);
            }
        }
    }
}

void PreGame(const beammp_fs_string& GamePath) {
    std::string GameVer = CheckVer(GamePath);
    info("Game Version : " + GameVer);

    CheckMP(GetGamePath() / beammp_wide("mods/multiplayer"));
    info(beammp_wide("Game user path: ") + beammp_fs_string(GetGamePath()));

    if (!options.no_download) {
        std::string LatestHash = HTTP::Get("https://backend.beammp.com/sha/mod?branch=" + Branch + "&pk=" + PublicKey);
        transform(LatestHash.begin(), LatestHash.end(), LatestHash.begin(), ::tolower);
        LatestHash.erase(std::remove_if(LatestHash.begin(), LatestHash.end(),
                             [](auto const& c) -> bool { return !std::isalnum(c); }),
            LatestHash.end());

        try {
            if (!fs::exists(GetGamePath() / beammp_wide("mods/multiplayer"))) {
                fs::create_directories(GetGamePath() / beammp_wide("mods/multiplayer"));
            }
            EnableMP();
        } catch (std::exception& e) {
            fatal(e.what());
        }
#if defined(_WIN32)
        std::wstring ZipPath(GetGamePath() / LR"(mods\multiplayer\BeamMP.zip)");
#elif defined(__linux__)
        // Linux version of the game cant handle mods with uppercase names
        std::string ZipPath(GetGamePath() / R"(mods/multiplayer/beammp.zip)");
#endif

        std::string FileHash = fs::exists(ZipPath) ? Utils::GetSha256HashReallyFastFile(ZipPath) : "";

        if (FileHash != LatestHash) {
            info("Downloading BeamMP Update " + LatestHash);
            HTTP::Download("https://backend.beammp.com/builds/client?download=true"
                           "&pk="
                    + PublicKey + "&branch=" + Branch,
                ZipPath, LatestHash);
        }

        beammp_fs_string Target(GetGamePath() / beammp_wide("mods/unpacked/beammp"));

        if (fs::is_directory(Target) && !fs::is_directory(Target + beammp_wide("/.git"))) {
            fs::remove_all(Target);
        }
    }
}
