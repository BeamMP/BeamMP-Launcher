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
    if(fs::exists("Launcher.cfg")) {
        toml::table config = toml::parse_file("Launcher.cfg");
        auto ui = config["UI"];
        auto build = config["Build"];
        if(ui.is_boolean()) {
            EnableUI = ui.as_boolean()->get();
        } else LOG(ERROR) << "Failed to get 'UI' boolean from config";

        //Default -1 / Release 1 / EA 2 / Dev 3 / Custom 3
        if(build.is_string()) {
            TargetBuild = build.as_string()->get();
            for(char& c : TargetBuild)c = char(tolower(c));
        } else LOG(ERROR) << "Failed to get 'Build' string from config";

    } else {
        std::ofstream cfg("Launcher.cfg");
        if(cfg.is_open()){
            cfg <<
                R"(UI = true
Build = "Default"
)";
            cfg.close();
        }else{
            LOG(FATAL) << "Failed to write config on disk!";
        }
    }
}