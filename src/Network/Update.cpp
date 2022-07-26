///
/// Created by Anonymous275 on 1/18/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#include "HttpAPI.h"
#include "Json.h"
#include "Launcher.h"
#include "Logger.h"

VersionParser::VersionParser(const std::string& from_string) {
   std::string token;
   std::istringstream tokenStream(from_string);
   while (std::getline(tokenStream, token, '.')) {
      data.emplace_back(std::stol(token));
      split.emplace_back(token);
   }
}

std::strong_ordering VersionParser::operator<=>(
    const VersionParser& rhs) const noexcept {
   size_t const fields = std::min(data.size(), rhs.data.size());
   for (size_t i = 0; i != fields; ++i) {
      if (data[i] == rhs.data[i]) continue;
      else if (data[i] < rhs.data[i]) return std::strong_ordering::less;
      else return std::strong_ordering::greater;
   }
   if (data.size() == rhs.data.size()) return std::strong_ordering::equal;
   else if (data.size() > rhs.data.size()) return std::strong_ordering::greater;
   else return std::strong_ordering::less;
}

bool VersionParser::operator==(const VersionParser& rhs) const noexcept {
   return std::is_eq(*this <=> rhs);
}

void Launcher::UpdateCheck() {
   std::string link;
   std::string HTTP =
       HTTP::Get("https://beammp.com/builds/launcher?version=true");
   bool fallback = false;
   if (HTTP.find_first_of("0123456789") == std::string::npos) {
      HTTP =
          HTTP::Get("https://backup1.beammp.com/builds/launcher?version=true");
      fallback = true;
      if (HTTP.find_first_of("0123456789") == std::string::npos) {
         LOG(FATAL) << "Primary Servers Offline! sorry for the inconvenience!";
         throw ShutdownException("Fatal Error");
      }
   }
   if (fallback) {
      link = "https://backup1.beammp.com/builds/launcher?download=true";
   } else link = "https://beammp.com/builds/launcher?download=true";

   std::string EP(CurrentPath.string()),
       Back(CurrentPath.parent_path().string() + "\\BeamMP-Launcher.back");

   if (fs::exists(Back)) remove(Back.c_str());
   std::string RemoteVer;
   for (char& c : HTTP) {
      if (std::isdigit(c) || c == '.') {
         RemoteVer += c;
      }
   }

   if (VersionParser(RemoteVer) > VersionParser(FullVersion)) {
      system("cls");
      LOG(INFO) << "Update found! Downloading...";
      if (std::rename(EP.c_str(), Back.c_str())) {
         LOG(ERROR) << "Failed to create a backup!";
      }

      if (!HTTP::Download(link, EP)) {
         LOG(ERROR) << "Launcher Update failed! trying again...";
         std::this_thread::sleep_for(std::chrono::seconds(2));

         if (!HTTP::Download(link, EP)) {
            LOG(ERROR) << "Launcher Update failed!";
            std::this_thread::sleep_for(std::chrono::seconds(5));
            AdminRelaunch();
         }
      }
      Relaunch();
   } else LOG(INFO) << "Launcher version is up to date";
}

size_t DirCount(const std::filesystem::path& path) {
   return (size_t)std::distance(std::filesystem::directory_iterator{path},
                                std::filesystem::directory_iterator{});
}

void Launcher::ResetMods() {
   if (!fs::exists(MPUserPath)) {
      fs::create_directories(MPUserPath);
      return;
   }
   if (DirCount(fs::path(MPUserPath)) > 3) {
      LOG(WARNING)
          << "mods/multiplayer will be cleared in 15 seconds, close to abort";
      std::this_thread::sleep_for(std::chrono::seconds(15));
   }
   fs::remove_all(MPUserPath);
   fs::create_directories(MPUserPath);
}

void Launcher::EnableMP() {
   std::string File(BeamUserPath + "mods\\db.json");
   if (!fs::exists(File)) return;
   auto Size = fs::file_size(File);
   if (Size < 2) return;
   std::ifstream db(File);
   if (db.is_open()) {
      std::string Data(Size, 0);
      db.read(&Data[0], std::streamsize(Size));
      db.close();
      Json d = Json::parse(Data, nullptr, false);
      if (Data.at(0) != '{' || d.is_discarded()) return;
      if (!d["mods"].is_null() && !d["mods"]["multiplayerbeammp"].is_null()) {
         d["mods"]["multiplayerbeammp"]["active"] = true;
         std::ofstream ofs(File);
         if (ofs.is_open()) {
            ofs << std::setw(4) << d;
            ofs.close();
         } else {
            LOG(ERROR) << "Failed to write " << File;
         }
      }
   }
}

void Launcher::SetupMOD() {
   ResetMods();
   EnableMP();
   LOG(INFO) << "Downloading mod please wait";
   HTTP::Download(
       "https://backend.beammp.com/builds/client?download=true"
       "&pk=" +
           PublicKey + "&branch=" + TargetBuild,
       MPUserPath + "\\BeamMP.zip");
}
