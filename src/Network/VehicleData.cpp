// Copyright (c) 2019-present Anonymous275.
// BeamMP Launcher code is not in the public domain and is not free software.
// One must be granted explicit permission by the copyright holder in order to modify or distribute any part of the source or binaries.
// Anything else is prohibited. Modified works may not be published and have be upstreamed to the official repository.
///
/// Created by Anonymous275 on 5/8/2020
///
#include "Network/network.hpp"
#include "Zlib/Compressor.h"
#include <stdexcept>

#if defined(_WIN32)
#include <ws2tcpip.h>
#elif defined(__linux__) || defined(__APPLE__)
#include "linuxfixes.h"
#include <arpa/inet.h>
#include <cstring>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#endif

#include "Logger.h"
#include <array>
#include <string>

SOCKET UDPSock = -1;
sockaddr_in* ToServer = nullptr;

void UDPSend(std::string Data) {
    if (ClientID == -1 || UDPSock == -1)
        return;
    if (Data.length() > 400) {
        auto res = Comp(std::span<char>(Data.data(), Data.size()));
        Data = "ABG:" + std::string(res.data(), res.size());
    }
    std::string Packet = char(ClientID + 1) + std::string(":") + Data;
    int sendOk = sendto(UDPSock, Packet.c_str(), int(Packet.size()), 0, (sockaddr*)ToServer, sizeof(*ToServer));
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
    sockaddr_in FromServer {};
#if defined(_WIN32)
    int clientLength = sizeof(FromServer);
#elif defined(__linux__) || defined(__APPLE__)
    socklen_t clientLength = sizeof(FromServer);
#endif
    ZeroMemory(&FromServer, clientLength);
    static thread_local std::array<char, 10240> Ret {};
    if (UDPSock == -1)
        return;
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
    ToServer = new sockaddr_in;
    ToServer->sin_family = AF_INET;
    ToServer->sin_port = htons(Port);
    inet_pton(AF_INET, IP.c_str(), &ToServer->sin_addr);
    UDPSock = socket(AF_INET, SOCK_DGRAM, 0);
    GameSend("P" + std::to_string(ClientID));
    TCPSend("H", TCPSock);
    UDPSend("p");
    debug("Starting UDP receive loop");
    while (!Terminate) {
        UDPRcv();
    }
    debug("UDP receive loop done");
    KillSocket(UDPSock);
    WSACleanup();
}
