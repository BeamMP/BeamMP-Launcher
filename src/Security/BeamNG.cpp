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
#elif defined(__linux__)
#include "vdf_parser.hpp"
#include <pwd.h>
#include <unistd.h>
#include <vector>
#endif
#include "Logger.h"
#include <fstream>
#include <sstream>
#include <string>
#include <thread>

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383

int TraceBack = 0;
std::string GameDir;

void lowExit(int code) {
    TraceBack = 0;
    std::string msg = "Failed to find the game please launch it. Report this if the issue persists code ";
    error(msg + std::to_string(code));
    std::this_thread::sleep_for(std::chrono::seconds(10));
    exit(2);
}
/*void Exit(int code){
    TraceBack = 0;
    std::string msg =
    "Sorry. We do not support cracked copies report this if you believe this is a mistake code ";
    error(msg+std::to_string(code));
    std::this_thread::sleep_for(std::chrono::seconds(10));
    exit(3);
}
void SteamExit(int code){
    TraceBack = 0;
    std::string msg =
    "Illegal steam modifications detected report this if you believe this is a mistake code ";
    error(msg+std::to_string(code));
    std::this_thread::sleep_for(std::chrono::seconds(10));
    exit(4);
}*/
std::string GetGameDir() {
// if(TraceBack != 4)Exit(0);
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
bool Find(const std::string& FName, const std::string& Path) {
    std::vector<std::string> FS;
    FileList(FS, Path + "\\userdata");
    for (std::string& a : FS) {
        if (a.find(FName) != std::string::npos) {
            FS.clear();
            return true;
        }
    }
    FS.clear();
    return false;
}
bool FindHack(const std::string& Path) {
    bool s = true;
    for (const auto& entry : fs::directory_iterator(Path)) {
        std::string Name = entry.path().filename().string();
        for (char& c : Name)
            c = char(tolower(c));
        if (Name == "steam.exe")
            s = false;
        if (Name.find("greenluma") != -1) {
            error("Found malicious file/folder \"" + Name + "\"");
            return true;
        }
        Name.clear();
    }
    return s;
}
std::vector<std::string> GetID(const std::string& log) {
    std::string vec, t, r;
    std::vector<std::string> Ret;
    std::ifstream f(log.c_str(), std::ios::binary);
    f.seekg(0, std::ios_base::end);
    std::streampos fileSize = f.tellg();
    vec.resize(size_t(fileSize) + 1);
    f.seekg(0, std::ios_base::beg);
    f.read(&vec[0], fileSize);
    f.close();
    std::stringstream ss(vec);
    bool S = false;
    while (std::getline(ss, t, '{')) {
        if (!S)
            S = true;
        else {
            for (char& c : t) {
                if (isdigit(c))
                    r += c;
            }
            break;
        }
    }
    Ret.emplace_back(r);
    r.clear();
    S = false;
    bool L = true;
    while (std::getline(ss, t, '}')) {
        if (L) {
            L = false;
            continue;
        }
        for (char& c : t) {
            if (c == '"') {
                if (!S)
                    S = true;
                else {
                    if (r.length() > 10) {
                        Ret.emplace_back(r);
                    }
                    r.clear();
                    S = false;
                    continue;
                }
            }
            if (isdigit(c))
                r += c;
        }
    }
    vec.clear();
    return Ret;
}
std::string GetManifest(const std::string& Man) {
    std::string vec;
    std::ifstream f(Man.c_str(), std::ios::binary);
    f.seekg(0, std::ios_base::end);
    std::streampos fileSize = f.tellg();
    vec.resize(size_t(fileSize) + 1);
    f.seekg(0, std::ios_base::beg);
    f.read(&vec[0], fileSize);
    f.close();
    std::string ToFind = "\"LastOwner\"\t\t\"";
    int pos = int(vec.find(ToFind));
    if (pos != -1) {
        pos += int(ToFind.length());
        vec = vec.substr(pos);
        return vec.substr(0, vec.find('\"'));
    } else
        return "";
}
bool IDCheck(std::string Man, std::string steam) {
    bool a = false, b = true;
    int pos = int(Man.rfind("steamapps"));
    //  if(pos == -1)Exit(5);
    Man = Man.substr(0, pos + 9) + "\\appmanifest_284160.acf";
    steam += "\\config\\loginusers.vdf";
    if (fs::exists(Man) && fs::exists(steam)) {
        for (const std::string& ID : GetID(steam)) {
            if (ID == GetManifest(Man))
                b = false;
        }
        // if(b)Exit(6);
    } else
        a = true;
    return a;
}
void LegitimacyCheck() {
#if defined(_WIN32)
    std::string Result;
    std::string K3 = R"(Software\BeamNG\BeamNG.drive)";
    HKEY hKey;
    LONG dwRegOPenKey = OpenKey(HKEY_CURRENT_USER, K3.c_str(), &hKey);
    if (dwRegOPenKey == ERROR_SUCCESS) {
        Result = QueryKey(hKey, 3);
        if (Result.empty())
            lowExit(3);
        GameDir = Result;
    } else
        lowExit(4);
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
#endif
}
std::string CheckVer(const std::string& dir) {
#if defined(_WIN32)
    std::string temp, Path = dir + "\\integrity.json";
#elif defined(__linux__)
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
