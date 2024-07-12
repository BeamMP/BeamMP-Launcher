// Copyright (c) 2019-present Anonymous275.
// BeamMP Launcher code is not in the public domain and is not free software.
// One must be granted explicit permission by the copyright holder in order to modify or distribute any part of the source or binaries.
// Anything else is prohibited. Modified works may not be published and have be upstreamed to the official repository.
///
/// Created by Anonymous275 on 5/8/2020
///
#include "Network/network.hpp"
#include "Zlib/Compressor.h"
#include "asio/ip/address.hpp"
#include "fmt/format.h"

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

std::shared_ptr<asio::ip::udp::socket> UDPSock = nullptr;

void UDPSend(const std::vector<char>& RawData) {
    if (ClientID == -1 || UDPSock == nullptr)
        return;
    std::string Data;
    if (Data.size() > 400) {
        auto res = Comp(RawData);
        Data = "ABG:" + std::string(res.data(), res.size());
    } else {
        Data = std::string(RawData.data(), RawData.size());
    }
    std::string Packet = char(ClientID + 1) + std::string(":") + Data;
    int sendOk = UDPSock->send(asio::buffer(Packet));
    if (sendOk == SOCKET_ERROR)
        error("Error Code : " + std::to_string(WSAGetLastError()));
}

void SendLarge(const std::vector<char>& Data) {
    if (Data.size() > 400) {
        auto res = Comp(Data);
        res.insert(res.begin(), { 'A', 'B', 'G', ':' });
        if (!TCPSock) {
            ::debug("TCPSock is null");
            return;
        }
        TCPSend(res, *TCPSock);
    } else {
        if (!TCPSock) {
            ::debug("TCPSock is null");
            return;
        }
        TCPSend(Data, *TCPSock);
    }
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
    static thread_local std::array<char, 10240> Ret {};
    if (UDPSock == nullptr) {
        ::debug("UDPSock is null");
        return;
    }
    asio::error_code ec;
    int32_t Rcv = UDPSock->receive(asio::buffer(Ret.data(), Ret.size() - 1), 0, ec);
    if (ec)
        return;
    Ret[Rcv] = 0;
    UDPParser(std::string_view(Ret.data(), Rcv));
}
void UDPClientMain(asio::ip::address addr, uint16_t port) {
    UDPSock = std::make_shared<asio::ip::udp::socket>(io);
    asio::error_code ec;
    UDPSock->connect(asio::ip::udp::endpoint(addr, port), ec);
    if (ec) {
        ::error(fmt::format("Failed to connect UDP to server: {}", ec.message()));
        Terminate = true;
    }
    GameSend("P" + std::to_string(ClientID));
    TCPSend(strtovec("H"), *TCPSock);
    UDPSend(strtovec("p"));
    while (!Terminate)
        UDPRcv();
    KillSocket(UDPSock);
    WSACleanup();
}
