// Copyright (c) 2019-present Anonymous275.
// BeamMP Launcher code is not in the public domain and is not free software.
// One must be granted explicit permission by the copyright holder in order to modify or distribute any part of the source or binaries.
// Anything else is prohibited. Modified works may not be published and have be upstreamed to the official repository.
///
/// Created by Anonymous275 on 5/8/2020
///
#include "Network/network.hpp"
#include "Zlib/Compressor.h"

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
#include <string>

SOCKET UDPSock = -1;
sockaddr_storage* ToServer = nullptr;
socklen_t addrLen = 0;

void UDPSend(std::string Data) {
    if (ClientID == -1 || UDPSock == -1 ||ToServer == nullptr)
        return;
    if (Data.length() > 400) {
        auto res = Comp(std::span<char>(Data.data(), Data.size()));
        Data = "ABG:" + std::string(res.data(), res.size());
    }
    std::string Packet = char(ClientID + 1) + std::string(":") + Data;
    int sendOk = sendto(UDPSock, Packet.c_str(), int(Packet.size()), 0, (sockaddr*)ToServer, addrLen);
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
        auto res = DeComp(std::span<const char>(substr.data(), substr.size()));
        std::string DeCompPacket = std::string(res.data(), res.size());
        ServerParser(DeCompPacket);
    } else {
        ServerParser(Packet);
    }
}

void UDPRcv() {
    sockaddr_storage FromServer {};
    static thread_local std::array<char, 10240> Ret {};
    if (UDPSock == -1)
        return;
    int32_t Rcv = recvfrom(UDPSock, Ret.data(), Ret.size() - 1, 0, (sockaddr*)&FromServer, &addrLen);
    if (Rcv == SOCKET_ERROR)
        return;
    Ret[Rcv] = 0;
    UDPParser(std::string_view(Ret.data(), Rcv));
}

void UDPClientMain(const std::string& IP, int Port)
{

#ifdef _WIN32
    WSADATA data;
    if (WSAStartup(514, &data)) {
        error("Can't start Winsock!");
        return;
    }
#endif
    //IPv6 or IPv4 ?
    int AF = (IP.find(':') != std::string::npos) ? AF_INET6 : AF_INET;

    ToServer = new sockaddr_storage;
    memset(ToServer, 0, sizeof(sockaddr_storage));

    if (AF == AF_INET) {
        // IPv4
        struct sockaddr_in serverAddrV4;
        memset(&serverAddrV4, 0, sizeof(sockaddr_in));
        serverAddrV4.sin_family = AF_INET;
        serverAddrV4.sin_port = htons(Port);
        inet_pton(AF_INET, IP.c_str(), &serverAddrV4.sin_addr);
        memcpy(ToServer, &serverAddrV4, sizeof(sockaddr_in));
        addrLen = sizeof(sockaddr_in);
    } else {
        // IPv6
        struct sockaddr_in6 serverAddrV6;
        memset(&serverAddrV6, 0, sizeof(sockaddr_in6));
        serverAddrV6.sin6_family = AF_INET6;
        serverAddrV6.sin6_port = htons(Port);
        inet_pton(AF_INET6, IP.c_str(), &serverAddrV6.sin6_addr);
        memcpy(ToServer, &serverAddrV6, sizeof(sockaddr_in6));
        addrLen = sizeof(sockaddr_in6);
    }

    
    //Open socket
    UDPSock = socket(AF, SOCK_DGRAM, 0);

    //Send to the game client
    GameSend("P" + std::to_string(ClientID));
    TCPSend("H", TCPSock);
    UDPSend("p");
    //Main loop
    while (!Terminate)
        UDPRcv();
    KillSocket(UDPSock);
    WSACleanup();
}
