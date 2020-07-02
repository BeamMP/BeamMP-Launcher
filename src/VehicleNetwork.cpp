///
/// Created by Anonymous275 on 4/23/2020
///

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <string>
#include <thread>
#include <queue>

extern bool TCPTerminate;
extern bool Dev;
void Print(const std::string&MSG);
std::queue<std::string> VNTCPQueue;
//void RUDPSEND(const std::string&Data,bool Rel);
#define DEFAULT_PORT "4446"

void Responder(const SOCKET *CS){
    SOCKET ClientSocket = *CS;
    int iSendResult;
    while(!TCPTerminate){
        while (!VNTCPQueue.empty()) {
            VNTCPQueue.front() += "\n";
            iSendResult = send(ClientSocket, VNTCPQueue.front().c_str(), VNTCPQueue.front().length(), 0);
            if (iSendResult == SOCKET_ERROR) {
                if(Dev)std::cout << "(VN) send failed with error: " << WSAGetLastError() << std::endl;
                break;
            } else {
                if(iSendResult > 1000){
                    if(Dev){std::cout << "(Launcher->Game VN) Bytes sent: " << iSendResult << " : " << VNTCPQueue.front().substr(0, 10)
                                        << VNTCPQueue.front().substr(VNTCPQueue.front().length()-10) << std::endl;}
                }
                VNTCPQueue.pop();
            }
        }
        std::this_thread::sleep_for(std::chrono::nanoseconds(1));
    }
}
std::string Compress(const std::string&Data);
std::string Decompress(const std::string&Data);
void VehicleNetworkStart(){
    do {
        if(Dev)std::cout << "VN on Start" << std::endl;
        WSADATA wsaData;
        int iResult;
        SOCKET ListenSocket = INVALID_SOCKET;
        SOCKET ClientSocket = INVALID_SOCKET;

        struct addrinfo *result = nullptr;
        struct addrinfo hints{};

        int iSendResult;
        char recvbuf[7507];
        int recvbuflen = 6507;

        // Initialize Winsock
        iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != 0) {
            if(Dev)std::cout << "(VN) WSAStartup failed with error: " << iResult << std::endl;
            std::cin.get();
            exit(-1);
        }

        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags = AI_PASSIVE;

        // Resolve the server address and port
        iResult = getaddrinfo(nullptr, DEFAULT_PORT, &hints, &result);
        if (iResult != 0) {
            if(Dev)std::cout << "(VN) getaddrinfo failed with error: " << iResult << std::endl;
            WSACleanup();
            break;
        }

        // Create a socket for connecting to server
        ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (ListenSocket == INVALID_SOCKET) {
            if(Dev)std::cout << "(VN) socket failed with error: " << WSAGetLastError() << std::endl;
            freeaddrinfo(result);
            WSACleanup();
            break;
        }

        // Setup the TCP listening socket
        iResult = bind(ListenSocket, result->ai_addr, (int) result->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            if(Dev)std::cout << "(VN) bind failed with error: " << WSAGetLastError() << std::endl;
            freeaddrinfo(result);
            closesocket(ListenSocket);
            WSACleanup();
            break;
        }

        freeaddrinfo(result);

        iResult = listen(ListenSocket, SOMAXCONN);
        if (iResult == SOCKET_ERROR) {
            if(Dev)std::cout << "(VN) listen failed with error: " << WSAGetLastError() << std::endl;
            closesocket(ListenSocket);
            WSACleanup();
            continue;
        }
        ClientSocket = accept(ListenSocket, NULL, NULL);
        if (ClientSocket == INVALID_SOCKET) {
            if(Dev)std::cout << "(VN) accept failed with error: " << WSAGetLastError() << std::endl;
            closesocket(ListenSocket);
            WSACleanup();
            continue;
        }
        closesocket(ListenSocket);
        if(Dev)std::cout << "(VN) Game Connected!" << std::endl;

        std::thread TCPSend(Responder,&ClientSocket);
        TCPSend.detach();
        do {
            //std::cout << "(Proxy) Waiting for Game Data..." << std::endl;
            iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
            if (iResult > 0) {
                std::string buff;
                buff.resize(iResult*2);
                memcpy(&buff[0],recvbuf,iResult);
                buff.resize(iResult);
                //Print(buff);
                if(Dev) {
                    std::string cmp = Compress(buff), dcm = Decompress(cmp);
                    std::cout << "Compressed Size : " << cmp.length() << std::endl;
                    std::cout << "Decompressed Size : " << dcm.length() << std::endl;
                    if (cmp == dcm) {
                        std::cout << "Success!" << std::endl;
                    } else {
                        std::cout << "Fail!" << std::endl;
                    }
                }
                //RUDPSEND(buff,false);
                //std::cout << "(Game->Launcher VN) Data : " << buff.length() << std::endl;
            } else if (iResult == 0) {
                if(Dev)std::cout << "(VN) Connection closing...\n";
                closesocket(ClientSocket);
                WSACleanup();

                continue;
            } else {
                if(Dev)std::cout << "(VN) recv failed with error: " << WSAGetLastError() << std::endl;
                closesocket(ClientSocket);
                WSACleanup();
                continue;
            }
        } while (iResult > 0);

        iResult = shutdown(ClientSocket, SD_SEND);
        if (iResult == SOCKET_ERROR) {
            if(Dev)std::cout << "(VN) shutdown failed with error: " << WSAGetLastError() << std::endl;
            closesocket(ClientSocket);
            WSACleanup();
            continue;
        }
        closesocket(ClientSocket);
        WSACleanup();
    }while (!TCPTerminate);
}