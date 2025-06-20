/*
 Copyright (C) 2024 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/

#include "Logger.h"
#include "Network/network.hpp"
#include "Options.h"
#include "Utils.h"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
namespace fs = std::filesystem;

std::string Branch;
std::filesystem::path CachingDirectory = std::filesystem::path("./Resources");
bool deleteDuplicateMods = false;

void ParseConfig(const nlohmann::json& d) {
    if (d["Port"].is_number()) {
        options.port = d["Port"].get<int>();
    }
    // Default -1
    // Release 1
    // EA 2
    // Dev 3
    // Custom 3
    if (d["Build"].is_string()) {
        Branch = d["Build"].get<std::string>();
        for (char& c : Branch)
            c = char(tolower(c));
    }
    if (d.contains("CachingDirectory") && d["CachingDirectory"].is_string()) {
        CachingDirectory = std::filesystem::path(d["CachingDirectory"].get<std::string>());
        info(beammp_wide("Mod caching directory: ") + beammp_fs_string(CachingDirectory.relative_path()));
    }

    if (d.contains("Dev") && d["Dev"].is_boolean()) {
        bool dev = d["Dev"].get<bool>();
        options.verbose = dev;
        options.no_download = dev;
        options.no_launch = dev;
        options.no_update = dev;
    }

    if (d.contains(("DeleteDuplicateMods")) && d["DeleteDuplicateMods"].is_boolean()) {
        deleteDuplicateMods = d["DeleteDuplicateMods"].get<bool>();
    }

}

void ConfigInit() {
    if (fs::exists("Launcher.cfg")) {
        std::ifstream cfg("Launcher.cfg");
        if (cfg.is_open()) {
            auto Size = fs::file_size("Launcher.cfg");
            std::string Buffer(Size, 0);
            cfg.read(&Buffer[0], Size);
            cfg.close();
            nlohmann::json d = nlohmann::json::parse(Buffer, nullptr, false);
            if (d.is_discarded()) {
                fatal("Config failed to parse make sure it's valid JSON!");
            }
            ParseConfig(d);
        } else
            fatal("Failed to open Launcher.cfg!");
    } else {
        std::ofstream cfg("Launcher.cfg");
        if (cfg.is_open()) {
            cfg <<
                R"({
    "Port": 4444,
    "Build": "Default",
    "CachingDirectory": "./Resources"
})";
            cfg.close();
        } else {
            fatal("Failed to write config on disk!");
        }
    }
}
