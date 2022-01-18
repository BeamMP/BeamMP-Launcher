///
/// Created by Anonymous275 on 12/26/21
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#define WIN32_LEAN_AND_MEAN
#include "Launcher.h"
#include "Logger.h"
#include <windows.h>
#include <shellapi.h>


Launcher::Launcher(int argc, char* argv[]) : CurrentPath(std::filesystem::path(argv[0])), DiscordMessage("Just launched") {
    Log::Init();
    WindowsInit();
    LOG(INFO) << "Starting Launcher V" << FullVersion;
    UpdateCheck();
}

Launcher::~Launcher() {
    Shutdown = true;
    LOG(INFO) << "Shutting down";
    if(DiscordRPC.joinable()) {
        DiscordRPC.join();
    }
}

bool Launcher::Terminate() const {
    return Shutdown;
}

void Launcher::LaunchGame() {
    ShellExecuteA(nullptr, nullptr, "steam://rungameid/284160", nullptr, nullptr, SW_SHOWNORMAL);
    //ShowWindow(GetConsoleWindow(), HIDE_WINDOW);
}

void Launcher::WindowsInit() {
    system("cls");
    SetConsoleTitleA(("BeamMP Launcher v" + FullVersion).c_str());
}

const std::string& Launcher::getFullVersion() {
    return FullVersion;
}

const std::string &Launcher::getVersion() {
    return Version;
}

const std::string& Launcher::getUserRole() {
    return UserRole;
}

void Launcher::AdminRelaunch() {
    system("cls");
    ShellExecuteA(nullptr, "runas", CurrentPath.string().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    ShowWindow(GetConsoleWindow(),0);
    Shutdown = true;
}

void Launcher::Relaunch() {
    ShellExecuteA(nullptr, "open", CurrentPath.string().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    ShowWindow(GetConsoleWindow(),0);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    Shutdown = true;
}

