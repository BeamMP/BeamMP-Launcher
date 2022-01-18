///
/// Created by Anonymous275 on 12/26/21
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#pragma once
#include <string>
#include <thread>

class Launcher {
public: //constructors
    Launcher(int argc, char* argv[]);
    ~Launcher();
public: //available functions
    std::string Login(const std::string& fields);
    void runDiscordRPC();
    void loadConfig();
    void launchGame();
    void checkKey();
public: //Getters
    const std::string& getFullVersion();
    const std::string& getWorkingDir();
    const std::string& getUserRole();
    const std::string& getVersion();
private: //functions
    void richPresence();
    void WindowsInit();
private: //variables
    bool EnableUI = true;
    bool Shutdown = false;
    std::string DirPath{};
    bool LoginAuth = false;
    std::string UserRole{};
    std::string PublicKey{};
    std::thread DiscordRPC{};
    std::string DiscordMessage{};
    std::string Version{"3.0"};
    std::string TargetBuild{"default"};
    std::string FullVersion{Version + ".0"};
};
