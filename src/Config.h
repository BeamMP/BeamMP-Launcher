#pragma once

#include <filesystem>
struct Config {
    Config();

    void write_to_file() const;

    bool is_valid = true;
    int port = 4444;
    std::string branch = "Default";
    std::string game_dir = "";
};

