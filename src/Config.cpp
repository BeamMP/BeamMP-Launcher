///
/// Created by Anonymous275 on 1/17/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///


#include <tomlplusplus/toml.hpp>
#include <filesystem>
#include "Launcher.h"
#include "Logger.h"

namespace fs = std::filesystem;

void Launcher::loadConfig() {
    if(fs::exists("Launcher.cfg")){
        std::ifstream cfg("Launcher.cfg");
        if(cfg.is_open()) {
            auto Size = fs::file_size("Launcher.cfg");
            std::string Buffer(Size, 0);
            cfg.read(&Buffer[0], std::streamsize(Size));
            cfg.close();
            toml::table config = toml::parse(cfg);
            LOG(INFO) << "Parsing";
            if(config["Port"].is_value()) {
                /*auto Port = config["Port"].as_integer()->get();
                LOG(INFO) << Port;*/
                LOG(INFO) << "Got port";
            }
        }else LOG(FATAL) << "Failed to open Launcher.cfg!";
    } else {
        std::ofstream cfg("Launcher.cfg");
        if(cfg.is_open()){
            cfg <<
                R"(Port = 4444
Build = "Default"
)";
            cfg.close();
        }else{
            LOG(FATAL) << "Failed to write config on disk!";
        }
    }
}