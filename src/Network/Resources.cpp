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
    if(stat( Sec("Resources"), &info) != 0){
        _wmkdir(SecW(L"Resources"));
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
    if(Res.empty() || Res[0] == 'E' || Res != "WS"){
        Abord();
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

void SyncResources(SOCKET Sock){
    std::string Ret = Auth(Sock);
    if(Ret.empty())return;

    info(Sec("Checking Resources..."));
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
    if(!FNames.empty())info(Sec("Syncing..."));
    for(auto FN = FNames.begin(),FS = FSizes.begin(); FN != FNames.end() && !Terminate; ++FN,++FS) {
        auto pos = FN->find_last_of('/');
        if (pos != std::string::npos) {
            a = Sec("Resources") + FN->substr(pos);
        } else continue;
        Pos++;
        if (fs::exists(a)) {
            if (FS->find_first_not_of("0123456789") != std::string::npos)continue;
            if (fs::file_size(a) == std::stoi(*FS)){
                UlStatus = Sec("UlLoading Resource: (") + std::to_string(Pos) + "/" + std::to_string(Amount) +
                           "): " + a.substr(a.find_last_of('/'));
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                try {
                    fs::copy_file(a, Sec("BeamNG/mods") + a.substr(a.find_last_of('/')),
                                  fs::copy_options::overwrite_existing);
                } catch (...) {
                    Terminate = true;
                    continue;
                }
                WaitForConfirm();
                continue;
            }else remove(a.c_str());
        }
        CheckForDir();
        do {
            TCPSend("f" + *FN,Sock);
            int Recv = 0,Size = std::stoi(*FS);
            char*File = new char[Size];
            ZeroMemory(File,Size);
            do {
                auto Data = TCPRcv(Sock);
                size_t BytesRcv = Data.size();
                if (Data == "Cannot Open" || Terminate)break;
                memcpy_s(File+Recv,BytesRcv,&Data[0],BytesRcv);
                Recv += int(BytesRcv);
                float per = float(Recv)/std::stof(*FS) * 100;
                std::string Percent = std::to_string(truncf(per * 10) / 10);
                UlStatus = Sec("UlDownloading Resource: (") + std::to_string(Pos) + "/" + std::to_string(Amount) +
                           "): " + a.substr(a.find_last_of('/')) + " (" +
                           Percent.substr(0, Percent.find('.') + 2) + "%)";
            } while (Recv != Size && Recv < Size && !Terminate);
            if(Terminate)break;
            UlStatus = Sec("UlLoading Resource: (") + std::to_string(Pos) + "/" + std::to_string(Amount) +
                       "): " + a.substr(a.find_last_of('/'));
            std::ofstream LFS;
            LFS.open(a.c_str(), std::ios_base::app | std::ios::binary);
            if (LFS.is_open()) {
                LFS.write(File, Recv);
                LFS.close();
            }
            delete[] File;
        }while(fs::file_size(a) != std::stoi(*FS) && !Terminate);
        if(!Terminate)fs::copy_file(a,Sec("BeamNG/mods")+a.substr(a.find_last_of('/')), fs::copy_options::overwrite_existing);
        WaitForConfirm();
    }
    FNames.clear();
    FSizes.clear();
    a.clear();
    if(!Terminate){
        TCPSend("Done",Sock);
        info("Done!");
    }else{
        UlStatus = "Ulstart";
        info("Connection Terminated!");
    }
}