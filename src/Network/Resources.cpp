// Copyright (c) 2019-present Anonymous275.
// BeamMP Launcher code is not in the public domain and is not free software.
// One must be granted explicit permission by the copyright holder in order to modify or distribute any part of the source or binaries.
// Anything else is prohibited. Modified works may not be published and have be upstreamed to the official repository.
///
/// Created by Anonymous275 on 4/11/2020
///

#include "Network/network.hpp"
#include "fmt/core.h"

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
#include <asio.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;
std::string ListOfMods;
std::vector<std::string> Split(const std::string& String, const std::string& delimiter) {
    std::vector<std::string> Val;
    size_t pos;
    std::string token, s = String;
    while ((pos = s.find(delimiter)) != std::string::npos) {
        token = s.substr(0, pos);
        if (!token.empty())
            Val.push_back(token);
        s.erase(0, pos + delimiter.length());
    }
    if (!s.empty())
        Val.push_back(s);
    return Val;
}

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

std::string Auth(asio::ip::tcp::socket& Sock) {
    TCPSend(strtovec("VC" + GetVer()), Sock);

    auto Res = TCPRcv(Sock);

    if (Res.empty() || Res[0] == 'E' || Res[0] == 'K') {
        Abord();
        return "";
    }

    TCPSend(strtovec(PublicKey), Sock);
    if (Terminate)
        return "";

    Res = TCPRcv(Sock);
    if (Res.empty() || Res[0] != 'P') {
        Abord();
        return "";
    }

    Res = Res.substr(1);
    if (Res.find_first_not_of("0123456789") == std::string::npos) {
        ClientID = std::stoi(Res);
    } else {
        Abord();
        UUl("Authentication failed!");
        return "";
    }
    TCPSend(strtovec("SR"), Sock);
    if (Terminate)
        return "";

    Res = TCPRcv(Sock);

    if (Res[0] == 'E' || Res[0] == 'K') {
        Abord();
        return "";
    }

    if (Res.empty() || Res == "-") {
        info("Didn't Receive any mods...");
        ListOfMods = "-";
        TCPSend(strtovec("Done"), Sock);
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

char* TCPRcvRaw(asio::ip::tcp::socket& Sock, uint64_t& GRcv, uint64_t Size) {
    char* File = new char[Size];
    uint64_t Rcv = 0;
    asio::error_code ec;
    do {
        int Len = int(Size - Rcv);
        if (Len > 1000000)
            Len = 1000000;
        int32_t Temp = asio::read(Sock, asio::buffer(&File[Rcv], Len), ec);
        if (ec) {
            ::error(fmt::format("Failed to receive data from server: {}", ec.message()));
            UUl("Failed to receive data from server, connection closed (Code 1)");
            KillSocket(Sock);
            Terminate = true;
            delete[] File;
            return nullptr;
        }
        Rcv += Temp;
        GRcv += Temp;
    } while (Rcv < Size && !Terminate);
    return File;
}
void MultiKill(asio::ip::tcp::socket& Sock, asio::ip::tcp::socket& Sock1) {
    KillSocket(Sock1);
    KillSocket(Sock);
    Terminate = true;
}
std::shared_ptr<asio::ip::tcp::socket> InitDSock(asio::ip::tcp::endpoint ep) {
    auto DSock = std::make_shared<asio::ip::tcp::socket>(io);
    asio::error_code ec;
    DSock->connect(ep, ec);
    if (ec) {
        KillSocket(DSock);
        Terminate = true;
        return nullptr;
    }
    char Code[2] = { 'D', char(ClientID) };
    asio::write(*DSock, asio::buffer(Code, 2), ec);
    if (ec) {
        KillSocket(DSock);
        Terminate = true;
        return 0;
    }
    return DSock;
}

std::string MultiDownload(asio::ip::tcp::socket& MSock, asio::ip::tcp::socket& DSock, uint64_t Size, const std::string& Name) {

    uint64_t GRcv = 0, MSize = Size / 2, DSize = Size - MSize;

    std::thread Au(AsyncUpdate, std::ref(GRcv), Size, Name);

    std::packaged_task<char*()> task([&] { return TCPRcvRaw(MSock, GRcv, MSize); });
    std::future<char*> f1 = task.get_future();
    std::thread Dt(std::move(task));
    Dt.detach();

    char* DData = TCPRcvRaw(DSock, GRcv, DSize);

    if (!DData) {
        MultiKill(MSock, DSock);
        return "";
    }

    f1.wait();
    char* MData = f1.get();

    if (!MData) {
        MultiKill(MSock, DSock);
        return "";
    }

    if (Au.joinable())
        Au.join();

    /// omg yes very ugly my god but i was in a rush will revisit
    std::string Ret(Size, 0);
    memcpy(&Ret[0], MData, MSize);
    delete[] MData;

    memcpy(&Ret[MSize], DData, DSize);
    delete[] DData;

    return Ret;
}

void InvalidResource(const std::string& File) {
    UUl("Invalid mod \"" + File + "\"");
    warn("The server tried to sync \"" + File + "\" that is not a .zip file!");
    Terminate = true;
}

void SyncResources(asio::ip::tcp::socket& Sock) {
    std::string Ret = Auth(Sock);
    if (Ret.empty())
        return;

    info("Checking Resources...");
    CheckForDir();

    std::vector<std::string> list = Split(Ret, ";");
    std::vector<std::string> FNames(list.begin(), list.begin() + (list.size() / 2));
    std::vector<std::string> FSizes(list.begin() + (list.size() / 2), list.end());
    list.clear();
    Ret.clear();

    int Amount = 0, Pos = 0;
    std::string a, t;
    for (const std::string& name : FNames) {
        if (!name.empty()) {
            t += name.substr(name.find_last_of('/') + 1) + ";";
        }
    }
    if (t.empty())
        ListOfMods = "-";
    else
        ListOfMods = t;
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
    auto DSock = InitDSock(Sock.remote_endpoint());
    for (auto FN = FNames.begin(), FS = FSizes.begin(); FN != FNames.end() && !Terminate; ++FN, ++FS) {
        auto pos = FN->find_last_of('/');
        if (pos != std::string::npos) {
            a = "Resources" + FN->substr(pos);
        } else
            continue;
        Pos++;
        if (fs::exists(a)) {
            if (FS->find_first_not_of("0123456789") != std::string::npos)
                continue;
            if (fs::file_size(a) == std::stoull(*FS)) {
                UpdateUl(false, std::to_string(Pos) + "/" + std::to_string(Amount) + ": " + a.substr(a.find_last_of('/')));
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                try {
                    if (!fs::exists(GetGamePath() + "mods/multiplayer")) {
                        fs::create_directories(GetGamePath() + "mods/multiplayer");
                    }
                    auto modname = a.substr(a.find_last_of('/'));
#if defined(__linux__)
                    // Linux version of the game doesnt support uppercase letters in mod names
                    for (char& c : modname) {
                        c = ::tolower(c);
                    }
#endif
                    auto name = GetGamePath() + "mods/multiplayer" + modname;
                    auto tmp_name = name + ".tmp";
                    fs::copy_file(a, tmp_name, fs::copy_options::overwrite_existing);
                    fs::rename(tmp_name, name);
                } catch (std::exception& e) {
                    error("Failed copy to the mods folder! " + std::string(e.what()));
                    Terminate = true;
                    continue;
                }
                WaitForConfirm();
                continue;
            } else
                remove(a.c_str());
        }
        CheckForDir();
        std::string FName = a.substr(a.find_last_of('/'));
        do {
            TCPSend(strtovec("f" + *FN), Sock);

            std::string Data = TCPRcv(Sock);
            if (Data == "CO" || Terminate) {
                Terminate = true;
                UUl("Server cannot find " + FName);
                break;
            }

            std::string Name = std::to_string(Pos) + "/" + std::to_string(Amount) + ": " + FName;

            Data = MultiDownload(Sock, *DSock, std::stoull(*FS), Name);

            if (Terminate)
                break;
            UpdateUl(false, std::to_string(Pos) + "/" + std::to_string(Amount) + ": " + FName);
            std::ofstream LFS;
            LFS.open(a.c_str(), std::ios_base::app | std::ios::binary);
            if (LFS.is_open()) {
                LFS.write(&Data[0], Data.size());
                LFS.close();
            }

        } while (fs::file_size(a) != std::stoull(*FS) && !Terminate);
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

            fs::copy_file(a, GetGamePath() + "mods/multiplayer" + FName, fs::copy_options::overwrite_existing);
        }
        WaitForConfirm();
    }
    KillSocket(DSock);
    if (!Terminate) {
        TCPSend(strtovec("Done"), Sock);
        info("Done!");
    } else {
        UlStatus = "Ulstart";
        info("Connection Terminated!");
    }
}
