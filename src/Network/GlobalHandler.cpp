// Copyright (c) 2019-present Anonymous275.
// BeamMP Launcher code is not in the public domain and is not free software.
// One must be granted explicit permission by the copyright holder in order to modify or distribute any part of the source or binaries.
// Anything else is prohibited. Modified works may not be published and have be upstreamed to the official repository.
///
/// Created by Anonymous275 on 7/25/2020
///
#include "Helpers.h"
#include "Network/network.hpp"
#include "NetworkHelpers.h"
#include "asio/socket_base.hpp"
#include <algorithm>
#include <span>
#include <vector>
#include <zlib.h>
#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#elif defined(__linux__)
#include "linuxfixes.h"
#include <arpa/inet.h>
#include <cstring>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include "Logger.h"
#include <charconv>
#include <fmt/format.h>
#include <mutex>
#include <string>
#include <thread>

std::chrono::time_point<std::chrono::high_resolution_clock> PingStart, PingEnd;
bool GConnected = false;
bool CServer = true;
std::shared_ptr<asio::ip::tcp::socket> CSocket = nullptr;
std::shared_ptr<asio::ip::tcp::socket> GSocket = nullptr;

void KillSocket(std::shared_ptr<asio::ip::tcp::socket>& Dead) {
    if (!Dead)
        return;
    asio::error_code ec;
    Dead->shutdown(asio::socket_base::shutdown_both, ec);
}

void KillSocket(std::shared_ptr<asio::ip::udp::socket>& Dead) {
    if (!Dead)
        return;
    asio::error_code ec;
    Dead->shutdown(asio::socket_base::shutdown_both, ec);
}

void KillSocket(asio::ip::tcp::socket& Dead) {
    asio::error_code ec;
    Dead.shutdown(asio::socket_base::shutdown_both, ec);
}

void KillSocket(asio::ip::udp::socket& Dead) {
    asio::error_code ec;
    Dead.shutdown(asio::socket_base::shutdown_both, ec);
}

bool CheckBytes(uint32_t Bytes) {
    if (Bytes == 0) {
        debug("(Proxy) Connection closing");
        return false;
    } else if (Bytes < 0) {
        debug("(Proxy) send failed with error: " + std::to_string(WSAGetLastError()));
        return false;
    }
    return true;
}

void GameSend(std::string_view RawData) {
    static std::mutex Lock;
    std::scoped_lock Guard(Lock);
    if (TCPTerminate || !GConnected || CSocket == nullptr)
        return;
    int32_t Size, Temp, Sent;
    uint32_t DataSize = RawData.size();
    std::vector<char> Data(sizeof(DataSize) + RawData.size());
    std::copy_n(reinterpret_cast<char*>(&DataSize), sizeof(DataSize), Data.begin());
    std::copy_n(RawData.data(), RawData.size(), Data.begin() + sizeof(DataSize));
    Size = Data.size();
    Sent = 0;

    asio::error_code ec;
    asio::write(*CSocket, asio::buffer(Data), ec);
    if (ec) {
        debug(fmt::format("(TCP CB) recv failed with error: {}", ec.message()));
        KillSocket(TCPSock);
        Terminate = true;
    }
}

void ServerSend(const std::vector<char>& Data, bool Rel) {
    if (Terminate || Data.empty())
        return;
    char C = 0;
    bool Ack = false;
    int DLen = int(Data.size());
    if (DLen > 3)
        C = Data.at(0);
    if (C == 'O' || C == 'T')
        Ack = true;
    if (C == 'N' || C == 'W' || C == 'Y' || C == 'V' || C == 'E' || C == 'C')
        Rel = true;
    if (compressBound(Data.size()) > 1024)
        Rel = true;
    if (Ack || Rel) {
        if (Ack || DLen > 1000)
            SendLarge(Data);
        else if (TCPSock)
            TCPSend(Data, *TCPSock);
    } else
        UDPSend(Data);
}

void NetReset() {
    TCPTerminate = false;
    GConnected = false;
    Terminate = false;
    UlStatus = "Ulstart";
    MStatus = " ";
    if (UDPSock != nullptr) {
        KillSocket(*UDPSock);
    }
    UDPSock = nullptr;
    if (TCPSock != nullptr) {
        KillSocket(*TCPSock);
    }
    TCPSock = nullptr;
    if (GSocket != nullptr) {
        KillSocket(*GSocket);
    }
    GSocket = nullptr;
}

void AutoPing() {
    while (!Terminate) {
        ServerSend(strtovec("p"), false);
        PingStart = std::chrono::high_resolution_clock::now();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
int ClientID = -1;
void ParserAsync(std::string_view Data) {
    if (Data.empty())
        return;
    char Code = Data.at(0), SubCode = 0;
    if (Data.length() > 1)
        SubCode = Data.at(1);
    switch (Code) {
    case 'p':
        PingEnd = std::chrono::high_resolution_clock::now();
        if (PingStart > PingEnd)
            ping = 0;
        else
            ping = int(std::chrono::duration_cast<std::chrono::milliseconds>(PingEnd - PingStart).count());
        return;
    case 'M':
        MStatus = Data;
        UlStatus = "Uldone";
        return;
    default:
        break;
    }
    GameSend(Data);
}
void ServerParser(std::string_view Data) {
    ParserAsync(Data);
}
void NetMain(asio::ip::address addr, uint16_t port) {
    std::thread Ping(AutoPing);
    Ping.detach();
    UDPClientMain(addr, port);
    CServer = true;
    Terminate = true;
    info("Connection Terminated!");
}
void TCPGameServer(asio::ip::tcp::socket&& Socket) {
    asio::ip::tcp::endpoint listen_ep(asio::ip::address::from_string("0.0.0.0"), static_cast<uint16_t>(DEFAULT_PORT + 1));
    asio::ip::tcp::socket listener(io);
    asio::error_code ec;
    listener.open(listen_ep.protocol(), ec);
    if (ec) {
        ::error(fmt::format("Failed to open game socket: {}", ec.message()));
        return;
    }
    asio::ip::tcp::socket::linger linger_opt {};
    linger_opt.enabled(false);
    listener.set_option(linger_opt, ec);
    if (ec) {
        ::error(fmt::format("Failed to set up listening game socket to not linger / reuse address. "
                            "This may cause the game socket to refuse to bind(). Error: {}",
            ec.message()));
    }
    asio::ip::tcp::socket::reuse_address reuse_opt { true };
    listener.set_option(reuse_opt, ec);
    if (ec) {
        ::error(fmt::format("Failed to set up listening core socket to not linger / reuse address. "
                            "This may cause the core socket to refuse to bind(). Error: {}",
            ec.message()));
        return;
    }
    auto acceptor = asio::ip::tcp::acceptor(io, listen_ep);
    acceptor.listen(asio::ip::tcp::socket::max_listen_connections, ec);
    if (ec) {
        debug(fmt::format("Proxy accept failed: {}", ec.message()));
        TCPTerminate = true; // skip the loop
    }
    debug(fmt::format("Game server listening on {}:{}", acceptor.local_endpoint().address().to_string(), acceptor.local_endpoint().port()));
    while (!TCPTerminate && acceptor.is_open()) {
        debug("MAIN LOOP OF GAME SERVER");
        GConnected = false;
        if (!CServer) {
            warn("Connection still alive terminating");
            NetReset();
            TCPTerminate = true;
            Terminate = true;
            break;
        }
        if (CServer) {
            std::thread Client(TCPClientMain, std::move(Socket));
            Client.detach();
        }

        CSocket = std::make_shared<asio::ip::tcp::socket>(acceptor.accept());
        debug("(Proxy) Game Connected!");
        GConnected = true;
        if (CServer) {
            std::thread t1(NetMain, CSocket->remote_endpoint().address(), CSocket->remote_endpoint().port());
            t1.detach();
            CServer = false;
        }
        std::vector<char> data {};

        // Read byte by byte until '>' is rcved then get the size and read based on it
        while (!TCPTerminate && !CSocket) {
            try {
                ReceiveFromGame(*CSocket, data);
                ServerSend(data, false);
            } catch (const std::exception& e) {
                error(std::string("Error while receiving from game: ") + e.what());
                break;
            }
        }
    }
    TCPTerminate = true;
    GConnected = false;
    Terminate = true;
    if (CSocket != nullptr)
        KillSocket(CSocket);
    debug("END OF GAME SERVER");
}
