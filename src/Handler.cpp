///
/// Created by Anonymous275 on 1/29/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#include "HttpAPI.h"
#include "Launcher.h"
#include "Logger.h"
#include "Memory/BeamNG.h"
#include "Memory/Memory.h"
#include <regex>

void Launcher::HandleIPC(const std::string& Data) {  //TODO Improve all cases since SendIPC can be called async
    char Code = Data.at(0), SubCode = 0;
    if (Data.length() > 1) SubCode = Data.at(1);
    switch (Code) {
        case 'A':
            ServerHandler.StartUDP();
            break;
        case 'B':
            ServerHandler.Close();
            SendIPC(Code + HTTP::Get("https://backend.beammp.com/servers-info"));
            LOG(INFO) << "Sent Server List";
            break;
        case 'C':
            ServerHandler.Close();
            ServerHandler.Connect(Data);
            while (ServerHandler.getModList().empty() &&
                   !ServerHandler.Terminated()) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            if (ServerHandler.getModList() == "-") SendIPC("L");
            else SendIPC("L" + ServerHandler.getModList());
            break;
        case 'U':
            SendIPC("Ul" + ServerHandler.getUIStatus());
            break;
        case 'M':
            SendIPC(ServerHandler.getMap());
            break;
        case 'Q':
            if (SubCode == 'S') {
                ServerHandler.Close();
            }
            if (SubCode == 'G') exit(2);
            break;
        case 'R':  // will send mod name
            ServerHandler.setModLoaded();
            break;
        case 'Z':
            SendIPC("Z" + Version);
            break;
        case 'N':
            if (SubCode == 'c') {
                SendIPC("N{\"Auth\":" + std::to_string(LoginAuth) + "}");
            } else {
                SendIPC("N" + Login(Data.substr(Data.find(':') + 1)));
            }
            break;
        case 'O': //open default browser with URL
            if(IsAllowedLink(Data.substr(1))) {
                ShellExecuteA(nullptr, "open", Data.substr(1).c_str(), nullptr, nullptr,SW_SHOW); ///TODO: Look at when working on linux port
            }
            break;
        default:
            break;
    }
}

bool Launcher::IsAllowedLink(const std::string& Link) {
    std::regex link_pattern(R"(https:\/\/(?:\w+)?(?:\.)?(?:beammp|discord)\.com)");
    std::smatch link_match;
    return std::regex_search(Link,link_match, link_pattern) && link_match.position() == 0;
}

void Server::ServerParser(const std::string& Data) {
   if (Data.empty()) return;
   char Code = Data.at(0), SubCode = 0;
   if (Data.length() > 1) SubCode = Data.at(1);
   switch (Code) {
      case 'p':
         PingEnd = std::chrono::high_resolution_clock::now();
         if (PingStart > PingEnd) Ping = 0;
         else {
            Ping = int(std::chrono::duration_cast<std::chrono::milliseconds>(PingEnd - PingStart).count());
            if (Ping > 800) {
               LauncherInstance->SendIPC("Up-2");
            } else LauncherInstance->SendIPC("Up" + std::to_string(Ping));
         }
         return;
      case 'M':
         MStatus = Data;
         UStatus = "done";
         return;
      case 'K':
         Terminate = true;
         UStatus   = Data.substr(1);
         return;
      default:
         break;
   }
   LauncherInstance->SendIPC(Data, false);
}
