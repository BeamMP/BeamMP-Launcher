///
/// Created by Anonymous275 on 12/26/21
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#pragma once
#include <filesystem>
#include <string>
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
    static bool Terminated() noexcept;
    static void setExit(bool exit);
    static bool getExit();
    void RunDiscordRPC();
    void QueryRegistry();
    void WaitForGame();
    void LoadConfig();
    void LaunchGame();
    void CheckKey();
    void SetupMOD();
public: //Getters
    const std::string& getFullVersion();
    const std::string& getUserRole();
    const std::string& getVersion();
private: //functions
    std::string GetLocalAppdata();
    void AdminRelaunch();
    void RichPresence();
    void WindowsInit();
    void UpdateCheck();
    void ResetMods();
    void EnableMP();
    void Relaunch();
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
    std::thread DiscordRPC{};
    std::string MPUserPath{};
    std::string BeamVersion{};
    std::string BeamUserPath{};
    std::string DiscordMessage{};
    std::string Version{"3.0"};
    std::string TargetBuild{"default"};
    static std::atomic<bool> Shutdown, Exit;
    std::string FullVersion{Version + ".0"};
    VersionParser SupportedVersion{"0.24.1.1"};
};

class ShutdownException : public std::runtime_error {
public:
    explicit ShutdownException(const std::string& message): runtime_error(message){};
};
