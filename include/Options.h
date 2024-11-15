#pragma once

#include <string>

struct Options {
#if defined(_WIN32)
    std::string executable_name = "BeamMP-Launcher.exe";
#elif defined(__linux__) || defined(__APPLE__)
    std::string executable_name = "BeamMP-Launcher";
#endif
    unsigned int port = 4444;
    bool verbose = false;
    bool no_download = false;
    bool no_update = false;
    bool no_launch = false;
    const char **game_arguments = nullptr;
    int game_arguments_length = 0;
#if defined(__APPLE__)
    std::string bottle;
#endif
    const char** argv = nullptr;
    int argc = 0;
};

void InitOptions(int argc, const char *argv[], Options &options);

extern Options options;
