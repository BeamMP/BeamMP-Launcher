///
/// Created by Anonymous275 on 4/11/2020
///

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WS2tcpip.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>

namespace fs = std::experimental::filesystem;
void Exit(const std::string& Msg);
extern bool MPDEV;

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
std::string STCPRecv(SOCKET socket){
    char buf[65535];
    int len = 65535;
    ZeroMemory(buf, len);
    int BytesRcv = recv(socket, buf, len,0);
    if (BytesRcv == 0){
        std::cout << "(TCP) Connection closing..." << std::endl;
        return "";
    }
    else if (BytesRcv < 0) {
        std::cout << "(TCP) recv failed with error: " << WSAGetLastError() << std::endl;
        closesocket(socket);
        return "";
    }
    return std::string(buf);
}

void ProxyThread(const std::string& IP, int port);
void SyncResources(const std::string&IP,int Port){
    if(MPDEV)std::cout << "Called" << std::endl;
    std::string FileList;
    struct stat info{};
    if(stat( "Resources", &info) != 0){
        _wmkdir(L"Resources");
    }

    /*WSADATA wsaData;
    SOCKET SendingSocket;
    SOCKADDR_IN ServerAddr;
    int RetCode;
    int BytesSent, nlen;

    WSAStartup(514, &wsaData); //2.2


    SendingSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if(SendingSocket == -1)
    {
        if(MPDEV)printf("Client: socket() failed! Error code: %d\n", WSAGetLastError());
        WSACleanup();
        return;
    }

    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_port = htons(Port);
    inet_pton(AF_INET, IP.c_str(), &ServerAddr.sin_addr);

    RetCode = connect(SendingSocket, (SOCKADDR *) &ServerAddr, sizeof(ServerAddr));

    if(RetCode != 0)
    {
        if(MPDEV)std::cout <<"Client: connect() failed! Error code: " << WSAGetLastError() << std::endl;
        closesocket(SendingSocket);
        WSACleanup();
        return;
    }

    getsockname(SendingSocket, (SOCKADDR *)&ServerAddr, (int *)sizeof(ServerAddr));
    BytesSent = send(SendingSocket, "a", 1, 0);

    std::string File, Response, toSend, Data = STCPRecv(SendingSocket);
    std::cout << Data << std::endl;
    if (!Data.empty()) {
        std::vector<std::string> list = Split(Data, ";");
        std::vector<std::string> FileNames(list.begin(), list.begin() + (list.size() / 2));
        std::vector<std::string> FileSizes(list.begin() + (list.size() / 2), list.end());
        list.clear();
        int index = 0;
        for (const std::string &a : FileNames) {
            if (a.empty() || a.length() < 2)continue;
            if (stat(a.c_str(), &info) == 0) {
                if (fs::file_size(a) == std::stoi(FileSizes.at(index))) {
                    index++;
                    continue;
                } else remove(a.c_str());
            }

            std::ofstream LFS;
            LFS.open(a.c_str());
            LFS.close();
            toSend = "b" + a;
            send(SendingSocket, toSend.c_str(), toSend.length(), 0);
            LFS.open(a.c_str(), std::ios_base::app | std::ios::binary);
            do {
                Data = STCPRecv(SendingSocket);
                if (Data.empty()) {
                    File.clear();
                    break;
                }

                if (Data.find("Cannot Open") != std::string::npos) {
                    File.clear();
                    break;
                }
                LFS << Data;
                float per = LFS.tellp() / std::stof(FileSizes.at(index)) * 100;
                std::string Percent = std::to_string(truncf(per * 10) / 10);
                UlStatus = "UlDownloading Resource: " + a.substr(a.find_last_of('/')) + " (" +
                           Percent.substr(0, Percent.find('.') + 2) + "%)";
            } while (LFS.tellp() != std::stoi(FileSizes.at(index)));
            LFS.close();
            File.clear();
        }

        toSend = "M";
        send(SendingSocket, toSend.c_str(), toSend.length(), 0);
        Data = STCPRecv(SendingSocket);
        MStatus = "M" + Data;
    }
    UlStatus = "Uldone";
    std::cout << "Done!" << std::endl;

    if(BytesSent == SOCKET_ERROR)
        if(MPDEV)printf("Client: send() error %d.\n", WSAGetLastError());



    if( shutdown(SendingSocket, SD_SEND) != 0)
        if(MPDEV)printf("Client: Well, there is something wrong with the shutdown() The error code: %d\n", WSAGetLastError());


    if(closesocket(SendingSocket) != 0)
        if(MPDEV)printf("Client: Cannot close \"SendingSocket\" socket. Error code: %d\n", WSAGetLastError());


    if(WSACleanup() != 0)
        if(MPDEV)printf("Client: WSACleanup() failed!...\n");
*/

    ProxyThread(IP,Port);
}