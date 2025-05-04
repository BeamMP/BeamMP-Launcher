/*
 Copyright (C) 2024 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/


#include <filesystem>
#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__) || defined(__APPLE__)
#include "vdf_parser.hpp"
#include <pwd.h>
#include <unistd.h>
#include <vector>
#endif
#include "Utils.h"
#include "Options.h"
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <algorithm>
#include "Logger.h"
#include <fstream>
#include <string>
#include <thread>

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383

int TraceBack = 0;
#if defined(__APPLE__)
    std::filesystem::path GameDir;
#else
    std::string GameDir;
#endif

std::filesystem::path BottlePath;

void lowExit(int code) {
    TraceBack = 0;
    std::string msg = "Failed to find the game please launch it. Report this if the issue persists code ";
    error(msg + std::to_string(code));
    std::this_thread::sleep_for(std::chrono::seconds(10));
    exit(2);
}

std::string GetGameDir() {
#if defined(_WIN32)
    return GameDir.substr(0, GameDir.find_last_of('\\'));
#elif defined(__linux__)
    return GameDir.substr(0, GameDir.find_last_of('/'));
#elif defined(__APPLE__)
    return GameDir;
#endif
}
#ifdef _WIN32
LONG OpenKey(HKEY root, const char* path, PHKEY hKey) {
    return RegOpenKeyEx(root, reinterpret_cast<LPCSTR>(path), 0, KEY_READ, hKey);
}
std::string QueryKey(HKEY hKey, int ID) {
    TCHAR achKey[MAX_KEY_LENGTH]; // buffer for subkey name
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

    TCHAR achValue[MAX_VALUE_NAME];
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
            retCode = RegEnumKeyEx(hKey, i, achKey, &cbName, nullptr, nullptr, nullptr, &ftLastWriteTime);
            if (retCode == ERROR_SUCCESS) {
                if (strcmp(achKey, "Steam App 284160") == 0) {
                    return achKey;
                }
            }
        }
    }
    if (cValues) {
        for (i = 0, retCode = ERROR_SUCCESS; i < cValues; i++) {
            cchValue = MAX_VALUE_NAME;
            achValue[0] = '\0';
            retCode = RegEnumValue(hKey, i, achValue, &cchValue, nullptr, nullptr, nullptr, nullptr);
            if (retCode == ERROR_SUCCESS) {
                DWORD lpData = cbMaxValueData;
                buffer[0] = '\0';
                LONG dwRes = RegQueryValueEx(hKey, achValue, nullptr, nullptr, buffer, &lpData);
                std::string data = (char*)(buffer);
                std::string key = achValue;

                switch (ID) {
                case 1:
                    if (key == "SteamExe") {
                        auto p = data.find_last_of("/\\");
                        if (p != std::string::npos) {
                            return data.substr(0, p);
                        }
                    }
                    break;
                case 2:
                    if (key == "Name" && data == "BeamNG.drive")
                        return data;
                    break;
                case 3:
                    if (key == "rootpath")
                        return data;
                    break;
                case 4:
                    if (key == "userpath_override")
                        return data;
                case 5:
                    if (key == "Local AppData")
                        return data;
                default:
                    break;
                }
            }
        }
    }
    delete[] buffer;
    return "";
}
#endif

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
    for (const auto& entry : std::filesystem::directory_iterator(Path)) {
        const auto& DPath = entry.path();
        if (!entry.is_directory()) {
            a.emplace_back(DPath.string());
        } else if (NameValid(DPath.filename().string())) {
            FileList(a, DPath.string());
        }
    }
}

#if defined(__APPLE__)

std::string GetBottlePath() {
    return BottlePath;
}

std::string GetBottleName() {
    std::filesystem::path bottlePath(BottlePath);
    return bottlePath.filename().string();
}

std::map<char, std::filesystem::path> GetDriveMappings(const std::filesystem::path& bottlePath) {
    std::map<char, std::filesystem::path> driveMappings;
    std::filesystem::path dosDevicesPath = bottlePath / "dosdevices/";

    if (std::filesystem::exists(dosDevicesPath)) {
        for (const auto& entry : std::filesystem::directory_iterator(dosDevicesPath)) {
            if (entry.is_symlink()) {
                char driveLetter = entry.path().filename().string()[0];
                driveLetter = std::tolower(driveLetter);
                std::string macPath = std::filesystem::read_symlink(entry.path()).string();
                if (!std::filesystem::path(macPath).is_absolute()) {
                    macPath = dosDevicesPath / macPath;
                }
                driveMappings[driveLetter] = macPath;

            }
        }
    } else {
        error("Failed to find dosdevices directory for bottle '" + bottlePath.string() + "'");
    }
    return driveMappings;
}

bool CheckForGame(const std::string& libraryPath, const std::map<char, std::filesystem::path>& driveMappings) {
    char driveLetter = std::tolower(libraryPath[0]);

    if (!driveMappings.contains(driveLetter)) {
        warn(std::string("Drive letter ") + driveLetter + " not found in mappings.");
        return false;
    }

    std::filesystem::path basePath = driveMappings.at(driveLetter);
    debug("Base path: " + basePath.string());

    std::string cleanPathStr = libraryPath.substr(2);
    std::replace(cleanPathStr.begin(), cleanPathStr.end(), '\\', '/');
    if (!cleanPathStr.empty() ){
        if (cleanPathStr[0] == '/') {
            cleanPathStr.erase(0, 1);
        }
        if (cleanPathStr.back() == '/') {
            cleanPathStr.pop_back();
        }
    }
    std::filesystem::path cleanLibraryPath = std::filesystem::path(cleanPathStr);


    debug("Cleaned library path: " + cleanLibraryPath.string());

    std::filesystem::path beamngPath = basePath / cleanLibraryPath / "steamapps/common/BeamNG.drive";

    beamngPath = beamngPath.lexically_normal();

    debug("Checking for BeamNG.drive at: " + beamngPath.string());

    if (std::filesystem::exists(beamngPath)) {
        GameDir = beamngPath;   
        info("BeamNG.drive found at: " + GameDir.string());
        return true;
    }


    return false;
}

void ProcessBottle(const std::filesystem::path& bottlePath) {
    info("Checking bottle: " + bottlePath.filename().string());
    auto driveMappings = GetDriveMappings(bottlePath);

    const std::filesystem::path libraryFilePath = bottlePath / "drive_c/Program Files (x86)/Steam/config/libraryfolders.vdf";
    if (!std::filesystem::exists(libraryFilePath)) {
        warn("Library file not found in bottle: " + bottlePath.filename().string());
        return;
    }

    std::ifstream libraryFile(libraryFilePath);
    if (!libraryFile.is_open()) {
        error("Failed to open libraryfolders.vdf in bottle: " + bottlePath.filename().string());
        return;
    }

    auto root = tyti::vdf::read(libraryFile);
    libraryFile.close();

    for (const auto& [key, folderInfo] : root.childs) {
        if (folderInfo->attribs.contains("path") && 
            CheckForGame(folderInfo->attribs.at("path"), driveMappings)) {
            BottlePath = bottlePath.string();
            return;
        }
    }
}
#endif

void LegitimacyCheck() {
#if defined(_WIN32)
    std::string Result;
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

#elif defined(__APPLE__)
    if (options.bottle_path.empty()) {
        const char* homeDir = getpwuid(getuid())->pw_dir;
        std::filesystem::path crossoverBottlesPath;

        auto [output, status] = Utils::runCommand("defaults read com.codeweavers.CrossOver.plist BottleDir");

        if (status != 0) {
            std::filesystem::path defaultBottlesPath = std::filesystem::path(homeDir) / "Library/Application Support/CrossOver/Bottles";
            
            defaultBottlesPath = std::filesystem::canonical(defaultBottlesPath);
            
            if (!std::filesystem::exists(defaultBottlesPath)) {                
                error("Failed to detect CrossOver installation, make sure you have installed it and have a bottle created.");
                std::this_thread::sleep_for(std::chrono::seconds(5));
                exit(1);
            }
            crossoverBottlesPath = defaultBottlesPath;
        } else {
            crossoverBottlesPath = std::filesystem::path(Utils::Trim(output));
        }

        if (options.bottle.empty()) {
            debug("Checking all bottles in: " + crossoverBottlesPath.string());
            //vérifier que le répertoire existe avant de continuer
            try {
                if (!std::filesystem::exists(crossoverBottlesPath) || !std::filesystem::is_directory(crossoverBottlesPath)) {
                    error("Chemin des bouteilles invalide: " + crossoverBottlesPath.string() + 
                        "\nExiste: " + std::to_string(std::filesystem::exists(crossoverBottlesPath)) +
                        "\nEst un dossier: " + std::to_string(std::filesystem::is_directory(crossoverBottlesPath)));
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                    exit(1);
                }

            } catch (const std::filesystem::filesystem_error& e) {
                error("Erreur d'accès: " + std::string(e.what()));
                std::this_thread::sleep_for(std::chrono::seconds(5));
                exit(1);
            }
            for (const auto& bottle : std::filesystem::directory_iterator(crossoverBottlesPath)) {
                debug("Checking bottle: " + bottle.path().filename().string());
                if (bottle.is_directory()) {

                    ProcessBottle(bottle);
                    if (!GameDir.empty()) {
                        return;
                    }
                }

            }
        } else {
            std::filesystem::path bottlePath = crossoverBottlesPath / options.bottle;
            debug("Checking bottle: " + bottlePath.string());
            if (!std::filesystem::exists(bottlePath)) {
                error("Bottle does not exist: " + bottlePath.string());
                exit(1);
            }
            ProcessBottle(bottlePath);
            if (!GameDir.empty()) {
                return;
            }
        }
    } else {
        std::filesystem::path bottlePath = options.bottle_path;
        if (!std::filesystem::exists(bottlePath)) {
            error("Bottle path does not exist: " + bottlePath.string());
            exit(1);
        }
        ProcessBottle(bottlePath);
        if (!GameDir.empty()) {
            return;
        }
    }

    error("Failed to find BeamNG.drive installation in any CrossOver bottle. Make sure BeamNG.drive is installed in a CrossOver bottle, or set it with the --bottle or --bottle-path argument.");
    exit(1);

#endif
}
std::string CheckVer(const std::string& dir) {
#if defined(_WIN32)
    std::string temp, Path = dir + "\\integrity.json";
#elif defined(__linux__) || defined(__APPLE__)
    std::string temp, Path = dir + "/integrity.json";
#endif
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