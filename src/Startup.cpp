// Copyright (c) 2019-present Anonymous275.
// BeamMP Launcher code is not in the public domain and is not free software.
// One must be granted explicit permission by the copyright holder in order to modify or distribute any part of the source or binaries.
// Anything else is prohibited. Modified works may not be published and have be upstreamed to the official repository.
///
/// Created by Anonymous275 on 7/16/2020
///

#include <nlohmann/json.hpp>
#include <httplib.h>
#include "zip_file.h"
#include <windows.h>
#include "Discord/discord_info.h"
#include "Network/network.h"
#include "Security/Init.h"
#include <filesystem>
#include "Startup.h"
#include "hashpp.h"
#include "Logger.h"
#include <fstream>
#include <thread>
#include "Http.h"


extern int TraceBack;
bool Dev = false;
int ProxyPort = 0;

namespace fs = std::filesystem;

VersionParser::VersionParser(const std::string& from_string) {
    std::string token;
    std::istringstream tokenStream(from_string);
    while (std::getline(tokenStream, token, '.')) {
        data.emplace_back(std::stol(token));
        split.emplace_back(token);
    }
}

std::strong_ordering VersionParser::operator<=>(
        const VersionParser& rhs) const noexcept {
    size_t const fields = std::min(data.size(), rhs.data.size());
    for (size_t i = 0; i != fields; ++i) {
        if (data[i] == rhs.data[i]) continue;
        else if (data[i] < rhs.data[i]) return std::strong_ordering::less;
        else return std::strong_ordering::greater;
    }
    if (data.size() == rhs.data.size()) return std::strong_ordering::equal;
    else if (data.size() > rhs.data.size()) return std::strong_ordering::greater;
    else return std::strong_ordering::less;
}

bool VersionParser::operator==(const VersionParser& rhs) const noexcept {
    return std::is_eq(*this <=> rhs);
}


std::string GetEN(){
    return "BeamMP-Launcher.exe";
}
std::string GetVer(){
    return "2.0";
}
std::string GetPatch(){
    return ".84";
}

std::string GetEP(char*P){
    static std::string Ret = [&](){
        std::string path(P);
        return path.substr(0, path.find_last_of("\\/") + 1);
    } ();
    return Ret;
}
void ReLaunch(int argc,char*args[]){
    std::string Arg;
    for(int c = 2; c <= argc; c++){
        Arg += " ";
        Arg += args[c-1];
    }
    system("cls");
    ShellExecute(nullptr,"runas",(GetEP() + GetEN()).c_str(),Arg.c_str(),nullptr,SW_SHOWNORMAL);
    ShowWindow(GetConsoleWindow(),0);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    exit(1);
}
void URelaunch(int argc,char* args[]){
    std::string Arg;
    for(int c = 2; c <= argc; c++){
        Arg += " ";
        Arg += args[c-1];
    }
    ShellExecute(nullptr,"open",(GetEP() + GetEN()).c_str(),Arg.c_str(),nullptr,SW_SHOWNORMAL);
    ShowWindow(GetConsoleWindow(),0);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    exit(1);
}
void CheckName(int argc,char* args[]){
    std::string DN = GetEN(),CDir = args[0],FN = CDir.substr(CDir.find_last_of('\\')+1);
    if(FN != DN){
        if(fs::exists(DN))remove(DN.c_str());
        if(fs::exists(DN))ReLaunch(argc,args);
        std::rename(FN.c_str(), DN.c_str());
        URelaunch(argc,args);
    }
}

void CheckForUpdates(int argc, char* args[], const std::string& CV) {
    std::string LatestHash = HTTP::Get("https://backend.beammp.com/sha/launcher?branch=" + Branch + "&pk=" + PublicKey);
    std::string LatestVersion = HTTP::Get(
            "https://backend.beammp.com/version/launcher?branch=" + Branch + "&pk=" + PublicKey);

    transform(LatestHash.begin(), LatestHash.end(), LatestHash.begin(), ::tolower);
    std::string EP(GetEP() + GetEN()), Back(GetEP() + "BeamMP-Launcher.back");

    std::string FileHash = hashpp::get::getFileHash(hashpp::ALGORITHMS::SHA2_256, EP);

    if (FileHash != LatestHash && VersionParser(LatestVersion) > VersionParser(GetVer()+GetPatch())) {
        info("Launcher update found!");
        fs::remove(Back);
        fs::rename(EP, Back);
        info("Downloading Launcher update " + LatestHash);
        HTTP::Download(
                "https://backend.beammp.com/builds/launcher?download=true"
                "&pk=" +
                PublicKey + "&branch=" + Branch,
                EP);
        URelaunch(argc, args);
    } else info("Launcher version is up to date");
    TraceBack++;
}

void CustomPort(int argc, char* argv[]){
    if(argc > 1){
        std::string Port = argv[1];
        if(Port.find_first_not_of("0123456789") == std::string::npos){
            if(std::stoi(Port) > 1000){
                DEFAULT_PORT = std::stoi(Port);
                warn("Running on custom port : " + std::to_string(DEFAULT_PORT));
            }
        }
        if(argc > 2)Dev = true;
    }
}

void LinuxPatch(){
    HKEY hKey = nullptr;
    LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, R"(Software\Wine)", 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS || getenv("USER") == nullptr)return;
    RegCloseKey(hKey);
    info("Wine/Proton Detected! If you are on windows delete HKEY_CURRENT_USER\\Software\\Wine in regedit");
    info("Applying patches...");

    result = RegCreateKey(HKEY_CURRENT_USER, R"(Software\Valve\Steam\Apps\284160)", &hKey);

    if (result != ERROR_SUCCESS){
        fatal(R"(failed to create HKEY_CURRENT_USER\Software\Valve\Steam\Apps\284160)");
        return;
    }

    result = RegSetValueEx(hKey, "Name", 0, REG_SZ, (BYTE*)"BeamNG.drive", 12);

    if (result != ERROR_SUCCESS){
        fatal(R"(failed to create the value "Name" under HKEY_CURRENT_USER\Software\Valve\Steam\Apps\284160)");
        return;
    }
    RegCloseKey(hKey);

    info("Patched!");
}

void InitLauncher(int argc, char* argv[]) {
    system("cls");
    SetConsoleTitleA(("BeamMP Launcher v" + std::string(GetVer()) + GetPatch()).c_str());
    InitLog();
    CheckName(argc, argv);
    LinuxPatch();
    CheckLocalKey();
    ConfigInit();
    CustomPort(argc, argv);
    Discord_Main();
    CheckForUpdates(argc, argv, std::string(GetVer()) + GetPatch());
}

size_t DirCount(const std::filesystem::path& path){
    return (size_t)std::distance(std::filesystem::directory_iterator{path}, std::filesystem::directory_iterator{});
}

void CheckMP(const std::string& Path) {
    if (!fs::exists(Path))return;
    size_t c = DirCount(fs::path(Path));
    try {
        for (auto& p : fs::directory_iterator(Path)){
            if(p.exists() && !p.is_directory()){
                std::string Name = p.path().filename().string();
                for(char&Ch : Name)Ch = char(tolower(Ch));
                if(Name != "beammp.zip")fs::remove(p.path());
            }
        }
    } catch (...) {
        fatal("We were unable to clean the multiplayer mods folder! Is the game still running or do you have something open in that folder?");
    }

}

void EnableMP(){
    std::string File(GetGamePath() + "mods/db.json");
    if(!fs::exists(File))return;
    auto Size = fs::file_size(File);
    if(Size < 2)return;
    std::ifstream db(File);
    if(db.is_open()) {
        std::string Data(Size, 0);
        db.read(&Data[0], Size);
        db.close();
        nlohmann::json d = nlohmann::json::parse(Data, nullptr, false);
        if(Data.at(0) != '{' || d.is_discarded()) {
            //error("Failed to parse " + File); //TODO illegal formatting
            return;
        }
        if(d.contains("mods") && d["mods"].contains("multiplayerbeammp")){
            d["mods"]["multiplayerbeammp"]["active"] = true;
            std::ofstream ofs(File);
            if(ofs.is_open()){
                ofs << d.dump();
                ofs.close();
            }else{
                error("Failed to write " + File);
            }
        }
    }
}

void PreGame(const std::string& GamePath){
    std::string GameVer = CheckVer(GamePath);
    info("Game Version : " + GameVer);
    
    CheckMP(GetGamePath() + "mods/multiplayer");

    if(!Dev) {
        std::string LatestHash = HTTP::Get("https://backend.beammp.com/sha/mod?branch=" + Branch + "&pk=" + PublicKey);
        transform(LatestHash.begin(), LatestHash.end(), LatestHash.begin(), ::tolower);
        LatestHash.erase(std::remove_if(LatestHash.begin(), LatestHash.end(),
                               [](auto const& c ) -> bool { return !std::isalnum(c); } ), LatestHash.end());

        try {
            if (!fs::exists(GetGamePath() + "mods/multiplayer")) {
                fs::create_directories(GetGamePath() + "mods/multiplayer");
            }
            EnableMP();
        }catch(std::exception&e){
            fatal(e.what());
        }

        std::string ZipPath(GetGamePath() + R"(mods\multiplayer\BeamMP.zip)");

        std::string FileHash = hashpp::get::getFileHash(hashpp::ALGORITHMS::SHA2_256, ZipPath);

        if (FileHash != LatestHash) {
            info("Downloading BeamMP Update " + LatestHash);
            HTTP::Download("https://backend.beammp.com/builds/client?download=true"
                           "&pk=" + PublicKey + "&branch=" + Branch, ZipPath);
        }

        std::string Target(GetGamePath() + "mods/unpacked/beammp");

        if(fs::is_directory(Target)) {
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
    std::thread proxy([&](){
        httplib::Server HTTPProxy;
        httplib::Headers headers = {
                {"User-Agent", "BeamMP-Launcher/" + GetVer() + GetPatch()},
                {"Accept", "*/*"}
        };
        std::string pattern = "/:any1";
        for (int i = 2; i <= 4; i++) {
            HTTPProxy.Get(pattern, [&](const httplib::Request &req, httplib::Response &res) {
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

            HTTPProxy.Post(pattern, [&](const httplib::Request &req, httplib::Response &res) {
                httplib::Client cli("https://backend.beammp.com");
                set_headers(res);
                if (req.has_header("X-BMP-Authentication")) {
                    headers.emplace("X-BMP-Authentication", PrivateKey);
                }
                if (req.has_header("X-API-Version")) {
                    headers.emplace("X-API-Version", req.get_header_value("X-API-Version"));
                }
                if (auto cli_res = cli.Post(req.path, headers, req.body,
                                            req.get_header_value("Content-Type")); cli_res) {
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
