///
/// Created by Anonymous275 on 5/8/2020
///


#include <WS2tcpip.h>
#include <iostream>
#include <thread>
#include <set>
#include <string>

extern bool Terminate;
extern int ClientID;
SOCKET UDPSock;
sockaddr_in ToServer{};

std::set<std::pair<int,std::string>> BigDataAcks;
void UDPSend(const std::string&Data){
    if(ClientID == -1 || UDPSock == INVALID_SOCKET)return;
    std::string Packet = char(ClientID+1) + std::string(":") + Data;
    int sendOk = sendto(UDPSock, Packet.c_str(), int(Packet.length()) + 1, 0, (sockaddr*)&ToServer, sizeof(ToServer));
    if (sendOk == SOCKET_ERROR)std::cout << "Error Code : " << WSAGetLastError() << std::endl;
}

void LOOP(){
    while(UDPSock != -1) {
        for (const std::pair<int, std::string>& a : BigDataAcks) {
            //UDPSend(a.second);
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

void AckID(int ID){
    if(BigDataAcks.empty())return;
    for(const std::pair<int,std::string>& a : BigDataAcks){
        if(a.first == ID)BigDataAcks.erase(a);
    }
}

void TCPSendLarge(const std::string&Data){
    static int ID = 0;
    std::string Header = "BD:" + std::to_string(ID) + ":";
    //BigDataAcks.insert(std::make_pair(ID,Header+Data));
    UDPSend(Header+Data);
    if(ID > 483647)ID = 0;
    else ID++;
}

void ServerParser(const std::string& Data);
void UDPParser(const std::string&Packet){
    if(Packet.substr(0,4) == "ACK:"){
        AckID(stoi(Packet.substr(4)));
        return;
    }else if(Packet.substr(0,3) == "BD:"){
        int pos = Packet.find(':',4);
        std::string pckt = "ACK:" + Packet.substr(3,pos-3);
        UDPSend(pckt);
        pckt = Packet.substr(pos+1);
        ServerParser(pckt);
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
    TCPSend("P");
    UDPSend("p");
    while (!Terminate){
        UDPRcv();
    }

    closesocket(UDPSock);
    WSACleanup();
}