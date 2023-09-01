///
/// Created by Anonymous275 on 1/17/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///


#include <tomlplusplus/toml.hpp>
#include "Launcher.h"
#include "Logger.h"
#include <shlguid.h>
#include <shlobj_core.h>
#include <shobjidl.h>
#include <strsafe.h>

void Launcher::LoadConfig(const fs::path& conf) {  // check if json (issue)
   if (fs::is_regular_file(conf)) {
      if(fs::is_empty(conf)) {
         fs::remove(conf);
         LoadConfig(conf);
      }
      toml::parse_result config = toml::parse_file(conf.string());
      auto build                = config["Build"];
      auto ProfilePath          = config["ProfilePath"];
      auto CachePath            = config["CachePath"];

      // Default -1 / Release 1 / EA 2 / Dev 3 / Custom 3
      if (build.is_string()) {
         TargetBuild = build.as_string()->get();
         for (char& c : TargetBuild) c = char(tolower(c));
      } else LOG(ERROR) << "Failed to get 'Build' string from config";

      if (ProfilePath.is_string() && !ProfilePath.as_string()->get().empty()) {
         BeamProfilePath = fs::path(ProfilePath.as_string()->get());
         BeamUserPath = BeamProfilePath/GetProfileVersion();
      } else {
         LOG(ERROR) << "'ProfilePath' string from config is empty defaulting";
         BeamUserPath = GetProfileRoot()/GetProfileVersion();
      }
      MPUserPath = BeamUserPath / "mods" / "multiplayer";

      if (CachePath.is_string()) {
         if (!CachePath.as_string()->get().empty()) {
            LauncherCache = CachePath.as_string()->get();
         } else throw ShutdownException("CachePath cannot be empty");
      } else LOG(ERROR) << "Failed to get 'CachePath' string from config";

   } else {
      auto GameProfile = GetProfileRoot();
      std::ofstream tml(conf);
      if (tml.is_open()) {
         tml << "# Build is the lua build, it can be either default, canary, or public\n"
                "Build = 'default'\n"
                "CachePath = 'Resources'\n"
                "ProfilePath = '"
             << GameProfile.string() << "'";

         tml.close();
         LoadConfig(conf);
      } else LOG(ERROR) << "Failed to create config file " << conf;
   }
}

fs::path Launcher::GetProfileRoot() {

      HKEY BeamNG;
      fs::path ProfilePath;
      LONG RegRes =
          RegOpenKeyExA(HKEY_CURRENT_USER, R"(Software\BeamNG\BeamNG.drive)", 0,
                        KEY_READ, &BeamNG);
      if (RegRes == ERROR_SUCCESS) {
         ProfilePath = QueryValue(BeamNG, "userpath_override");
         RegCloseKey(BeamNG);
      }

      if (ProfilePath.empty()) {
         PWSTR folderPath = nullptr;
         HRESULT hr =
             SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &folderPath);

         if (FAILED(hr)) {
            throw ShutdownException("Please launch the game at least once, failed to read registry key Software\\BeamNG\\BeamNG.drive");
         } else {
            ProfilePath = fs::path(folderPath)/"BeamNG.drive";
            CoTaskMemFree(folderPath);
         }
      }
      BeamProfilePath = ProfilePath;
      return BeamProfilePath;
}

std::string Launcher::GetProfileVersion() {
      //load latest.lnk from profile

      if (CoInitializeEx(nullptr, COINIT_MULTITHREADED) != S_OK) {
         throw ShutdownException("Failed to read link: CoInitializeEx");
      }

      HRESULT rc;

      IShellLink* iShellLink;
      rc = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                            IID_IShellLink, (LPVOID*)&iShellLink);

      if (FAILED(rc)) {
         throw ShutdownException("Failed to read link: CoCreateInstance");
      }

      IPersistFile* iPersistFile;

      rc = iShellLink->QueryInterface(IID_IPersistFile, (LPVOID*)&iPersistFile);

      if (FAILED(rc)) {
         throw ShutdownException("Failed to read link: QueryInterface(IID_IPersistFile)");
      }

      rc = iPersistFile->Load((BeamProfilePath/"latest.lnk").wstring().c_str(), STGM_READ);

      if (FAILED(rc)) {
         throw ShutdownException("Failed to read link: Please launch the game at least once, check ProfilePath in Launcher.toml");
      }

      rc = iShellLink->Resolve(nullptr, 0);

      if (FAILED(rc)) {
         throw ShutdownException("Failed to read link: IShellLink failed to resolve");
      }

      std::wstring linkTarget(MAX_PATH, '\x00');
      rc = iShellLink->GetPath(&linkTarget[0], MAX_PATH, nullptr, SLGP_SHORTPATH);

      if (FAILED(rc)) {
         throw ShutdownException("Failed to read link: IShellLink failed to get path");
      }

      linkTarget.resize(linkTarget.find(L'\000'));

      iPersistFile->Release();
      iShellLink->Release();

      BeamVersion = std::filesystem::path(linkTarget).filename().string();
      LOG(INFO) << "Found profile for BeamNG " << BeamVersion;
      return BeamVersion;
}
