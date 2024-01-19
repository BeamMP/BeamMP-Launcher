#include "Config.h"

#include <boost/iostreams/device/mapped_file.hpp>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

Config::Config() {
    if (std::filesystem::exists("Launcher.cfg")) {
        boost::iostreams::mapped_file cfg("Launcher.cfg", boost::iostreams::mapped_file::mapmode::readonly);
        nlohmann::json d = nlohmann::json::parse(cfg.const_data(), nullptr, false);
        if (d.is_discarded()) {
            is_valid = false;
        }
        // parse config
        if (d["Port"].is_number()) {
            port = d["Port"].get<int>();
        }
        if (d["Build"].is_string()) {
            branch = d["Build"].get<std::string>();
            for (char& c : branch) {
                c = char(tolower(c));
            }
        }
        if (d["GameDir"].is_string()) {
            game_dir = d["GameDir"].get<std::string>();
        }
    } else {
        write_to_file();
    }
}

void Config::write_to_file() const {
    nlohmann::json d {
        { "Port", port },
        { "Branch", branch },
        { "GameDir", game_dir },
    };
    std::ofstream of("Launcher.cfg", std::ios::trunc);
    of << d.dump(4);
}
