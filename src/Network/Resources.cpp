///
/// Created by Anonymous275 on 4/11/2020
///
#include "Discord/discord_info.h"
#include "Network/network.h"
#include "Security/Enc.h"
#include <WS2tcpip.h>
#include <filesystem>
#include "Startup.h"
#include "Logger.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <cstring>
#include <algorithm>

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
void STCPSendRaw(SOCKET socket, const std::vector<char>& Data) {
    if (socket == -1) {
        Terminate = true;
        return;
    }
    int BytesSent = send(socket, Data.data(), int(Data.size()), 0);
    if (BytesSent == 0) {
        debug(Sec("(TCP) Connection closing..."));
        Terminate = true;
        return;
    } else if (BytesSent < 0) {
        debug(Sec("(TCP) send failed with error: ") + std::to_string(WSAGetLastError()));
        closesocket(socket);
        Terminate = true;
        return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

void STCPSend(SOCKET socket, const std::string& Data) {
    STCPSendRaw(socket, std::vector<char>(Data.begin(), Data.begin() + Data.size() + 1));
}
std::pair<char*,size_t> STCPRecv(SOCKET socket){
    char buf[64000];
    int len = 64000;
    ZeroMemory(buf, len);
    int BytesRcv = recv(socket, buf, len,0);
    if (BytesRcv == 0){
        info(Sec("(TCP) Connection closing..."));
        Terminate = true;
        return std::make_pair((char*)"",0);
    }else if (BytesRcv < 0) {
        info(Sec("(TCP) recv failed with error: ") + std::to_string(WSAGetLastError()));
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

int N,E;
void Parse(const std::string& msg){
    std::stringstream ss(msg);
    std::string t;
    while (std::getline(ss, t, 'g')) {
        if(t.find_first_not_of(Sec("0123456789abcdef")) != std::string::npos)return;
        if(N == 0){
            N = std::stoi(t, nullptr, 16);
        }else if(E == 0){
            E = std::stoi(t, nullptr, 16);
        }else return;
    }
}
std::string GenerateM(RSA*key){
    std::stringstream stream;
    stream << std::hex << key->n << "g" << key->e << "g" << RSA_E(Sec("IDC"),key->e,key->n);
    return stream.str();
}
struct Hold{
    SOCKET TCPSock{};
    bool Done = false;
};
void Check(Hold* S){
    std::this_thread::sleep_for(std::chrono::seconds(5));
    if(S != nullptr){
        if(!S->Done && S->TCPSock != -1){
            closesocket(S->TCPSock);
        }
    }
}
std::vector<char> PrependSize(const std::string& str) {
    uint32_t Size = htonl(uint32_t(str.size()) + 1);
    std::vector<char> result;
    // +1 for \0, +4 for the size
    result.resize(str.size() + 1 + 4);
    memcpy(result.data(), &Size, 4);
    memcpy(result.data() + 4, str.data(), str.size() + 1);
    return result;
}
std::string HandShake(SOCKET Sock,Hold*S,RSA*LKey){
    S->TCPSock = Sock;
    std::thread Timeout(Check,S);
    Timeout.detach();
    N = 0;E = 0;
    auto Res = STCPRecv(Sock);
    std::string msg(Res.first,Res.second);
    Parse(msg);
    if(N != 0 && E != 0) {
        STCPSendRaw(Sock,PrependSize(GenerateM(LKey)));
        Res = STCPRecv(Sock);
        msg = std::string(Res.first,Res.second);
        if(RSA_D(msg,LKey->d,LKey->n) != "HC"){
            Terminate = true;
        }
    }else Terminate = true;
    S->Done = true;
    if(Terminate){
        TCPTerminate = true;
        UlStatus = Sec("UlDisconnected: full or outdated server");
        info(Sec("Terminated!"));
        return "";
    }
    msg = RSA_E("NR" + GetDName() + ":" + GetDID(),E,N);
    if(!msg.empty()) {
        STCPSendRaw(Sock, PrependSize(msg));
        STCPSendRaw(Sock, PrependSize(RSA_E("VC" + GetVer(),E,N)));
        Res = STCPRecv(Sock);
        msg = Res.first;
    }

    if(N == 0 || E == 0 || msg.size() < 2 || msg.substr(0,2) != "WS"){
        Terminate = true;
        TCPTerminate = true;
        UlStatus = Sec("UlDisconnected: full or outdated server");
        info(Sec("Terminated!"));
        return "";
    }
    STCPSend(Sock,Sec("SR"));
    Res = STCPRecv(Sock);
    if(strlen(Res.first) == 0 || std::string(Res.first) == "-"){
        info(Sec("Didn't Receive any mods..."));
        ListOfMods = "-";
        STCPSend(Sock,Sec("Done"));
        info(Sec("Done!"));
        return "";
    }
    return Res.first;
}

void SyncResources(SOCKET Sock){
    RSA*LKey = GenKey();
    auto* S = new Hold;
    std::string Ret = HandShake(Sock,S,LKey);
    delete LKey;
    delete S;
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
                fs::copy_file(a, Sec("BeamNG/mods")+a.substr(a.find_last_of('/')), fs::copy_options::overwrite_existing);
                WaitForConfirm();
                continue;
            }else remove(a.c_str());
        }
        CheckForDir();
        do {
            STCPSend(Sock, "f" + *FN);
            int Recv = 0,Size = std::stoi(*FS);
            char*File = new char[Size];
            ZeroMemory(File,Size);
            do {
                auto Pair = STCPRecv(Sock);
                char* Data = Pair.first;
                size_t BytesRcv = Pair.second;
                if (strcmp(Data, Sec("Cannot Open")) == 0 || Terminate){
                    if(BytesRcv != 0)delete[] Data;
                    break;
                }
                memcpy_s(File+Recv,BytesRcv,Data,BytesRcv);
                Recv += int(BytesRcv);
                float per = float(Recv)/std::stof(*FS) * 100;
                std::string Percent = std::to_string(truncf(per * 10) / 10);
                UlStatus = Sec("UlDownloading Resource: (") + std::to_string(Pos) + "/" + std::to_string(Amount) +
                           "): " + a.substr(a.find_last_of('/')) + " (" +
                           Percent.substr(0, Percent.find('.') + 2) + "%)";
                delete[] Data;
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
            ZeroMemory(File,Size);
            delete[] File;
        }while(fs::file_size(a) != std::stoi(*FS) && !Terminate);
        if(!Terminate)fs::copy_file(a,Sec("BeamNG/mods")+a.substr(a.find_last_of('/')), fs::copy_options::overwrite_existing);
        WaitForConfirm();
    }
    FNames.clear();
    FSizes.clear();
    a.clear();
    if(!Terminate){
        STCPSend(Sock,Sec("Done"));
        info(Sec("Done!"));
    }else{
        UlStatus = Sec("Ulstart");
        info(Sec("Connection Terminated!"));
    }
}
