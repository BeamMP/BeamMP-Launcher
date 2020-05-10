///
/// Created by Anonymous275 on 5/8/2020
///


#include <WS2tcpip.h>
#include <iostream>
#include <thread>

extern bool Terminate;
extern int ClientID;
SOCKET UDPSock;
sockaddr_in ToServer{};

void UDPSend(const std::string&Data){
    if(ClientID == -1 || UDPSock == INVALID_SOCKET)return;
    std::string Packet = char(ClientID+1) + std::string(":") + Data;
    int sendOk = sendto(UDPSock, Packet.c_str(), int(Packet.length()) + 1, 0, (sockaddr*)&ToServer, sizeof(ToServer));
    if (sendOk == SOCKET_ERROR)std::cout << "Error Code : " << WSAGetLastError() << std::endl;
}

void ServerParser(const std::string& Data);
void UDPRcv(){
    char buf[4096];
    int len = 4096;
    sockaddr_in FromServer{};
    int clientLength = sizeof(FromServer);
    ZeroMemory(&FromServer, clientLength);
    ZeroMemory(buf, len);
    if(UDPSock == INVALID_SOCKET)return;
    int bytesIn = recvfrom(UDPSock, buf, len, 0, (sockaddr*)&FromServer, &clientLength);
    if (bytesIn == SOCKET_ERROR)
    {
        //std::cout << "Error receiving from Server " << WSAGetLastError() << std::endl;
        return;
    }
    ServerParser(std::string(buf));
}
void TCPSend(const std::string&Data);
void UDPClientMain(const std::string& IP,int Port){
    WSADATA data;
    if (WSAStartup(514, &data)) //2.2
    {
        std::cout << "Can't start Winsock! " << std::endl;
        return;
    }

    ToServer.sin_family = AF_INET;
    ToServer.sin_port = htons(Port);
    inet_pton(AF_INET, IP.c_str(), &ToServer.sin_addr);
    UDPSock = socket(AF_INET, SOCK_DGRAM, 0);


    TCPSend("P");
    UDPSend("p");
    while (!Terminate){
        UDPRcv();
    }

    closesocket(UDPSock);
    WSACleanup();
}