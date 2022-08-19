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
typedef HKEY__ *HKEY;

class Launcher {
   public:  // constructors
   Launcher();
   ~Launcher();

   public:  // available functions
   static void StaticAbort(Launcher* Instance = nullptr);
   std::string Login(const std::string& fields);
   void SendIPC(const std::string& Data, bool core = true);
   void RunDiscordRPC();
   void WaitForGame();
   void LoadConfig();
   void LaunchGame();
   void CheckKey();
   void SetupMOD();
   static std::string QueryValue(HKEY& hKey, const char* Name);

   public:  // Getters and Setters
   void setDiscordMessage(const std::string& message);
   static void setExit(bool exit) noexcept;
   const std::string& getFullVersion();
   const std::string& getMPUserPath();
   const std::string& getCachePath();
   static bool Terminated() noexcept;
   const std::string& getPublicKey();
   const std::string& getUserRole();
   const std::string& getVersion();
   static bool getExit() noexcept;

   private:  // functions
   void HandleIPC(const std::string& Data);
   void UpdatePresence();
   void RichPresence();
   void WindowsInit();
   void ResetMods();
   void EnableMP();
   void ListenIPC();
   void Abort();

   public: // variables
   static inline std::thread EntryThread{};
   static inline VersionParser SupportedVersion{"0.25.4.0"};
   static inline std::string Version{"2.0"};
   static inline std::string FullVersion{Version + ".99"};

   private:  // variables
   uint32_t GamePID{0};
   bool EnableUI = true;
   int64_t DiscordTime{};
   bool LoginAuth = false;
   fs::path CurrentPath{};
   std::string BeamRoot{};
   std::string UserRole{};
   std::string PublicKey{};
   std::thread IPCSystem{};
   std::thread DiscordRPC{};
   std::string MPUserPath{};
   std::string BeamVersion{};
   std::string BeamUserPath{};
   std::string DiscordMessage{};
   Server ServerHandler{this};
   std::string TargetBuild{"default"};
   std::string LauncherCache{};
   static inline std::atomic<bool> Shutdown{false}, Exit{false};
   std::unique_ptr<IPC> IPCToGame{};
   std::unique_ptr<IPC> IPCFromGame{};
};

class ShutdownException : public std::runtime_error {
   public:
   explicit ShutdownException(const std::string& message) :
       runtime_error(message){};
};

struct UIData {
   static inline std::string GamePath, ProfilePath, CachePath, Build, PublicKey, UserRole, Username, GameVer;
   static inline bool LoginAuth{false}, Console{false};
};

void UpdateKey(const std::string& newKey);
void entry();
