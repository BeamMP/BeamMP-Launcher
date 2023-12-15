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
#include "Logger.h"
#include <fstream>
#include <thread>
#include "Http.h"

extern int TraceBack;
bool Dev = false;
int ProxyPort = 0;

namespace fs = std::filesystem;

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

void CheckForUpdates(int argc,char*args[],const std::string& CV){
    std::string link;
    std::string HTTP = HTTP::Get("https://beammp.com/builds/launcher?version=true");
    bool fallback = false;
    if(HTTP.find_first_of("0123456789") == std::string::npos){
        HTTP = HTTP::Get("https://backup1.beammp.com/builds/launcher?version=true");
        fallback = true;
        if(HTTP.find_first_of("0123456789") == std::string::npos) {
            fatal("Primary Servers Offline! sorry for the inconvenience!");
        }
    }
    if(fallback){
        link = "https://backup1.beammp.com/builds/launcher?download=true";
    }else link = "https://beammp.com/builds/launcher?download=true";

    std::string EP(GetEP() + GetEN()), Back(GetEP() + "BeamMP-Launcher.back");

    if(fs::exists(Back))remove(Back.c_str());

    if(HTTP > CV){
        system("cls");
        info("Update found!");
        info("Updating...");
        if(std::rename(EP.c_str(), Back.c_str()))error("failed creating a backup!");

        if(!HTTP::Download(link, EP)){
            error("Launcher Update failed! trying again...");
            std::this_thread::sleep_for(std::chrono::seconds(2));

            if(!HTTP::Download(link, EP)){
                error("Launcher Update failed!");
                std::this_thread::sleep_for(std::chrono::seconds(5));
                ReLaunch(argc,args);
            }
        }
        URelaunch(argc,args);
    }else info("Launcher version is up to date");
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
                std::string Name = p.path().filename().u8string();
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
        info("Downloading mod please wait...");
        try {
            if (!fs::exists(GetGamePath() + "mods/multiplayer")) {
                fs::create_directories(GetGamePath() + "mods/multiplayer");
            }
            EnableMP();
        }catch(std::exception&e){
            fatal(e.what());
        }
        std::string ZipPath(GetGamePath() + R"(mods\multiplayer\BeamMP.zip)");

        HTTP::Download("https://backend.beammp.com/builds/client?download=true"
                 "&pk=" + PublicKey + "&branch=" + Branch, ZipPath);

        std::string Target(GetGamePath() + "mods/unpacked/beammp");

        if(fs::is_directory(Target)) {
            fs::remove_all(Target);
        }

        //HTTP::Download("beammp.com/builds/client", GetGamePath() + R"(mods\multiplayer\BeamMP.zip)");
    }

}

void StartProxy() {
    std::thread proxy([&](){
        httplib::Server HTTPProxy;

        HTTPProxy.Get("/:any", [](const httplib::Request& req, httplib::Response& res) {
            httplib::Client cli("https://backend.beammp.com");
            auto headers = req.headers;
            if (req.has_header("X-BMP-Authentication")) {
                headers.emplace("X-BMP-Authentication", PrivateKey);
            }
            if (auto cli_res = cli.Get(req.path, headers); cli_res) {
                res.set_content(cli_res->body,cli_res->get_header_value("Content-Type"));
            } else {
                res.set_content(to_string(cli_res.error()), "text/plain");
            }
        });

        HTTPProxy.Post("/:any", [](const httplib::Request& req, httplib::Response& res) {
            httplib::Client cli("https://backend.beammp.com");
            auto headers = req.headers;
            if (req.has_header("X-BMP-Authentication")) {
                headers.emplace("X-BMP-Authentication", PrivateKey);
            }
            if (auto cli_res = cli.Post(req.path, headers, req.body, req.get_header_value("Content-Type")); cli_res) {
                res.set_content(cli_res->body,cli_res->get_header_value("Content-Type"));
            } else {
                res.set_content(to_string(cli_res.error()), "text/plain");
            }
        });

        ProxyPort = HTTPProxy.bind_to_any_port("0.0.0.0");
        HTTPProxy.listen_after_bind();
    });
    proxy.detach();
}
