///
/// Created by Anonymous275 on 1/18/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#include "Launcher.h"
#include "Logger.h"
#include "Http.h"


void Launcher::UpdateCheck() {
    std::string link;
    std::string HTTP = HTTP::Get("https://beammp.com/builds/launcher?version=true");
    bool fallback = false;
    if(HTTP.find_first_of("0123456789") == std::string::npos){
        HTTP = HTTP::Get("https://backup1.beammp.com/builds/launcher?version=true");
        fallback = true;
        if(HTTP.find_first_of("0123456789") == std::string::npos) {
            LOG(FATAL) << "Primary Servers Offline! sorry for the inconvenience!";
        }
    }
    if(fallback){
        link = "https://backup1.beammp.com/builds/launcher?download=true";
    }else link = "https://beammp.com/builds/launcher?download=true";

    std::string EP(CurrentPath.string()), Back(CurrentPath.parent_path().string() + "\\BeamMP-Launcher.back");

    if(fs::exists(Back))remove(Back.c_str());
    std::string RemoteVer;
    for(char& c : HTTP) {
        if(std::isdigit(c) || c == '.') {
            RemoteVer += c;
        }
    }
    if(RemoteVer > FullVersion){
        system("cls");
        LOG(INFO) << "Update found! Downloading...";
        if(std::rename(EP.c_str(), Back.c_str())){
            LOG(ERROR) << "Failed to create a backup!";
        }

        if(!HTTP::Download(link, EP)){
            LOG(ERROR) << "Launcher Update failed! trying again...";
            std::this_thread::sleep_for(std::chrono::seconds(2));

            if(!HTTP::Download(link, EP)){
                LOG(ERROR) << "Launcher Update failed!";
                std::this_thread::sleep_for(std::chrono::seconds(5));
                AdminRelaunch();
            }
        }
        Relaunch();
    }else LOG(INFO) << "Launcher version is up to date";
}
