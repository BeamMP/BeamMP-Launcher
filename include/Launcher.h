///
/// Created by Anonymous275 on 12/26/21
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#pragma once
#include <string>

class Launcher {
public: //constructors
    Launcher(int argc, char* argv[]);
public: //available functions
    std::string Login(const std::string& fields);
    void checkLocalKey();
    void loadConfig();
    void launchGame();
public: //Getters
    const std::string& getFullVersion();
    const std::string& getWorkingDir();
    const std::string& getUserRole();
    const std::string& getVersion();
private: //functions
    void WindowsInit();
private: //variables
    std::string DirPath;
    std::string UserRole;
    std::string PublicKey;
    bool LoginAuth = false;
    std::string Version{"3.0"};
    std::string FullVersion{Version + ".0"};
};
