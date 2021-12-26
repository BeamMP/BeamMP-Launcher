///
/// Created by Anonymous275 on 12/26/21
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#pragma once
#include <string>

class Launcher {
public:
    Launcher(int argc, char* argv[]);
    const std::string& getWorkingDir(){return DirPath;}
    const std::string& getVersion(){return Version;}
    const std::string& getFullVersion(){return FullVersion;}
private:
    void WindowsInit();

private:
    std::string DirPath;
    std::string Version{"3.0"};
    std::string FullVersion{Version + ".0"};
};
