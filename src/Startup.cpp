// Copyright (c) 2019-present Anonymous275.
// BeamMP Launcher code is not in the public domain and is not free software.
// One must be granted explicit permission by the copyright holder in order to modify or distribute any part of the source or binaries.
// Anything else is prohibited. Modified works may not be published and have be upstreamed to the official repository.
///
/// Created by Anonymous275 on 7/16/2020
///
#include "zip_file.h"
#include <string>
#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <unistd.h>
#endif
#include "Discord/discord_info.h"
#include "Network/network.hpp"
#include "Security/Init.h"
#include <filesystem>
#include "Startup.h"
#include "Logger.h"
#include <fstream>
#include <thread>
#include "Http.h"
#include "Json.h"

extern int TraceBack;
bool Dev = false;
namespace fs = std::filesystem;

#if defined(_WIN32)
std::string GetEN(){
    return "BeamMP-Launcher.exe";
}
#elif defined(__linux__)
std::string GetEN(){
    return "BeamMP-Launcher";
}
#endif

std::string GetVer(){
    return "2.0";
}
std::string GetPatch(){
    return ".82";
}

std::string GetEP(char*P){
    static std::string Ret = [&](){
        std::string path(P);
        return path.substr(0, path.find_last_of("\\/") + 1);
    } ();
    return Ret;
}
#if defined(_WIN32)
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
#elif defined(__linux__)
void ReLaunch(int argc,char*args[]){
    std::string Arg;
    for(int c = 2; c <= argc; c++){
        Arg += " ";
        Arg += args[c-1];
    }
    system("clear");
    execl((GetEP() + GetEN()).c_str(), Arg.c_str(), NULL);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    exit(1);
}
void URelaunch(int argc,char* args[]){
    std::string Arg;
    for(int c = 2; c <= argc; c++){
        Arg += " ";
        Arg += args[c-1];
    }
    execl((GetEP() + GetEN()).c_str(), Arg.c_str(), NULL);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    exit(1);
}
#endif

void CheckName(int argc,char* args[]){
    #if defined(_WIN32)
    std::string DN = GetEN(),CDir = args[0],FN = CDir.substr(CDir.find_last_of('\\')+1);
    #elif defined(__linux__)
    std::string DN = GetEN(),CDir = args[0],FN = CDir.substr(CDir.find_last_of('/')+1);
    #endif
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
        #if defined(_WIN32)
        system("cls");
        #elif defined(__linux__)
        system("clear");
        #endif
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

#ifdef _WIN32
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

    std::string Path = R"(Z:\home\)" + std::string(getenv("USER")) + R"(\.steam\steam\Steam.exe)";

    if(!fs::exists(Path)) {
        std::ofstream ofs(Path);
        if (!ofs.is_open()) {
            fatal("Failed to create file \"" + Path + "\"");
            return;
        } else ofs.close();
    }

    result = RegOpenKeyEx(HKEY_CURRENT_USER, R"(Software\Valve\Steam)", 0, KEY_ALL_ACCESS, &hKey);
    if (result != ERROR_SUCCESS){
        fatal(R"(failed to open HKEY_CURRENT_USER\Software\Valve\Steam)");
        return;
    }

    result = RegSetValueEx(hKey, "SteamExe", 0, REG_SZ, (BYTE*)Path.c_str(), Path.size());

    if (result != ERROR_SUCCESS){
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
    Discord_Main();
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
    CheckForUpdates(argc, argv, std::string(GetVer()) + GetPatch());
}
#endif

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
        json::Document d;
        d.Parse(Data.c_str());
        if(Data.at(0) != '{' || d.HasParseError()){
            //error("Failed to parse " + File); //TODO illegal formatting
            return;
        }
        if(d.HasMember("mods") && !d["mods"].IsNull() && d["mods"].HasMember("multiplayerbeammp") && !d["mods"]["multiplayerbeammp"].IsNull()){
            d["mods"]["multiplayerbeammp"]["active"] = true;
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            d.Accept(writer);
            std::ofstream ofs(File);
            if(ofs.is_open()){
                ofs << buffer.GetString();
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
        #if defined(_WIN32)
        std::string ZipPath(GetGamePath() + R"(mods\multiplayer\BeamMP.zip)");
        #elif defined(__linux__)
        // Linux version of the game cant handle mods with uppercase names
        std::string ZipPath(GetGamePath() + R"(mods/multiplayer/beammp.zip)");
        #endif

        HTTP::Download("https://backend.beammp.com/builds/client?download=true"
                 "&pk=" + PublicKey + "&branch=" + Branch, ZipPath);

        std::string Target(GetGamePath() + "mods/unpacked/beammp");

        if(fs::is_directory(Target)) {
            fs::remove_all(Target);
        }

        //HTTP::Download("beammp.com/builds/client", GetGamePath() + R"(mods\multiplayer\BeamMP.zip)");
    }

}
