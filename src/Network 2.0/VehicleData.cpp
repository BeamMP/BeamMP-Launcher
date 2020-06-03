///
/// Created by Anonymous275 on 5/8/2020
///


#include <WS2tcpip.h>
#include <iostream>
#include <thread>
#include <string>
#include <array>
#include <set>

extern bool Terminate;
extern int ClientID;
extern bool MPDEV;
SOCKET UDPSock;
sockaddr_in ToServer{};
struct PacketData{
    int ID;
    std::string Data;
    int Tries;
};

struct SplitData{
    int Total;
    int ID;
    std::set<std::pair<int,std::string>> Fragments;
};

std::set<SplitData*> SplitPackets;
std::set<PacketData*> BigDataAcks;
void UDPSend(const std::string&Data){
    if(ClientID == -1 || UDPSock == INVALID_SOCKET)return;
    std::string Packet = char(ClientID+1) + std::string(":") + Data;
    int sendOk = sendto(UDPSock, Packet.c_str(), int(Packet.length()) + 1, 0, (sockaddr*)&ToServer, sizeof(ToServer));
    if (sendOk == SOCKET_ERROR)std::cout << "Error Code : " << WSAGetLastError() << std::endl;
}

void LOOP(){
    while(UDPSock != -1) {
        for (PacketData* p : BigDataAcks) {
            if(p->Tries < 20){
                p->Tries++;
                UDPSend(p->Data);
            }else{
                BigDataAcks.erase(p);
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

void AckID(int ID){
    for(PacketData* p : BigDataAcks){
        if(p->ID == ID){
            BigDataAcks.erase(p);
            break;
        }
    }
}

int PacktID(){
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
void SendLarge(const std::string&Data){
    int ID = PacktID();
    std::string Packet;
    if(Data.length() > 1000){
        std::string pckt = Data;
        int S = 1,Split = ceil(float(pckt.length()) / 1000);
        int SID = SplitID();
        while(pckt.length() > 1000){
            Packet = "SC"+std::to_string(S)+"/"+std::to_string(Split)+":"+std::to_string(ID)+"|"+
                    std::to_string(SID)+":"+pckt.substr(0,1000);
            BigDataAcks.insert(new PacketData{ID,Packet,1});
            UDPSend(Packet);
            pckt = pckt.substr(1000);
            S++;
            ID = PacktID();
        }
        Packet = "SC"+std::to_string(S)+"/"+std::to_string(Split)+":"+
                std::to_string(ID)+"|"+std::to_string(SID)+":"+pckt;
        BigDataAcks.insert(new PacketData{ID,Packet,1});
        UDPSend(Packet);
    }else{
        Packet = "BD:" + std::to_string(ID) + ":" + Data;
        BigDataAcks.insert(new PacketData{ID,Packet,1});
        UDPSend(Packet);
    }
}
std::array<int, 50> HandledIDs;
void IDReset(){
    for(int C = 0;C < 50;C++){
        HandledIDs.at(C) = -1;
    }
}
bool Handled(int ID){
    static int Pos = 0;
    for(int id : HandledIDs){
        if(id == ID)return true;
    }
    if(Pos > 49)Pos = 0;
    HandledIDs.at(Pos) = ID;
    Pos++;
    return false;
}
SplitData*GetSplit(int SplitID){
    for(SplitData* a : SplitPackets){
        if(a->ID == SplitID)return a;
    }
    SplitData* a = new SplitData();
    SplitPackets.insert(a);
    return a;
}

void ServerParser(const std::string& Data);
void HandleChunk(const std::string&Data){
    int pos1 = Data.find(':')+1,pos2 = Data.find(':',pos1),pos3 = Data.find('/');
    int pos4 = Data.find('|');
    int Max = stoi(Data.substr(pos3+1,pos1-pos3-2));
    int Current = stoi(Data.substr(2,pos3-2));
    int ID = stoi(Data.substr(pos1,pos4-pos1));
    int SplitID = stoi(Data.substr(pos4+1,pos2-pos4-1));
    std::string ack = "ACK:" + Data.substr(pos1,pos4-pos1);
    UDPSend(ack);
    if(Handled(ID))return;
    SplitData* SData = GetSplit(SplitID);
    SData->Total = Max;
    SData->ID = SplitID;
    SData->Fragments.insert(std::make_pair(Current,Data.substr(pos2+1)));
    if(SData->Fragments.size() == SData->Total){
        std::string ToHandle;
        for(const std::pair<int,std::string>& a : SData->Fragments){
            ToHandle += a.second;
        }
        ServerParser(ToHandle);
        SplitPackets.erase(SData);
    }
}
void UDPParser(const std::string&Packet){
    if(Packet.substr(0,4) == "ACK:"){
        AckID(stoi(Packet.substr(4)));
        if(MPDEV)std::cout << "Got Ack for data" << std::endl;
        return;
    }else if(Packet.substr(0,3) == "BD:"){
        int pos = Packet.find(':',4);
        int ID = stoi(Packet.substr(3,pos-3));
        std::string pckt = "ACK:" + std::to_string(ID);
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
    char buf[10240];
    int len = 10240;
    sockaddr_in FromServer{};
    int clientLength = sizeof(FromServer);
    ZeroMemory(&FromServer, clientLength);
    ZeroMemory(buf, len);
    if(UDPSock == INVALID_SOCKET)return;
    int bytesIn = recvfrom(UDPSock, buf, len, 0, (sockaddr*)&FromServer, &clientLength);
    if (bytesIn == SOCKET_ERROR)
    {
        //std::cout << "Error receiving from Server " << WSAGetLastError() << std::endl;
        return;
    }
    UDPParser(std::string(buf));
}
void TCPSend(const std::string&Data);
void UDPClientMain(const std::string& IP,int Port){
    WSADATA data;
    if (WSAStartup(514, &data)) //2.2
    {
        std::cout << "Can't start Winsock! " << std::endl;
        return;
    }
    ToServer.sin_family = AF_INET;
    ToServer.sin_port = htons(Port);
    inet_pton(AF_INET, IP.c_str(), &ToServer.sin_addr);
    UDPSock = socket(AF_INET, SOCK_DGRAM, 0);
    BigDataAcks.clear();
    std::thread Ack(LOOP);
    Ack.detach();
    IDReset();
    TCPSend("P");
    UDPSend("p");
    while(!Terminate)UDPRcv();
    closesocket(UDPSock);
    WSACleanup();
}