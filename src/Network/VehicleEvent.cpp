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
    SOCKADDR_IN ServerAddr;
    int RetCode;
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(514, &wsaData); // 2.2
#endif
    TCPSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (TCPSock == -1) {
        printf("Client: socket failed! Error code: %d\n", WSAGetLastError());
        WSACleanup();
        return;
    }

    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_port = htons(Port);
    inet_pton(AF_INET, IP.c_str(), &ServerAddr.sin_addr);
    RetCode = connect(TCPSock, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr));
    if (RetCode != 0) {
        UlStatus = "UlConnection Failed!";
        error("Client: connect failed! Error code: " + std::to_string(WSAGetLastError()));
        KillSocket(TCPSock);
        WSACleanup();
        Terminate = true;
        CoreSend("L");
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
