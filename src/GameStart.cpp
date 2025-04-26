/*
 Copyright (C) 2024 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/


#if defined(_WIN32)
#include <windows.h>
#include <shlobj.h>
#elif defined(__linux__)
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
#include <fstream>

unsigned long GamePID = 0;
#if defined(_WIN32)
std::string QueryKey(HKEY hKey, int ID);
std::string GetGamePath() {
    static std::filesystem::path Path;
    if (!Path.empty())
        return Path.string();

    if (options.user_path) {
        if (std::filesystem::exists(options.user_path)) {
            Path = options.user_path;
            debug("Using custom user folder path: " + Path.string());
        } else
            warn("Invalid or non-existent path (" + std::string(options.user_path) + ") specified using --user-path, skipping");
    }

    if (const auto startupIniPath = std::filesystem::path(GetGameDir()) / "startup.ini"; exists(startupIniPath)) {

        std::ifstream startupIni(startupIniPath);
        std::string line;
        while (std::getline(startupIni, line))
            if (line.find("UserPath = ") == 0) {
                std::string userPath = line.substr(11);
                if (!userPath.empty())
                    if (std::filesystem::exists(userPath)) {
                        Path = userPath;
                        debug("Using custom user folder path from startup.ini: " + Path.string());
                    } else
                        warn("Found custom user folder path ("+ userPath + ") in startup.ini but it doesn't exist, skipping");

            }

        if (Path.empty()) {
            HKEY hKey;
            LPCTSTR sk = "Software\\BeamNG\\BeamNG.drive";
            LONG openRes = RegOpenKeyEx(HKEY_CURRENT_USER, sk, 0, KEY_ALL_ACCESS, &hKey);
            if (openRes != ERROR_SUCCESS) {
                fatal("Please launch the game at least once!");
            }
            Path = QueryKey(hKey, 4);

            if (Path.empty()) {
                char appDataPath[MAX_PATH];
                HRESULT result = SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, appDataPath);

                if (!SUCCEEDED(result)) {
                    fatal("Cannot get Local Appdata directory");
                }

                Path = std::filesystem::path(appDataPath) / "BeamNG.drive";
            }
        }
    }

    std::string Ver = CheckVer(GetGameDir());
    Ver = Ver.substr(0, Ver.find('.', Ver.find('.') + 1));
    Path = Path / (Ver + "\\");
    return Path.string();
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

    debug("BeamNG executable path: " + Dir);

    bSuccess = CreateProcessA(nullptr, (LPSTR)(Dir + gameArgs).c_str(), nullptr, nullptr, TRUE, 0, nullptr, BaseDir.c_str(), &si, &pi);
    if (bSuccess) {
        info("Game Launched!");
        GamePID = pi.dwProcessId;
        WaitForSingleObject(pi.hProcess, INFINITE);
        error("Game Closed! launcher closing soon");
    } else {
        std::string err = "";

        DWORD dw = GetLastError();
        LPVOID lpErrorMsgBuffer;

        if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dw,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpErrorMsgBuffer, 0, nullptr) == 0) {
            err = "Unknown error code: " + std::to_string(dw);
        } else {
            err = "Error " + std::to_string(dw) + ": " + (char*)lpErrorMsgBuffer;
        }

        error("Failed to Launch the game! launcher closing soon. " + err);
    }
    std::this_thread::sleep_for(std::chrono::seconds(5));
    exit(2);
}
#elif defined(__linux__)
void StartGame(std::string Dir) {
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
#endif

void InitGame(const std::string& Dir) {
    if (!options.no_launch) {
        std::thread Game(StartGame, Dir);
        Game.detach();
    }
}
