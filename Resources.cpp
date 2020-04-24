///
/// Created by Anonymous275 on 4/11/2020
///

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>

namespace fs = std::experimental::filesystem;

std::string UlStatus = "Ulstart";
std::string MStatus = " ";

std::vector<std::string> Split(const std::string& String,const std::string& delimiter){
    std::vector<std::string> Val;
    size_t pos = 0;
    std::string token,s = String;
    while ((pos = s.find(delimiter)) != std::string::npos) {
        token = s.substr(0, pos);
        Val.push_back(token);
        s.erase(0, pos + delimiter.length());
    }
    Val.push_back(s);
    return Val;
}
void ProxyThread(const std::string& IP, int port);
void SyncResources(const std::string&IP,int Port){
    std::cout << "Called" << std::endl;
    std::string FileList;
    struct stat info{};
    if(stat( "Resources", &info) != 0){
        _wmkdir(L"Resources");
    }

    WSADATA wsaData;
    SOCKET SendingSocket;
    SOCKADDR_IN ServerAddr, ThisSenderInfo;
    int RetCode;
    int BytesSent, nlen;

    WSAStartup(MAKEWORD(2,2), &wsaData);


    SendingSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if(SendingSocket == INVALID_SOCKET)
    {
        printf("Client: socket() failed! Error code: %d\n", WSAGetLastError());
        WSACleanup();
        return;
    }

    ServerAddr.sin_family = AF_INET;

    ServerAddr.sin_port = htons(Port);

    ServerAddr.sin_addr.s_addr = inet_addr(IP.c_str());

    RetCode = connect(SendingSocket, (SOCKADDR *) &ServerAddr, sizeof(ServerAddr));

    if(RetCode != 0)
    {
        printf("Client: connect() failed! Error code: %ld\n", WSAGetLastError());
        closesocket(SendingSocket);
        WSACleanup();
        return;
    }

    getsockname(SendingSocket, (SOCKADDR *)&ServerAddr, (int *)sizeof(ServerAddr));
    BytesSent = send(SendingSocket, "a", 1, 0);

    int iResult;
    char recvbuf[65535];
    int recvbuflen = 65535;

    do {
        iResult = recv(SendingSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0) {
            std::string File, Data = recvbuf,Response,toSend;
            Data.resize(iResult);
            std::vector<std::string> list = Split(Data,";");
            std::vector<std::string> FileNames(list.begin(),list.begin() + (list.size()/2));
            std::vector<std::string> FileSizes(list.begin() + (list.size()/2), list.end());
            list.clear();
            Data.clear();
            Data.resize(recvbuflen);
            int index = 0;
            for(const std::string& a : FileNames){
                if(a.empty() || a.length() < 2)continue;
                if(stat(a.c_str(),&info)==0){
                    if(fs::file_size(a)==std::stoi(FileSizes.at(index))){
                        index++;
                        continue;
                    }else remove(a.c_str());
                }

                std::ofstream LFS;
                LFS.open(a.c_str());
                LFS.close();
                toSend = "b"+a;
                //std::cout << a << std::endl;
                send(SendingSocket, toSend.c_str(), toSend.length(), 0);
                LFS.open (a.c_str(), std::ios_base::app | std::ios::binary);
                do{
                    iResult = recv(SendingSocket, recvbuf, recvbuflen, 0);
                    if(iResult < 0){File.clear(); break;}
                    Data.resize(iResult);
                    memcpy(&Data[0],recvbuf,iResult);
                    if(Data.find("Cannot Open") != std::string::npos){File.clear();break;}
                    LFS << Data;
                    float per = LFS.tellp()/std::stof(FileSizes.at(index))*100;
                    std::string Percent = std::to_string(truncf(per*10)/10);
                    UlStatus="UlDownloading Resource: "+a.substr(a.find_last_of('/'))+" ("+Percent.substr(0,Percent.find('.')+2)+"%)";
                    //std::cout << UlStatus << std::endl;
                }while(LFS.tellp() != std::stoi(FileSizes.at(index)));
                LFS.close();
                File.clear();
            }

            toSend = "M";
            send(SendingSocket, toSend.c_str(), toSend.length(), 0);
            iResult = recv(SendingSocket, recvbuf, recvbuflen, 0);
            if(iResult < 0)break;
            Data.resize(iResult);
            memcpy(&Data[0],recvbuf,iResult);
            MStatus = "M" + Data;
            break;
        }
        else if (iResult == 0)
            printf("Connection closing...\n");
        else  {
            printf("(Resources) recv failed with error: %d\n", WSAGetLastError());
            closesocket(SendingSocket);
            break;
        }
    }while (iResult > 0);
    if(BytesSent == SOCKET_ERROR)
        printf("Client: send() error %d.\n", WSAGetLastError());



    if( shutdown(SendingSocket, SD_SEND) != 0)
        printf("Client: Well, there is something wrong with the shutdown() The error code: %d\n", WSAGetLastError());


    if(closesocket(SendingSocket) != 0)
        printf("Client: Cannot close \"SendingSocket\" socket. Error code: %d\n", WSAGetLastError());


    if(WSACleanup() != 0)
        printf("Client: WSACleanup() failed!...\n");

    UlStatus = "Uldone";
    std::cout << "Done!" << std::endl;
    ProxyThread(IP,Port);
}