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

SOCKET TCPSock = -1;
bool CheckBytes(int32_t Bytes){
    if (Bytes == 0){
        debug("(TCP) Connection closing... CheckBytes(16)");
        Terminate = true;
        return false;
    }else if (Bytes < 0) {
        debug("(TCP CB) recv failed with error: " + std::to_string(WSAGetLastError()));
        KillSocket(TCPSock);
        Terminate = true;
        return false;
    }
    return true;
}
void UpdateUl(const std::string& R){
    UlStatus = "UlDisconnected: " + R;
}

void TCPSend(const std::string&Data,uint64_t Sock){
   if(Sock == -1){
       Terminate = true;
       UpdateUl("Invalid Socket");
       return;
   }

   int32_t Size,Sent,Temp;
   std::string Send(4,0);
   Size = int32_t(Data.size());
   memcpy(&Send[0],&Size,sizeof(Size));
   Send += Data;
   // Do not use Size before this point for anything but the header
   Sent = 0;
   Size += 4;
   do{
       if (size_t(Sent) >= Send.size()) {
           error("string OOB in " + std::string(__func__));
           UpdateUl("TCP Send OOB");
           return;
       }
       Temp = send(Sock, &Send[Sent], Size - Sent, 0);
       if(!CheckBytes(Temp)){
           UpdateUl("Socket Closed");
           return;
       }
       Sent += Temp;
   }while(Sent < Size);
}

std::string TCPRcv(SOCKET Sock){
    if(Sock == -1){
        Terminate = true;
        UpdateUl("Invalid Socket");
        return "";
    }
    int32_t Header,BytesRcv = 0,Temp;
    std::vector<char> Data(sizeof(Header));
    do{
        Temp = recv(Sock,&Data[BytesRcv],4-BytesRcv,0);
        if(!CheckBytes(Temp)){
            UpdateUl("Socket Closed");
            return "";
        }
        BytesRcv += Temp;
    }while(BytesRcv < 4);
    memcpy(&Header,&Data[0],sizeof(Header));

    if(!CheckBytes(BytesRcv)){
        UpdateUl("Socket Closed");
        return "";
    }
    Data.resize(Header);
    BytesRcv = 0;
    do{
        Temp = recv(Sock,&Data[BytesRcv],Header-BytesRcv,0);
        if(!CheckBytes(Temp)){
            UpdateUl("Socket Closed");
            return "";
        }
        BytesRcv += Temp;
    }while(BytesRcv < Header);

    std::string Ret(Data.data(),Header);

    if (Ret.substr(0, 4) == "ABG:") {
        Ret = DeComp(Ret.substr(4));
    }

#ifdef DEBUG
    //debug("Parsing from server -> " + std::to_string(Ret.size()));
#endif
    if(Ret[0] == 'E')UpdateUl(Ret.substr(1));
    return Ret;
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
        KillSocket(TCPSock);
        WSACleanup();
        Terminate = true;
        return;
    }
    getsockname(TCPSock, (SOCKADDR *)&ServerAddr, (int *)sizeof(ServerAddr));

    SyncResources(TCPSock);
    while(!Terminate){
        ServerParser(TCPRcv(TCPSock));
    }
    GameSend(Sec("T"));
    ////Game Send Terminate
    if(KillSocket(TCPSock) != 0)
        debug(Sec("(TCP) Cannot close socket. Error code: ") + std::to_string(WSAGetLastError()));

    if(WSACleanup() != 0)
        debug(Sec("(TCP) Client: WSACleanup() failed!..."));
}
