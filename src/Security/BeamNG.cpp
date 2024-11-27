// Copyright (c) 2019-present Anonymous275.
// BeamMP Launcher code is not in the public domain and is not free software.
// One must be granted explicit permission by the copyright holder in order to modify or distribute any part of the source or binaries.
// Anything else is prohibited. Modified works may not be published and have be upstreamed to the official repository.
///
/// Created by Anonymous275 on 7/18/2020
///

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
std::string GameDir;
std::string BottlePath;

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

#if defined(__APPLE__)

std::string GetBottlePath() {
    return BottlePath;
}

std::string GetBottleName() {
    std::filesystem::path bottlePath(BottlePath);
    return bottlePath.filename().string();
}

std::map<std::string, std::string> GetDriveMappings(const std::string& bottlePath) {
    std::map<std::string, std::string> driveMappings;
    std::string dosDevicesPath = bottlePath + "/dosdevices/";

    if (std::filesystem::exists(dosDevicesPath)) {
        for (const auto& entry : std::filesystem::directory_iterator(dosDevicesPath)) {
            if (entry.is_symlink()) {
                std::string driveName = Utils::ToLower(entry.path().filename().string());
                driveName = driveName.substr(0, 1);
                std::string macPath = std::filesystem::read_symlink(entry.path()).string();
                if (!std::filesystem::path(macPath).is_absolute()) {
                    macPath = dosDevicesPath + macPath;
                }
                driveMappings[driveName] = macPath;
            }
        }
    } else {
        error("Failed to find dosdevices directory for bottle '" + bottlePath + "'");
    }
    return driveMappings;
}

bool CheckForGame(const std::string& libraryPath, const std::map<std::string, std::string>& driveMappings) {
    //Convert the Windows path to Unix path
    char driveLetterChar = std::tolower(libraryPath[0]);
    std::string driveLetter(1, driveLetterChar);

    if (!driveMappings.contains(driveLetter)) {
        warn("Drive letter " + driveLetter + " not found in mappings.");
        return false;
    }

    std::filesystem::path basePath = driveMappings.at(driveLetter);
    debug("Base path: " + basePath.string());


    std::filesystem::path cleanLibraryPath = std::filesystem::path(libraryPath.substr(2)).make_preferred();
    std::string cleanPathStr = cleanLibraryPath.string();
    std::replace(cleanPathStr.begin(), cleanPathStr.end(), '\\', '/');
    cleanLibraryPath = std::filesystem::path(cleanPathStr);
    if (!cleanPathStr.empty() && cleanPathStr[0] == '/') {
        cleanPathStr.erase(0, 1);
    }
    cleanLibraryPath = std::filesystem::path(cleanPathStr);

    debug("Cleaned library path: " + cleanLibraryPath.string());

    fs::path beamngPath = basePath / cleanLibraryPath / "steamapps/common/BeamNG.drive";

    // Normalise the path
    beamngPath = beamngPath.lexically_normal();

    debug("Checking for BeamNG.drive at: " + beamngPath.string());

    if (fs::exists(beamngPath)) {
        GameDir = beamngPath.string();
        info("BeamNG.drive found at: " + GameDir);
        return true;
    }

    return false;
}

void ProcessBottle(const fs::path& bottlePath) {
    info("Checking bottle: " + bottlePath.filename().string());
    auto driveMappings = GetDriveMappings(bottlePath.string());

    const fs::path libraryFilePath = bottlePath / "drive_c/Program Files (x86)/Steam/config/libraryfolders.vdf";
    if (!fs::exists(libraryFilePath)) {
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
    std::string homeDir = pw->pw_dir;
    // Right now only steam is supported
    std::ifstream libraryFolders(homeDir + "/.steam/root/steamapps/libraryfolders.vdf");
    auto root = tyti::vdf::read(libraryFolders);

    for (auto folderInfo : root.childs) {
        if (std::filesystem::exists(folderInfo.second->attribs["path"] + "/steamapps/common/BeamNG.drive/")) {
            GameDir = folderInfo.second->attribs["path"] + "/steamapps/common/BeamNG.drive/";
            break;
        }
    }

#elif defined(__APPLE__)
    if (options.bottle_path.empty()) {
        const char* homeDir = getpwuid(getuid())->pw_dir;
        std::string crossoverBottlesPath;

        auto [output, status] = Utils::runCommand("defaults read com.codeweavers.CrossOver.plist BottleDir");
        if (status != 0) {
            fs::path defaultBottlesPath = fs::path(homeDir) / "Library/Application Support/CrossOver/Bottles";
            if (!fs::exists(defaultBottlesPath)) {
                error("Failed to detect CrossOver installation.");
                exit(1);
            }
            crossoverBottlesPath = defaultBottlesPath.string();
        } else {
            crossoverBottlesPath = output;
            crossoverBottlesPath.pop_back();
        }

        debug("Crossover bottles path: " + crossoverBottlesPath);

        if (options.bottle.empty()) {
            for (const auto& bottle : fs::directory_iterator(crossoverBottlesPath)) {
                if (bottle.is_directory()) {
                    ProcessBottle(bottle.path());
                    if (!GameDir.empty()) {
                        return;
                    }
                }
            }
        } else {
            fs::path bottlePath = crossoverBottlesPath + "/" + options.bottle;
            if (!fs::exists(bottlePath)) {
                error("Bottle does not exist: " + bottlePath.string());
                exit(1);
            }
            ProcessBottle(bottlePath);
            if (!GameDir.empty()) {
                return;
            }
        }
    } else {
        fs::path bottlePath = options.bottle_path;
        if (!fs::exists(bottlePath)) {
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
