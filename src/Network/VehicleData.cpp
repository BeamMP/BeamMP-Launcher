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
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#endif

#include "Logger.h"
#include <set>
#include <string>

SOCKET UDPSock = -1;
sockaddr_in* ToServer = nullptr;

void UDPSend(std::string Data) {
    if (ClientID == -1 || UDPSock == -1)
        return;
    if (Data.length() > 400) {
        std::string CMP(Comp(Data));
        Data = "ABG:" + CMP;
    }
    std::string Packet = char(ClientID + 1) + std::string(":") + Data;
    int sendOk = sendto(UDPSock, Packet.c_str(), int(Packet.size()), 0, (sockaddr*)ToServer, sizeof(*ToServer));
    if (sendOk == SOCKET_ERROR)
        error("Error Code : " + std::to_string(WSAGetLastError()));
}

void SendLarge(std::string Data) {
    if (Data.length() > 400) {
        std::string CMP(Comp(Data));
        Data = "ABG:" + CMP;
    }
    TCPSend(Data, TCPSock);
}

void UDPParser(std::string Packet) {
    if (Packet.substr(0, 4) == "ABG:") {
        Packet = DeComp(Packet.substr(4));
    }
    ServerParser(Packet);
}
void UDPRcv() {
    sockaddr_in FromServer {};
#if defined(_WIN32)
    int clientLength = sizeof(FromServer);
#elif defined(__linux__)
    socklen_t clientLength = sizeof(FromServer);
#endif
    ZeroMemory(&FromServer, clientLength);
    std::string Ret(10240, 0);
    if (UDPSock == -1)
        return;
    int32_t Rcv = recvfrom(UDPSock, &Ret[0], 10240, 0, (sockaddr*)&FromServer, &clientLength);
    if (Rcv == SOCKET_ERROR)
        return;
    UDPParser(Ret.substr(0, Rcv));
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
    while (!Terminate)
        UDPRcv();
    KillSocket(UDPSock);
    WSACleanup();
}