///
/// Created by Anonymous275 on 5/8/2020
///

#include <chrono>
#include <iostream>
#include <WS2tcpip.h>
#include <thread>

#pragma comment (lib, "ws2_32.lib")
extern bool Terminate;
extern bool MPDEV;
SOCKET TCPSock;


void TCPSend(const std::string&Data){
   if(TCPSock == INVALID_SOCKET){
       Terminate = true;
       return;
   }
   int BytesSent = send(TCPSock, Data.c_str(), int(Data.length())+1, 0);
   if (BytesSent == 0){
       if(MPDEV)std::cout << "(TCP) Connection closing..." << std::endl;
       Terminate = true;
       return;
   }
   else if (BytesSent < 0) {
       if(MPDEV)std::cout << "(TCP) send failed with error: " << WSAGetLastError() << std::endl;
       closesocket(TCPSock);
       Terminate = true;
       return;
   }
}


void ServerParser(const std::string& Data);
void TCPRcv(){
    char buf[4096];
    int len = 4096;
    ZeroMemory(buf, len);
    if(TCPSock == INVALID_SOCKET){
        Terminate = true;
        return;
    }
    int BytesRcv = recv(TCPSock, buf, len,0);
    if (BytesRcv == 0){
        if(MPDEV)std::cout << "(TCP) Connection closing..." << std::endl;
        Terminate = true;
        return;
    }
    else if (BytesRcv < 0) {
        if(MPDEV)std::cout << "(TCP) recv failed with error: " << WSAGetLastError() << std::endl;
        closesocket(TCPSock);
        Terminate = true;
        return;
    }
    ServerParser(std::string(buf));
}

void GameSend(const std::string&Data);
void SyncResources(SOCKET TCPSock);
void TCPClientMain(const std::string& IP,int Port){
    WSADATA wsaData;
    SOCKADDR_IN ServerAddr;
    int RetCode;

    WSAStartup(514, &wsaData); //2.2
    TCPSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(TCPSock == -1)
    {
        printf("Client: socket failed! Error code: %d\n", WSAGetLastError());
        WSACleanup();
        return;
    }

    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_port = htons(Port);
    inet_pton(AF_INET, IP.c_str(), &ServerAddr.sin_addr);

    RetCode = connect(TCPSock, (SOCKADDR *) &ServerAddr, sizeof(ServerAddr));
    if(RetCode != 0)
    {
        std::cout << "Client: connect failed! Error code: " << WSAGetLastError() << std::endl;
        closesocket(TCPSock);
        WSACleanup();
        return;
    }
    getsockname(TCPSock, (SOCKADDR *)&ServerAddr, (int *)sizeof(ServerAddr));

    SyncResources(TCPSock);
    while(!Terminate)TCPRcv();
    GameSend("T");
    ////Game Send Terminate
    if( shutdown(TCPSock, SD_SEND) != 0 && MPDEV)
        std::cout << "(TCP) shutdown error code: " << WSAGetLastError() << std::endl;

    if(closesocket(TCPSock) != 0 && MPDEV)
        std::cout << "(TCP) Cannot close socket. Error code: " << WSAGetLastError() << std::endl;

    if(WSACleanup() != 0 && MPDEV)
        std::cout << "(TCP) Client: WSACleanup() failed!..." << std::endl;
}
