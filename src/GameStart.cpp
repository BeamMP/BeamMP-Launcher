// Copyright (c) 2019-present Anonymous275.
// BeamMP Launcher code is not in the public domain and is not free software.
// One must be granted explicit permission by the copyright holder in order to modify or distribute any part of the source or binaries.
// Anything else is prohibited. Modified works may not be published and have be upstreamed to the official repository.
///
/// Created by Anonymous275 on 7/19/2020
///

#include <Windows.h>
#include "Startup.h"
#include "Logger.h"
#include <iostream>
#include <thread>

unsigned long GamePID = 0;
std::string QueryKey(HKEY hKey,int ID);
std::string GetGamePath(){
    static std::string Path;
    if(!Path.empty())return Path;

    HKEY hKey;
    LPCTSTR sk = "Software\\BeamNG\\BeamNG.drive";
    LONG openRes = RegOpenKeyEx(HKEY_CURRENT_USER, sk, 0, KEY_ALL_ACCESS, &hKey);
    if (openRes != ERROR_SUCCESS){
        fatal("Please launch the game at least once!");
    }
    Path = QueryKey(hKey,4);

    if(Path.empty()){
        sk = R"(SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders)";
        openRes = RegOpenKeyEx(HKEY_CURRENT_USER, sk, 0, KEY_ALL_ACCESS, &hKey);
        if (openRes != ERROR_SUCCESS){
           fatal("Cannot get Documents directory!");
        }
        Path = QueryKey(hKey,5);
        Path += "\\BeamNG.drive\\";
        return Path;
    }

    return Path;
}

void StartGame(std::string Dir){
    BOOL bSuccess = FALSE;
    PROCESS_INFORMATION pi;
    STARTUPINFO si = {0};
    si.cb = sizeof(si);
    std::string BaseDir = Dir; //+"\\Bin64";
    //Dir += R"(\Bin64\BeamNG.drive.x64.exe)";
    Dir += "\\BeamNG.drive.exe";
    bSuccess = CreateProcessA(Dir.c_str(), nullptr, nullptr, nullptr, TRUE, 0, nullptr, BaseDir.c_str(), &si, &pi);
    if (bSuccess){
        info("Game Launched!");
        GamePID = pi.dwProcessId;
        WaitForSingleObject(pi.hProcess, INFINITE);
        error("Game Closed! launcher closing soon");
    }else{
        error("Failed to Launch the game! launcher closing soon");
    }
    std::this_thread::sleep_for(std::chrono::seconds(5));
    exit(2);
}
void InitGame(const std::string& Dir){
    if(!Dev){
        std::thread Game(StartGame, Dir);
        Game.detach();
    }
}
