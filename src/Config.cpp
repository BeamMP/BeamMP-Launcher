///
/// Created by Anonymous275 on 1/17/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#include "Launcher.h"
#include "Logger.h"
#include <tomlplusplus/toml.hpp>

void Launcher::LoadConfig() {
    if (fs::exists("Launcher.toml")) {
        toml::parse_result config = toml::parse_file("Launcher.toml");
        auto ui = config["UI"];
        auto build = config["Build"];
        if (ui.is_boolean()) {
            EnableUI = ui.as_boolean()->get();
        } else
            LOG(ERROR) << "Failed to get 'UI' boolean from config";

        // Default -1 / Release 1 / EA 2 / Dev 3 / Custom 3
        if (build.is_string()) {
            TargetBuild = build.as_string()->get();
            for (char& c : TargetBuild)
                c = char(tolower(c));
        } else
            LOG(ERROR) << "Failed to get 'Build' string from config";

    } else {
        std::ofstream tml("Launcher.toml");
        if (tml.is_open()) {
            tml <<
                R"(UI = true
Build = "Default"
)";
            tml.close();
        } else {
            LOG(FATAL) << "Failed to write config on disk!";
            throw ShutdownException("Fatal Error");
        }
    }
}
