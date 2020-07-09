///
/// Created by Anonymous275 on 4/3/2020
///
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <set>
extern int DEFAULT_PORT;
std::string HTTP_REQUEST(const std::string&url,int port);
void ProxyThread(const std::string& IP, int port);
void Exit(const std::string& Msg);
extern std::string UlStatus;
extern std::string MStatus;
extern int ping;
extern bool Terminate;
extern bool TCPTerminate;
extern bool Dev;
extern std::string ListOfMods;
bool Confirm = false;
void Reset();
std::set<std::string> Conf;

void StartSync(const std::string &Data){
    UlStatus = "UlLoading...";
    Terminate = false;
    TCPTerminate = false;
    Conf.clear();
    ping = -1;
    std::thread t1(ProxyThread,Data.substr(1,Data.find(':')-1),std::stoi(Data.substr(Data.find(':')+1)));
    //std::thread t1(ProxyThread,"127.0.0.1",30814);
    t1.detach();
}

std::string Parse(const std::string& Data){
    char Code = Data.substr(0,1).at(0), SubCode = 0;
    if(Data.length() > 1)SubCode = Data.substr(1,1).at(0);
    switch (Code){
        case 'A':
            return Data.substr(0,1);
        case 'B':
            Reset();
            Terminate = true;
            TCPTerminate = true;
            return Code + HTTP_REQUEST("s1.yourthought.co.uk/servers-info",3599);
        case 'C':
            ListOfMods.clear();
            StartSync(Data);
            std::cout << "Connecting to server" << std::endl;
            while(ListOfMods.empty() && !Terminate){
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            if(ListOfMods == "-")return "L";
            else return "L"+ListOfMods;
        case 'U':
            if(SubCode == 'l')return UlStatus;
            if(SubCode == 'p')return "Up" + std::to_string(ping);
            if(!SubCode)return UlStatus+ "\n" + "Up" + std::to_string(ping);
        case 'M':
            return MStatus;
        case 'Q':
            if(SubCode == 'S'){
                Reset();
                Terminate = true;
                TCPTerminate = true;
                ping = -1;
            }
            if(SubCode == 'G')exit(2);
            return "";
        case 'R': //will send mod name
            if(Conf.find(Data) == Conf.end()){
                Conf.insert(Data);
                Confirm = true;
            }
            return "";
        default:
            return "";
    }
}
bool once = false;
[[noreturn]] void MemoryInit();
[[noreturn]] void CoreNetworkThread(){
    try {
        std::cout << "Ready!" << std::endl;
        do {
            if (Dev)std::cout << "Core Network on start!" << std::endl;
            WSADATA wsaData;
            int iResult;
            SOCKET ListenSocket;
            SOCKET ClientSocket;

            struct addrinfo *result = nullptr;
            struct addrinfo hints{};

            int iSendResult;
            char recvbuf[64000];
            int recvbuflen = 64000;

            // Initialize Winsock
            iResult = WSAStartup(514, &wsaData); //2.2
            if (iResult != 0) {
                if (Dev)std::cout << "WSAStartup failed with error: " << iResult << std::endl;
            }

            ZeroMemory(&hints, sizeof(hints));
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_protocol = IPPROTO_TCP;
            hints.ai_flags = AI_PASSIVE;

            // Resolve the server address and port
            iResult = getaddrinfo(nullptr, std::to_string(DEFAULT_PORT).c_str(), &hints, &result);
            if (iResult != 0) {
                if (Dev)std::cout << "(Core) getaddrinfo failed with error: " << iResult << std::endl;
                WSACleanup();
            }

            // Create a socket for connecting to server
            ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
            if (ListenSocket == -1) {
                if (Dev)std::cout << "(Core) socket failed with error: " << WSAGetLastError() << std::endl;
                freeaddrinfo(result);
                WSACleanup();
            }else{
                iResult = bind(ListenSocket, result->ai_addr, (int) result->ai_addrlen);
                if (iResult == SOCKET_ERROR) {
                    if (Dev)Exit("(Core) bind failed with error: " + std::to_string(WSAGetLastError()));
                    freeaddrinfo(result);
                    closesocket(ListenSocket);
                    WSACleanup();
                }
            }

            
            iResult = listen(ListenSocket, SOMAXCONN);
            if (iResult == SOCKET_ERROR) {
                if (Dev)std::cout << "(Core) listen failed with error: " << WSAGetLastError() << std::endl;
                closesocket(ListenSocket);
                WSACleanup();
            }
            ClientSocket = accept(ListenSocket, nullptr, nullptr);
            if (ClientSocket == -1) {
                if (Dev)std::cout << "(Core) accept failed with error: " << WSAGetLastError() << std::endl;
                closesocket(ListenSocket);
                WSACleanup();
            }
            closesocket(ListenSocket);
            if (!once) {
                std::thread Memory(MemoryInit);
                Memory.detach();
                once = true;
            }
            do {
                std::string Response;
                iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
                if (iResult > 0) {
                    std::string data = recvbuf;
                    data.resize(iResult);
                    Response = Parse(data);
                } else if (iResult == 0) {
                    if (Dev)std::cout << "(Core) Connection closing...\n";
                } else {
                    if (Dev)std::cout << "(Core) recv failed with error: " << WSAGetLastError() << std::endl;
                    closesocket(ClientSocket);
                    WSACleanup();
                }
                if (!Response.empty()) {
                    iSendResult = send(ClientSocket, (Response+"\n").c_str(), int(Response.length())+1, 0);
                    if (iSendResult == SOCKET_ERROR) {
                        if (Dev)std::cout << "send failed with error: " << WSAGetLastError() << std::endl;
                        closesocket(ClientSocket);
                        WSACleanup();
                    } else {
                        ///std::cout << "Bytes sent: " << iSendResult << std::endl;
                    }
                }
            } while (iResult > 0);

            iResult = shutdown(ClientSocket, SD_SEND);
            if (iResult == SOCKET_ERROR) {
                if (Dev)std::cout << "(Core) shutdown failed with error: " << WSAGetLastError() << std::endl;
                closesocket(ClientSocket);
                WSACleanup();
                Sleep(500);
            }
            closesocket(ClientSocket);
            WSACleanup();
        } while (true);
    } catch (std::exception&e) {
        std::cout << "Exception! : " << e.what() << std::endl;
        system("pause");
        exit(1);
    }
}