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
    if(DiscordRPC.joinable()) {
        DiscordRPC.join();
    }
}

void Launcher::LaunchGame() {
    VersionParser GameVersion(BeamVersion);
    if(GameVersion.data[0] > SupportedVersion.data[0]) {
        LOG(FATAL) << "BeamNG V" << BeamVersion << " not yet supported, please wait until we update BeamMP!";
        throw ShutdownException("Fatal Error");
    } else if(GameVersion.data[0] < SupportedVersion.data[0]) {
        LOG(FATAL) << "BeamNG V" << BeamVersion << " not supported, please update and launch the new update!";
        throw ShutdownException("Fatal Error");
    } else if(GameVersion > SupportedVersion) {
        LOG(WARNING) << "BeamNG V" << BeamVersion << " is slightly newer than recommended, this might cause issues!";
    } else if(GameVersion < SupportedVersion) {
        LOG(WARNING) << "BeamNG V" << BeamVersion << " is slightly older than recommended, this might cause issues!";
    }

    ShellExecuteA(nullptr, nullptr, "steam://rungameid/284160", nullptr, nullptr, SW_SHOWNORMAL);
    //ShowWindow(GetConsoleWindow(), HIDE_WINDOW);
}

void Launcher::WindowsInit() {
    system("cls");
    SetConsoleTitleA(("BeamMP Launcher v" + FullVersion).c_str());
}

std::string QueryValue(HKEY& hKey, const char* Name) {
    DWORD keySize;
    BYTE buffer[16384];
    if(RegQueryValueExA(hKey, Name, nullptr, nullptr, buffer, &keySize) == ERROR_SUCCESS) {
        return {(char*)buffer, keySize};
    }
    return {};
}
std::string Launcher::GetLocalAppdata() {
    HKEY Folders;
    LONG RegRes = RegOpenKeyExA(HKEY_CURRENT_USER, R"(SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders)", 0, KEY_READ, &Folders);
    if(RegRes == ERROR_SUCCESS) {
        std::string Path = QueryValue(Folders, "Local AppData");
        if(!Path.empty()) {
            Path += "\\BeamNG.drive\\";
            VersionParser GameVer(BeamVersion);
            Path += std::to_string(GameVer.data[0]) + '.' + std::to_string(GameVer.data[1]) + '\\';
            return Path;
        }
    }
    return {};
}
void Launcher::QueryRegistry() {
    HKEY BeamNG;
    LONG RegRes = RegOpenKeyExA(HKEY_CURRENT_USER, R"(Software\BeamNG\BeamNG.drive)", 0, KEY_READ, &BeamNG);
    if(RegRes == ERROR_SUCCESS) {
        BeamRoot = QueryValue(BeamNG, "rootpath");
        BeamVersion = QueryValue(BeamNG, "version");
        BeamUserPath = QueryValue(BeamNG, "userpath_override");
        RegCloseKey(BeamNG);
        if(BeamUserPath.empty() && !BeamVersion.empty()) {
            BeamUserPath = GetLocalAppdata();
        } else if(!BeamUserPath.empty()) {
            VersionParser GameVer(BeamVersion);
            BeamUserPath += std::to_string(GameVer.data[0]) + '.' + std::to_string(GameVer.data[1]) + '\\';
        }
        if(!BeamRoot.empty() && !BeamVersion.empty() && !BeamUserPath.empty())return;
    }
    LOG(FATAL) << "Please launch the game at least once, failed to read registry key Software\\BeamNG\\BeamNG.drive";
    throw ShutdownException("Fatal Error");
}

void Launcher::AdminRelaunch() {
    system("cls");
    ShellExecuteA(nullptr, "runas", CurrentPath.string().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    ShowWindow(GetConsoleWindow(),0);
    throw ShutdownException("Relaunching");
}

void Launcher::Relaunch() {
    ShellExecuteA(nullptr, "open", CurrentPath.string().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    ShowWindow(GetConsoleWindow(),0);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    throw ShutdownException("Relaunching");
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

