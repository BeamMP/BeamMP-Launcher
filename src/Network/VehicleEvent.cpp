///
/// Created by Anonymous275 on 5/8/2020
///

#include <chrono>
#include <vector>
#include "Logger.h"
#include <iostream>
#include <WS2tcpip.h>
#include <Zlib/Compressor.h>
#include "Security/Enc.h"
#include "Network/network.h"

SOCKET TCPSock;
bool CheckBytes(int32_t Bytes){
    if (Bytes == 0){
        debug(Sec("(TCP) Connection closing... CheckBytes(16)"));

        Terminate = true;
        return false;
    }else if (Bytes < 0) {
        debug(Sec("(TCP) recv failed with error: ") + std::to_string(WSAGetLastError()));
        closesocket(TCPSock);
        Terminate = true;
        return false;
    }
    return true;
}
void TCPSend(const std::string&Data){
   if(TCPSock == -1){
       Terminate = true;
       return;
   }

   // Size is BIG-ENDIAN!
   //auto Size = htonl(int32_t(Data.size()));
   ///TODO
   int32_t Size,Sent,Temp;
   std::string Send(4,0);
   Size = int32_t(Data.size());
   memcpy(&Send[0],&Size,sizeof(Size));
   Send += Data;
   // Do not use Size before this point for anything but the header
   Sent = 0;
   Size += 4;
   do{
       Temp = send(TCPSock, &Send[Sent], Size - Sent, 0);
       if(!CheckBytes(Temp))return;
       Sent += Temp;
   }while(Sent < Size);
}

void TCPRcv(){
    if(TCPSock == -1){
        Terminate = true;
        return;
    }
    int32_t Header,BytesRcv,Temp;
    BytesRcv = recv(TCPSock, reinterpret_cast<char*>(&Header), sizeof(Header),0);
    // convert back to LITTLE ENDIAN
    //Header = ntohl(Header);
    if(!CheckBytes(BytesRcv))return;
    std::vector<char> Data(Header);
    BytesRcv = 0;
    do{
        Temp = recv(TCPSock,&Data[BytesRcv],Header-BytesRcv,0);
        if(!CheckBytes(Temp))return;
        BytesRcv += Temp;
    }while(BytesRcv < Header);

    std::string Ret(Data.data(),Header);
    if (Ret.substr(0, 4) == "ABG:") {
        Ret = DeComp(Ret.substr(4));
    }
#ifdef DEBUG
    //debug("Parsing from server -> " + std::to_string(Ret.size()));
#endif
    ServerParser(Ret);
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
