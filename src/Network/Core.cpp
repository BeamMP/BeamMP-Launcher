///
/// Created by Anonymous275 on 7/20/2020
///
#include "Network/network.h"
#include "Security/Enc.h"
#include "Curl/http.h"
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "Startup.h"
#include "Memory.h"
#include "Logger.h"
#include <thread>
#include <set>
#include <charconv>

extern int TraceBack;
std::set<std::string>* ConfList = nullptr;
bool TCPTerminate = false;
int DEFAULT_PORT = 4444;
bool Terminate = false;
std::string UlStatus;
std::string MStatus;
bool once = false;
bool ModLoaded;
long long ping = -1;

void StartSync(const std::string &Data){
    std::string IP = GetAddr(Data.substr(1,Data.find(':')-1));
    if(IP.find('.') == -1){
        if(IP == "DNS")UlStatus = Sec("UlConnection Failed! (DNS Lookup Failed)");
        else UlStatus = Sec("UlConnection Failed! (WSA failed to start)");
        ListOfMods = "-";
        Terminate = true;
        return;
    }
    UlStatus = Sec("UlLoading...");
    TCPTerminate = false;
    Terminate = false;
    ConfList->clear();
    ping = -1;
    std::thread GS(TCPGameServer,IP,std::stoi(Data.substr(Data.find(':')+1)));
    GS.detach();
    info(Sec("Connecting to server"));
}
void Parse(std::string Data,SOCKET CSocket){
    char Code = Data.at(0), SubCode = 0;
    if(Data.length() > 1)SubCode = Data.at(1);
    switch (Code){
        case 'A':
            Data = Data.substr(0,1);
            break;
        case 'B':
            NetReset();
            Terminate = true;
            TCPTerminate = true;
            Data = Code + HTTP_REQUEST(Sec("https://beammp.com/servers-info"),443);
            break;
        case 'C':
            ListOfMods.clear();
            StartSync(Data);
            while(ListOfMods.empty() && !Terminate){
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            if(ListOfMods == "-")Data = "L";
            else Data = "L"+ListOfMods;
            break;
        case 'U':
            if(SubCode == 'l')Data = UlStatus;
            if(SubCode == 'p'){
                if(ping == -1 && Terminate){
                    Data = "Up-2";
                }else Data = "Up" + std::to_string(ping);
            }
            if(!SubCode){
                std::string Ping;
                if(ping == -1 && Terminate)Ping = "-2";
                else Ping = std::to_string(ping);
                Data = std::string(UlStatus) + "\n" + "Up" + Ping;
            }
            break;
        case 'M':
            Data = MStatus;
            break;
        case 'Q':
            if(SubCode == 'S'){
                NetReset();
                Terminate = true;
                TCPTerminate = true;
                ping = -1;
            }
            if(SubCode == 'G')exit(2);
            Data.clear();
            break;
        case 'R': //will send mod name
            if(ConfList->find(Data) == ConfList->end()){
                ConfList->insert(Data);
                ModLoaded = true;
            }
            Data.clear();
            break;
        case 'Z':
            Data = "Z" + GetVer();
            break;
        default:
            Data.clear();
            break;
    }
    if(!Data.empty() && CSocket != -1){
        int res = send(CSocket, (Data+"\n").c_str(), int(Data.size())+1, 0);
        if(res < 0){
            debug(Sec("(Core) send failed with error: ") + std::to_string(WSAGetLastError()));
        }
    }
}
void GameHandler(SOCKET Client){
    if (!once){
        std::thread Memory(MemoryInit);
        Memory.detach();
        once = true;
    }
    //Read byte by byte until '>' is rcved then get the size and read based on it
    int32_t Size,Temp,Rcv;
    char Header[10] = {0};
    do{
        Rcv = 0;
        do{
            Temp = recv(Client,&Header[Rcv],1,0);
            if(Temp < 1)break;
            if(!isdigit(Header[Rcv]) && Header[Rcv] != '>') {
                error(Sec("(Core) Invalid lua communication"));
                closesocket(Client);
                return;
            }
        }while(Header[Rcv++] != '>');
        if(Temp < 1)break;
        if(std::from_chars(Header,&Header[Rcv],Size).ptr[0] != '>'){
            debug(Sec("(Core) Invalid lua Header -> ") + std::string(Header,Rcv));
            break;
        }
        std::string Ret(Size,0);
        Rcv = 0;

        do{
            Temp = recv(Client,&Ret[Rcv],Size-Rcv,0);
            if(Temp < 1)break;
            Rcv += Temp;
        }while(Rcv < Size);
        if(Temp < 1)break;

        std::thread Respond(Parse, Ret, Client);
        Respond.detach();
    }while(Temp > 0);
    if (Temp == 0) {
        debug(Sec("(Core) Connection closing"));
    } else {
        debug(Sec("(Core) recv failed with error: ") + std::to_string(WSAGetLastError()));
    }
    closesocket(Client);
}
void localRes(){
    MStatus = " ";
    UlStatus = Sec("Ulstart");
    if(ConfList != nullptr){
        ConfList->clear();
        delete ConfList;
        ConfList = nullptr;
    }
    ConfList = new std::set<std::string>;
}
void CoreMain() {
    debug(Sec("Core Network on start!"));
    WSADATA wsaData;
    SOCKET LSocket,CSocket;
    struct addrinfo *res = nullptr;
    struct addrinfo hints{};
    int iRes = WSAStartup(514, &wsaData); //2.2
    if (iRes)debug(Sec("WSAStartup failed with error: ") + std::to_string(iRes));
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;
    iRes = getaddrinfo(nullptr, std::to_string(DEFAULT_PORT).c_str(), &hints, &res);
    if (iRes){
        debug(Sec("(Core) addr info failed with error: ") + std::to_string(iRes));
        WSACleanup();
        return;
    }
    LSocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (LSocket == -1){
        debug(Sec("(Core) socket failed with error: ") + std::to_string(WSAGetLastError()));
        freeaddrinfo(res);
        WSACleanup();
        return;
    }
    iRes = bind(LSocket, res->ai_addr, int(res->ai_addrlen));
    if (iRes == SOCKET_ERROR) {
        error(Sec("(Core) bind failed with error: ") + std::to_string(WSAGetLastError()));
        freeaddrinfo(res);
        closesocket(LSocket);
        WSACleanup();
        return;
    }
    iRes = listen(LSocket, SOMAXCONN);
    if (iRes == SOCKET_ERROR) {
        debug(Sec("(Core) listen failed with error: ") + std::to_string(WSAGetLastError()));
        freeaddrinfo(res);
        closesocket(LSocket);
        WSACleanup();
        return;
    }
    do{
        CSocket = accept(LSocket, nullptr, nullptr);
        if (CSocket == -1) {
            error(Sec("(Core) accept failed with error: ") + std::to_string(WSAGetLastError()));
            continue;
        }
        localRes();
        info(Sec("Game Connected!"));
        GameHandler(CSocket);
        warn(Sec("Game Reconnecting..."));
    }while(CSocket);
    closesocket(LSocket);
    WSACleanup();
}
int Handle(EXCEPTION_POINTERS *ep){
    char* hex = new char[100];
    sprintf_s(hex,100, "%lX", ep->ExceptionRecord->ExceptionCode);
    except(Sec("(Core) Code : ") + std::string(hex));
    delete [] hex;
    return 1;
}


void CoreNetwork(){
    while(TraceBack >= 4){
        __try{
                CoreMain();
        }__except(Handle(GetExceptionInformation())){}
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
