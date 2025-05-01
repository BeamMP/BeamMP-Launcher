/*
 Copyright (C) 2024 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/


#include "Network/network.hpp"
#include <chrono>
#include <iomanip>
#include <ios>
#include <mutex>
#include <nlohmann/json.hpp>
#include <openssl/err.h>
#include <openssl/evp.h>

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
#include <Utils.h>
#include <atomic>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <thread>

#include "hashpp.h"

namespace fs = std::filesystem;

void CheckForDir() {
    if (!fs::exists(CachingDirectory)) {
        try {
            fs::create_directories(CachingDirectory);
        } catch (const std::exception& e) {
            error(std::string("Failed to create caching directory: ") + e.what() + ". This is a fatal error. Please make sure to configure a directory which you have permission to create, read and write from/to.");
            std::this_thread::sleep_for(std::chrono::seconds(3));
            std::exit(1);
        }
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

float DownloadSpeed = 0;

void AsyncUpdate(uint64_t& Rcv, uint64_t Size, const std::string& Name) {
    do {
        double pr = double(Rcv) / double(Size) * 100;
        std::string Per = std::to_string(trunc(pr * 10) / 10);
        std::string SpeedString = "";
        if (DownloadSpeed > 0.01) {
            std::stringstream ss;
            ss << " at " << std::setprecision(1) << std::fixed << DownloadSpeed << " Mbit/s";
            SpeedString = ss.str();
        }
        UpdateUl(true, Name + " (" + Per.substr(0, Per.find('.') + 2) + "%)" + SpeedString);
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

    auto start = std::chrono::high_resolution_clock::now();

    int i = 0;
    do {
        // receive at most some MB at a time
        int Len = std::min(int(Size - Rcv), 1 * 1024 * 1024);
        int Temp = recv(Sock, &File[Rcv], Len, MSG_WAITALL);
        if (Temp == -1 || Temp == 0) {
            debug("Recv returned: " + std::to_string(Temp));
            if (Temp == -1) {
                error("Socket error during download: " + std::to_string(WSAGetLastError()));
            }
            UUl("Socket Closed Code 1");
            KillSocket(Sock);
            Terminate = true;
            return {};
        }
        Rcv += Temp;
        GRcv += Temp;

        auto end = std::chrono::high_resolution_clock::now();
        auto difference = end - start;
        double bits_per_s = double(Rcv * 8) / double(std::chrono::duration_cast<std::chrono::milliseconds>(difference).count());
        double megabits_per_s = bits_per_s / 1000;
        DownloadSpeed = megabits_per_s;
        // every 8th iteration print the speed
        if (i % 8 == 0) {
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

std::vector<char> SingleNormalDownload(SOCKET MSock, uint64_t Size, const std::string& Name) {
    DownloadSpeed = 0;

    uint64_t GRcv = 0;

    std::thread Au([&] { AsyncUpdate(GRcv, Size, Name); });

    const std::vector<char> MData = TCPRcvRaw(MSock, GRcv, Size);

    if (MData.empty()) {
        KillSocket(MSock);
        Terminate = true;
        Au.join();
        return {};
    }

    // ensure that GRcv is good before joining the async update thread
    GRcv = MData.size();
    if (GRcv != Size) {
        error("Something went wrong during download; didn't get enough data. Expected " + std::to_string(Size) + " bytes, got " + std::to_string(GRcv) + " bytes instead");
        Terminate = true;
        Au.join();
        return {};
    }

    Au.join();
    return MData;
}

std::vector<char> MultiDownload(SOCKET MSock, SOCKET DSock, uint64_t Size, const std::string& Name) {
    DownloadSpeed = 0;

    uint64_t GRcv = 0;

    uint64_t MSize = Size / 2;
    uint64_t DSize = Size - MSize;

    std::thread Au([&] { AsyncUpdate(GRcv, Size, Name); });

    const std::vector<char> MData = TCPRcvRaw(MSock, GRcv, MSize);

    if (MData.empty()) {
        MultiKill(MSock, DSock);
        Terminate = true;
        Au.join();
        return {};
    }

    const std::vector<char> DData = TCPRcvRaw(DSock, GRcv, DSize);

    if (DData.empty()) {
        MultiKill(MSock, DSock);
        Terminate = true;
        Au.join();
        return {};
    }

    // ensure that GRcv is good before joining the async update thread
    GRcv = MData.size() + DData.size();
    if (GRcv != Size) {
        error("Something went wrong during download; didn't get enough data. Expected " + std::to_string(Size) + " bytes, got " + std::to_string(GRcv) + " bytes instead");
        Terminate = true;
        Au.join();
        return {};
    }

    Au.join();

    std::vector<char> Result {};
    Result.insert(Result.begin(), MData.begin(), MData.end());
    Result.insert(Result.end(), DData.begin(), DData.end());
    return Result;
}

void InvalidResource(const std::string& File) {
    UUl("Invalid mod \"" + File + "\"");
    warn("The server tried to sync \"" + File + "\" that is not a .zip file!");
    Terminate = true;
}

std::string GetSha256HashReallyFast(const std::string& filename) {
    try {
        EVP_MD_CTX* mdctx;
        const EVP_MD* md;
        uint8_t sha256_value[EVP_MAX_MD_SIZE];
        md = EVP_sha256();
        if (md == nullptr) {
            throw std::runtime_error("EVP_sha256() failed");
        }

        mdctx = EVP_MD_CTX_new();
        if (mdctx == nullptr) {
            throw std::runtime_error("EVP_MD_CTX_new() failed");
        }
        if (!EVP_DigestInit_ex2(mdctx, md, NULL)) {
            EVP_MD_CTX_free(mdctx);
            throw std::runtime_error("EVP_DigestInit_ex2() failed");
        }

        std::ifstream stream(filename, std::ios::binary);

        const size_t FileSize = std::filesystem::file_size(filename);
        size_t Read = 0;
        std::vector<char> Data;
        while (Read < FileSize) {
            Data.resize(size_t(std::min<size_t>(FileSize - Read, 4096)));
            size_t RealDataSize = Data.size();
            stream.read(Data.data(), std::streamsize(Data.size()));
            if (stream.eof() || stream.fail()) {
                RealDataSize = size_t(stream.gcount());
            }
            Data.resize(RealDataSize);
            if (RealDataSize == 0) {
                break;
            }
            if (RealDataSize > 0 && !EVP_DigestUpdate(mdctx, Data.data(), Data.size())) {
                EVP_MD_CTX_free(mdctx);
                throw std::runtime_error("EVP_DigestUpdate() failed");
            }
            Read += RealDataSize;
        }
        unsigned int sha256_len = 0;
        if (!EVP_DigestFinal_ex(mdctx, sha256_value, &sha256_len)) {
            EVP_MD_CTX_free(mdctx);
            throw std::runtime_error("EVP_DigestFinal_ex() failed");
        }
        EVP_MD_CTX_free(mdctx);

        std::string result;
        for (size_t i = 0; i < sha256_len; i++) {
            char buf[3];
            sprintf(buf, "%02x", sha256_value[i]);
            buf[2] = 0;
            result += buf;
        }
        return result;
    } catch (const std::exception& e) {
        error("Sha256 hashing of '" + filename + "' failed: " + e.what());
        return "";
    }
}

struct ModInfo {
    static std::pair<bool, std::vector<ModInfo>> ParseModInfosFromPacket(const std::string& packet) {
        bool success = false;
        std::vector<ModInfo> modInfos;
        try {
            auto json = nlohmann::json::parse(packet);
            if (json.empty()) {
                return std::make_pair(true, modInfos);
            }

            for (const auto& entry : json) {
                ModInfo modInfo {
                    .FileName = entry["file_name"],
                    .FileSize = entry["file_size"],
                    .Hash = entry["hash"],
                    .HashAlgorithm = entry["hash_algorithm"],
                };
                modInfos.push_back(modInfo);
                success = true;
            }
        } catch (const std::exception& e) {
            debug(std::string("Failed to receive mod list: ") + e.what());
            debug("Failed to receive new mod list format! This server may be outdated, but everything should still work as expected.");
        }
        return std::make_pair(success, modInfos);
    }
    std::string FileName;
    size_t FileSize;
    std::string Hash;
    std::string HashAlgorithm;
};

nlohmann::json modUsage = {};

void UpdateModUsage(const std::string& fileName) {
    try {
        fs::path usageFile = fs::path(CachingDirectory) / "mods.json";

        if (!fs::exists(usageFile)) {
            if (std::ofstream file(usageFile); !file.is_open()) {
                error("Failed to create mods.json");
                return;
            } else {
                file.close();
            }
        }

        std::fstream file(usageFile, std::ios::in | std::ios::out);
        if (!file.is_open()) {
            error("Failed to open or create mods.json");
            return;
        }

        if (modUsage.empty()) {
            auto Size = fs::file_size(fs::path(CachingDirectory) / "mods.json");
            std::string modsJson(Size, 0);
            file.read(&modsJson[0], Size);

            if (!modsJson.empty()) {
                auto parsedModJson = nlohmann::json::parse(modsJson, nullptr, false);

                if (parsedModJson.is_object())
                    modUsage = parsedModJson;
            }
        }

        modUsage[fileName] = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

        file.clear();
        file.seekp(0, std::ios::beg);
        file << modUsage.dump();
        file.close();
    } catch (std::exception& e) {
        error("Failed to update mods.json: " + std::string(e.what()));
    }
}


void NewSyncResources(SOCKET Sock, const std::string& Mods, const std::vector<ModInfo> ModInfos) {
    if (ModInfos.empty()) {
        CoreSend("L");
        TCPSend("Done", Sock);
        info("Done!");
        return;
    }

    if (!SecurityWarning())
        return;

    info("Checking Resources...");

    CheckForDir();

    std::string t;
    for (const auto& mod : ModInfos) {
        t += mod.FileName + ";";
    }

    if (t.empty())
        CoreSend("L");
    else
        CoreSend("L" + t);
    t.clear();

    info("Syncing...");

    int ModNo = 1;
    int TotalMods = ModInfos.size();
    for (auto ModInfoIter = ModInfos.begin(), AlsoModInfoIter = ModInfos.begin(); ModInfoIter != ModInfos.end() && !Terminate; ++ModInfoIter, ++AlsoModInfoIter) {
        if (ModInfoIter->Hash.length() < 8 || ModInfoIter->HashAlgorithm != "sha256") {
            error("Unsupported hash algorithm or invalid hash for '" + ModInfoIter->FileName + "'");
            Terminate = true;
            return;
        }
        auto FileName = std::filesystem::path(ModInfoIter->FileName).stem().string() + "-" + ModInfoIter->Hash.substr(0, 8) + std::filesystem::path(ModInfoIter->FileName).extension().string();
        auto PathToSaveTo = (fs::path(CachingDirectory) / FileName).string();
        if (fs::exists(PathToSaveTo) && GetSha256HashReallyFast(PathToSaveTo) == ModInfoIter->Hash) {
            debug("Mod '" + FileName + "' found in cache");
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            try {
                if (!fs::exists(GetGamePath() + "mods/multiplayer")) {
                    fs::create_directories(GetGamePath() + "mods/multiplayer");
                }
                auto modname = ModInfoIter->FileName;
#if defined(__linux__)
                // Linux version of the game doesnt support uppercase letters in mod names
                for (char& c : modname) {
                    c = ::tolower(c);
                }
#endif
                debug("Mod name: " + modname);
                auto name = std::filesystem::path(GetGamePath()) / "mods/multiplayer" / modname;
                std::string tmp_name = name.string();
                tmp_name += ".tmp";

                fs::copy_file(PathToSaveTo, tmp_name, fs::copy_options::overwrite_existing);
                fs::rename(tmp_name, name);
                UpdateModUsage(FileName);
            } catch (std::exception& e) {
                error("Failed copy to the mods folder! " + std::string(e.what()));
                Terminate = true;
                continue;
            }
            WaitForConfirm();
            continue;
        } else if (auto OldCachedPath = fs::path(CachingDirectory) / std::filesystem::path(ModInfoIter->FileName).filename();
                   fs::exists(OldCachedPath) && GetSha256HashReallyFast(OldCachedPath.string()) == ModInfoIter->Hash) {
            debug("Mod '" + FileName + "' found in old cache, copying it to the new cache");
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            try {
                fs::copy_file(OldCachedPath, PathToSaveTo, fs::copy_options::overwrite_existing);

                if (!fs::exists(GetGamePath() + "mods/multiplayer")) {
                    fs::create_directories(GetGamePath() + "mods/multiplayer");
                }

                auto modname = ModInfoIter->FileName;

#if defined(__linux__)
                // Linux version of the game doesnt support uppercase letters in mod names
                for (char& c : modname) {
                    c = ::tolower(c);
                }
#endif

                debug("Mod name: " + modname);
                auto name = std::filesystem::path(GetGamePath()) / "mods/multiplayer" / modname;
                std::string tmp_name = name.string();
                tmp_name += ".tmp";

                fs::copy_file(PathToSaveTo, tmp_name, fs::copy_options::overwrite_existing);
                fs::rename(tmp_name, name);
                UpdateModUsage(FileName);
            } catch (std::exception& e) {
                error("Failed copy to the mods folder! " + std::string(e.what()));
                Terminate = true;
                continue;
            }
            WaitForConfirm();
            continue;
        }
        CheckForDir();
        std::string FName = ModInfoIter->FileName;
        do {
            debug("Loading file '" + FName + "' to '" + PathToSaveTo + "'");
            TCPSend("f" + ModInfoIter->FileName, Sock);

            std::string Data = TCPRcv(Sock);
            if (Data == "CO" || Terminate) {
                Terminate = true;
                UUl("Server cannot find " + FName);
                break;
            }

            if (Data != "AG") {
                UUl("Received corrupted download confirmation, aborting download.");
                debug("Corrupted download confirmation: " + Data);
                Terminate = true;
                break;
            }

            std::string Name = std::to_string(ModNo) + "/" + std::to_string(TotalMods) + ": " + FName;

            std::vector<char> DownloadedFile = SingleNormalDownload(Sock, ModInfoIter->FileSize, Name);

            if (Terminate)
                break;
            UpdateUl(false, std::to_string(ModNo) + "/" + std::to_string(TotalMods) + ": " + FName);

            // 1. write downloaded file to disk
            {
                std::ofstream OutFile(PathToSaveTo, std::ios::binary | std::ios::trunc);
                OutFile.write(DownloadedFile.data(), DownloadedFile.size());
                OutFile.flush();
            }
            // 2. verify size and hash
            if (std::filesystem::file_size(PathToSaveTo) != DownloadedFile.size()) {
                error("Failed to write the entire file '" + PathToSaveTo + "' correctly (file size mismatch)");
                Terminate = true;
            }

            if (GetSha256HashReallyFast(PathToSaveTo) != ModInfoIter->Hash) {
                error("Failed to write or download the entire file '" + PathToSaveTo + "' correctly (hash mismatch)");
                Terminate = true;
            }
        } while (fs::file_size(PathToSaveTo) != ModInfoIter->FileSize && !Terminate);
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

            fs::copy_file(PathToSaveTo, std::filesystem::path(GetGamePath()) / "mods/multiplayer" / FName, fs::copy_options::overwrite_existing);
            UpdateModUsage(FName);
        }
        WaitForConfirm();
        ++ModNo;
    }

    if (!Terminate) {
        TCPSend("Done", Sock);
        info("Done!");
    } else {
        UlStatus = "Ulstart";
        info("Connection Terminated!");
    }
}

void SyncResources(SOCKET Sock) {
    std::string Ret = Auth(Sock);

    debug("Mod info: " + Ret);

    if (Ret.starts_with("R")) {
        debug("This server is likely outdated, not trying to parse new mod info format");
    } else {
        auto [success, modInfo] = ModInfo::ParseModInfosFromPacket(Ret);

        if (success) {
            NewSyncResources(Sock, Ret, modInfo);
            return;
        }
    }

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
            PathToSaveTo = CachingDirectory + FN->substr(pos);
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
                    UpdateModUsage(modname);
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
            UpdateModUsage(FN->substr(pos));
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
