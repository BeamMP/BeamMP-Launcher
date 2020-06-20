///
/// Created by Anonymous275 on 4/11/2020
///

#include <WS2tcpip.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

extern std::vector<std::string> GlobalInfo;
void Exit(const std::string& Msg);
namespace fs = std::experimental::filesystem;
extern std::string UlStatus;
extern bool Terminate;
extern bool Confirm;
extern bool MPDEV;
std::string ListOfMods;
std::vector<std::string> Split(const std::string& String,const std::string& delimiter){
    std::vector<std::string> Val;
    size_t pos = 0;
    std::string token,s = String;
    while ((pos = s.find(delimiter)) != std::string::npos) {
        token = s.substr(0, pos);
        if(!token.empty())Val.push_back(token);
        s.erase(0, pos + delimiter.length());
    }
    if(!s.empty())Val.push_back(s);
    return Val;
}

void STCPSend(SOCKET socket,const std::string&Data){
    if(socket == INVALID_SOCKET){
        Terminate = true;
        return;
    }
    int BytesSent = send(socket, Data.c_str(), int(Data.length())+1, 0);
    if (BytesSent == 0){
        if(MPDEV)std::cout << "(TCP) Connection closing..." << std::endl;
        Terminate = true;
        return;
    }
    else if (BytesSent < 0) {
        if(MPDEV)std::cout << "(TCP) send failed with error: " << WSAGetLastError() << std::endl;
        closesocket(socket);
        Terminate = true;
        return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}
std::pair<char*,int> STCPRecv(SOCKET socket){
    char buf[64000];
    int len = 64000;
    ZeroMemory(buf, len);
    int BytesRcv = recv(socket, buf, len,0);
    if (BytesRcv == 0){
        std::cout << "(TCP) Connection closing..." << std::endl;
        Terminate = true;
        return std::make_pair((char*)"",0);
    }
    else if (BytesRcv < 0) {
        std::cout << "(TCP) recv failed with error: " << WSAGetLastError() << std::endl;
        closesocket(socket);
        Terminate = true;
        return std::make_pair((char*)"",0);
    }
    char* Ret = new char[BytesRcv];
    memcpy_s(Ret,BytesRcv,buf,BytesRcv);
    ZeroMemory(buf, len);
    return std::make_pair(Ret,BytesRcv);
}
void CheckForDir(){
    struct stat info{};
    if(stat( "Resources", &info) != 0){
        _wmkdir(L"Resources");
    }
}
void WaitForConfirm(){
    while(!Terminate && !Confirm){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    Confirm = false;
}

void SyncResources(SOCKET Sock){
    if(MPDEV)std::cout << "SyncResources Called" << std::endl;
    CheckForDir();
    STCPSend(Sock,"NR" + GlobalInfo.at(0)+":"+GlobalInfo.at(2));
    STCPSend(Sock,"SR");
    char* Res = STCPRecv(Sock).first;
    if(strlen(Res) == 0){
        std::cout << "Didn't Receive any mod from server skipping..." << std::endl;
        STCPSend(Sock,"Done");
        UlStatus = "UlDone";
        ListOfMods = "-";
        std::cout << "Done!" << std::endl;
        return;
    }
    std::vector<std::string> list = Split(std::string(Res), ";");
    std::vector<std::string> FNames(list.begin(), list.begin() + (list.size() / 2));
    std::vector<std::string> FSizes(list.begin() + (list.size() / 2), list.end());
    list.clear();
    int Amount = 0,Pos = 0;
    struct stat info{};
    std::string a,t;
    for(const std::string&N : FNames){
        if(!N.empty()){
            t += N.substr(N.find_last_of('/')+1) + ";";
        }
    }
    if(t.empty())ListOfMods = "-";
    else ListOfMods = t;
    t.clear();

    for(auto FN = FNames.begin(),FS = FSizes.begin(); FN != FNames.end() && !Terminate; ++FN,++FS) {
        int pos = FN->find_last_of('/');
        if (pos == std::string::npos)continue;
        Amount++;
    }
    for(auto FN = FNames.begin(),FS = FSizes.begin(); FN != FNames.end() && !Terminate; ++FN,++FS) {
        int pos = FN->find_last_of('/');
        if (pos != std::string::npos) {
            a = "Resources" + FN->substr(pos);
        } else continue;
        char *Data;
        Pos++;
        if (stat(a.c_str(), &info) == 0) {
            if (FS->find_first_not_of("0123456789") != std::string::npos)continue;
            if (fs::file_size(a) == std::stoi(*FS)){
                UlStatus = "UlLoading Resource: (" + std::to_string(Pos) + "/" + std::to_string(Amount) +
                           "): " + a.substr(a.find_last_of('/'));
                fs::copy_file(a, "BeamNG/mods"+a.substr(a.find_last_of('/')), fs::copy_options::overwrite_existing);
                WaitForConfirm();
                continue;
            }else remove(a.c_str());
        }
        CheckForDir();
        std::ofstream LFS;
        STCPSend(Sock, "f" + *FN);
        do {
            auto Pair = STCPRecv(Sock);
            Data = Pair.first;
            if (strcmp(Data, "Cannot Open") == 0 || Terminate)break;
            if(!LFS.is_open()){
                LFS.open(a.c_str(), std::ios_base::app | std::ios::binary);
            }
            LFS.write(Data, Pair.second);
            float per = LFS.tellp() / std::stof(*FS) * 100;
            std::string Percent = std::to_string(truncf(per * 10) / 10);
            UlStatus = "UlDownloading Resource: (" + std::to_string(Pos) + "/" + std::to_string(Amount) +
                       "): " + a.substr(a.find_last_of('/')) + " (" +
                       Percent.substr(0, Percent.find('.') + 2) + "%)";
        } while (LFS.tellp() != std::stoi(*FS));
        LFS.close();
        UlStatus = "UlLoading Resource: (" + std::to_string(Pos) + "/" + std::to_string(Amount) +
                   "): " + a.substr(a.find_last_of('/'));
        fs::copy_file(a, "BeamNG/mods"+a.substr(a.find_last_of('/')), fs::copy_options::overwrite_existing);
        WaitForConfirm();
    }
    FNames.clear();
    FSizes.clear();
    a.clear();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    UlStatus = "UlDone";
    STCPSend(Sock,"Done");
    std::cout << "Done!" << std::endl;
}