///
/// Created by Anonymous275 on 7/18/2020
///
#pragma once
#include <string>
void NetReset();
extern long long ping;
extern bool Dev;
void ClearAll();
extern int ClientID;
extern bool ModLoaded;
extern bool Terminate;
extern int DEFAULT_PORT;
extern bool TCPTerminate;
extern std::string MStatus;
extern std::string UlStatus;
extern std::string ListOfMods;
void UDPSend(std::string Data);
[[noreturn]] void CoreNetwork();
void TCPSend(const std::string&Data);
void GameSend(const std::string&Data);
void SendLarge(const std::string&Data);
void TCPClientMain(const std::string& IP,int Port);
void UDPClientMain(const std::string& IP,int Port);
void TCPGameServer(const std::string& IP, int Port);
