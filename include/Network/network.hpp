// Copyright (c) 2019-present Anonymous275.
// BeamMP Launcher code is not in the public domain and is not free software.
// One must be granted explicit permission by the copyright holder in order to modify or distribute any part of the source or binaries.
// Anything else is prohibited. Modified works may not be published and have be upstreamed to the official repository.
///
/// Created by Anonymous275 on 7/18/2020
///

#pragma once
#include "Helpers.h"
#include "asio/io_context.hpp"
#include "asio/ip/address.hpp"
#include <span>
#include <string>

#ifdef __linux__
#include "linuxfixes.h"
#include <bits/types/siginfo_t.h>
#include <cstdint>
#include <sys/ucontext.h>
#include <vector>
#endif

#include <asio.hpp>

extern asio::io_context io;

void NetReset();
extern bool Dev;
extern int ping;

[[noreturn]] void CoreNetwork();
extern int ProxyPort;
extern int ClientID;
extern int LastPort;
extern bool ModLoaded;
extern bool Terminate;
extern int DEFAULT_PORT;
extern std::shared_ptr<asio::ip::udp::socket> UDPSock;
extern std::shared_ptr<asio::ip::tcp::socket> TCPSock;
extern std::string Branch;
extern bool TCPTerminate;
extern std::string LastIP;
extern std::string MStatus;
extern std::string UlStatus;
extern std::string PublicKey;
extern std::string PrivateKey;
extern std::string ListOfMods;
void KillSocket(std::shared_ptr<asio::ip::tcp::socket>& Dead);
void KillSocket(std::shared_ptr<asio::ip::udp::socket>& Dead);
void KillSocket(asio::ip::tcp::socket& Dead);
void KillSocket(asio::ip::udp::socket& Dead);
void UUl(const std::string& R);
void UDPSend(const std::vector<char>& Data);
bool CheckBytes(int32_t Bytes);
void GameSend(std::string_view Data);
void SendLarge(const std::vector<char>& Data);
std::string TCPRcv(asio::ip::tcp::socket& Sock);
void SyncResources(asio::ip::tcp::socket& TCPSock);
std::string GetAddr(const std::string& IP);
void ServerParser(std::string_view Data);
std::string Login(const std::string& fields);
void TCPSend(const std::vector<char>& Data, asio::ip::tcp::socket& Sock);
void TCPClientMain(asio::ip::tcp::socket&& Socket);
void UDPClientMain(asio::ip::address addr, uint16_t port);
void TCPGameServer(asio::ip::tcp::socket&& Socket);
