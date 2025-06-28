/*
 Copyright (C) 2024 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/


#include <filesystem>
#if defined(_WIN32)
#elif defined(__linux__)
#include "vdf_parser.hpp"
#include <pwd.h>
#include <unistd.h>
#include <vector>
#endif
#include "Logger.h"
#include "Utils.h"

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
    std::wstring Result;
    std::string K3 = R"(Software\BeamNG\BeamNG.drive)";
    HKEY hKey;
    LONG dwRegOPenKey = OpenKey(HKEY_CURRENT_USER, K3.c_str(), &hKey);
    if (dwRegOPenKey == ERROR_SUCCESS) {
        Result = QueryKey(hKey, 3);
        if (Result.empty()) {
            debug("Failed to QUERY key HKEY_CURRENT_USER\\Software\\BeamNG\\BeamNG.drive");
            lowExit(3);
        }
        GameDir = Result;
    } else {
        debug("Failed to OPEN key HKEY_CURRENT_USER\\Software\\BeamNG\\BeamNG.drive");
        lowExit(4);
    }
    K3.clear();
    Result.clear();
    RegCloseKey(hKey);
#elif defined(__linux__)
    struct passwd* pw = getpwuid(getuid());
    std::filesystem::path homeDir = pw->pw_dir;

    // Right now only steam is supported
    std::vector<std::filesystem::path> steamappsCommonPaths = {
        ".steam/root/steamapps", // default
        ".var/app/com.valvesoftware.Steam/.steam/root/steamapps", // flatpak
        "snap/steam/common/.local/share/Steam/steamapps" // snap
    };

    std::filesystem::path steamappsPath;
    std::filesystem::path libraryFoldersPath;
    bool steamappsFolderFound = false;
    bool libraryFoldersFound = false;

    for (const auto& path : steamappsCommonPaths) {
        steamappsPath = homeDir / path;
        if (std::filesystem::exists(steamappsPath)) {
            steamappsFolderFound = true;
            libraryFoldersPath = steamappsPath / "libraryfolders.vdf";
            if (std::filesystem::exists(libraryFoldersPath)) {
                libraryFoldersPath = libraryFoldersPath;
                libraryFoldersFound = true;
                break;
            }
        }
    }

    if (!steamappsFolderFound) {
        error("Unsupported Steam installation.");
        return;
    }
    if (!libraryFoldersFound) {
        error("libraryfolders.vdf is missing.");
        return;
    }

    std::ifstream libraryFolders(libraryFoldersPath);
    auto root = tyti::vdf::read(libraryFolders);
    for (auto folderInfo : root.childs) {
        if (std::filesystem::exists(folderInfo.second->attribs["path"] + "/steamapps/common/BeamNG.drive/integrity.json")){
            GameDir = folderInfo.second->attribs["path"] + "/steamapps/common/BeamNG.drive/";
            break;
        }
    }
    if (GameDir.empty()) {
        error("The game directory was not found.");
        return;
    }
#endif
}
std::string CheckVer(const beammp_fs_string& dir) {
    std::string temp;
    beammp_fs_string Path = dir + beammp_wide("\\integrity.json");
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