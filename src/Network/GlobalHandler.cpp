///
/// Created by Anonymous275 on 7/25/2020
///
#include "Network/network.h"
#include "Security/Enc.h"
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "Logger.h"
#include <charconv>
#include <string>
#include <thread>
#include <mutex>

std::chrono::time_point<std::chrono::steady_clock> PingStart,PingEnd;
bool GConnected = false;
bool CServer = true;
extern SOCKET UDPSock;
extern SOCKET TCPSock;
SOCKET CSocket;

bool CheckBytes(uint32_t Bytes){
    if(Bytes == 0){
        debug(Sec("(Proxy) Connection closing"));
        return false;
    }else if(Bytes < 0){
        debug(Sec("(Proxy) send failed with error: ") + std::to_string(WSAGetLastError()));
        return false;
    }
    return true;
}

void GameSend(std::string Data){
    static std::mutex Lock;
    Lock.lock();
    if(TCPTerminate || !GConnected || CSocket == -1)return;
    static thread_local int32_t Size,Temp,Sent;
    Data += '\n';
    Size = int32_t(Data.size());
    Sent = 0;
    #ifdef DEBUG
        if(Size > 1000){
            debug("Launcher -> game (" +std::to_string(Size)+")");
        }
    #endif
    do{
        Temp = send(CSocket, &Data[Sent], Size - Sent, 0);
        if(!CheckBytes(Temp))return;
        Sent += Temp;
    }while(Sent < Size);
    Lock.unlock();
}
void ServerSend(std::string Data, bool Rel){
    if(Terminate || Data.empty())return;
    if(Data.find("Zp") != std::string::npos && Data.size() > 500){
        abort();
    }
    char C = 0;
    bool Ack = false;
    int DLen = int(Data.length());
    if(DLen > 3)C = Data.at(0);
    if (C == 'O' || C == 'T')Ack = true;
    if(C == 'W' || C == 'Y' || C == 'V' || C == 'E')Rel = true;
    if(Ack || Rel){
        if(Ack || DLen > 1000)SendLarge(Data);
        else TCPSend(Data);
    }else UDPSend(Data);

    if (DLen > 1000) {
        debug(Sec("(Launcher->Server) Bytes sent: ") + std::to_string(Data.length()) + " : "
                     + Data.substr(0, 10)
                     + Data.substr(Data.length() - 10));
    }else if(C == 'Z'){
        //debug("(Game->Launcher) : " + Data);
    }
}
void NetReset(){
    TCPTerminate = false;
    GConnected = false;
    Terminate = false;
    UlStatus = Sec("Ulstart");
    MStatus = " ";
    if(UDPSock != SOCKET_ERROR)closesocket(UDPSock);
    UDPSock = -1;
    if(TCPSock != SOCKET_ERROR)closesocket(TCPSock);
    TCPSock = -1;
    ClearAll();
}

SOCKET SetupListener(){
    static SOCKET LSocket = -1;
    if(LSocket != -1)return LSocket;
    struct addrinfo *result = nullptr;
    struct addrinfo hints{};
    WSADATA wsaData;
    int iRes = WSAStartup(514, &wsaData); //2.2
    if (iRes != 0) {
        error(Sec("(Proxy) WSAStartup failed with error: ") + std::to_string(iRes));
        return -1;
    }
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;
    iRes = getaddrinfo(nullptr, std::to_string(DEFAULT_PORT+1).c_str(), &hints, &result);
    if (iRes != 0) {
        error(Sec("(Proxy) info failed with error: ") + std::to_string(iRes));
        WSACleanup();
    }
    LSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (LSocket == -1) {
        error(Sec("(Proxy) socket failed with error: ") + std::to_string(WSAGetLastError()));
        freeaddrinfo(result);
        WSACleanup();
        return -1;
    }
    iRes = bind(LSocket, result->ai_addr, (int) result->ai_addrlen);
    if (iRes == SOCKET_ERROR) {
        error(Sec("(Proxy) bind failed with error: ") + std::to_string(WSAGetLastError()));
        freeaddrinfo(result);
        closesocket(LSocket);
        WSACleanup();
        return -1;
    }
    freeaddrinfo(result);
    iRes = listen(LSocket, SOMAXCONN);
    if (iRes == SOCKET_ERROR) {
        error(Sec("(Proxy) listen failed with error: ") + std::to_string(WSAGetLastError()));
        closesocket(LSocket);
        WSACleanup();
        return -1;
    }
    return LSocket;
}
void AutoPing(){
    while(!Terminate){
        ServerSend(Sec("p"),false);
        PingStart = std::chrono::high_resolution_clock::now();
        std::this_thread::sleep_for(std::chrono::seconds (1));
    }
}
int ClientID = -1;
void ParserAsync(const std::string& Data){
    if(Data.empty())return;
    char Code = Data.at(0),SubCode = 0;
    if(Data.length() > 1)SubCode = Data.at(1);
    switch (Code) {
        case 'P':
            ClientID = std::stoi(Data.substr(1));
            break;
        case 'p':
            PingEnd = std::chrono::high_resolution_clock::now();
            if(PingStart > PingEnd)ping = 0;
            else ping = std::chrono::duration_cast<std::chrono::milliseconds>(PingEnd-PingStart).count();
            return;
        case 'M':
            MStatus = Data;
            UlStatus = Sec("Uldone");
            return;
        default:
            break;
    }
    GameSend(Data);
}
void ServerParser(const std::string& Data){
    ParserAsync(Data);
}
void NetMain(const std::string& IP, int Port){
    std::thread Ping(AutoPing);
    Ping.detach();
    UDPClientMain(IP,Port);
    CServer = true;
    Terminate = true;
    info(Sec("Connection Terminated!"));
}
void TCPGameServer(const std::string& IP, int Port){
    SOCKET LSocket = SetupListener();
    while (!TCPTerminate && LSocket != -1){
        GConnected = false;
        if(!CServer){
            warn(Sec("Connection still alive terminating"));
            NetReset();
            TCPTerminate = true;
            Terminate = true;
            break;
        }
        if(CServer) {
            std::thread Client(TCPClientMain, IP, Port);
            Client.detach();
        }
        CSocket = accept(LSocket, nullptr, nullptr);
        if (CSocket == -1) {
            error(Sec("(Proxy) accept failed with error: ") + std::to_string(WSAGetLastError()));
            break;
        }
        debug(Sec("(Proxy) Game Connected!"));
        GConnected = true;
        if(CServer){
            std::thread t1(NetMain, IP, Port);
            t1.detach();
            CServer = false;
        }
        int32_t Size,Temp,Rcv;
        char Header[10] = {0};

        //Read byte by byte until '>' is rcved then get the size and read based on it
        do{
            Rcv = 0;

            do{
                Temp = recv(CSocket,&Header[Rcv],1,0);
                if(Temp < 1)break;
            }while(Header[Rcv++] != '>');
            if(Temp < 1)break;
            if(std::from_chars(Header,&Header[Rcv],Size).ptr[0] != '>'){
                debug(Sec("(Game) Invalid lua Header -> ") + std::string(Header,Rcv));
                break;
            }
            std::string Ret(Size,0);
            Rcv = 0;
            do{
                Temp = recv(CSocket,&Ret[Rcv],Size-Rcv,0);
                if(Temp < 1)break;
                Rcv += Temp;
            }while(Rcv < Size);
            if(Temp < 1)break;

            ServerSend(Ret,false);

        }while(Temp > 0);
        if(Temp == 0)debug(Sec("(Proxy) Connection closing"));
        else debug(Sec("(Proxy) recv failed error : ") + std::to_string(WSAGetLastError()));
    }
    TCPTerminate = true;
    GConnected = false;
    Terminate = true;
    if(LSocket == -1){
        UlStatus = Sec("Critical error! check the launcher logs");
    }
    if(CSocket != SOCKET_ERROR)closesocket(CSocket);
}