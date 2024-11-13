// Copyright (c) 2019-present Anonymous275.
// BeamMP Launcher code is not in the public domain and is not free software.
// One must be granted explicit permission by the copyright holder in order to modify or distribute any part of the source or binaries.
// Anything else is prohibited. Modified works may not be published and have be upstreamed to the official repository.
///
/// Created by Anonymous275 on 7/19/2020
///

#if defined(_WIN32)
#include <windows.h>
#include <shlobj.h>
#elif defined(__linux__) || defined(__APPLE__)
#include "vdf_parser.hpp"
#include <pwd.h>
#include <spawn.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include "Logger.h"
#include "Startup.h"
#include <Security/Init.h>
#include <filesystem>
#include <thread>
#include "Options.h"

unsigned long GamePID = 0;
#if defined(_WIN32)
std::string QueryKey(HKEY hKey, int ID);
std::string GetGamePath() {
    static std::string Path;
    if (!Path.empty())
        return Path;

    HKEY hKey;
    LPCTSTR sk = "Software\\BeamNG\\BeamNG.drive";
    LONG openRes = RegOpenKeyEx(HKEY_CURRENT_USER, sk, 0, KEY_ALL_ACCESS, &hKey);
    if (openRes != ERROR_SUCCESS) {
        fatal("Please launch the game at least once!");
    }
    Path = QueryKey(hKey, 4);

    if (Path.empty()) {
        Path = "";
        char appDataPath[MAX_PATH];
        HRESULT result = SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, appDataPath);
        if (SUCCEEDED(result)) {
            Path = appDataPath;
        }

        if (Path.empty()) {
            fatal("Cannot get Local Appdata directory");
        }

        Path += "\\BeamNG.drive\\";
    }

    std::string Ver = CheckVer(GetGameDir());
    Ver = Ver.substr(0, Ver.find('.', Ver.find('.') + 1));
    Path += Ver + "\\";
    return Path;
}
#elif defined(__linux__)
std::string GetGamePath() {
    // Right now only steam is supported
    struct passwd* pw = getpwuid(getuid());
    std::string homeDir = pw->pw_dir;

    std::string Path = homeDir + "/.local/share/BeamNG.drive/";
    std::string Ver = CheckVer(GetGameDir());
    Ver = Ver.substr(0, Ver.find('.', Ver.find('.') + 1));
    Path += Ver + "/";
    return Path;
}
#elif defined(__APPLE__)
std::string GetGamePath() {
    std::string BotlePath = GetBottlePath();
    std::string Path = BotlePath + "/drive_c/users/crossover/AppData/Local/BeamNG.drive/";
    std::string Ver = CheckVer(GetGameDir());
    Ver = Ver.substr(0, Ver.find('.', Ver.find('.') + 1));
    Path += Ver + "/";
    return Path;
}
#endif

#if defined(_WIN32)
void StartGame(std::string Dir) {
    BOOL bSuccess = FALSE;
    PROCESS_INFORMATION pi;
    STARTUPINFO si = { 0 };
    si.cb = sizeof(si);
    std::string BaseDir = Dir; //+"\\Bin64";
    // Dir += R"(\Bin64\BeamNG.drive.x64.exe)";
    Dir += "\\BeamNG.drive.exe";
    std::string gameArgs = "";

    for (int i = 0; i < options.game_arguments_length; i++) {
        gameArgs += " ";
        gameArgs += options.game_arguments[i];
    }

    bSuccess = CreateProcessA(nullptr, (LPSTR)(Dir + gameArgs).c_str(), nullptr, nullptr, TRUE, 0, nullptr, BaseDir.c_str(), &si, &pi);
    if (bSuccess) {
        info("Game Launched!");
        GamePID = pi.dwProcessId;
        WaitForSingleObject(pi.hProcess, INFINITE);
        error("Game Closed! launcher closing soon");
    } else {
        error("Failed to Launch the game! launcher closing soon");
    }
    std::this_thread::sleep_for(std::chrono::seconds(5));
    exit(2);
}
#elif defined(__linux__)
void StartGame(std::string Dir) {
    extern char **environ;
    int status;
    std::string filename = (Dir + "/BinLinux/BeamNG.drive.x64");
    std::vector<const char*> argv;
    argv.push_back(filename.data());
    for (int i = 0; i < options.game_arguments_length; i++) {
        argv.push_back(options.game_arguments[i]);
    }

    argv.push_back(nullptr);
    pid_t pid;
    posix_spawn_file_actions_t spawn_actions;
    posix_spawn_file_actions_init(&spawn_actions);
    posix_spawn_file_actions_addclose(&spawn_actions, STDOUT_FILENO);
    posix_spawn_file_actions_addclose(&spawn_actions, STDERR_FILENO);
    int result = posix_spawn(&pid, filename.c_str(), &spawn_actions, nullptr, const_cast<char**>(argv.data()), environ);

    if (result != 0) {
        error("Failed to Launch the game! launcher closing soon");
        return;
    } else {
        waitpid(pid, &status, 0);
        error("Game Closed! launcher closing soon");
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));
    exit(2);
}

#elif defined(__APPLE__)
void StartGame(std::string Dir) {
    extern char **environ;
    int status;
    
    std::string executable = Dir + "/Bin64/BeamNG.drive.x64.exe";
    
    std::string wineExecutable = "/Applications/CrossOver.app/Contents/SharedSupport/CrossOver/bin/wine";
    std::string bottleName = GetBottleName();
    
    std::vector<const char*> argv;
    argv.push_back(wineExecutable.c_str());
    argv.push_back("--bottle");
    argv.push_back(bottleName.c_str());
    argv.push_back(executable.c_str());
    argv.push_back(nullptr);
    
    for (int i = 0; i < options.game_arguments_length; i++) {
        argv.push_back(options.game_arguments[i]);
        info("options.game_arguments[i]");
        info(options.game_arguments[i]);
    }
    argv.push_back(nullptr);
    
    pid_t pid;
    posix_spawn_file_actions_t spawn_actions;
    posix_spawn_file_actions_init(&spawn_actions);
    posix_spawn_file_actions_addclose(&spawn_actions, STDOUT_FILENO);
    posix_spawn_file_actions_addclose(&spawn_actions, STDERR_FILENO);
    
    int result = posix_spawn(&pid, wineExecutable.c_str(), &spawn_actions, nullptr, const_cast<char**>(argv.data()), environ);
    
    if (result != 0) {
        error("Failed to Launch the game! launcher closing soon");
        return;
    } else {
        waitpid(pid, &status, 0);
        info("Game Closed! launcher closing soon");
    }
    
    std::this_thread::sleep_for(std::chrono::seconds(5));
    exit(2);
}
#endif

void InitGame(const std::string& Dir) {
    if (!options.no_launch) {
        std::thread Game(StartGame, Dir);
        Game.detach();
    }
}
