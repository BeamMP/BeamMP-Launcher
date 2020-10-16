///
/// Created by Anonymous275 on 7/18/2020
///
#pragma once
#include <string>
#include "Buffer.h"
void NetReset();
extern long long ping;
extern bool Dev;
void ClearAll();
extern int ClientID;
extern Buffer Handler;
extern bool ModLoaded;
extern bool Terminate;
extern int DEFAULT_PORT;
extern bool TCPTerminate;
extern std::string MStatus;
extern std::string UlStatus;
extern std::string ListOfMods;
void UDPSend(std::string Data);
[[noreturn]] void CoreNetwork();
void SendLarge(std::string Data);
void TCPSend(const std::string&Data);
void GameSend(const std::string&Data);
std::string GetAddr(const std::string&IP);
void ServerParser(const std::string& Data);
void TCPClientMain(const std::string& IP,int Port);
void UDPClientMain(const std::string& IP,int Port);
void TCPGameServer(const std::string& IP, int Port);

