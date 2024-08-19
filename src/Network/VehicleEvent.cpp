// Copyright (c) 2019-present Anonymous275.
// BeamMP Launcher code is not in the public domain and is not free software.
// One must be granted explicit permission by the copyright holder in order to modify or distribute any part of the source or binaries.
// Anything else is prohibited. Modified works may not be published and have be upstreamed to the official repository.
///
/// Created by Anonymous275 on 5/8/2020
///

#include "Logger.h"
#include <Zlib/Compressor.h>
#include <chrono>
#include <iostream>
#include <vector>

#include "Network/network.hpp"

int LastPort;
std::string LastIP;
SOCKET TCPSock = -1;

bool CheckBytes(int32_t Bytes) {
    if (Bytes == 0) {
        debug("(TCP) Connection closing... CheckBytes(16)");
        Terminate = true;
        return false;
    } else if (Bytes < 0) {
        debug("(TCP CB) recv failed with error: " + std::to_string(WSAGetLastError()));
        KillSocket(TCPSock);
        Terminate = true;
        return false;
    }
    return true;
}
void UUl(const std::string& R) {
    UlStatus = "UlDisconnected: " + R;
}

void TCPSend(const std::string& Data, uint64_t Sock) {
    if (Sock == -1) {
        Terminate = true;
        UUl("Invalid Socket");
        return;
    }

    int32_t Size, Sent, Temp;
    std::string Send(4, 0);
    Size = int32_t(Data.size());
    memcpy(&Send[0], &Size, sizeof(Size));
    Send += Data;
    // Do not use Size before this point for anything but the header
    Sent = 0;
    Size += 4;
    do {
        if (size_t(Sent) >= Send.size()) {
            error("string OOB in " + std::string(__func__));
            UUl("TCP Send OOB");
            return;
        }
        Temp = send(Sock, &Send[Sent], Size - Sent, 0);
        if (!CheckBytes(Temp)) {
            UUl("Socket Closed Code 2");
            return;
        }
        Sent += Temp;
    } while (Sent < Size);
}

std::string TCPRcv(SOCKET Sock) {
    if (Sock == -1) {
        Terminate = true;
        UUl("Invalid Socket");
        return "";
    }
    int32_t Header, BytesRcv = 0, Temp;
    std::vector<char> Data(sizeof(Header));
    do {
        Temp = recv(Sock, &Data[BytesRcv], 4 - BytesRcv, 0);
        if (!CheckBytes(Temp)) {
            UUl("Socket Closed Code 3");
            return "";
        }
        BytesRcv += Temp;
    } while (BytesRcv < 4);
    memcpy(&Header, &Data[0], sizeof(Header));

    if (!CheckBytes(BytesRcv)) {
        UUl("Socket Closed Code 4");
        return "";
    }
    Data.resize(Header);
    BytesRcv = 0;
    do {
        Temp = recv(Sock, &Data[BytesRcv], Header - BytesRcv, 0);
        if (!CheckBytes(Temp)) {
            UUl("Socket Closed Code 5");
            return "";
        }
        BytesRcv += Temp;
    } while (BytesRcv < Header);

    std::string Ret(Data.data(), Header);

    if (Ret.substr(0, 4) == "ABG:") {
        auto substr = Ret.substr(4);
        auto res = DeComp(std::span<char>(substr.data(), substr.size()));
        Ret = std::string(res.data(), res.size());
    }

#ifdef DEBUG
    // debug("Parsing from server -> " + std::to_string(Ret.size()));
#endif
    if (Ret[0] == 'E' || Ret[0] == 'K')
        UUl(Ret.substr(1));
    return Ret;
}

void TCPClientMain(const std::string& IP, int Port) {
    LastIP = IP;
    LastPort = Port;

    sockaddr_storage server {};

    auto result = initSocket(IP, Port, SOCK_STREAM, &server);
    
    if (result.second != 0) {
        UlStatus = "UlConnection Failed!";
        error("Client: connect failed! Error code: " + std::to_string(WSAGetLastError()));
        KillSocket(TCPSock);
        WSACleanup();
        Terminate = true;
        return;
    }
    TCPSock = result.first;
    //Try to connect to the distant server, using the socket created
    if (connect(TCPSock, (struct sockaddr*)&server, sizeof(sockaddr_storage)))
    {
        UlStatus = "UlConnection Failed!";
        error("Client: connect failed! Error code: " + std::to_string(WSAGetLastError()));
        KillSocket(TCPSock);
        WSACleanup();
        Terminate = true;
        return;
    }
    info("Connected!");

    char Code = 'C';
    send(TCPSock, &Code, 1, 0);
    SyncResources(TCPSock);
    while (!Terminate) {
        ServerParser(TCPRcv(TCPSock));
    }
    GameSend("T");
    ////Game Send Terminate
    if (KillSocket(TCPSock) != 0)
        debug("(TCP) Cannot close socket. Error code: " + std::to_string(WSAGetLastError()));

#ifdef _WIN32
    if (WSACleanup() != 0)
        debug("(TCP) Client: WSACleanup() failed!...");
#endif
}

std::pair<SOCKET, int> initSocket(std::string ip, int port, int sockType, sockaddr_storage* storeAddrInfo) {
    int AF = (ip.find(':') != std::string::npos) ? AF_INET6 : AF_INET;
    int code = 0;
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Can't start Winsock!" << std::endl;
        return std::make_pair(INVALID_SOCKET, -1);
    }
#endif
    // Create the socket
    SOCKET sock = socket(AF, sockType, (sockType == SOCK_STREAM) ? IPPROTO_TCP : IPPROTO_UDP);

    if (sock == -1) {
        std::cerr << "Client: socket failed! Error code: %d\n" << WSAGetLastError() << std::endl;
        WSACleanup();
        return std::make_pair(INVALID_SOCKET, -1);
    }

    if (AF == AF_INET) {
        // IPv4
        struct sockaddr_in* saddr = (sockaddr_in*) storeAddrInfo;
        saddr->sin_family = AF_INET;
        saddr->sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &(saddr->sin_addr));

    } else {
        // IPv6
        struct sockaddr_in6 *saddr = (sockaddr_in6*) storeAddrInfo;
        saddr->sin6_family = AF_INET6;
        saddr->sin6_port = htons(port);
        inet_pton(AF_INET6, ip.c_str(), &(saddr->sin6_addr));
    }

    return std::make_pair( sock, code);
}
