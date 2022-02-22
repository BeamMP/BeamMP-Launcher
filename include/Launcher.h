///
/// Created by Anonymous275 on 12/26/21
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#pragma once
#include "Memory/IPC.h"
#include <filesystem>
#include "Server.h"
#include <thread>


namespace fs = std::filesystem;

struct VersionParser {
    explicit VersionParser(const std::string& from_string);
    std::strong_ordering operator<=>(VersionParser const& rhs) const noexcept;
    bool operator==(VersionParser const& rhs) const noexcept;
    std::vector<std::string> split;
    std::vector<size_t> data;
};

class Launcher {
public: //constructors
    Launcher(int argc, char* argv[]);
    ~Launcher();
public: //available functions
    static void StaticAbort(Launcher* Instance = nullptr);
    std::string Login(const std::string& fields);
    void SendIPC(const std::string& Data, bool core = true);
    void RunDiscordRPC();
    void QueryRegistry();
    void WaitForGame();
    void LoadConfig();
    void LaunchGame();
    void CheckKey();
    void SetupMOD();
public: //Getters and Setters
    void setDiscordMessage(const std::string& message);
    static void setExit(bool exit) noexcept;
    const std::string& getFullVersion();
    const std::string& getMPUserPath();
    static bool Terminated() noexcept;
    const std::string& getPublicKey();
    const std::string& getUserRole();
    const std::string& getVersion();
    static bool getExit() noexcept;

private: //functions
    void HandleIPC(const std::string& Data);
    std::string GetLocalAppdata();
    void UpdatePresence();
    void AdminRelaunch();
    void RichPresence();
    void WindowsInit();
    void UpdateCheck();
    void ResetMods();
    void EnableMP();
    void Relaunch();
    void ListenIPC();
    void Abort();
private: //variables
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
    std::string Version{"2.0"};
    Server ServerHandler{this};
    std::string TargetBuild{"default"};
    static std::atomic<bool> Shutdown, Exit;
    std::string FullVersion{Version + ".99"};
    VersionParser SupportedVersion{"0.24.1.2"};
    IPC IPCToGame{"BeamMP_OUT", "BeamMP_Sem1", "BeamMP_Sem2", 0x1900000};
    IPC IPCFromGame{"BeamMP_IN", "BeamMP_Sem3", "BeamMP_Sem4", 0x1900000};
};

class ShutdownException : public std::runtime_error {
public:
    explicit ShutdownException(const std::string& message): runtime_error(message){};
};
