///
/// Created by Anonymous275 on 5/8/2020
///

#include <chrono>
#include "Logger.h"
#include <iostream>
#include <WS2tcpip.h>
#include "Security/Enc.h"
#include "Network/network.h"

SOCKET TCPSock;
void TCPSend(const std::string&Data){
   if(TCPSock == -1){
       Terminate = true;
       return;
   }
   std::string Send = "\n" + Data.substr(0,Data.find(char(0))) + "\n";
   size_t Sent = send(TCPSock, Send.c_str(), int(Send.size()), 0);
   if (Sent == 0){
       debug(Sec("(TCP) Connection closing..."));
       Terminate = true;
       return;
   }
   else if (Sent < 0) {
       debug(Sec("(TCP) send failed with error: ") + std::to_string(WSAGetLastError()));
       closesocket(TCPSock);
       Terminate = true;
       return;
   }
}
void TCPRcv(){
    char buf[4096];
    int len = 4096;
    ZeroMemory(buf, len);
    if(TCPSock == -1){
        Terminate = true;
        return;
    }
    int BytesRcv = recv(TCPSock, buf, len,0);
    if (BytesRcv == 0){
        debug(Sec("(TCP) Connection closing..."));
        Terminate = true;
        return;
    }
    else if (BytesRcv < 0) {
        debug(Sec("(TCP) recv failed with error: ") + std::to_string(WSAGetLastError()));
        closesocket(TCPSock);
        Terminate = true;
        return;
    }
    Handler.Handle(std::string(buf));
}

void SyncResources(SOCKET TCPSock);
void TCPClientMain(const std::string& IP,int Port){
    WSADATA wsaData;
    SOCKADDR_IN ServerAddr;
    int RetCode;
    WSAStartup(514, &wsaData); //2.2
    TCPSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(TCPSock == -1){
        printf(Sec("Client: socket failed! Error code: %d\n"), WSAGetLastError());
        WSACleanup();
        return;
    }
    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_port = htons(Port);
    inet_pton(AF_INET, IP.c_str(), &ServerAddr.sin_addr);
    RetCode = connect(TCPSock, (SOCKADDR *) &ServerAddr, sizeof(ServerAddr));
    if(RetCode != 0){
        UlStatus = Sec("UlConnection Failed!");
        std::cout << Sec("Client: connect failed! Error code: ") << WSAGetLastError() << std::endl;
        closesocket(TCPSock);
        WSACleanup();
        Terminate = true;
        return;
    }
    getsockname(TCPSock, (SOCKADDR *)&ServerAddr, (int *)sizeof(ServerAddr));

    SyncResources(TCPSock);
    while(!Terminate)TCPRcv();
    GameSend(Sec("T"));
    ////Game Send Terminate
    if(closesocket(TCPSock) != 0)
        debug(Sec("(TCP) Cannot close socket. Error code: ") + std::to_string(WSAGetLastError()));

    if(WSACleanup() != 0)
        debug(Sec("(TCP) Client: WSACleanup() failed!..."));
}
