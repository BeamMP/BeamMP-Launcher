///
/// Created by Anonymous275 on 2/23/2021
///

#include "Network/network.h"
#include <filesystem>
#include "Logger.h"
#include <fstream>
#include "Json.h"
#include <cstdint>
namespace fs = std::filesystem;

std::string Branch;
void ParseConfig(const json::Document& d){
    if(d["Port"].IsInt()){
        DEFAULT_PORT = d["Port"].GetInt();
    }
    //Default -1
    //Release 1
    //EA 2
    //Dev 3
    //Custom 3

    if(d["Build"].IsString()){
        Branch = d["Build"].GetString();
        for(char& c : Branch)c = char(tolower(c));
    }
}

void ConfigInit(){
    if(fs::exists("Launcher.cfg")){
        std::ifstream cfg("Launcher.cfg");
        if(cfg.is_open()){
            auto Size = fs::file_size("Launcher.cfg");
            std::string Buffer(Size, 0);
            cfg.read(&Buffer[0], Size);
            cfg.close();
            json::Document d;
            d.Parse(Buffer.c_str());
            if(d.HasParseError()){
                fatal("Config failed to parse make sure it's valid JSON! Code : " + std::to_string(d.GetParseError()));
            }
            ParseConfig(d);
        }else fatal("Failed to open Launcher.cfg!");
    }else{
        std::ofstream cfg("Launcher.cfg");
        if(cfg.is_open()){
            cfg <<
            R"({
"Port": 4444,
"Build": "Default"
})";
            cfg.close();
        }else{
            fatal("Failed to write config on disk!");
        }
    }
}

