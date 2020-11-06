///
/// Created by Anonymous275 on 7/16/2020
///
#include "Discord/discord_info.h"
#include "Network/network.h"
#include "Security/Init.h"
#include "Security/Enc.h"
#include "Curl/http.h"
#include "Curl/curl.h"
#include <filesystem>
#include <iostream>
#include "Logger.h"
#include <thread>

bool Dev = false;
namespace fs = std::experimental::filesystem;
std::string GetEN(){
    static std::string r = Sec("BeamMP-Launcher.exe");
    return r;
}
std::string GetVer(){
    static std::string r = Sec("1.63");
    return r;
}
std::string GetPatch(){
    static std::string r = Sec(".7");
    return r;
}
void ReLaunch(int argc,char*args[]){
    std::string Arg;
    for(int c = 2; c <= argc; c++){
        Arg += " ";
        Arg += args[c-1];
    }
    system(Sec("cls"));
    ShellExecute(nullptr,Sec("runas"),GetEN().c_str(),Arg.c_str(),nullptr,SW_SHOWNORMAL);
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
    ShellExecute(nullptr,Sec("open"),GetEN().c_str(),Arg.c_str(),nullptr,SW_SHOWNORMAL);
    ShowWindow(GetConsoleWindow(),0);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    exit(1);
}
void CheckName(int argc,char* args[]){
    struct stat info{};
    std::string DN = GetEN(),CDir = args[0],FN = CDir.substr(CDir.find_last_of('\\')+1);
    if(FN != DN){
        if(stat(DN.c_str(),&info)==0)remove(DN.c_str());
        if(stat(DN.c_str(),&info)==0)ReLaunch(argc,args);
        std::rename(FN.c_str(), DN.c_str());
        URelaunch(argc,args);
    }
}
void RequestRole(){
    auto NPos = std::string::npos;
    std::string HTTP_Result = HTTP_REQUEST(Sec("https://beammp.com/entitlement?did=")+GetDID()+Sec("&t=l"),443);
    if(HTTP_Result == "-1"){
        HTTP_Result = HTTP_REQUEST(Sec("https://backup1.beammp.com/entitlement?did=")+GetDID()+Sec("&t=l"),443);
        if(HTTP_Result == "-1") {
            error(Sec("Sorry Backend System Outage! Don't worry it will back on soon!"));
            std::this_thread::sleep_for(std::chrono::seconds(3));
            exit(-1);
        }
    }
    if(HTTP_Result.find(Sec("\"MDEV\"")) != NPos || HTTP_Result.find(Sec("\"CON\"")) != NPos){
        if(GetDID() != "125792589621231616"){
            Dev = true;
        }
    }
    if(HTTP_Result.find(Sec("Error")) != NPos){
        error(Sec("Sorry You need to be in the official BeamMP Discord to proceed! https://discord.gg/beammp"));
        std::this_thread::sleep_for(std::chrono::seconds(3));
        exit(-1);
    }
    info(Sec("Client Connected!"));
}

void CheckForUpdates(int argc,char*args[],const std::string& CV){
    std::string link = Sec("https://beammp.com/builds/launcher?version=true");
    std::string HTTP = HTTP_REQUEST(link,443);
    bool fallback = false;
    if(HTTP.find_first_of("0123456789") == std::string::npos){
        link = Sec("https://backup1.beammp.com/builds/launcher?version=true");
        HTTP = HTTP_REQUEST(link,443);
        fallback = true;
        if(HTTP.find_first_of("0123456789") == std::string::npos) {
            error(Sec("Primary Servers Offline! sorry for the inconvenience!"));
            std::this_thread::sleep_for(std::chrono::seconds(4));
            exit(-1);
        }
    }
    if(fallback){
        link = Sec("https://backup1.beammp.com/builds/launcher?download=true");
    }else link = Sec("https://beammp.com/builds/launcher?download=true");

    struct stat buffer{};
    std::string Back = Sec("BeamMP-Launcher.back");
    if(stat(Back.c_str(), &buffer) == 0)remove(Back.c_str());
    if(HTTP > CV){
        system(Sec("cls"));
        info(Sec("Update found!"));
        info(Sec("Updating..."));
        if(std::rename(GetEN().c_str(), Back.c_str()))error(Sec("failed creating a backup!"));
        int i = Download(link, GetEN(),true);
        if(i != -1){
            error(Sec("Launcher Update failed! trying again... code : ") + std::to_string(i));
            std::this_thread::sleep_for(std::chrono::seconds(2));
            int i2 = Download(link, GetEN(),true);
            if(i2 != -1){
                error(Sec("Launcher Update failed! code : ") + std::to_string(i2));
                std::this_thread::sleep_for(std::chrono::seconds(5));
                ReLaunch(argc,args);
            }
        }
        URelaunch(argc,args);
    }else{
        info(Sec("Version is up to date"));
    }
}
void CheckDir(int argc,char*args[]){
    std::string CDir = args[0];
    std::string MDir = Sec("BeamNG\\mods");
    if(!fs::is_directory(Sec("BeamNG"))){
        if(!fs::create_directory(Sec("BeamNG"))){
            error(Sec("Cannot Create BeamNG Directory! Retrying..."));
            std::this_thread::sleep_for(std::chrono::seconds(3));
            ReLaunch(argc,args);
        }
    }
    if(fs::is_directory(MDir) && !Dev){
        int c = 0;
        for (auto& p : fs::directory_iterator(MDir))c++;
        if(c > 2) {
            warn(std::to_string(c-1) + Sec(" local mods will be wiped! Close this window if you don't want that!"));
            std::this_thread::sleep_for(std::chrono::seconds(15));
        }
        try{
            fs::remove_all(MDir);
        } catch (...) {
            error(Sec("Please close the game and try again"));
            std::this_thread::sleep_for(std::chrono::seconds(5));
            exit(1);
        }
    }
    if(fs::is_directory(MDir) && !Dev)ReLaunch(argc,args);
    if(!fs::create_directory(MDir) && !Dev){
        error(Sec("Cannot Create Mods Directory! Retrying..."));
        std::this_thread::sleep_for(std::chrono::seconds(3));
        ReLaunch(argc,args);
    }
    if(!fs::is_directory(Sec("BeamNG\\settings"))){
        if(!fs::create_directory(Sec("BeamNG\\settings"))){
            error(Sec("Cannot Create Settings Directory! Retrying..."));
            std::this_thread::sleep_for(std::chrono::seconds(3));
            ReLaunch(argc,args);
        }
    }
}
void CustomPort(int argc, char* argv[]){
    if(argc > 1){
        std::string Port = argv[1];
        if(Port.find_first_not_of("0123456789") == std::string::npos){
            if(std::stoi(Port) > 1000){
                DEFAULT_PORT = std::stoi(Port);
                warn(Sec("Running on custom port : ") + std::to_string(DEFAULT_PORT));
            }
        }
        if(argc > 2)Dev = false;
    }
}
void InitLauncher(int argc, char* argv[]) {
    system(Sec("cls"));
    curl_global_init(CURL_GLOBAL_DEFAULT);
    SetConsoleTitleA((Sec("BeamMP Launcher v") + std::string(GetVer()) + GetPatch()).c_str());
    InitLog();
    CheckName(argc, argv);
    SecurityCheck(argv);
    Discord_Main();
    RequestRole();
    CustomPort(argc, argv);
    CheckForUpdates(argc, argv, std::string(GetVer()) + GetPatch());
}

void PreGame(int argc, char* argv[],const std::string& GamePath){
    info(Sec("Game Version : ") + CheckVer(GamePath));
    std::string DUI = Sec(R"(BeamNG\settings\uiapps-layouts.json)");
    std::string GS = Sec(R"(BeamNG\settings\game-settings.ini)");
    std::string link = Sec("https://beammp.com/client-ui-data");
    bool fallback = false;
    int i;
    if(!fs::exists(DUI)){
        info(Sec("Downloading default ui data..."));
        i = Download(link,DUI,true);
        if(i != -1){
            fallback = true;
            remove(DUI.c_str());
            link = Sec("https://backup1.beammp.com/client-ui-data");
            i = Download(link,DUI,true);
            if(i != -1) {
                error(Sec("Failed to download code : ") + std::to_string(i));
                std::this_thread::sleep_for(std::chrono::seconds(3));
                ReLaunch(argc, argv);
            }
        }
        info(Sec("Download Complete!"));
    }
    if(!fs::exists(GS)) {
        info(Sec("Downloading default game settings..."));
        if(fallback)link = Sec("https://backup1.beammp.com/client-settings-data");
        else link = Sec("https://beammp.com/client-settings-data");
        Download(link, GS,true);
        info(Sec("Download Complete!"));
    }
    if(!Dev) {
        info(Sec("Downloading mod..."));
        if(fallback)link = Sec("https://backup1.beammp.com/builds/client?did=") + GetDID();
        else link = Sec("https://beammp.com/builds/client?did=") + GetDID();
        Download(link, Sec(R"(BeamNG\mods\BeamMP.zip)"), false);
        info(Sec("Download Complete!"));
    }
    debug(Sec("Name : ") + GetDName());
    debug(Sec("Discriminator : ") + GetDTag());
    debug(Sec("Unique ID : ") + GetDID());
}