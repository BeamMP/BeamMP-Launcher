// Copyright (c) 2019-present Anonymous275.
// BeamMP Launcher code is not in the public domain and is not free software.
// One must be granted explicit permission by the copyright holder in order to modify or distribute any part of the source or binaries.
// Anything else is prohibited. Modified works may not be published and have be upstreamed to the official repository.
///
/// Created by Anonymous275 on 4/11/2020
///

#include "Network/network.hpp"
#include <chrono>
#include <mutex>

#if defined(_WIN32)
#include <ws2tcpip.h>
#elif defined(__linux__)
#include <arpa/inet.h>
#include <cstring>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#endif

#include "Logger.h"
#include "Startup.h"
#include <atomic>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <thread>
#include <Utils.h>

namespace fs = std::filesystem;

void CheckForDir() {
    if (!fs::exists("Resources")) {
// Could we just use fs::create_directory instead?
#if defined(_WIN32)
        _wmkdir(L"Resources");
#elif defined(__linux__)
        fs::create_directory(L"Resources");
#endif
    }
}
void WaitForConfirm() {
    while (!Terminate && !ModLoaded) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ModLoaded = false;
}

void Abord() {
    Terminate = true;
    TCPTerminate = true;
    info("Terminated!");
}

std::string Auth(SOCKET Sock) {
    TCPSend("VC" + GetVer(), Sock);

    auto Res = TCPRcv(Sock);

    if (Res.empty() || Res[0] == 'E' || Res[0] == 'K') {
        Abord();
        CoreSend("L");
        return "";
    }

    TCPSend(PublicKey, Sock);
    if (Terminate) {
        CoreSend("L");
        return "";
    }

    Res = TCPRcv(Sock);
    if (Res.empty() || Res[0] != 'P') {
        Abord();
        CoreSend("L");
        return "";
    }

    Res = Res.substr(1);
    if (Res.find_first_not_of("0123456789") == std::string::npos) {
        ClientID = std::stoi(Res);
    } else {
        Abord();
        CoreSend("L");
        UUl("Authentication failed!");
        return "";
    }
    TCPSend("SR", Sock);
    if (Terminate) {
        CoreSend("L");
        return "";
    }

    Res = TCPRcv(Sock);

    if (Res[0] == 'E' || Res[0] == 'K') {
        Abord();
        CoreSend("L");
        return "";
    }

    if (Res.empty() || Res == "-") {
        info("Didn't Receive any mods...");
        CoreSend("L");
        TCPSend("Done", Sock);
        info("Done!");
        return "";
    }
    return Res;
}

void UpdateUl(bool D, const std::string& msg) {
    if (D)
        UlStatus = "UlDownloading Resource " + msg;
    else
        UlStatus = "UlLoading Resource " + msg;
}

void AsyncUpdate(uint64_t& Rcv, uint64_t Size, const std::string& Name) {
    do {
        double pr = double(Rcv) / double(Size) * 100;
        std::string Per = std::to_string(trunc(pr * 10) / 10);
        UpdateUl(true, Name + " (" + Per.substr(0, Per.find('.') + 2) + "%)");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } while (!Terminate && Rcv < Size);
}

// MICROSOFT, I DONT CARE, WRITE BETTER CODE
#undef min

std::vector<char> TCPRcvRaw(SOCKET Sock, uint64_t& GRcv, uint64_t Size) {
    if (Sock == -1) {
        Terminate = true;
        UUl("Invalid Socket");
        return {};
    }
    std::vector<char> File(Size);
    uint64_t Rcv = 0;

    int i = 0;
    do {
        auto start = std::chrono::high_resolution_clock::now();

        // receive at most some MB at a time
        int Len = std::min(int(Size - Rcv), 2 * 1024 * 1024);
        int32_t Temp = recv(Sock, &File[Rcv], Len, MSG_WAITALL);
        if (Temp < 1) {
            info(std::to_string(Temp));
            UUl("Socket Closed Code 1");
            KillSocket(Sock);
            Terminate = true;
            return {};
        }
        Rcv += Temp;
        GRcv += Temp;

        // every 8th iteration calculate download speed for that iteration
        if (i % 8 == 0) {
            auto end = std::chrono::high_resolution_clock::now();
            auto difference = end - start;
            float bits_per_s = float(Temp * 8) / float(std::chrono::duration_cast<std::chrono::milliseconds>(difference).count());
            float megabits_per_s = bits_per_s / 1000;
            debug("Download speed: " + std::to_string(uint32_t(megabits_per_s)) + "Mbit/s");
        }
        ++i;
    } while (Rcv < Size && !Terminate);
    return File;
}
void MultiKill(SOCKET Sock, SOCKET Sock1) {
    KillSocket(Sock1);
    KillSocket(Sock);
    Terminate = true;
}
SOCKET InitDSock() {
    SOCKET DSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    SOCKADDR_IN ServerAddr;
    if (DSock < 1) {
        KillSocket(DSock);
        Terminate = true;
        return 0;
    }
    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_port = htons(LastPort);
    inet_pton(AF_INET, LastIP.c_str(), &ServerAddr.sin_addr);
    if (connect(DSock, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr)) != 0) {
        KillSocket(DSock);
        Terminate = true;
        return 0;
    }
    char Code[2] = { 'D', char(ClientID) };
    if (send(DSock, Code, 2, 0) != 2) {
        KillSocket(DSock);
        Terminate = true;
        return 0;
    }
    return DSock;
}

std::vector<char> MultiDownload(SOCKET MSock, SOCKET DSock, uint64_t Size, const std::string& Name) {
    uint64_t GRcv = 0;

    uint64_t MSize = Size / 2;
    uint64_t DSize = Size - MSize;

    std::thread Au([&] { AsyncUpdate(GRcv, Size, Name); });

    const std::vector<char> MData = TCPRcvRaw(MSock, GRcv, MSize);

    if (MData.empty()) {
        MultiKill(MSock, DSock);
        return {};
    }

    const std::vector<char> DData = TCPRcvRaw(DSock, GRcv, DSize);

    if (DData.empty()) {
        MultiKill(MSock, DSock);
        return {};
    }

    // ensure that GRcv is good before joining the async update thread
    GRcv = MData.size() + DData.size();
    if (GRcv != Size) {
        error("Something went wrong during download; didn't get enough data. Expected " + std::to_string(Size) + " bytes, got " + std::to_string(GRcv) + " bytes instead");
        return {};
    }

    Au.join();

    std::vector<char> Result{};
    Result.insert(Result.begin(), MData.begin(), MData.end());
    Result.insert(Result.end(), DData.begin(), DData.end());
    return Result;
}

void InvalidResource(const std::string& File) {
    UUl("Invalid mod \"" + File + "\"");
    warn("The server tried to sync \"" + File + "\" that is not a .zip file!");
    Terminate = true;
}

void SyncResources(SOCKET Sock) {
    std::string Ret = Auth(Sock);
    if (Ret.empty())
        return;

    if (!SecurityWarning())
		return;

    info("Checking Resources...");
    CheckForDir();

    std::vector<std::string> list = Utils::Split(Ret, ";");
    std::vector<std::string> FNames(list.begin(), list.begin() + (list.size() / 2));
    std::vector<std::string> FSizes(list.begin() + (list.size() / 2), list.end());
    list.clear();
    Ret.clear();

    int Amount = 0, Pos = 0;
    std::string PathToSaveTo, t;
    for (const std::string& name : FNames) {
        if (!name.empty()) {
            t += name.substr(name.find_last_of('/') + 1) + ";";
        }
    }
    if (t.empty())
        CoreSend("L");
    else
        CoreSend("L" + t);
    t.clear();
    for (auto FN = FNames.begin(), FS = FSizes.begin(); FN != FNames.end() && !Terminate; ++FN, ++FS) {
        auto pos = FN->find_last_of('/');
        auto ZIP = FN->find(".zip");
        if (ZIP == std::string::npos || FN->length() - ZIP != 4) {
            InvalidResource(*FN);
            return;
        }
        if (pos == std::string::npos)
            continue;
        Amount++;
    }
    if (!FNames.empty())
        info("Syncing...");
    SOCKET DSock = InitDSock();
    for (auto FN = FNames.begin(), FS = FSizes.begin(); FN != FNames.end() && !Terminate; ++FN, ++FS) {
        auto pos = FN->find_last_of('/');
        if (pos != std::string::npos) {
            PathToSaveTo = "Resources" + FN->substr(pos);
        } else {
            continue;
        }
        Pos++;
        auto FileSize = std::stoull(*FS);
        if (fs::exists(PathToSaveTo)) {
            if (FS->find_first_not_of("0123456789") != std::string::npos)
                continue;
            if (fs::file_size(PathToSaveTo) == FileSize) {
                UpdateUl(false, std::to_string(Pos) + "/" + std::to_string(Amount) + ": " + PathToSaveTo.substr(PathToSaveTo.find_last_of('/')));
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                try {
                    if (!fs::exists(GetGamePath() + "mods/multiplayer")) {
                        fs::create_directories(GetGamePath() + "mods/multiplayer");
                    }
                    auto modname = PathToSaveTo.substr(PathToSaveTo.find_last_of('/'));
#if defined(__linux__)
                    // Linux version of the game doesnt support uppercase letters in mod names
                    for (char& c : modname) {
                        c = ::tolower(c);
                    }
#endif
                    auto name = GetGamePath() + "mods/multiplayer" + modname;
                    auto tmp_name = name + ".tmp";
                    fs::copy_file(PathToSaveTo, tmp_name, fs::copy_options::overwrite_existing);
                    fs::rename(tmp_name, name);
                } catch (std::exception& e) {
                    error("Failed copy to the mods folder! " + std::string(e.what()));
                    Terminate = true;
                    continue;
                }
                WaitForConfirm();
                continue;
            } else
                remove(PathToSaveTo.c_str());
        }
        CheckForDir();
        std::string FName = PathToSaveTo.substr(PathToSaveTo.find_last_of('/'));
        do {
            debug("Loading file '" + FName + "' to '" + PathToSaveTo + "'");
            TCPSend("f" + *FN, Sock);

            std::string Data = TCPRcv(Sock);
            if (Data == "CO" || Terminate) {
                Terminate = true;
                UUl("Server cannot find " + FName);
                break;
            }

            std::string Name = std::to_string(Pos) + "/" + std::to_string(Amount) + ": " + FName;

            std::vector<char> DownloadedFile = MultiDownload(Sock, DSock, FileSize, Name);

            if (Terminate)
                break;
            UpdateUl(false, std::to_string(Pos) + "/" + std::to_string(Amount) + ": " + FName);

            // 1. write downloaded file to disk
            {
                std::ofstream OutFile(PathToSaveTo, std::ios::binary | std::ios::trunc);
                OutFile.write(DownloadedFile.data(), DownloadedFile.size());
            }
            // 2. verify size
            if (std::filesystem::file_size(PathToSaveTo) != DownloadedFile.size()) {
                error("Failed to write the entire file '" + PathToSaveTo + "' correctly (file size mismatch)");
                Terminate = true;
            }
        } while (fs::file_size(PathToSaveTo) != std::stoull(*FS) && !Terminate);
        if (!Terminate) {
            if (!fs::exists(GetGamePath() + "mods/multiplayer")) {
                fs::create_directories(GetGamePath() + "mods/multiplayer");
            }

// Linux version of the game doesnt support uppercase letters in mod names
#if defined(__linux__)
            for (char& c : FName) {
                c = ::tolower(c);
            }
#endif

            fs::copy_file(PathToSaveTo, GetGamePath() + "mods/multiplayer" + FName, fs::copy_options::overwrite_existing);
        }
        WaitForConfirm();
    }
    KillSocket(DSock);
    if (!Terminate) {
        TCPSend("Done", Sock);
        info("Done!");
    } else {
        UlStatus = "Ulstart";
        info("Connection Terminated!");
    }
}
