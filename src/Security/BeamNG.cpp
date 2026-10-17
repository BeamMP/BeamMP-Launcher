/*
 Copyright (C) 2024 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/


#include <filesystem>
#include "Utils.h"
#if defined(_WIN32)
#include <shlobj_core.h>
#elif defined(__linux__)
#include "vdf_parser.hpp"
#include <stdexcept>
#include <pwd.h>
#include <unistd.h>
#include <vector>
#endif
#include "Logger.h"
#include <fstream>
#include <string>
#include <thread>

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383

int TraceBack = 0;
beammp_fs_string GameDir;

void lowExit(int code) {
    TraceBack = 0;
    std::string msg = "Failed to find the game please launch it. Report this if the issue persists code ";
    error(msg + std::to_string(code));
    std::this_thread::sleep_for(std::chrono::seconds(10));
    exit(2);
}

beammp_fs_string GetGameDir() {
#if defined(_WIN32)
    return GameDir.substr(0, GameDir.find_last_of('\\'));
#elif defined(__linux__)
    return GameDir.substr(0, GameDir.find_last_of('/'));
#endif
}
#ifdef _WIN32
LONG OpenKey(HKEY root, const char* path, PHKEY hKey) {
    return RegOpenKeyEx(root, reinterpret_cast<LPCSTR>(path), 0, KEY_READ, hKey);
}
std::wstring QueryKey(HKEY hKey, int ID) {
    wchar_t* achKey; // buffer for subkey name
    DWORD cbName; // size of name string
    TCHAR achClass[MAX_PATH] = TEXT(""); // buffer for class name
    DWORD cchClassName = MAX_PATH; // size of class string
    DWORD cSubKeys = 0; // number of subkeys
    DWORD cbMaxSubKey; // longest subkey size
    DWORD cchMaxClass; // longest class string
    DWORD cValues; // number of values for key
    DWORD cchMaxValue; // longest value name
    DWORD cbMaxValueData; // longest value data
    DWORD cbSecurityDescriptor; // size of security descriptor
    FILETIME ftLastWriteTime; // last write time

    DWORD i, retCode;

    wchar_t* achValue = new wchar_t[MAX_VALUE_NAME];
    DWORD cchValue = MAX_VALUE_NAME;

    retCode = RegQueryInfoKey(
        hKey, // key handle
        achClass, // buffer for class name
        &cchClassName, // size of class string
        nullptr, // reserved
        &cSubKeys, // number of subkeys
        &cbMaxSubKey, // longest subkey size
        &cchMaxClass, // longest class string
        &cValues, // number of values for this key
        &cchMaxValue, // longest value name
        &cbMaxValueData, // longest value data
        &cbSecurityDescriptor, // security descriptor
        &ftLastWriteTime); // last write time

    BYTE* buffer = new BYTE[cbMaxValueData];
    ZeroMemory(buffer, cbMaxValueData);
    if (cSubKeys) {
        for (i = 0; i < cSubKeys; i++) {
            cbName = MAX_KEY_LENGTH;
            retCode = RegEnumKeyExW(hKey, i, achKey, &cbName, nullptr, nullptr, nullptr, &ftLastWriteTime);
            if (retCode == ERROR_SUCCESS) {
                if (wcscmp(achKey, L"Steam App 284160") == 0) {
                    return achKey;
                }
            }
        }
    }
    if (cValues) {
        for (i = 0, retCode = ERROR_SUCCESS; i < cValues; i++) {
            cchValue = MAX_VALUE_NAME;
            achValue[0] = '\0';
            retCode = RegEnumValueW(hKey, i, achValue, &cchValue, nullptr, nullptr, nullptr, nullptr);
            if (retCode == ERROR_SUCCESS) {
                DWORD lpData = cbMaxValueData;
                buffer[0] = '\0';
                LONG dwRes = RegQueryValueExW(hKey, achValue, nullptr, nullptr, buffer, &lpData);
                std::wstring data = (wchar_t*)(buffer);
                std::wstring key = achValue;


                switch (ID) {
                case 1:
                    if (key == L"SteamExe") {
                        auto p = data.find_last_of(L"/\\");
                        if (p != std::string::npos) {
                            return data.substr(0, p);
                        }
                    }
                    break;
                case 2:
                    if (key == L"Name" && data == L"BeamNG.drive")
                        return data;
                    break;
                case 3:
                    if (key == L"rootpath")
                        return data;
                    break;
                case 4:
                    if (key == L"userpath_override")
                        return data;
                case 5:
                    if (key == L"Local AppData")
                        return data;
                default:
                    break;
                }
            }
        }
    }
    delete[] achValue;
    delete[] buffer;
    return L"";
}
#endif

namespace fs = std::filesystem;

bool NameValid(const std::string& N) {
    if (N == "config" || N == "librarycache") {
        return true;
    }
    if (N.find_first_not_of("0123456789") == std::string::npos) {
        return true;
    }
    return false;
}
void FileList(std::vector<std::string>& a, const std::string& Path) {
    for (const auto& entry : fs::directory_iterator(Path)) {
        const auto& DPath = entry.path();
        if (!entry.is_directory()) {
            a.emplace_back(DPath.string());
        } else if (NameValid(DPath.filename().string())) {
            FileList(a, DPath.string());
        }
    }
}
void LegitimacyCheck() {
#if defined(_WIN32)
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

            if (contents.size() >= 3 && (unsigned char)contents[0] == 0xEF && (unsigned char)contents[1] == 0xBB && (unsigned char)contents[2] == 0xBF) {
                contents = contents.substr(3);
            }

            auto ini = Utils::ParseINI(contents);
            if (ini.empty())
                lowExit(3);
            else
                debug("Successfully parsed BeamNG.Drive.ini");

            if (ini.contains("installPath")) {
                std::wstring installPath = Utils::ToWString(std::get<std::string>(ini["installPath"]));
                installPath.erase(0, installPath.find_first_not_of(L" \t"));

                if (installPath = std::filesystem::path(Utils::ExpandEnvVars(installPath)); std::filesystem::exists(installPath)) {
                    GameDir = installPath;
                    debug(L"GameDir from BeamNG.Drive.ini: " + installPath);
                } else {
                    lowExit(4);
                }
            } else {
                lowExit(5);
            }
        }
    } else {
        std::wstring Result;

        std::string K3 = R"(Software\BeamNG\BeamNG.drive)";

        HKEY hKey;

        LONG dwRegOPenKey = OpenKey(HKEY_CURRENT_USER, K3.c_str(), &hKey);
        if (dwRegOPenKey == ERROR_SUCCESS) {
            Result = QueryKey(hKey, 3);
            if (Result.empty()) {
                debug("Failed to QUERY key HKEY_CURRENT_USER\\Software\\BeamNG\\BeamNG.drive");
                lowExit(6);
            }
            GameDir = Result;
            debug(L"GameDir from registry: " + Result);
        } else {
            debug("Failed to OPEN key HKEY_CURRENT_USER\\Software\\BeamNG\\BeamNG.drive");
            lowExit(7);
        }
        K3.clear();
        Result.clear();
        RegCloseKey(hKey);
    }

    delete[] appDataPath;
#elif defined(__linux__)
    struct passwd* pw = getpwuid(getuid());
    std::filesystem::path homeDir = pw->pw_dir;

    // Right now only steam is supported. A machine can legitimately have more than one of these at
    // once (a native install alongside a Flatpak one, say) and the game may be registered in any of
    // them, so every one that exists is searched rather than only the first that happens to have a
    // libraryfolders.vdf.
    const std::vector<std::filesystem::path> steamLibraryRoots = {
        ".steam/root/steamapps", // default
        ".steam/steam/steamapps", // Legacy Steam installations
        ".local/share/Steam/steamapps", // native install without the ~/.steam symlinks
        ".var/app/com.valvesoftware.Steam/.steam/root/steamapps", // flatpak
        ".var/app/com.valvesoftware.Steam/.local/share/Steam/steamapps", // flatpak, direct
        "snap/steam/common/.local/share/Steam/steamapps" // snap
    };

    bool steamappsFolderFound = false;
    bool libraryFoldersFound = false;

    for (const auto& relative : steamLibraryRoots) {
        std::error_code ec;
        const std::filesystem::path steamappsPath = homeDir / relative;
        if (!std::filesystem::exists(steamappsPath, ec)) {
            continue;
        }
        steamappsFolderFound = true;

        const std::filesystem::path libraryFoldersPath = steamappsPath / "libraryfolders.vdf";
        if (!std::filesystem::exists(libraryFoldersPath, ec)) {
            continue;
        }
        libraryFoldersFound = true;
        debug("Searching Steam libraries listed in " + libraryFoldersPath.string());

        std::ifstream libraryFolders(libraryFoldersPath);
        auto root = tyti::vdf::read(libraryFolders);
        for (const auto& folderInfo : root.childs) {
            if (!folderInfo.second) {
                continue;
            }
            // `childs` maps to shared_ptr, so operator[] would INSERT a null one for a library that
            // has no "apps" block, and dereferencing that is undefined. Look the key up instead.
            const auto apps = folderInfo.second->childs.find("apps");
            if (apps == folderInfo.second->childs.end() || !apps->second) {
                continue;
            }
            if (!apps->second->attribs.contains("284160")) {
                continue;
            }
            const auto libraryPath = folderInfo.second->attribs.find("path");
            if (libraryPath == folderInfo.second->attribs.end()) {
                continue;
            }
            const std::string candidate = libraryPath->second + "/steamapps/common/BeamNG.drive/";
            if (std::filesystem::exists(candidate + "integrity.json", ec)) {
                GameDir = candidate;
                break;
            }
            debug("Steam library " + libraryPath->second + " lists app 284160, but " + candidate + "integrity.json is missing");
        }
        if (!GameDir.empty()) {
            break;
        }
    }

    // Reported by throwing rather than returning. LegitimacyCheck() is already called inside a
    // try/catch in main(), so this reaches the user as one clear line. Returning left GameDir
    // empty and the very next call -- PreGame(GetGameDir()) -> CheckVer() ->
    // std::filesystem::file_size() -- threw "cannot get file size: No such file or directory
    // [integrity.json]", which is the error users actually reported instead of the real cause.
    if (!steamappsFolderFound) {
        throw std::runtime_error("No Steam installation found: no steamapps folder under "
            + homeDir.string() + " in .steam/root, .steam/steam, .local/share/Steam, or the Flatpak/Snap locations.");
    }
    if (!libraryFoldersFound) {
        throw std::runtime_error("Found Steam, but none of its steamapps folders contain a libraryfolders.vdf.");
    }
    if (GameDir.empty()) {
        throw std::runtime_error("BeamNG.drive (Steam app 284160) was not found in any Steam library. If it is "
            "installed, check that its library is mounted and that the path recorded in libraryfolders.vdf "
            "matches the directory on disk.");
    }
#endif
}
std::string CheckVer(const std::filesystem::path& dir) {
    std::string temp;
    std::filesystem::path Path = dir / beammp_wide("integrity.json");
    std::ifstream f(Path.c_str(), std::ios::binary);
    int Size = int(std::filesystem::file_size(Path));
    std::string vec(Size, 0);
    f.read(&vec[0], Size);
    f.close();

    vec = vec.substr(vec.find_last_of("version"), vec.find_last_of('"'));
    for (const char& a : vec) {
        if (isdigit(a) || a == '.')
            temp += a;
    }
    return temp;
}