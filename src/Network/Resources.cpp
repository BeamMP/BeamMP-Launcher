///
/// Created by Anonymous275 on 1/30/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#include <ws2tcpip.h>
#include <atomic>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <future>
#include <string>
#include <thread>
#include <vector>
#include "Launcher.h"
#include "Logger.h"
#include "Server.h"

namespace fs = std::filesystem;
std::vector<std::string> Split(const std::string& String,
                               const std::string& delimiter) {
   std::vector<std::string> Val;
   size_t pos;
   std::string token, s = String;
   while ((pos = s.find(delimiter)) != std::string::npos) {
      token = s.substr(0, pos);
      if (!token.empty()) Val.push_back(token);
      s.erase(0, pos + delimiter.length());
   }
   if (!s.empty()) Val.push_back(s);
   return Val;
}

void CheckForDir() {
   if (!fs::exists("Resources")) {
      _wmkdir(L"Resources");
   }
}

void Server::WaitForConfirm() {
   while (!Terminate && !ModLoaded) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
   }
   ModLoaded = false;
}

void Server::Abort() {
   Terminate = true;
   LOG(INFO) << "Terminated!";
}

std::string Server::Auth() {
   TCPSend("VC" + LauncherInstance->getVersion());

   auto Res = TCPRcv();

   if (Res.empty() || Res[0] == 'E') {
      Abort();
      return "";
   }

   TCPSend(LauncherInstance->getPublicKey());
   if (Terminate) return "";

   Res = TCPRcv();
   if (Res.empty() || Res[0] != 'P') {
      Abort();
      return "";
   }

   Res = Res.substr(1);
   if (std::all_of(Res.begin(), Res.end(), isdigit)) {
      ClientID = std::stoi(Res);
   } else {
      Abort();
      UUl("Authentication failed!");
      return "";
   }
   TCPSend("SR");
   if (Terminate) return "";

   Res = TCPRcv();

   if (Res[0] == 'E') {
      Abort();
      return "";
   }

   if (Res.empty() || Res == "-") {
      LOG(INFO) << "Didn't Receive any mods...";
      ModList = "-";
      TCPSend("Done");
      LOG(INFO) << "Done!";
      return "";
   }
   return Res;
}

void Server::UpdateUl(bool D, const std::string& msg) {
   if (D) UStatus = "UlDownloading Resource " + msg;
   else UStatus = "UlLoading Resource " + msg;
}

void Server::AsyncUpdate(uint64_t& Rcv, uint64_t Size,
                         const std::string& Name) {
   do {
      double pr       = double(Rcv) / double(Size) * 100;
      std::string Per = std::to_string(trunc(pr * 10) / 10);
      UpdateUl(true, Name + " (" + Per.substr(0, Per.find('.') + 2) + "%)");
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
   } while (!Terminate && Rcv < Size);
}

char* Server::TCPRcvRaw(uint64_t Sock, uint64_t& GRcv, uint64_t Size) {
   if (Sock == -1) {
      Terminate = true;
      UUl("Invalid Socket");
      return nullptr;
   }
   char* File   = new char[Size];
   uint64_t Rcv = 0;
   do {
      int Len = int(Size - Rcv);
      if (Len > 1000000) Len = 1000000;
      int32_t Temp = recv(Sock, &File[Rcv], Len, MSG_WAITALL);
      if (Temp < 1) {
         UUl("Socket Closed Code 1");
         KillSocket(Sock);
         Terminate = true;
         delete[] File;
         return nullptr;
      }
      Rcv += Temp;
      GRcv += Temp;
   } while (Rcv < Size && !Terminate);
   return File;
}

void Server::MultiKill(uint64_t Sock) {
   KillSocket(TCPSocket);
   KillSocket(Sock);
   Terminate = true;
}

uint64_t Server::InitDSock() {
   SOCKET DSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   SOCKADDR_IN ServerAddr;
   if (DSock < 1) {
      KillSocket(DSock);
      Terminate = true;
      return 0;
   }
   ServerAddr.sin_family = AF_INET;
   ServerAddr.sin_port   = htons(Port);
   inet_pton(AF_INET, IP.c_str(), &ServerAddr.sin_addr);
   if (connect(DSock, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr)) != 0) {
      KillSocket(DSock);
      Terminate = true;
      return 0;
   }
   char Code[2] = {'D', char(ClientID)};
   if (send(DSock, Code, 2, 0) != 2) {
      KillSocket(DSock);
      Terminate = true;
      return 0;
   }
   return DSock;
}

std::string Server::MultiDownload(uint64_t DSock, uint64_t Size,
                                  const std::string& Name) {
   uint64_t GRcv = 0, MSize = Size / 2, DSize = Size - MSize;

   std::thread Au(&Server::AsyncUpdate, this, std::ref(GRcv), Size, Name);

   std::packaged_task<char*()> task(
       [&] { return TCPRcvRaw(TCPSocket, GRcv, MSize); });
   std::future<char*> f1 = task.get_future();
   std::thread Dt(std::move(task));
   Dt.detach();

   char* DData = TCPRcvRaw(DSock, GRcv, DSize);

   if (!DData) {
      MultiKill(DSock);
      return "";
   }

   f1.wait();
   char* MData = f1.get();

   if (!MData) {
      MultiKill(DSock);
      return "";
   }

   if (Au.joinable()) Au.join();

   /// omg yes very ugly my god but i was in a rush will revisit
   std::string Ret(Size, 0);
   memcpy_s(&Ret[0], MSize, MData, MSize);
   delete[] MData;

   memcpy_s(&Ret[MSize], DSize, DData, DSize);
   delete[] DData;

   return Ret;
}

void Server::InvalidResource(const std::string& File) {
   UUl("Invalid mod \"" + File + "\"");
   LOG(WARNING) << "The server tried to sync \"" << File
                << "\" that is not a .zip file!";
   Terminate = true;
}

void Server::SyncResources() {
   std::string Ret = Auth();
   if (Ret.empty()) return;
   LOG(INFO) << "Checking Resources...";
   CheckForDir();

   std::vector<std::string> list = Split(Ret, ";");
   std::vector<std::string> FNames(list.begin(),
                                   list.begin() + (list.size() / 2));
   std::vector<std::string> FSizes(list.begin() + (list.size() / 2),
                                   list.end());
   list.clear();
   Ret.clear();

   int Amount = 0, Pos = 0;
   std::string a, t;
   for (const std::string& name : FNames) {
      if (!name.empty()) {
         t += name.substr(name.find_last_of('/') + 1) + ";";
      }
   }
   if (t.empty()) ModList = "-";
   else ModList = t;
   t.clear();
   for (auto FN = FNames.begin(), FS = FSizes.begin();
        FN != FNames.end() && !Terminate; ++FN, ++FS) {
      auto pos = FN->find_last_of('/');
      auto ZIP = FN->find(".zip");
      if (ZIP == std::string::npos || FN->length() - ZIP != 4) {
         InvalidResource(*FN);
         return;
      }
      if (pos == std::string::npos) continue;
      Amount++;
   }
   if (!FNames.empty()) LOG(INFO) << "Syncing...";
   SOCKET DSock = InitDSock();
   for (auto FN = FNames.begin(), FS = FSizes.begin();
        FN != FNames.end() && !Terminate; ++FN, ++FS) {
      auto pos = FN->find_last_of('/');
      if (pos != std::string::npos) {
         a = LauncherInstance->getCachePath() + FN->substr(pos);
      } else continue;
      Pos++;
      if (fs::exists(a)) {
         if (!std::all_of(FS->begin(), FS->end(), isdigit)) continue;
         if (fs::file_size(a) == std::stoull(*FS)) {
            UpdateUl(false, std::to_string(Pos) + "/" + std::to_string(Amount) +
                                ": " + a.substr(a.find_last_of('/')));
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            try {
               if (!fs::exists(LauncherInstance->getMPUserPath())) {
                  fs::create_directories(LauncherInstance->getMPUserPath());
               }
               fs::copy_file(a,
                             LauncherInstance->getMPUserPath() +
                                 a.substr(a.find_last_of('/')),
                             fs::copy_options::overwrite_existing);
            } catch (std::exception& e) {
               LOG(ERROR) << "Failed copy to the mods folder! " << e.what();
               Terminate = true;
               continue;
            }
            WaitForConfirm();
            continue;
         } else remove(a.c_str());
      }
      CheckForDir();
      std::string FName = a.substr(a.find_last_of('/'));
      do {
         TCPSend("f" + *FN);

         std::string Data = TCPRcv();
         if (Data == "CO" || Terminate) {
            Terminate = true;
            UUl("Server cannot find " + FName);
            break;
         }

         std::string Name =
             std::to_string(Pos) + "/" + std::to_string(Amount) + ": " + FName;

         Data = MultiDownload(DSock, std::stoull(*FS), Name);

         if (Terminate) break;
         UpdateUl(false, std::to_string(Pos) + "/" + std::to_string(Amount) +
                             ": " + FName);
         std::ofstream LFS;
         LFS.open(a.c_str(), std::ios_base::app | std::ios::binary);
         if (LFS.is_open()) {
            LFS.write(&Data[0], std::streamsize(Data.size()));
            LFS.close();
         }

      } while (fs::file_size(a) != std::stoull(*FS) && !Terminate);
      if (!Terminate) {
         if (!fs::exists(LauncherInstance->getMPUserPath())) {
            fs::create_directories(LauncherInstance->getMPUserPath());
         }
         fs::copy_file(a, LauncherInstance->getMPUserPath() + FName,
                       fs::copy_options::overwrite_existing);
      }
      WaitForConfirm();
   }
   KillSocket(DSock);
   if (!Terminate) {
      TCPSend("Done");
      LOG(INFO) << "Done!";
   } else {
      UStatus = "start";
      LOG(INFO) << "Connection Terminated!";
   }
}
