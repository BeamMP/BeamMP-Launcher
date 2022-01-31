///
/// Created by Anonymous275 on 1/29/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///


#include "Memory/Memory.h"
#include "Memory/BeamNG.h"
#include "Launcher.h"
#include "Logger.h"
#include "Http.h"

void Launcher::HandleIPC(const std::string& Data) {
    char Code = Data.at(0), SubCode = 0;
    if(Data.length() > 1)SubCode = Data.at(1);
    LOG(INFO) << Data;
    switch (Code) {
        case 'A':
            SendIPC(Data.substr(0,1));
            break;
        case 'B':
            ServerHandler.Close();
            SendIPC(Code + HTTP::Post("https://backend.beammp.com/servers",""));
            LOG(INFO) << "Sent Server List";
            break;
        case 'C':
            ServerHandler.Connect(Data);
            while(ServerHandler.getModList().empty() && !ServerHandler.Terminated()){
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            if(ServerHandler.getModList() == "-")SendIPC("L");
            else SendIPC("L"+ServerHandler.getModList());
            break;
        case 'U':
            if(SubCode == 'l')SendIPC("Ul" + ServerHandler.getUIStatus());
            else if(SubCode == 'p') {
                if(ServerHandler.getPing() > 800) {
                    SendIPC("Up-2");
                } else SendIPC("Up" + std::to_string(ServerHandler.getPing()));
            } else if(!SubCode) {
                std::string Ping;
                if(ServerHandler.getPing() > 800)Ping = "-2";
                else Ping = std::to_string(ServerHandler.getPing());
                SendIPC("Ul" + ServerHandler.getUIStatus() + "\nUp" + Ping);
            }
            break;
        case 'M':
            SendIPC(ServerHandler.getMap());
            break;
        case 'Q':
            if(SubCode == 'S') {
                ServerHandler.Close();
            }
            if(SubCode == 'G')exit(2);
            break;
        case 'R': //will send mod name
            ServerHandler.setModLoaded();
            break;
        case 'Z':
            SendIPC("Z" + Version);
            break;
        case 'N':
            if(SubCode == 'c') {
                SendIPC("N{\"Auth\":"+std::to_string(LoginAuth)+"}");
            } else {
                SendIPC("N" + Login(Data.substr(Data.find(':') + 1)));
            }
            break;
        default:
            break;
    }
}

void Server::ServerParser(const std::string& Data) {
    if(Data.empty())return;
    LOG(INFO) << "IPC Parser: " << Data;
    char Code = Data.at(0),SubCode = 0;
    if(Data.length() > 1)SubCode = Data.at(1);
    switch (Code) {
        case 'p':
            PingEnd = std::chrono::high_resolution_clock::now();
            if(PingStart > PingEnd)Ping = 0;
            else Ping = int(std::chrono::duration_cast<std::chrono::milliseconds>(PingEnd-PingStart).count());
            return;
        case 'M':
            MStatus = Data;
            UStatus = "done";
            return;
        default:
            break;
    }
    LauncherInstance->SendIPC(Data, false);
}
