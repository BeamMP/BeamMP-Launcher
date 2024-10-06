#pragma once

#include <string>

struct Options {
    std::string executable_name = "BeamMP-Launcher.exe";
    unsigned int port = 4444;
    bool verbose = false;
    bool no_download = false;
    bool no_update = false;
    bool no_launch = false;
    char **game_arguments = nullptr;
    int game_arguments_length = 0;
    char** argv = nullptr;
    int argc = 0;
};

void InitOptions(int argc, char *argv[], Options &options);

extern Options options;
