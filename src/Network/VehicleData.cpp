///
/// Created by Anonymous275 on 5/8/2020
///
#include "Zlib/Compressor.h"
#include "Network/network.h"
#include "Security/Enc.h"
#include <WS2tcpip.h>
#include "Logger.h"
#include <sstream>
#include <thread>
#include <string>
#include <array>
#include <set>

SOCKET UDPSock;
sockaddr_in ToServer{};
struct PacketData{
    int ID;
    std::string Data;
    int Tries;
};
struct SplitData{
    int Total{};
    int ID{};
    std::set<std::pair<int,std::string>> Fragments;
};
std::set<SplitData*> SplitPackets;
std::set<PacketData*> BigDataAcks;
int FC(const std::string& s,const std::string& p,int n) {
    auto i = s.find(p);
    int j;
    for (j = 1; j < n && i != std::string::npos; ++j){
        i = s.find(p, i+1);
    }
    if (j == n)return int(i);
    else return -1;
}
void ClearAll(){
    __try{
        for (SplitData*S : SplitPackets){
            if (S != nullptr) {
                delete S;
                S = nullptr;
            }
        }
        for (PacketData*S : BigDataAcks){
            if (S != nullptr) {
                delete S;
                S = nullptr;
            }
        }
    }__except(1){}
    SplitPackets.clear();
    BigDataAcks.clear();
}
void UDPSend(std::string Data){
    if(ClientID == -1 || UDPSock == -1)return;
    if(Data.length() > 400){
        std::string CMP(Comp(Data));
        Data = "ABG:" + CMP;
    }
    std::string Packet = char(ClientID+1) + std::string(":") + Data;
    int sendOk = sendto(UDPSock, Packet.c_str(), int(Packet.size()), 0, (sockaddr*)&ToServer, sizeof(ToServer));
    if (sendOk == SOCKET_ERROR)error(Sec("Error Code : ") + std::to_string(WSAGetLastError()));
}

void LOOP(){
    while(UDPSock != -1) {
        for (PacketData* p : BigDataAcks) {
            if(p != nullptr && p->Tries < 15){
                p->Tries++;
                UDPSend(p->Data);
            }else{
                BigDataAcks.erase(p);
                if(p != nullptr){
                    delete p;
                    p = nullptr;
                }
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
}

void AckID(int ID){
    for(PacketData* p : BigDataAcks){
        if(p != nullptr && p->ID == ID){
            p->Tries = 100;
            break;
        }
    }
}
int PackID(){
    static int ID = -1;
    if(ID > 999999)ID = 0;
    else ID++;
    return ID;
}
int SplitID(){
    static int SID = -1;
    if(SID > 999999)SID = 0;
    else SID++;
    return SID;
}
void SendLarge(std::string Data){
    if(Data.length() > 400){
        std::string CMP(Comp(Data));
        Data = "ABG:" + CMP;
    }
    TCPSend(Data);
}
std::array<int, 100> HandledIDs = {-1};
int APos = 0;
void IDReset(){
    for(int C = 0;C < 100;C++){
        HandledIDs.at(C) = -1;
    }
}
bool Handled(int ID){
    for(int id : HandledIDs){
        if(id == ID)return true;
    }
    if(APos > 99)APos = 0;
    HandledIDs.at(APos) = ID;
    APos++;
    return false;
}
SplitData*GetSplit(int SplitID){
    for(SplitData* a : SplitPackets){
        if(a != nullptr && a->ID == SplitID)return a;
    }
    auto* a = new SplitData();
    SplitPackets.insert(a);
    return a;
}


void HandleChunk(const std::string&Data){
    int pos = FC(Data,"|",5);
    if(pos == -1)return;
    std::stringstream ss(Data.substr(0,pos++));
    std::string t;
    int I = -1;
    //Current Max ID SID
    std::vector<int> Num(4,0);
    while (std::getline(ss, t, '|')) {
        if(I != -1)Num.at(I) = std::stoi(t);
        I++;
    }
    std::string ack = "TRG:" + std::to_string(Num.at(2));
    UDPSend(ack);
    if(Handled(Num.at(2))){
        return;
    }
    std::string Packet = Data.substr(pos);
    SplitData* SData = GetSplit(Num.at(3));
    SData->Total = Num.at(1);
    SData->ID = Num.at(3);
    SData->Fragments.insert(std::make_pair(Num.at(0),Packet));
    if(SData->Fragments.size() == SData->Total){
        std::string ToHandle;
        for(const std::pair<int,std::string>& a : SData->Fragments){
            ToHandle += a.second;
        }
        ServerParser(ToHandle);
        SplitPackets.erase(SData);
        delete SData;
        SData = nullptr;
    }
}
void UDPParser(std::string Packet){
    if(Packet.substr(0,4) == "ABG:"){
        Packet = DeComp(Packet.substr(4));
    }
    if(Packet.substr(0,4) == "TRG:"){
        AckID(stoi(Packet.substr(4)));
        return;
    }else if(Packet.substr(0,3) == "BD:"){
        auto pos = Packet.find(':',4);
        int ID = stoi(Packet.substr(3,pos-3));
        std::string pckt = "TRG:" + std::to_string(ID);
        UDPSend(pckt);
        if(!Handled(ID)) {
            pckt = Packet.substr(pos + 1);
            ServerParser(pckt);
        }
        return;
    }else if(Packet.substr(0,2) == "SC"){
        HandleChunk(Packet);
        return;
    }
    ServerParser(Packet);
}
void UDPRcv(){
    sockaddr_in FromServer{};
    int clientLength = sizeof(FromServer);
    ZeroMemory(&FromServer, clientLength);
    std::string Ret(10240,0);
    if(UDPSock == -1)return;
    int32_t Rcv = recvfrom(UDPSock, &Ret[0], 10240, 0, (sockaddr*)&FromServer, &clientLength);
    if (Rcv == SOCKET_ERROR)return;
    UDPParser(Ret.substr(0,Rcv));
}
void UDPClientMain(const std::string& IP,int Port){
    WSADATA data;
    if (WSAStartup(514, &data)){
        error(Sec("Can't start Winsock!"));
        return;
    }
    ToServer.sin_family = AF_INET;
    ToServer.sin_port = htons(Port);
    inet_pton(AF_INET, IP.c_str(), &ToServer.sin_addr);
    UDPSock = socket(AF_INET, SOCK_DGRAM, 0);
    std::thread Ack(LOOP);
    Ack.detach();
    IDReset();
    TCPSend(Sec("P"));
    UDPSend(Sec("p"));
    while(!Terminate)UDPRcv();
    closesocket(UDPSock);
    WSACleanup();
}