///
/// Created by Anonymous275 on 1/30/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#define WIN32_LEAN_AND_MEAN

#include "Compressor.h"
#include "Server.h"
#include "Launcher.h"
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "Logger.h"

Server::Server(Launcher *Instance) : LauncherInstance(Instance) {
    WSADATA wsaData;
    int iRes = WSAStartup(514, &wsaData); //2.2
    if (iRes != 0) {
        LOG(ERROR) << "WSAStartup failed with error: " << iRes;
    }
    UDPSockAddress = std::make_unique<sockaddr_in>();
}

Server::~Server() {
    Close();
    WSACleanup();
}

void Server::TCPClientMain() {
    SOCKADDR_IN ServerAddr;
    TCPSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (TCPSocket == -1) {
        LOG(ERROR) << "Socket failed! Error code: " << GetSocketApiError();
        return;
    }
    const char optval = 0;
    int status = ::setsockopt(TCPSocket, SOL_SOCKET, SO_DONTLINGER, &optval, sizeof(optval));
    if (status < 0) {
        LOG(INFO) << "Failed to set DONTLINGER: " << GetSocketApiError();
    }
    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_port = htons(Port);
    inet_pton(AF_INET, IP.c_str(), &ServerAddr.sin_addr);
    status = connect(TCPSocket, (SOCKADDR *) &ServerAddr, sizeof(ServerAddr));
    if (status != 0) {
        UStatus = "Connection Failed!";
        LOG(ERROR) << "Connect failed! Error code: " << GetSocketApiError();
        Terminate.store(true);
        return;
    }

    char Code = 'C';
    if (send(TCPSocket, &Code, 1, 0) != 1) {
        Terminate.store(true);
        return;
    }
    LOG(INFO) << "Connected!";
    SyncResources();
    while (!Terminate.load()) {
        ServerParser(TCPRcv());
    }
    LauncherInstance->SendIPC("T", false);
    KillSocket(TCPSocket);
}

void Server::StartUDP() {
    if (TCPConnection.joinable() && !UDPConnection.joinable()) {
        LOG(INFO) << "Connecting UDP";
        UDPConnection = std::thread(&Server::UDPMain, this);
    }
}

void Server::UDPSend(std::string Data) {
    if (ClientID == -1 || UDPSocket == -1)return;
    if (Data.length() > 400) {
        std::string CMP(Zlib::Comp(Data));
        Data = "ABG:" + CMP;
    }
    std::string Packet = char(ClientID + 1) + std::string(":") + Data;
    int sendOk = sendto(UDPSocket, Packet.c_str(), int(Packet.size()), 0, (sockaddr *) UDPSockAddress.get(),
                        sizeof(sockaddr_in));
    if (sendOk == SOCKET_ERROR)LOG(ERROR) << "UDP Socket Error Code : " << GetSocketApiError();
}

void Server::UDPParser(std::string Packet) {
    if (Packet.substr(0, 4) == "ABG:") {
        Packet = Zlib::DeComp(Packet.substr(4));
    }
    ServerParser(Packet);
}

void Server::UDPRcv() {
    sockaddr_in FromServer{};
    int clientLength = sizeof(FromServer);
    ZeroMemory(&FromServer, clientLength);
    std::string Ret(10240, 0);
    if (UDPSocket == -1)return;
    int32_t Rcv = recvfrom(UDPSocket, &Ret[0], 10240, 0, (sockaddr *) &FromServer, &clientLength);
    if (Rcv == SOCKET_ERROR)return;
    UDPParser(Ret.substr(0, Rcv));
}

void Server::UDPClient() {
    UDPSockAddress->sin_family = AF_INET;
    UDPSockAddress->sin_port = htons(Port);
    inet_pton(AF_INET, IP.c_str(), &UDPSockAddress->sin_addr);
    UDPSocket = socket(AF_INET, SOCK_DGRAM, 0);
    LauncherInstance->SendIPC("P" + std::to_string(ClientID), false);
    TCPSend("H");
    UDPSend("p");
    while (!Terminate)UDPRcv();
    KillSocket(UDPSocket);
}

void Server::UDPMain() {
    AutoPing = std::thread(&Server::PingLoop, this);
    UDPClient();
    Terminate = true;
    LOG(INFO) << "Connection terminated";
}

void Server::Connect(const std::string &Data) {
    ModList.clear();
    Terminate.store(false);
    IP = GetAddress(Data.substr(1, Data.find(':') - 1));
    std::string port = Data.substr(Data.find(':') + 1);
    bool ValidPort = std::all_of(port.begin(), port.end(), ::isdigit);
    if (IP.find('.') == -1 || !ValidPort) {
        if (IP == "DNS") UStatus = "Connection Failed! (DNS Lookup Failed)";
        else if (!ValidPort) UStatus = "Connection Failed! (Invalid Port)";
        else UStatus = "Connection Failed! (WSA failed to start)";
        ModList = "-";
        Terminate.store(true);
        return;
    }
    Port = std::stoi(port);
    LauncherInstance->CheckKey();
    UStatus = "Loading...";
    Ping = -1;
    TCPConnection = std::thread(&Server::TCPClientMain, this);
    LOG(INFO) << "Connecting to server";
}

void Server::SendLarge(std::string Data) {
    if (Data.length() > 400) {
        std::string CMP(Zlib::Comp(Data));
        Data = "ABG:" + CMP;
    }
    TCPSend(Data);
}

std::string Server::GetSocketApiError() {
    // This will provide us with the error code and an error message, all in one.
    // The resulting format is "<CODE> - <MESSAGE>"
    int err;
    char msgbuf[256];
    msgbuf[0] = '\0';

    err = WSAGetLastError();

    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr,
                   err,
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   msgbuf,
                   sizeof(msgbuf),
                   nullptr);

    if (*msgbuf) {
        return std::to_string(WSAGetLastError()) + " - " + std::string(msgbuf);
    } else {
        return std::to_string(WSAGetLastError());
    }
}

void Server::ServerSend(std::string Data, bool Rel) {
    if (Terminate || Data.empty())return;
    char C = 0;
    int DLen = int(Data.length());
    if (DLen > 3)C = Data.at(0);
    bool Ack = C == 'O' || C == 'T';
    if (C == 'N' || C == 'W' || C == 'Y' || C == 'V' || C == 'E' || C == 'C')Rel = true;
    if (Ack || Rel) {
        if (Ack || DLen > 1000)SendLarge(Data);
        else TCPSend(Data);
    } else UDPSend(Data);
}

void Server::PingLoop() {
    while (!Terminate) {
        ServerSend("p", false);
        PingStart = std::chrono::high_resolution_clock::now();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void Server::Close() {
    Terminate.store(true);
    KillSocket(TCPSocket);
    KillSocket(UDPSocket);
    Ping = -1;
    if (TCPConnection.joinable()) {
        TCPConnection.join();
    }
    if (UDPConnection.joinable()) {
        UDPConnection.join();
    }
    if (AutoPing.joinable()) {
        AutoPing.join();
    }
}

const std::string &Server::getMap() {
    return MStatus;
}

bool Server::Terminated() {
    return Terminate;
}

const std::string &Server::getModList() {
    return ModList;
}

int Server::getPing() const {
    return Ping;
}

const std::string &Server::getUIStatus() {
    return UStatus;
}

std::string Server::GetAddress(const std::string &Data) {
    if (Data.find_first_not_of("0123456789.") == -1)return Data;
    hostent *host;
    host = gethostbyname(Data.c_str());
    if (!host) {
        LOG(ERROR) << "DNS lookup failed! on " << Data;
        return "DNS";
    }
    std::string Ret = inet_ntoa(*((struct in_addr *) host->h_addr));
    return Ret;
}

int Server::KillSocket(uint64_t Dead) {
    if (Dead == (SOCKET) -1)return 0;
    shutdown(Dead, SD_BOTH);
    return closesocket(Dead);
}

void Server::setModLoaded() {
    ModLoaded.store(true);
}

void Server::UUl(const std::string &R) {
    UStatus = "Disconnected: " + R;
}

bool Server::CheckBytes(int32_t Bytes) {
    if (Bytes == 0) {
        //debug("(TCP) Connection closing... CheckBytes(16)");
        Terminate = true;
        return false;
    } else if (Bytes < 0) {
        //debug("(TCP CB) recv failed with error: " + GetSocketApiError();
        KillSocket(TCPSocket);
        Terminate = true;
        return false;
    }
    return true;
}

void Server::TCPSend(const std::string &Data) {

    if (TCPSocket == -1) {
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
            LOG(ERROR) << "string OOB in " << std::string(__func__);
            UUl("TCP Send OOB");
            Terminate = true;
            return;
        }
        Temp = send(TCPSocket, &Send[Sent], Size - Sent, 0);
        if (!CheckBytes(Temp)) {
            UUl("Socket Closed Code 2");
            Terminate = true;
            return;
        }
        Sent += Temp;
    } while (Sent < Size);
}

std::string Server::TCPRcv() {
    if (TCPSocket == -1) {
        Terminate = true;
        UUl("Invalid Socket");
        return "";
    }
    int32_t Header, BytesRcv = 0, Temp;
    std::vector<char> Data(sizeof(Header));
    do {
        Temp = recv(TCPSocket, &Data[BytesRcv], 4 - BytesRcv, 0);
        if (!CheckBytes(Temp)) {
            UUl("Socket Closed Code 3");
            Terminate = true;
            return "";
        }
        BytesRcv += Temp;
    } while (BytesRcv < 4);
    memcpy(&Header, &Data[0], sizeof(Header));

    if (!CheckBytes(BytesRcv)) {
        UUl("Socket Closed Code 4");
        Terminate = true;
        return "";
    }
    Data.resize(Header);
    BytesRcv = 0;
    do {
        Temp = recv(TCPSocket, &Data[BytesRcv], Header - BytesRcv, 0);
        if (!CheckBytes(Temp)) {
            UUl("Socket Closed Code 5");
            Terminate = true;
            return "";
        }
        BytesRcv += Temp;
    } while (BytesRcv < Header);

    std::string Ret(Data.data(), Header);

    if (Ret.substr(0, 4) == "ABG:") {
        Ret = Zlib::DeComp(Ret.substr(4));
    }

    if (Ret[0] == 'E')UUl(Ret.substr(1));
    return Ret;
}
