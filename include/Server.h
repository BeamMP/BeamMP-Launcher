///
/// Created by Anonymous275 on 1/30/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#pragma once
#include <string>
#include <atomic>
#include <thread>
#include <chrono>

struct sockaddr_in;
class Launcher;
class Server {
public:
    Server() = delete;
    explicit Server(Launcher* Instance);
    ~Server();
public:
    void ServerSend(std::string Data, bool Rel);
    void Connect(const std::string& Data);
    const std::string& getModList();
    const std::string& getUIStatus();
    const std::string& getMap();
    void StartUDP();
    void setModLoaded();
    bool Terminated();
    int getPing() const;
    void Close();
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> PingStart, PingEnd;
    std::string MultiDownload(uint64_t DSock, uint64_t Size, const std::string& Name);
    void AsyncUpdate(uint64_t& Rcv,uint64_t Size,const std::string& Name);
    std::atomic<bool> Terminate{false}, ModLoaded{false};
    char* TCPRcvRaw(uint64_t Sock, uint64_t& GRcv, uint64_t Size);
    std::string GetAddress(const std::string& Data);
    void InvalidResource(const std::string& File);
    void UpdateUl(bool D, const std::string& msg);
    std::unique_ptr<sockaddr_in> UDPSockAddress;
    void ServerParser(const std::string& Data);
    void TCPSend(const std::string& Data);
    void UDPParser(std::string Packet);
    void SendLarge(std::string Data);
    void UDPSend(std::string Data);
    void UUl(const std::string& R);
    bool CheckBytes(int32_t Bytes);
    int KillSocket(uint64_t Dead);
    void MultiKill(uint64_t Sock);
    Launcher* LauncherInstance;
    std::thread TCPConnection;
    std::thread UDPConnection;
    std::thread AutoPing;
    uint64_t TCPSocket = -1;
    uint64_t UDPSocket = -1;
    void WaitForConfirm();
    std::string UStatus{};
    std::string MStatus{};
    std::string ModList{};
    void TCPClientMain();
    void SyncResources();
    std::string TCPRcv();
    uint64_t InitDSock();
    std::string Auth();
    std::string IP{};
    void UDPClient();
    void PingLoop();
    int ClientID{0};
    void UDPMain();
    void UDPRcv();
    void Abort();
    int Port{0};
    int Ping{0};
};
