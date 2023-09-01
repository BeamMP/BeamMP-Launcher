///
/// Created by Anonymous275 on 1/18/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#include "HttpAPI.h"
#include "Json.h"
#include "Launcher.h"
#include "Logger.h"
#include "hashpp.h"

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

size_t DirCount(const std::filesystem::path& path) {
   return (size_t)std::distance(std::filesystem::directory_iterator{path},
                                std::filesystem::directory_iterator{});
}

void Launcher::ResetMods() {
    try {
        if (!fs::exists(MPUserPath)) {
            fs::create_directories(MPUserPath);
            return;
        }
        if (DirCount(MPUserPath) > 3) {
            LOG(WARNING)
                    << "mods/multiplayer will be cleared in 15 seconds, close to abort";
            std::this_thread::sleep_for(std::chrono::seconds(15));
        }
        for (auto &de: std::filesystem::directory_iterator(MPUserPath)) {
            if (de.path().filename() != "BeamMP.zip") {
                std::filesystem::remove(de.path());
            }
        }
    } catch (...) {
        throw ShutdownException("We were unable to clean the multiplayer mods folder! Is the game still running or do you have something open in that folder?");
    }
}

void Launcher::EnableMP() {
   fs::path File(BeamUserPath/"mods"/"db.json");
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
    std::string LatestHash = HTTP::Get("https://backend.beammp.com/sha/mod?branch=" + TargetBuild + "&pk=" + PublicKey);
    transform(LatestHash.begin(), LatestHash.end(), LatestHash.begin(), ::tolower);

    std::string FileHash = hashpp::get::getFileHash(hashpp::ALGORITHMS::SHA2_256, (MPUserPath / "BeamMP.zip").string());

    ResetMods();
    EnableMP();

    if (FileHash != LatestHash) {
        LOG(INFO) << "Downloading BeamMP Update " << LatestHash;
        HTTP::Download(
                "https://backend.beammp.com/builds/mod?download=true"
                "&pk=" +
                PublicKey + "&branch=" + TargetBuild,
                (MPUserPath / "BeamMP.zip").string());
    }
}

void Launcher::UpdateCheck() {
    if(DebugMode){
        LOG(DEBUG) << "Debug mode active skipping update checks";
        return;
    }
    std::string LatestHash = HTTP::Get("https://backend.beammp.com/sha/launcher?branch=" + TargetBuild + "&pk=" + PublicKey);
    std::string LatestVersion = HTTP::Get("https://backend.beammp.com/version/launcher?branch=" + TargetBuild + "&pk=" + PublicKey);

    transform(LatestHash.begin(), LatestHash.end(), LatestHash.begin(), ::tolower);

    std::string FileHash = hashpp::get::getFileHash(hashpp::ALGORITHMS::SHA2_256, "BeamMP-Launcher.exe");

    if(FileHash != LatestHash && VersionParser(LatestVersion) > VersionParser(FullVersion)) {
        LOG(INFO) << "Launcher update found!";
        fs::remove("BeamMP-Launcher.back");
        fs::rename("BeamMP-Launcher.exe", "BeamMP-Launcher.back");
        LOG(INFO) << "Downloading Launcher update " << LatestHash;
        HTTP::Download(
                "https://backend.beammp.com/builds/launcher?download=true"
                "&pk=" +
                PublicKey + "&branch=" + TargetBuild,
                "BeamMP-Launcher.exe");
        throw ShutdownException("Launcher update");
    }

}
