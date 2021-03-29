// Copyright (c) 2019-present Anonymous275.
// BeamMP Launcher code is not in the public domain and is not free software.
// One must be granted explicit permission by the copyright holder in order to modify or distribute any part of the source or binaries.
// Anything else is prohibited. Modified works may not be published and have be upstreamed to the official repository.
///
/// Created by Anonymous275 on 7/16/2020
///

#include "Discord/discord_info.h"
#include "Network/network.h"
#include "Security/Init.h"
#include <filesystem>
#include "Startup.h"
#include "Logger.h"
#include <thread>
#include "http.h"

extern int TraceBack;
bool Dev = false;
namespace fs = std::filesystem;

std::string GetEN(){
    return "BeamMP-Launcher.exe";
}
std::string GetVer(){
    return "1.80";
}
std::string GetPatch(){
    return ".95";
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
    std::string HTTP = HTTP::Get("beammp.com/builds/launcher?version=true");
    bool fallback = false;
    if(HTTP.find_first_of("0123456789") == std::string::npos){
        HTTP = HTTP::Get("backup1.beammp.com/builds/launcher?version=true");
        fallback = true;
        if(HTTP.find_first_of("0123456789") == std::string::npos) {
            fatal("Primary Servers Offline! sorry for the inconvenience!");
        }
    }
    if(fallback){
        link = "backup1.beammp.com/builds/launcher?download=true";
    }else link = "beammp.com/builds/launcher?download=true";

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
void InitLauncher(int argc, char* argv[]) {
    system("cls");
    SetConsoleTitleA(("BeamMP Launcher v" + std::string(GetVer()) + GetPatch()).c_str());
    InitLog();
    CheckName(argc, argv);
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
    if (c > 3) {
        warn(std::to_string(c - 1) + " multiplayer mods will be wiped from mods/multiplayer! Close this if you don't want that!");
        std::this_thread::sleep_for(std::chrono::seconds(15));
    }
    try {
        for (auto& p : fs::directory_iterator(Path)){
            if(p.exists() && !p.is_directory()){
                std::string Name = p.path().filename().u8string();
                for(char&Ch : Name)Ch = char(tolower(Ch));
                if(Name != "beammp.zip")fs::remove(p.path());
            }
        }
    } catch (...) {
        fatal("Please close the game, and try again!");
    }

}
void PreGame(const std::string& GamePath){
    const std::string CurrVer("0.22.0.1");
    std::string GameVer = CheckVer(GamePath);
    info("Game Version : " + GameVer);
    if(GameVer < CurrVer){
        fatal("Game version is old! Please update.");
    }else if(GameVer > CurrVer){
        warn("Game is newer than recommended, multiplayer may not work as intended!");
    }
    CheckMP(GetGamePath() + "mods/multiplayer");

    if(!Dev) {
        info("Downloading mod...");
        try {
            if (!fs::exists(GetGamePath() + "mods/multiplayer")) {
                fs::create_directories(GetGamePath() + "mods/multiplayer");
            }
        }catch(std::exception&e){
            fatal(e.what());
        }

       HTTP::Download("backend.beammp.com/builds/client?download=true"
                 "&pk=" + PublicKey +
                 "&branch=" + Branch, GetGamePath() + R"(mods\multiplayer\BeamMP.zip)");

        //HTTP::Download("beammp.com/builds/client", GetGamePath() + R"(mods\multiplayer\BeamMP.zip)");
        info("Download Complete!");
    }

}