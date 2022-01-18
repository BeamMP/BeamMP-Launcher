///
/// Created by Anonymous275 on 12/26/21
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#define WIN32_LEAN_AND_MEAN
#include "Launcher.h"
#include "Logger.h"
#include <windows.h>
#include <shellapi.h>

Launcher::Launcher(int argc, char* argv[]) : DirPath(argv[0]), DiscordMessage("Just launched") {
    DirPath = DirPath.substr(0, DirPath.find_last_of("\\/") + 1);
    Log::Init();
    WindowsInit();
}

Launcher::~Launcher() {
    Shutdown = true;
    if(DiscordRPC.joinable()) {
        DiscordRPC.join();
    }
}

void Launcher::launchGame() {
    ShellExecuteA(nullptr, nullptr, "steam://rungameid/284160", nullptr, nullptr, SW_SHOWNORMAL);
    ShowWindow(GetConsoleWindow(), HIDE_WINDOW);
    LOG(INFO) << "Sus";
}

void Launcher::WindowsInit() {
    system("cls");
    SetConsoleTitleA(("BeamMP Launcher v" + FullVersion).c_str());
}

const std::string& Launcher::getFullVersion() {
    return FullVersion;
}

const std::string& Launcher::getWorkingDir() {
    return DirPath;
}

const std::string &Launcher::getVersion() {
    return Version;
}

const std::string& Launcher::getUserRole() {
    return UserRole;
}



