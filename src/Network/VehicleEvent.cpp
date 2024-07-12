// Copyright (c) 2019-present Anonymous275.
// BeamMP Launcher code is not in the public domain and is not free software.
// One must be granted explicit permission by the copyright holder in order to modify or distribute any part of the source or binaries.
// Anything else is prohibited. Modified works may not be published and have be upstreamed to the official repository.
///
/// Created by Anonymous275 on 5/8/2020
///

#include "Logger.h"
#include "fmt/format.h"
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
std::shared_ptr<asio::ip::tcp::socket> TCPSock = nullptr;

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

void TCPSend(const std::vector<char>& Data, asio::ip::tcp::socket& Sock) {
    if (!Sock.is_open()) {
        Terminate = true;
        UUl("Invalid Socket");
        return;
    }

    int32_t Size, Sent, Temp;
    std::string Send(4, 0);
    Size = int32_t(Data.size());
    memcpy(&Send[0], &Size, sizeof(Size));
    Send += std::string(Data.data(), Data.size());
    // Do not use Size before this point for anything but the header
    Sent = 0;
    Size += 4;
    asio::error_code ec;
    asio::write(Sock, asio::buffer(Send), ec);
    if (ec) {
        UUl(fmt::format("Failed to send data: {}", ec.message()));
    }
}

std::string TCPRcv(asio::ip::tcp::socket& Sock) {
    if (!Sock.is_open()) {
        Terminate = true;
        UUl("Invalid Socket");
        return "";
    }
    int32_t Header, BytesRcv = 0, Temp;
    std::vector<char> Data(sizeof(Header));
    asio::error_code ec;
    asio::read(Sock, asio::buffer(Data), ec);
    if (ec) {
        UUl(fmt::format("Failed to receive header: {}", ec.message()));
    }
    memcpy(&Header, &Data[0], sizeof(Header));

    Data.resize(Header);
    asio::read(Sock, asio::buffer(Data), ec);
    if (ec) {
        UUl(fmt::format("Failed to receive data: {}", ec.message()));
    }

    std::string Ret(Data.data(), Header);

    if (Ret.substr(0, 4) == "ABG:") {
        auto substr = Ret.substr(4);
        auto res = DeComp(strtovec(substr));
        Ret = std::string(res.data(), res.size());
    }

#ifdef DEBUG
    // debug("Parsing from server -> " + std::to_string(Ret.size()));
#endif
    if (Ret[0] == 'E' || Ret[0] == 'K')
        UUl(Ret.substr(1));
    return Ret;
}

void TCPClientMain(asio::ip::tcp::socket&& socket) {
    if (!TCPSock) {
        return;
    }
    LastIP = socket.remote_endpoint().address().to_string();
    LastPort = socket.remote_endpoint().port();
    SOCKADDR_IN ServerAddr;
    int RetCode;
    TCPSock = std::make_shared<asio::ip::tcp::socket>(std::move(socket));

    info("Connected!");

    char Code = 'C';
    asio::write(*TCPSock, asio::buffer(&Code, 1));
    SyncResources(*TCPSock);
    while (!Terminate && TCPSock) {
        ServerParser(TCPRcv(*TCPSock));
    }
    GameSend("T");
    KillSocket(TCPSock);
}
