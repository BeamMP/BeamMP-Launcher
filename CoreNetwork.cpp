///
/// Created by Anonymous275 on 4/3/2020
///
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <string>

#define DEFAULT_BUFLEN 64000
#define DEFAULT_PORT "4444"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

std::string HTTP_REQUEST(const std::string&url,int port);
void ProxyThread(const std::string& IP, int port);

std::string Parse(const std::string& Data){
    char Code = Data.substr(0,1).at(0);
    std::cout << "Code : " << Code << std::endl;
    std::cout << "Data : " << Data.substr(1) << std::endl;
    switch (Code){
        case 'A':
            return Data.substr(0,1);
        case 'B':
            return Code + HTTP_REQUEST("s1.yourthought.co.uk/servers-info",3599);
        case 'C':
            ProxyThread(Data.substr(1,Data.find(':')-1),std::stoi(Data.substr(Data.find(':')+1)));
            return "";
        default:
            return "";
    }
}


void CoreNetworkThread(){
    do{
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
            std::cout <<"WSAStartup failed with error: " << iResult << std::endl;
        }

        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags = AI_PASSIVE;

        // Resolve the server address and port
        iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
        if ( iResult != 0 ) {
            std::cout << "getaddrinfo failed with error: " << iResult << std::endl;
            WSACleanup();
        }

        // Create a socket for connecting to server
        ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (ListenSocket == INVALID_SOCKET) {
            std::cout << "socket failed with error: " << WSAGetLastError() << std::endl;
            freeaddrinfo(result);
            WSACleanup();
        }

        // Setup the TCP listening socket
        iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            std::cout << "bind failed with error: " << WSAGetLastError() << std::endl;
            freeaddrinfo(result);
            closesocket(ListenSocket);
            WSACleanup();
        }

        freeaddrinfo(result);

        iResult = listen(ListenSocket, SOMAXCONN);
        if (iResult == SOCKET_ERROR) {
            std::cout << "listen failed with error: " << WSAGetLastError() << std::endl;
            closesocket(ListenSocket);
            WSACleanup();
        }
        ClientSocket = accept(ListenSocket, nullptr, nullptr);
        if (ClientSocket == INVALID_SOCKET) {
            std::cout << "accept failed with error: " << WSAGetLastError() << std::endl;
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
                std::cout << "Connection closing...\n";
            else  {
                std::cout << "recv failed with error: " << WSAGetLastError() << std::endl;
                closesocket(ClientSocket);
                WSACleanup();
            }
            if(!Response.empty()){
                iSendResult = send( ClientSocket, Response.c_str(), int(Response.length()), 0);
                if (iSendResult == SOCKET_ERROR) {
                    std::cout << "send failed with error: " << WSAGetLastError() << std::endl;
                    closesocket(ClientSocket);
                    WSACleanup();
                }else{
                    std::cout << "Bytes sent: " << iSendResult << std::endl;
                }
            }
        } while (iResult > 0);

        iResult = shutdown(ClientSocket, SD_SEND);
        if (iResult == SOCKET_ERROR) {
            std::cout << "shutdown failed with error: " << WSAGetLastError() << std::endl;
            closesocket(ClientSocket);
            WSACleanup();
        }
        closesocket(ClientSocket);
        WSACleanup();
    }while (true);
}