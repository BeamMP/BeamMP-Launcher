///
/// Created by Anonymous275 on 5/8/2020
///
#include "Zlib/Compressor.h"
#include "Network/network.h"
#include "Security/Enc.h"
#include <WS2tcpip.h>
#include "Logger.h"
#include <string>
#include <set>

SOCKET UDPSock = -1;
sockaddr_in* ToServer = nullptr;

void UDPSend(std::string Data){
    if(ClientID == -1 || UDPSock == -1)return;
    if(Data.length() > 400){
        std::string CMP(Comp(Data));
        Data = "ABG:" + CMP;
    }
    std::string Packet = char(ClientID+1) + std::string(":") + Data;
    int sendOk = sendto(UDPSock, Packet.c_str(), int(Packet.size()), 0, (sockaddr*)ToServer, sizeof(*ToServer));
    if (sendOk == SOCKET_ERROR)error(Sec("Error Code : ") + std::to_string(WSAGetLastError()));
}


void SendLarge(std::string Data){
    if(Data.length() > 400){
        std::string CMP(Comp(Data));
        Data = "ABG:" + CMP;
    }
    TCPSend(Data);
}

void UDPParser(std::string Packet){
    if(Packet.substr(0,4) == "ABG:"){
        Packet = DeComp(Packet.substr(4));
    }
    ServerParser(Packet);
}
void UDPRcv(){
    sockaddr_in FromServer{};
    int clientLength = sizeof(FromServer);
    ZeroMemory(&FromServer, clientLength);
    std::string Ret(10240,0);
    if(UDPSock == -1)return;
    int32_t Rcv = recvfrom(UDPSock, &Ret[0], 10240, 0, (sockaddr*)&FromServer, &clientLength);
    if (Rcv == SOCKET_ERROR)return;
    UDPParser(Ret.substr(0,Rcv));
}
void UDPClientMain(const std::string& IP,int Port){
    WSADATA data;
    if (WSAStartup(514, &data)){
        error(Sec("Can't start Winsock!"));
        return;
    }
    delete ToServer;
    ToServer = new sockaddr_in;
    ToServer->sin_family = AF_INET;
    ToServer->sin_port = htons(Port);
    inet_pton(AF_INET, IP.c_str(), &ToServer->sin_addr);
    UDPSock = socket(AF_INET, SOCK_DGRAM, 0);

    TCPSend(Sec("P"));
    UDPSend(Sec("p"));
    while(!Terminate)UDPRcv();
    KillSocket(UDPSock);
    WSACleanup();
}