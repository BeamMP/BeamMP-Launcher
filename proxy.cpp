////
//// Created by Anonymous275 on 3/3/2020.
////

#define ENET_IMPLEMENTATION
#include <condition_variable>
#include "include/enet.hpp"
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <string>
#include <thread>
#include <queue>

extern int DEFAULT_PORT;
typedef struct {
    ENetHost *host;
    ENetPeer *peer;
} Client;

std::vector<std::string> Split(const std::string& String,const std::string& delimiter);
std::chrono::time_point<std::chrono::steady_clock> PingStart,PingEnd;
extern std::vector<std::string> GlobalInfo;
bool TCPTerminate = false;
bool Terminate = false;
bool CServer = true;
ENetPeer*ServerPeer;
SOCKET*ClientSocket;
extern bool MPDEV;
int ping = 0;

[[noreturn]] void CoreNetworkThread();

void TCPSEND(const std::string&Data){
    if(!TCPTerminate) {
        int iSendResult = send(*ClientSocket, (Data + "\n").c_str(), int(Data.length()) + 1, 0);
        if (iSendResult == SOCKET_ERROR) {
            if (MPDEV)std::cout << "(Proxy) send failed with error: " << WSAGetLastError() << std::endl;
            TCPTerminate = true;
        } else {
            if (MPDEV && Data.length() > 1000) {
                std::cout << "(Launcher->Game) Bytes sent: " << iSendResult << std::endl;
            }
            //std::cout << "(Launcher->Game) Bytes sent: " << iSendResult <<  " : " << Data << std::endl;
        }
    }
}
void RUDPSEND(const std::string&Data,bool Rel){
    if(!Terminate && ServerPeer != nullptr){
        char C = 0;
        if(Data.length() > 3)C = Data.at(0);
        if (C == 'O' || C == 'T')Rel = true;
        enet_peer_send(ServerPeer, 0, enet_packet_create(Data.c_str(), Data.length()+1, Rel?1:8));
        if (MPDEV && Data.length() > 1000) {
            std::cout << "(Launcher->Server) Bytes sent: " << Data.length()
            << " : "
            << Data.substr(0, 10)
            << Data.substr(Data.length() - 10) << std::endl;
        }else if(MPDEV){
            std::cout << "(Game->Launcher) : " << Data << std::endl;
        }
    }
}
void NameRespond(){
    std::string Packet = "NR" + GlobalInfo.at(0)+":"+GlobalInfo.at(2);
    RUDPSEND(Packet,true);
}

void AutoPing(ENetPeer*peer){
    while(!Terminate && peer != nullptr){
        RUDPSEND("p",true);
        PingStart = std::chrono::high_resolution_clock::now();
        std::this_thread::sleep_for(std::chrono::seconds (1));
    }
}

void RUDPParser(const std::string& Data){
    char Code = Data.at(0),SubCode = 0;
    if(Data.length() > 1)SubCode = Data.at(1);
    switch (Code) {
        case 'p':
            PingEnd = std::chrono::high_resolution_clock::now();
            ping = int(std::chrono::duration_cast<std::chrono::microseconds>(PingEnd-PingStart).count())/1000;
            return;
        case 'N':
            if(SubCode == 'R')NameRespond();
            return;
    }
    ///std::cout << "Received: " << Data << std::endl;
    TCPSEND(Data);
}

void HandleEvent(ENetEvent event,Client client){
    switch (event.type){
        case ENET_EVENT_TYPE_CONNECT:
            std::cout << "Connected to server!" << std::endl;
            //printf("Client Connected port : %u.\n",event.peer->address.port);
            event.peer->data = (void*)"Connected Server";
            break;
        case ENET_EVENT_TYPE_RECEIVE:
            RUDPParser((char*)event.packet->data);
            enet_packet_destroy(event.packet);
            break;
        case ENET_EVENT_TYPE_DISCONNECT:
            printf ("%s disconnected.\n", (char *)event.peer->data);
            CServer = true;
            Terminate = true;
            event.peer->data = nullptr;
            break;

        case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
            printf ("%s timeout.\n", (char *)event.peer->data);
            CServer = true;
            Terminate = true;
            TCPSEND("TTimeout");
            event.peer->data = nullptr;
            break;
        case ENET_EVENT_TYPE_NONE: break;
    }
}
void RUDPClientThread(const std::string& IP, int Port){
    std::condition_variable lock;
    if (enet_initialize() != 0) {
       std::cout << "An error occurred while initializing!\n";
    }
    Client client;
    ENetAddress address = {0};

    address.host = ENET_HOST_ANY;
    address.port = Port;
    if(MPDEV)std::cout << "(Launcher->Server) Connecting...\n";

    enet_address_set_host(&address, IP.c_str());
    client.host = enet_host_create(nullptr, 1, 2, 0, 0);
    client.peer = enet_host_connect(client.host, &address, 2, 0);
    if (client.peer == nullptr) {
        if(MPDEV)std::cout << "could not connect\n";
        TCPSEND("TTimeout");
        TCPTerminate = true;
        Terminate = true;
    }
    std::thread Ping(AutoPing,client.peer);
    Ping.detach();
    ENetEvent event;
    while (!Terminate) {
        ServerPeer = client.peer;
        enet_host_service(client.host, &event, 1);
        HandleEvent(event,client);
    }
    enet_peer_disconnect(client.peer,0);
    enet_host_service(client.host, &event, 1);
    HandleEvent(event,client);
    CServer = true;
    std::cout << "Connection Terminated!" << std::endl;
}

std::string Compress(const std::string&Data);
std::string Decompress(const std::string&Data);
void TCPServerThread(const std::string& IP, int Port){
    if(MPDEV)std::cout << "Proxy Started! " << IP << ":" << Port << std::endl;
    do {
        if(MPDEV)std::cout << "Proxy on Start" << std::endl;
        WSADATA wsaData;
        int iResult;
        SOCKET ListenSocket = INVALID_SOCKET;
        SOCKET Socket = INVALID_SOCKET;

        struct addrinfo *result = nullptr;
        struct addrinfo hints{};
        char recvbuf[10000];
        int recvbuflen = 10000;

        // Initialize Winsock
        iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != 0) {
            if(MPDEV)std::cout << "(Proxy) WSAStartup failed with error: " << iResult << std::endl;
            exit(-1);
        }

        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags = AI_PASSIVE;
        // Resolve the server address and port
        iResult = getaddrinfo(nullptr, std::to_string(DEFAULT_PORT+1).c_str(), &hints, &result);
        if (iResult != 0) {
            if(MPDEV)std::cout << "(Proxy) getaddrinfo failed with error: " << iResult << std::endl;
            WSACleanup();
            break;
        }

        // Create a socket for connecting to server
        ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (ListenSocket == INVALID_SOCKET) {
            if(MPDEV)std::cout << "(Proxy) socket failed with error: " << WSAGetLastError() << std::endl;
            freeaddrinfo(result);
            WSACleanup();
            break;
        }

        // Setup the TCP listening socket
        iResult = bind(ListenSocket, result->ai_addr, (int) result->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            if(MPDEV)std::cout << "(Proxy) bind failed with error: " << WSAGetLastError() << std::endl;
            freeaddrinfo(result);
            closesocket(ListenSocket);
            WSACleanup();
            break;
        }

        freeaddrinfo(result);

        iResult = listen(ListenSocket, SOMAXCONN);
        if (iResult == SOCKET_ERROR) {
            if(MPDEV)std::cout << "(Proxy) listen failed with error: " << WSAGetLastError() << std::endl;
            closesocket(ListenSocket);
            WSACleanup();
            continue;
        }
        Socket = accept(ListenSocket, nullptr, nullptr);
        if (Socket == INVALID_SOCKET) {
            if(MPDEV)std::cout << "(Proxy) accept failed with error: " << WSAGetLastError() << std::endl;
            closesocket(ListenSocket);
            WSACleanup();
            continue;
        }
        closesocket(ListenSocket);
        if(MPDEV)std::cout << "(Proxy) Game Connected!" << std::endl;
        if(CServer){
            std::thread t1(RUDPClientThread, IP, Port);
            t1.detach();
            CServer = false;
        }
       ClientSocket = &Socket;
        do {
            //std::cout << "(Proxy) Waiting for Game Data..." << std::endl;
            iResult = recv(Socket, recvbuf, recvbuflen, 0);
            if (iResult > 0) {
                std::string buff;
                buff.resize(iResult*2);
                memcpy(&buff[0],recvbuf,iResult);
                buff.resize(iResult);
                /*if(MPDEV && buff.length() > 1000) {
                    std::string cmp = Compress(buff), dcm = Decompress(cmp);
                    std::cout << "Compressed Size : " << cmp.length() << std::endl;
                    std::cout << "Decompressed Size : " << dcm.length() << std::endl;
                    if (cmp == dcm) {
                        std::cout << "Success!" << std::endl;
                    } else {
                        std::cout << "Fail!" << std::endl;
                    }
                }*/
                RUDPSEND(buff,false);
                //std::cout << "(Game->Launcher) Data : " << buff.length() << std::endl;
            } else if (iResult == 0) {
                if(MPDEV)std::cout << "(Proxy) Connection closing...\n";
                closesocket(Socket);
                WSACleanup();

                continue;
            } else {
                if(MPDEV)std::cout << "(Proxy) recv failed with error: " << WSAGetLastError() << std::endl;
                closesocket(Socket);
                WSACleanup();
                continue;
            }
        } while (iResult > 0);

        iResult = shutdown(Socket, SD_SEND);
        if (iResult == SOCKET_ERROR) {
            if(MPDEV)std::cout << "(Proxy) shutdown failed with error: " << WSAGetLastError() << std::endl;
            TCPTerminate = true;
            Terminate = true;
            closesocket(Socket);
            WSACleanup();
            continue;
        }
        closesocket(Socket);
        WSACleanup();
    }while (!TCPTerminate);
}

void VehicleNetworkStart();

void ProxyStart(){
    std::thread t1(CoreNetworkThread);
    if(MPDEV)std::cout << "Core Network Started!\n";
    t1.join();
}
void Reset() {
    ClientSocket = nullptr;
    ServerPeer = nullptr;
    TCPTerminate = false;
    Terminate = false;
}

void ProxyThread(const std::string& IP, int Port){
    Reset();
    auto*t1 = new std::thread(TCPServerThread,IP,Port);
    t1->detach();
    /*std::thread t2(VehicleNetworkStart);
    t2.detach();*/
}