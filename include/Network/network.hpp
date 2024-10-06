// Copyright (c) 2019-present Anonymous275.
// BeamMP Launcher code is not in the public domain and is not free software.
// One must be granted explicit permission by the copyright holder in order to modify or distribute any part of the source or binaries.
// Anything else is prohibited. Modified works may not be published and have be upstreamed to the official repository.
///
/// Created by Anonymous275 on 7/18/2020
///

#pragma once
#include <string>

#ifdef __linux__
#include "linuxfixes.h"
#include <bits/types/siginfo_t.h>
#include <cstdint>
#include <sys/ucontext.h>
#endif

void NetReset();
extern bool Dev;
extern int ping;

[[noreturn]] void CoreNetwork();
extern int ProxyPort;
extern int ClientID;
extern int LastPort;
extern bool ModLoaded;
extern bool Terminate;
extern uint64_t UDPSock;
extern uint64_t TCPSock;
extern std::string Branch;
extern std::string CachingDirectory;
extern bool TCPTerminate;
extern std::string LastIP;
extern std::string MStatus;
extern std::string UlStatus;
extern std::string PublicKey;
extern std::string PrivateKey;
int KillSocket(uint64_t Dead);
void UUl(const std::string& R);
void UDPSend(std::string Data);
bool CheckBytes(int32_t Bytes);
void GameSend(std::string_view Data);
void SendLarge(std::string Data);
std::string TCPRcv(uint64_t Sock);
void SyncResources(uint64_t TCPSock);
std::string GetAddr(const std::string& IP);
void ServerParser(std::string_view Data);
std::string Login(const std::string& fields);
void TCPSend(const std::string& Data, uint64_t Sock);
void TCPClientMain(const std::string& IP, int Port);
void UDPClientMain(const std::string& IP, int Port);
void TCPGameServer(const std::string& IP, int Port);
bool SecurityWarning();
void CoreSend(std::string data);
