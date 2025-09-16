/*
 Copyright (C) 2024 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/

#if defined(_WIN32)
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
#include "Options.h"
#include "Startup.h"
#include "Utils.h"
#include <Security/Init.h>
#include <filesystem>
#include <thread>

#include <fstream>

unsigned long GamePID = 0;
#if defined(_WIN32)
std::wstring QueryKey(HKEY hKey, int ID);
std::filesystem::path GetGamePath() {
    static std::filesystem::path Path;
    if (!Path.empty())
        return Path.wstring();

    if (options.user_path) {
        if (std::filesystem::exists(options.user_path)) {
            Path = options.user_path;
            debug(L"Using custom user folder path: " + Path.wstring());
        } else
            warn(L"Invalid or non-existent path (" + Utils::ToWString(options.user_path) + L") specified using --user-path, skipping");
    }

    if (const auto startupIniPath = std::filesystem::path(GetGameDir()) / "startup.ini"; exists(startupIniPath)) {

        if (std::ifstream startupIni(startupIniPath); startupIni.is_open()) {
            std::string contents((std::istreambuf_iterator(startupIni)), std::istreambuf_iterator<char>());
            startupIni.close();

            auto ini = Utils::ParseINI(contents);
            if (ini.empty()) 
                warn("Failed to parse startup.ini");
            else
                debug("Successfully parsed startup.ini");

            std::wstring userPath;
            if (ini.contains("filesystem") && std::get<std::map<std::string, std::string>>(ini["filesystem"]).contains("UserPath"))
                userPath = Utils::ToWString(std::get<std::map<std::string, std::string>>(ini["filesystem"])["UserPath"]);

            if (!userPath.empty() && Path.empty())
                if (userPath = Utils::ExpandEnvVars(userPath); std::filesystem::exists(userPath)) {
                    Path = userPath;
                    debug(L"Using custom user folder path from startup.ini: " + Path.wstring());
                } else
                    warn(L"Found custom user folder path (" + userPath + L") in startup.ini but it doesn't exist, skipping");
        }

        if (Path.empty()) {
            wchar_t* appDataPath = new wchar_t[MAX_PATH];
            HRESULT result = SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, appDataPath);

            if (!SUCCEEDED(result)) {
                fatal("Cannot get Local Appdata directory");
            }

            auto BeamNGAppdataPath = std::filesystem::path(appDataPath) / "BeamNG";

            if (const auto beamngIniPath = BeamNGAppdataPath / "BeamNG.Drive.ini"; exists(beamngIniPath)) {
                if (std::ifstream beamngIni(beamngIniPath); beamngIni.is_open()) {
                    std::string contents((std::istreambuf_iterator(beamngIni)), std::istreambuf_iterator<char>());
                    beamngIni.close();

                    auto ini = Utils::ParseINI(contents);
                    if (ini.empty())
                        warn("Failed to parse BeamNG.Drive.ini");
                    else
                        debug("Successfully parsed BeamNG.Drive.ini");

                    std::wstring userPath;

                    if (ini.contains("userFolder")) {
                        userPath = Utils::ToWString(std::get<std::string>(ini["userFolder"]));
                        userPath.erase(0, userPath.find_first_not_of(L" \t"));

                    }

                    if (!userPath.empty() && Path.empty())
                        if (userPath = std::filesystem::path(Utils::ExpandEnvVars(userPath)); std::filesystem::exists(userPath)) {
                            Path = userPath;
                            debug(L"Using custom user folder path from BeamNG.Drive.ini: " + Path.wstring());
                        } else
                            warn(L"Found custom user folder path (" + userPath + L") in BeamNG.Drive.ini but it doesn't exist, skipping");
                }
            }

            if (Path.empty()) {
                Path = BeamNGAppdataPath / "BeamNG.drive";
            }

            delete[] appDataPath;
        }
    }

    std::string Ver = CheckVer(GetGameDir());
    Ver = Ver.substr(0, Ver.find('.', Ver.find('.') + 1));
    Path /= Utils::ToWString("current");
    return Path;
}
#elif defined(__linux__)
std::filesystem::path GetGamePath() {
    // Right now only steam is supported
    struct passwd* pw = getpwuid(getuid());
    std::string homeDir = pw->pw_dir;

    std::string Path = homeDir + "/.local/share/BeamNG/BeamNG.drive/";
    std::string Ver = CheckVer(GetGameDir());
    Ver = Ver.substr(0, Ver.find('.', Ver.find('.') + 1));
    Path += "current/";
    return Path;
}
#endif

#if defined(_WIN32)
void StartGame(std::wstring Dir) {
    BOOL bSuccess = FALSE;
    PROCESS_INFORMATION pi;
    STARTUPINFOW si = { 0 };
    si.cb = sizeof(si);
    std::wstring BaseDir = Dir; //+"\\Bin64";
    // Dir += R"(\Bin64\BeamNG.drive.x64.exe)";
    Dir += L"\\BeamNG.drive.exe";
    std::wstring gameArgs = L"";

    for (int i = 0; i < options.game_arguments_length; i++) {
        gameArgs += L" ";
        gameArgs += Utils::ToWString(options.game_arguments[i]);
    }

    debug(L"BeamNG executable path: " + Dir);

    bSuccess = CreateProcessW(nullptr, (wchar_t*)(Dir + gameArgs).c_str(), nullptr, nullptr, TRUE, 0, nullptr, BaseDir.c_str(), &si, &pi);
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
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpErrorMsgBuffer, 0, nullptr)
            == 0) {
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

void InitGame(const beammp_fs_string& Dir) {
    if (!options.no_launch) {
        std::thread Game(StartGame, Dir);
        Game.detach();
    }
}
