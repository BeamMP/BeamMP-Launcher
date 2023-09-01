///
/// Created by Anonymous275 on 12/26/21
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#pragma once
#include <filesystem>
#include <thread>
#include "Memory/IPC.h"
#include "Server.h"

namespace fs = std::filesystem;

struct VersionParser {
   explicit VersionParser(const std::string& from_string);
   std::strong_ordering operator<=>(VersionParser const& rhs) const noexcept;
   bool operator==(VersionParser const& rhs) const noexcept;
   std::vector<std::string> split;
   std::vector<size_t> data;
};

struct HKEY__;
typedef HKEY__* HKEY;

class Launcher {
   public:  // constructors
   Launcher(int argc, char* argv[]);
   ~Launcher();

   public:  // available functions
   static void StaticAbort(Launcher* Instance = nullptr);
   std::string Login(const std::string& fields);
   void SendIPC(const std::string& Data, bool core = true);
   void LoadConfig(const fs::path& conf);
   void RunDiscordRPC();
   void UpdateCheck();
   void WaitForGame();
   void LaunchGame();
   void CheckKey();
   void SetupMOD();

   static std::string QueryValue(HKEY& hKey, const char* Name);

   public:  // Getters and Setters
   void setDiscordMessage(const std::string& message);
   static void setExit(bool exit) noexcept;
   const std::string& getFullVersion();
   const fs::path& getMPUserPath();
   const fs::path& getCachePath();
   static bool Terminated() noexcept;
   const std::string& getPublicKey();
   const std::string& getUserRole();
   const std::string& getVersion();
   static bool getExit() noexcept;

   private:  // functions
   void HandleIPC(const std::string& Data);
   bool IsAllowedLink(const std::string& Link);
   std::string GetProfileVersion();
   fs::path GetProfileRoot();
   void UpdatePresence();
   void RichPresence();
   void WindowsInit();
   void ResetMods();
   void EnableMP();
   void ListenIPC();
   void Abort();

   public:  // variables
   static inline std::thread EntryThread{};
   static inline std::string Version{"2.1"};
   static inline std::string FullVersion{Version + ".0"};

   private:  // variables
   uint32_t GamePID{0};
   fs::path MPUserPath{};
   fs::path BeamUserPath{};
   fs::path BeamProfilePath{};
   fs::path LauncherCache{"Resources"};
   int64_t DiscordTime{};
   bool LoginAuth = false;
   bool DebugMode = false;
   fs::path CurrentPath{};
   std::string BeamRoot{};
   std::string UserRole{};
   std::string PublicKey{};
   std::thread IPCSystem{};
   std::thread DiscordRPC{};
   std::string ConnectURI{};
   std::string BeamVersion{};
   std::string DiscordMessage{};
   Server ServerHandler{this};
   std::string TargetBuild{"default"};

   static inline std::atomic<bool> Shutdown{false}, Exit{false};
   std::unique_ptr<IPC> IPCToGame{};
   std::unique_ptr<IPC> IPCFromGame{};
};

class ShutdownException : public std::runtime_error {
   public:
   explicit ShutdownException(const std::string& message) :
       runtime_error(message){};
};


