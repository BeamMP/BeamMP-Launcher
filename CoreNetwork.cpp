///
/// Created by Anonymous275 on 4/3/2020
///
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <string>
#include <thread>
#define DEFAULT_BUFLEN 64000
#define DEFAULT_PORT "4444"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

std::string HTTP_REQUEST(const std::string&url,int port);
void SyncResources(const std::string& IP, int port);
void Exit(const std::string& Msg);
extern std::string UlStatus;
extern std::string MStatus;
extern int ping;
extern bool Terminate;
extern bool TCPTerminate;
extern bool MPDEV;
void StartSync(const std::string &Data){
    std::thread t1(SyncResources,Data.substr(1,Data.find(':')-1),std::stoi(Data.substr(Data.find(':')+1)));
    //std::thread t1(SyncResources,"127.0.0.1",30814);
    t1.detach();
}

std::string Parse(const std::string& Data){
    char Code = Data.substr(0,1).at(0), SubCode = 0;
    if(Data.length() > 1)SubCode = Data.substr(1,1).at(0);
    ////std::cout << "Code : " << Code << std::endl;
    ////std::cout << "Data : " << Data.substr(1) << std::endl;
    switch (Code){
        case 'A':
            return Data.substr(0,1);
        case 'B':
            return Code + HTTP_REQUEST("s1.yourthought.co.uk/servers-info",3599);
        case 'C':
            StartSync(Data);
            return "";
        case 'U':
            if(SubCode == 'l')return UlStatus;
            if(SubCode == 'p')return "Up" + std::to_string(ping);
            if(!SubCode)return UlStatus+ "\n" + "Up" + std::to_string(ping);
        case 'M':
            return MStatus;
        case 'Q':
            if(SubCode == 'S'){
                Terminate = true;
                TCPTerminate = true; ////Revisit later when TCP is stable
            }
            if(SubCode == 'G')exit(2);
            return "";
        default:
            return "";
    }
}


[[noreturn]] void CoreNetworkThread(){
    std::cout << "Ready!" << std::endl;
    do{
        if(MPDEV)std::cout << "Core Network on start!" << std::endl;
        WSADATA wsaData;
        int iResult;
        SOCKET ListenSocket = INVALID_SOCKET;
        SOCKET ClientSocket = INVALID_SOCKET;

        struct addrinfo *result = nullptr;
        struct addrinfo hints{};

        int iSendResult;
        char recvbuf[DEFAULT_BUFLEN];
        int recvbuflen = DEFAULT_BUFLEN;

        // Initialize Winsock
        iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
        if (iResult != 0) {
            if(MPDEV)std::cout <<"WSAStartup failed with error: " << iResult << std::endl;
        }

        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags = AI_PASSIVE;

        // Resolve the server address and port
        iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
        if ( iResult != 0 ) {
            if(MPDEV)std::cout << "(Core) getaddrinfo failed with error: " << iResult << std::endl;
            WSACleanup();
        }

        // Create a socket for connecting to server
        ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (ListenSocket == INVALID_SOCKET) {
            if(MPDEV)std::cout << "(Core) socket failed with error: " << WSAGetLastError() << std::endl;
            freeaddrinfo(result);
            WSACleanup();
        }

        // Setup the TCP listening socket
        iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            if(MPDEV)Exit("(Core) bind failed with error: " + std::to_string(WSAGetLastError()));
            freeaddrinfo(result);
            closesocket(ListenSocket);
            WSACleanup();
        }

        freeaddrinfo(result);

        iResult = listen(ListenSocket, SOMAXCONN);
        if (iResult == SOCKET_ERROR) {
            if(MPDEV)std::cout << "(Core) listen failed with error: " << WSAGetLastError() << std::endl;
            closesocket(ListenSocket);
            WSACleanup();
        }
        ClientSocket = accept(ListenSocket, nullptr, nullptr);
        if (ClientSocket == INVALID_SOCKET) {
            if(MPDEV)std::cout << "(Core) accept failed with error: " << WSAGetLastError() << std::endl;
            closesocket(ListenSocket);
            WSACleanup();
        }
        closesocket(ListenSocket);

        do {
            std::string Response;

            iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
            if (iResult > 0) {
                std::string data = recvbuf;
                data.resize(iResult);
                Response = Parse(data) + "\n";
            }

            else if (iResult == 0)
                if(MPDEV)std::cout << "(Core) Connection closing...\n";
            else  {
                    if(MPDEV)std::cout << "(Core) recv failed with error: " << WSAGetLastError() << std::endl;
                closesocket(ClientSocket);
                WSACleanup();
            }
            if(!Response.empty()){
                iSendResult = send( ClientSocket, Response.c_str(), Response.length(), 0);
                if (iSendResult == SOCKET_ERROR) {
                    if(MPDEV)std::cout << "send failed with error: " << WSAGetLastError() << std::endl;
                    closesocket(ClientSocket);
                    WSACleanup();
                }else{
                    ///std::cout << "Bytes sent: " << iSendResult << std::endl;
                }
            }
        } while (iResult > 0);

        iResult = shutdown(ClientSocket, SD_SEND);
        if (iResult == SOCKET_ERROR) {
            if(MPDEV)std::cout << "(Core) shutdown failed with error: " << WSAGetLastError() << std::endl;
            closesocket(ClientSocket);
            WSACleanup();
            Sleep(500);
        }
        closesocket(ClientSocket);
        WSACleanup();
    }while (true);
}