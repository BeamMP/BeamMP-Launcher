/*
 Copyright (C) 2024 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/

#include "Network/network.hpp"
#include "Zlib/Compressor.h"
#include <stdexcept>
#include <thread>

#if defined(_WIN32)
#include <ws2tcpip.h>
#elif defined(__linux__)
#include "linuxfixes.h"
#include <arpa/inet.h>
#include <cstring>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#endif

#include "Logger.h"
#include <array>
#include <future>
#include <mutex>
#include <string>

SOCKET UDPSock = -1;
sockaddr_storage* ToServer = nullptr;
socklen_t ToServerLen = 0;

std::mutex UDPSendMutex;

void UDPSend(std::string Data) {
    std::scoped_lock lock(UDPSendMutex);
    if (ClientID == -1 || UDPSock == -1)
        return;
    if (Data.length() > 400) {
        auto res = Comp(std::span<char>(Data.data(), Data.size()));
        Data = "ABG:" + std::string(res.data(), res.size());
    }
    std::string Packet = char(ClientID + 1) + std::string(":") + Data;
    int sendOk = sendto(UDPSock, Packet.c_str(), int(Packet.size()), 0, (sockaddr*)ToServer, ToServerLen);
    if (sendOk == SOCKET_ERROR)
        error("Error Code : " + std::to_string(WSAGetLastError()));
}

void SendLarge(std::string Data) {
    if (Data.length() > 400) {
        auto res = Comp(std::span<char>(Data.data(), Data.size()));
        Data = "ABG:" + std::string(res.data(), res.size());
    }
    TCPSend(Data, TCPSock);
}

void UDPParser(std::string_view Packet) {
    if (Packet.substr(0, 4) == "ABG:") {
        auto substr = Packet.substr(4);
        try {
            auto res = DeComp(std::span<const char>(substr.data(), substr.size()));
            std::string DeCompPacket = std::string(res.data(), res.size());
            ServerParser(DeCompPacket);
        } catch (const std::runtime_error& err) {
            error("Error in decompression of UDP, ignoring");
        }
    } else {
        ServerParser(Packet);
    }
}
void UDPRcv() {
    sockaddr_storage FromServer {};
#if defined(_WIN32)
    int clientLength = sizeof(FromServer);
#elif defined(__linux__)
    socklen_t clientLength = sizeof(FromServer);
#endif
    ZeroMemory(&FromServer, clientLength);
    static thread_local std::array<char, 10240> Ret {};
    if (UDPSock == -1) {
        debug("UDPSock == -1");
        return;
    }
    int32_t Rcv = recvfrom(UDPSock, Ret.data(), Ret.size() - 1, 0, (sockaddr*)&FromServer, &clientLength);
    if (Rcv == SOCKET_ERROR)
        return;
    Ret[Rcv] = 0;
    UDPParser(std::string_view(Ret.data(), Rcv));
}

void UDPClientMain(const std::string& IP, int Port) {
#ifdef _WIN32
    WSADATA data;
    if (WSAStartup(514, &data)) {
        error("Can't start Winsock!");
        return;
    }
#endif

    delete ToServer;
    ToServer = nullptr;
    ToServerLen = 0;

    struct addrinfo Hints {};
    Hints.ai_family = AF_UNSPEC;
    Hints.ai_socktype = SOCK_DGRAM;
    struct addrinfo* Result = nullptr;
    int Res = getaddrinfo(IP.c_str(), std::to_string(Port).c_str(), &Hints, &Result);
    if (Res != 0 || !Result) {
        error("UDP: failed to resolve " + IP + " (" + std::to_string(Res) + ")");
        if (Result)
            freeaddrinfo(Result);
        WSACleanup();
        return;
    }
    ToServer = new sockaddr_storage {};
    memcpy(ToServer, Result->ai_addr, Result->ai_addrlen);
    ToServerLen = socklen_t(Result->ai_addrlen);
    UDPSock = socket(Result->ai_family, Result->ai_socktype, Result->ai_protocol);
    freeaddrinfo(Result);
    if (UDPSock == -1) {
        error("UDP: socket failed! Error code: " + std::to_string(WSAGetLastError()));
        WSACleanup();
        return;
    }
    std::future<void> magicSend;

    if (!magic.empty()) {
        magicSend = std::async(std::launch::async, ([](){
            for (int i = 0; i < 10; i++) {
                UDPSend(magic);
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }));
    }
    GameSend("P" + std::to_string(ClientID));
    TCPSend("H", TCPSock);
    UDPSend("p");
    debug("Starting UDP receive loop");
    while (!Terminate) {
        UDPRcv();
    }
    debug("UDP receive loop done");

    if (magicSend.valid()) {
        magicSend.get();
    }

    KillSocket(UDPSock);
    WSACleanup();
}
