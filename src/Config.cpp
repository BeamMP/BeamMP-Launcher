///
/// Created by Anonymous275 on 1/17/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#include <tomlplusplus/toml.hpp>
#include "Launcher.h"
#include "Logger.h"

void Launcher::LoadConfig() {
   if (fs::exists(UIData::ConfigPath)) {
      toml::parse_result config = toml::parse_file(UIData::ConfigPath);
      auto ui                   = config["UI"];
      auto build                = config["Build"];
      auto GamePath             = config["GamePath"];
      auto ProfilePath          = config["ProfilePath"];
      auto CachePath            = config["CachePath"];

      EnableUI = false;
      /*if (ui.is_boolean()) {
         EnableUI = ui.as_boolean()->get();
      } else LOG(ERROR) << "Failed to get 'UI' boolean from config";*/

      // Default -1 / Release 1 / EA 2 / Dev 3 / Custom 3
      if (build.is_string()) {
         TargetBuild = build.as_string()->get();
         for (char& c : TargetBuild) c = char(tolower(c));
      } else LOG(ERROR) << "Failed to get 'Build' string from config";

      if (GamePath.is_string()) {
         if (!GamePath.as_string()->get().empty()) {
            BeamRoot = GamePath.as_string()->get();
         } else throw ShutdownException("GamePath cannot be empty");
      } else LOG(ERROR) << "Failed to get 'GamePath' string from config";

      if (ProfilePath.is_string()) {
         if (!UIData::GameVer.empty()) {
            auto GameVer = VersionParser(UIData::GameVer).split;

            if (!ProfilePath.as_string()->get().empty()) {
               BeamUserPath = fs::path(ProfilePath.as_string()->get()) / (GameVer[0] + '.' + GameVer[1]);
               MPUserPath   = BeamUserPath / "mods" / "multiplayer";
            } else throw ShutdownException("ProfilePath cannot be empty");
         } else throw ShutdownException ("Check game path in config");
      } else LOG(ERROR) << "Failed to get 'ProfilePath' string from config";

      if (CachePath.is_string()) {
         if (!CachePath.as_string()->get().empty()) {
            LauncherCache = CachePath.as_string()->get();
         } else throw ShutdownException("CachePath cannot be empty");
      } else LOG(ERROR) << "Failed to get 'CachePath' string from config";

      BeamVersion = UIData::GameVer;
   } else {
      LOG(FATAL) << "Failed to find config on disk!";
      throw ShutdownException("Fatal Error");
   }
}
