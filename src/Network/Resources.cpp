///
/// Created by Anonymous275 on 4/11/2020
///

#include "Network/network.h"
#include "Security/Init.h"
#include "Security/Enc.h"
#include <WS2tcpip.h>
#include <filesystem>
#include "Startup.h"
#include "Logger.h"
#include <iostream>
#include <cstring>
#include <fstream>
#include <string>
#include <thread>
#include <atomic>
#include <vector>
#include <future>

namespace fs = std::experimental::filesystem;
std::string ListOfMods;
std::vector<std::string> Split(const std::string& String,const std::string& delimiter){
    std::vector<std::string> Val;
    size_t pos;
    std::string token,s = String;
    while ((pos = s.find(delimiter)) != std::string::npos) {
        token = s.substr(0, pos);
        if(!token.empty())Val.push_back(token);
        s.erase(0, pos + delimiter.length());
    }
    if(!s.empty())Val.push_back(s);
    return Val;
}

void CheckForDir(){
    struct stat info{};
    if(stat( "Resources", &info) != 0){
        _wmkdir(L"Resources");
    }
}
void WaitForConfirm(){
    while(!Terminate && !ModLoaded){
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ModLoaded = false;
}


void Abord(){
    Terminate = true;
    TCPTerminate = true;
    info("Terminated!");
}

std::string Auth(SOCKET Sock){
    TCPSend("VC" + GetVer(),Sock);

    auto Res = TCPRcv(Sock);

    if(Res.empty() || Res[0] == 'E'){
        Abord();
        return "";
    }

    TCPSend(PublicKey,Sock);
    if(Terminate)return "";

    Res = TCPRcv(Sock);
    if(Res.empty() || Res[0] != 'P'){
        Abord();
        return "";
    }

    Res = Res.substr(1);
    if(Res.find_first_not_of("0123456789") == std::string::npos){
        ClientID = std::stoi(Res);
    }else{
        Abord();
        UUl("Authentication failed!");
        return "";
    }
    TCPSend("SR",Sock);
    if(Terminate)return "";

    Res = TCPRcv(Sock);

    if(Res[0] == 'E'){
        Abord();
        return "";
    }

    if(Res.empty() || Res == "-"){
        info("Didn't Receive any mods...");
        ListOfMods = "-";
        TCPSend("Done",Sock);
        info("Done!");
        return "";
    }
    return Res;
}

void UpdateUl(bool D,const std::string&msg){
    if(D)UlStatus = "UlDownloading Resource: " + msg;
    else UlStatus = "UlLoading Resource: " + msg;
}

void AsyncUpdate(uint64_t& Rcv,uint64_t Size,const std::string& Name){
    do {
        double pr = Rcv / double(Size) * 100;
        std::string Per = std::to_string(trunc(pr * 10) / 10);
        UpdateUl(true, Name + " (" + Per.substr(0, Per.find('.') + 2) + "%)");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    }while(!Terminate && Rcv < Size);
}

std::string TCPRcvRaw(SOCKET Sock,uint64_t& GRcv, uint64_t Size){
    if(Sock == -1){
        Terminate = true;
        UUl("Invalid Socket");
        return "";
    }
    char* File = new char[Size];
    uint64_t Rcv = 0;
    int32_t Temp;
    do{
        int Len = int(Size-Rcv);
        if(Len > 1000000)Len = 1000000;
        Temp = recv(Sock, &File[Rcv], Len, MSG_WAITALL);
        if(Temp < 1){
            info(std::to_string(Temp));
            UUl("Socket Closed Code 1");
            KillSocket(Sock);
            Terminate = true;
            delete[] File;
            return "";
        }
        Rcv += Temp;
        GRcv += Temp;
    }while(Rcv < Size && !Terminate);
    std::string Ret = std::string(File,Size);
    delete[] File;
    return Ret;
}
void MultiKill(SOCKET Sock,SOCKET Sock1){
    KillSocket(Sock1);
    KillSocket(Sock);
    Terminate = true;
}
SOCKET InitDSock(){
    SOCKET DSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    SOCKADDR_IN ServerAddr;
    if(DSock < 1){
        Terminate = true;
        return 0;
    }
    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_port = htons(LastPort);
    inet_pton(AF_INET, LastIP.c_str(), &ServerAddr.sin_addr);
    if(connect(DSock, (SOCKADDR *) &ServerAddr, sizeof(ServerAddr)) != 0){
        KillSocket(DSock);
        Terminate = true;
        return 0;
    }
    char Code[2] = {'D',char(ClientID)};
    if(send(DSock,Code,2,0) != 2){
        KillSocket(DSock);
        Terminate = true;
        return 0;
    }
    return DSock;
}

std::string MultiDownload(SOCKET MSock,SOCKET DSock, uint64_t Size, const std::string& Name){

    uint64_t GRcv = 0, MSize = Size/2, DSize = Size - MSize;

    std::thread Au(AsyncUpdate,std::ref(GRcv),Size,Name);
    Au.detach();

    std::packaged_task<std::string()> task([&] { return TCPRcvRaw(MSock,GRcv,MSize); });
    std::future<std::string> f1 = task.get_future();
    std::thread Dt(std::move(task));
    Dt.detach();

    std::string Ret = TCPRcvRaw(DSock,GRcv,DSize);

    if(Ret.empty()){
        MultiKill(MSock,DSock);
        return "";
    }

    f1.wait();
    std::string Temp = f1.get();

    if(Temp.empty()){
        MultiKill(MSock,DSock);
        return "";
    }
    if(Au.joinable())Au.join();


    Ret += Temp;
    return Ret;
}


void SyncResources(SOCKET Sock){
    std::string Ret = Auth(Sock);
    if(Ret.empty())return;

    info("Checking Resources...");
    CheckForDir();

    std::vector<std::string> list = Split(Ret, ";");
    std::vector<std::string> FNames(list.begin(), list.begin() + (list.size() / 2));
    std::vector<std::string> FSizes(list.begin() + (list.size() / 2), list.end());
    list.clear();
    Ret.clear();

    int Amount = 0,Pos = 0;
    std::string a,t;
    for(const std::string&name : FNames){
        if(!name.empty()){
            t += name.substr(name.find_last_of('/') + 1) + ";";
        }
    }
    if(t.empty())ListOfMods = "-";
    else ListOfMods = t;
    t.clear();
    for(auto FN = FNames.begin(),FS = FSizes.begin(); FN != FNames.end() && !Terminate; ++FN,++FS) {
        auto pos = FN->find_last_of('/');
        if (pos == std::string::npos)continue;
        Amount++;
    }
    if(!FNames.empty())info("Syncing...");
    for(auto FN = FNames.begin(),FS = FSizes.begin(); FN != FNames.end() && !Terminate; ++FN,++FS) {
        auto pos = FN->find_last_of('/');
        if (pos != std::string::npos) {
            a = "Resources" + FN->substr(pos);
        } else continue;
        Pos++;
        if (fs::exists(a)) {
            if (FS->find_first_not_of("0123456789") != std::string::npos)continue;
            if (fs::file_size(a) == std::stoi(*FS)){
                UpdateUl(false,"(" + std::to_string(Pos) + "/" + std::to_string(Amount) + "): " + a.substr(a.find_last_of('/')));
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                try {
                    fs::copy_file(a, "BeamNG/mods" + a.substr(a.find_last_of('/')),
                                  fs::copy_options::overwrite_existing);
                } catch (std::exception& e) {
                    error("Failed copy to the mods folder! " + std::string(e.what()));
                    Terminate = true;
                    continue;
                }
                WaitForConfirm();
                continue;
            }else remove(a.c_str());
        }
        CheckForDir();
        std::string FName = a.substr(a.find_last_of('/'));
        SOCKET DSock = InitDSock();
        do {
            TCPSend("f" + *FN,Sock);

            std::string Data = TCPRcv(Sock);
            if (Data == "CO" || Terminate){
                Terminate = true;
                UUl("Server cannot find " + FName);
                break;
            }

            std::string Name = "(" + std::to_string(Pos) + "/" + std::to_string(Amount) + "): " + FName;

            Data = MultiDownload(Sock,DSock,std::stoull(*FS), Name);

            if(Terminate)break;
            UpdateUl(false,"("+std::to_string(Pos)+"/"+std::to_string(Amount)+"): "+FName);
            std::ofstream LFS;
            LFS.open(a.c_str(), std::ios_base::app | std::ios::binary);
            if (LFS.is_open()) {
                LFS.write(Data.c_str(), Data.size());
                LFS.close();
            }

        }while(fs::file_size(a) != std::stoi(*FS) && !Terminate);
        KillSocket(DSock);
        if(!Terminate)fs::copy_file(a,"BeamNG/mods"+FName, fs::copy_options::overwrite_existing);
        WaitForConfirm();
    }
    if(!Terminate){
        TCPSend("Done",Sock);
        info("Done!");
    }else{
        UlStatus = "Ulstart";
        info("Connection Terminated!");
    }
}