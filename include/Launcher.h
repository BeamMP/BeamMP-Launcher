///
/// Created by Anonymous275 on 12/26/21
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#pragma once
#include <filesystem>
#include <string>
#include <thread>

namespace fs = std::filesystem;

class Launcher {
public: //constructors
    Launcher(int argc, char* argv[]);
    ~Launcher();
public: //available functions
    std::string Login(const std::string& fields);
    bool Terminate() const;
    void RunDiscordRPC();
    void LoadConfig();
    void LaunchGame();
    void CheckKey();
public: //Getters
    const std::string& getFullVersion();
    const std::string& getUserRole();
    const std::string& getVersion();
private: //functions
    void AdminRelaunch();
    void RichPresence();
    void WindowsInit();
    void UpdateCheck();
    void Relaunch();
private: //variables
    bool EnableUI = true;
    bool Shutdown = false;
    bool LoginAuth = false;
    fs::path CurrentPath{};
    std::string UserRole{};
    std::string PublicKey{};
    std::thread DiscordRPC{};
    std::string DiscordMessage{};
    std::string Version{"3.0"};
    std::string TargetBuild{"default"};
    std::string FullVersion{Version + ".0"};
};
