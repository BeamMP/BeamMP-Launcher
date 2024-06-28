// Copyright (c) 2019-present Anonymous275.
// BeamMP Launcher code is not in the public domain and is not free software.
// One must be granted explicit permission by the copyright holder in order to modify or distribute any part of the source or binaries.
// Anything else is prohibited. Modified works may not be published and have be upstreamed to the official repository.
///
/// Created by Anonymous275 on 7/16/2020
///

#include "zip_file.h"
#include <charconv>
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <string>
#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <unistd.h>
#endif
#include "Http.h"
#include "Logger.h"
#include "Network/network.hpp"
#include "Security/Init.h"
#include "Startup.h"
#include "hashpp.h"
#include <filesystem>
#include <fstream>
#include <thread>

extern int TraceBack;
bool Dev = false;
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

std::string GetEN() {
#if defined(_WIN32)
    return "BeamMP-Launcher.exe";
#elif defined(__linux__)
    return "BeamMP-Launcher";
#endif
}

std::string GetVer() {
    return "2.1";
}
std::string GetPatch() {
    return ".0";
}

std::string GetEP(char* P) {
    static std::string Ret = [&]() {
        std::string path(P);
        return path.substr(0, path.find_last_of("\\/") + 1);
    }();
    return Ret;
}
#if defined(_WIN32)
void ReLaunch(int argc, char* args[]) {
    std::string Arg;
    for (int c = 2; c <= argc; c++) {
        Arg += " ";
        Arg += args[c - 1];
    }
    system("cls");
    ShellExecute(nullptr, "runas", (GetEP() + GetEN()).c_str(), Arg.c_str(), nullptr, SW_SHOWNORMAL);
    ShowWindow(GetConsoleWindow(), 0);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    exit(1);
}
void URelaunch(int argc, char* args[]) {
    std::string Arg;
    for (int c = 2; c <= argc; c++) {
        Arg += " ";
        Arg += args[c - 1];
    }
    ShellExecute(nullptr, "open", (GetEP() + GetEN()).c_str(), Arg.c_str(), nullptr, SW_SHOWNORMAL);
    ShowWindow(GetConsoleWindow(), 0);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    exit(1);
}
#elif defined(__linux__)
void ReLaunch(int argc, char* args[]) {
    std::string Arg;
    for (int c = 2; c <= argc; c++) {
        Arg += " ";
        Arg += args[c - 1];
    }
    system("clear");
    execl((GetEP() + GetEN()).c_str(), Arg.c_str(), NULL);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    exit(1);
}
void URelaunch(int argc, char* args[]) {
    std::string Arg;
    for (int c = 2; c <= argc; c++) {
        Arg += " ";
        Arg += args[c - 1];
    }
    execl((GetEP() + GetEN()).c_str(), Arg.c_str(), NULL);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    exit(1);
}
#endif

void CheckName(int argc, char* args[]) {
#if defined(_WIN32)
    std::string DN = GetEN(), CDir = args[0], FN = CDir.substr(CDir.find_last_of('\\') + 1);
#elif defined(__linux__)
    std::string DN = GetEN(), CDir = args[0], FN = CDir.substr(CDir.find_last_of('/') + 1);
#endif
    if (FN != DN) {
        if (fs::exists(DN))
            remove(DN.c_str());
        if (fs::exists(DN))
            ReLaunch(argc, args);
        std::rename(FN.c_str(), DN.c_str());
        URelaunch(argc, args);
    }
}

void CheckForUpdates(int argc, char* args[], const std::string& CV) {
    std::string LatestHash = HTTP::Get("https://backend.beammp.com/sha/launcher?branch=" + Branch + "&pk=" + PublicKey);
    std::string LatestVersion = HTTP::Get(
        "https://backend.beammp.com/version/launcher?branch=" + Branch + "&pk=" + PublicKey);

    transform(LatestHash.begin(), LatestHash.end(), LatestHash.begin(), ::tolower);
    std::string EP(GetEP() + GetEN()), Back(GetEP() + "BeamMP-Launcher.back");

    std::string FileHash = hashpp::get::getFileHash(hashpp::ALGORITHMS::SHA2_256, EP);
#if defined(_WIN32)
#elif defined(__linux__)
    system("clear");
#endif

    if (FileHash != LatestHash && IsOutdated(Version(VersionStrToInts(GetVer() + GetPatch())), Version(VersionStrToInts(LatestVersion)))) {
        info("Launcher update found!");
#if defined(__linux__)
        error("Auto update is NOT implemented for the Linux version. Please update manually ASAP as updates contain security patches.");
#else
        fs::remove(Back);
        fs::rename(EP, Back);
        info("Downloading Launcher update " + LatestHash);
        HTTP::Download(
            "https://backend.beammp.com/builds/launcher?download=true"
            "&pk="
                + PublicKey + "&branch=" + Branch,
            EP);
        URelaunch(argc, args);
#endif
    } else
        info("Launcher version is up to date");
    TraceBack++;
}

void CustomPort(int argc, char* argv[]) {
    if (argc > 1) {
        std::string Port = argv[1];
        if (Port.find_first_not_of("0123456789") == std::string::npos) {
            if (std::stoi(Port) > 1000) {
                DEFAULT_PORT = std::stoi(Port);
                warn("Running on custom port : " + std::to_string(DEFAULT_PORT));
            }
        }
        if (argc > 2)
            Dev = true;
    }
    for (int i = 1; i < argc; ++i) {
        if (std::string_view(argv[i]) == "--dev") {
            Dev = true;
        } else if (std::string_view(argv[i]) == "--no-dev") {
            Dev = false;
        }
    }
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
void InitLauncher(int argc, char* argv[]) {
    system("cls");
    SetConsoleTitleA(("BeamMP Launcher v" + std::string(GetVer()) + GetPatch()).c_str());
    InitLog();
    CheckName(argc, argv);
    LinuxPatch();
    CheckLocalKey();
    ConfigInit();
    CustomPort(argc, argv);
    CheckForUpdates(argc, argv, std::string(GetVer()) + GetPatch());
}
#elif defined(__linux__)
void InitLauncher(int argc, char* argv[]) {
    system("clear");
    InitLog();
    CheckName(argc, argv);
    CheckLocalKey();
    ConfigInit();
    CustomPort(argc, argv);
    bool update = true;
    for (int i = 1; i < argc; ++i) {
        if (std::string_view(argv[i]) == "--no-update") {
            update = false;
        }
    }
    if (update) {
        CheckForUpdates(argc, argv, std::string(GetVer()) + GetPatch());
    }
}
#endif

size_t DirCount(const std::filesystem::path& path) {
    return (size_t)std::distance(std::filesystem::directory_iterator { path }, std::filesystem::directory_iterator {});
}

void CheckMP(const std::string& Path) {
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
    std::string File(GetGamePath() + "mods/db.json");
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
                error("Failed to write " + File);
            }
        }
    }
}

void PreGame(const std::string& GamePath) {
    std::string GameVer = CheckVer(GamePath);
    info("Game Version : " + GameVer);

    CheckMP(GetGamePath() + "mods/multiplayer");

    info("Game user path: '" + GetGamePath() + "'");

    if (!Dev) {
        std::string LatestHash = HTTP::Get("https://backend.beammp.com/sha/mod?branch=" + Branch + "&pk=" + PublicKey);
        transform(LatestHash.begin(), LatestHash.end(), LatestHash.begin(), ::tolower);
        LatestHash.erase(std::remove_if(LatestHash.begin(), LatestHash.end(),
                             [](auto const& c) -> bool { return !std::isalnum(c); }),
            LatestHash.end());

        try {
            if (!fs::exists(GetGamePath() + "mods/multiplayer")) {
                fs::create_directories(GetGamePath() + "mods/multiplayer");
            }
            EnableMP();
        } catch (std::exception& e) {
            fatal(e.what());
        }
#if defined(_WIN32)
        std::string ZipPath(GetGamePath() + R"(mods\multiplayer\BeamMP.zip)");
#elif defined(__linux__)
        // Linux version of the game cant handle mods with uppercase names
        std::string ZipPath(GetGamePath() + R"(mods/multiplayer/beammp.zip)");
#endif

        std::string FileHash = hashpp::get::getFileHash(hashpp::ALGORITHMS::SHA2_256, ZipPath);

        if (FileHash != LatestHash) {
            info("Downloading BeamMP Update " + LatestHash);
            HTTP::Download("https://backend.beammp.com/builds/client?download=true"
                           "&pk="
                    + PublicKey + "&branch=" + Branch,
                ZipPath);
        }

        std::string Target(GetGamePath() + "mods/unpacked/beammp");

        if (fs::is_directory(Target)) {
            fs::remove_all(Target);
        }
    }
}

void set_headers(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Request-Method", "POST, OPTIONS, GET");
    res.set_header("Access-Control-Request-Headers", "X-API-Version");
}

void StartProxy() {
    std::thread proxy([&]() {
        httplib::Server HTTPProxy;
        httplib::Headers headers = {
            { "User-Agent", "BeamMP-Launcher/" + GetVer() + GetPatch() },
            { "Accept", "*/*" }
        };
        std::string pattern = "/:any1";
        for (int i = 2; i <= 4; i++) {
            HTTPProxy.Get(pattern, [&](const httplib::Request& req, httplib::Response& res) {
                httplib::Client cli("https://backend.beammp.com");
                set_headers(res);
                if (req.has_header("X-BMP-Authentication")) {
                    headers.emplace("X-BMP-Authentication", PrivateKey);
                }
                if (req.has_header("X-API-Version")) {
                    headers.emplace("X-API-Version", req.get_header_value("X-API-Version"));
                }
                if (auto cli_res = cli.Get(req.path, headers); cli_res) {
                    res.set_content(cli_res->body, cli_res->get_header_value("Content-Type"));
                } else {
                    res.set_content(to_string(cli_res.error()), "text/plain");
                }
            });

            HTTPProxy.Post(pattern, [&](const httplib::Request& req, httplib::Response& res) {
                httplib::Client cli("https://backend.beammp.com");
                set_headers(res);
                if (req.has_header("X-BMP-Authentication")) {
                    headers.emplace("X-BMP-Authentication", PrivateKey);
                }
                if (req.has_header("X-API-Version")) {
                    headers.emplace("X-API-Version", req.get_header_value("X-API-Version"));
                }
                if (auto cli_res = cli.Post(req.path, headers, req.body,
                        req.get_header_value("Content-Type"));
                    cli_res) {
                    res.set_content(cli_res->body, cli_res->get_header_value("Content-Type"));
                } else {
                    res.set_content(to_string(cli_res.error()), "text/plain");
                }
            });
            pattern += "/:any" + std::to_string(i);
        }
        ProxyPort = HTTPProxy.bind_to_any_port("0.0.0.0");
        HTTPProxy.listen_after_bind();
    });
    proxy.detach();
}
