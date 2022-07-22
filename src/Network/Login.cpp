///
/// Created by Anonymous275 on 1/17/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#include "Launcher.h"
#include "HttpAPI.h"
#include "Logger.h"
#include "Json.h"

void UpdateKey(const std::string& newKey){
    if(!newKey.empty()){
        std::ofstream Key("key");
        if(Key.is_open()){
            Key << newKey;
            Key.close();
        } else {
            LOG(FATAL) << "Cannot write to disk!";
            throw ShutdownException("Fatal Error");
        }
    }else if(fs::exists("key")){
        remove("key");
    }
}

/// "username":"value","password":"value"
/// "Guest":"Name"
/// "pk":"private_key"

std::string GetFail(const std::string& R){
    std::string DRet = R"({"success":false,"message":)";
    DRet += "\""+R+"\"}";
    LOG(ERROR) << R;
    return DRet;
}

std::string Launcher::Login(const std::string& fields) {
    if(fields == "LO"){
        LoginAuth = false;
        UpdateKey("");
        return "";
    }
    LOG(INFO) << "Attempting to authenticate...";
    std::string Buffer = HTTP::Post("https://auth.beammp.com/userlogin", fields);
    Json d = Json::parse(Buffer, nullptr, false);

    if(Buffer == "-1"){
        return GetFail("Failed to communicate with the auth system!");
    }

    if (Buffer.at(0) != '{' || d.is_discarded()) {
        LOG(ERROR) << Buffer;
        return GetFail("Invalid answer from authentication servers, please try again later!");
    }

    if(!d["success"].is_null() && d["success"].get<bool>()){
        LoginAuth = true;
        if(!d["private_key"].is_null()){
            UpdateKey(d["private_key"].get<std::string>());
        }
        if(!d["public_key"].is_null()){
            PublicKey = d["public_key"].get<std::string>();
        }
        LOG(INFO) << "Authentication successful!";
    }else LOG(WARNING) << "Authentication failed!";

    if(!d["message"].is_null()) {
        d.erase("private_key");
        d.erase("public_key");
        return d.dump();
    }
    return GetFail("Invalid message parsing!");
}

void Launcher::CheckKey() {
    if(fs::exists("key") && fs::file_size("key") < 100){
        std::ifstream Key("key");
        if(Key.is_open()) {
            auto Size = fs::file_size("key");
            std::string Buffer(Size, 0);
            Key.read(&Buffer[0], std::streamsize(Size));
            Key.close();

            Buffer = HTTP::Post("https://auth.beammp.com/userlogin", R"({"pk":")" + Buffer + "\"}");

            Json d = Json::parse(Buffer, nullptr, false);
            if (Buffer == "-1" || Buffer.at(0) != '{' || d.is_discarded()) {
                LOG(DEBUG) << Buffer;
                LOG(FATAL) << "Invalid answer from authentication servers, please try again later!";
                throw ShutdownException("Fatal Error");
            }
            if(d["success"].get<bool>()){
                LoginAuth = true;
                UpdateKey(d["private_key"].get<std::string>());
                PublicKey = d["public_key"].get<std::string>();
                UserRole = d["role"].get<std::string>();
                LOG(INFO) << "Auto-Authentication was successful";
            }else{
                LOG(WARNING) << "Auto-Authentication unsuccessful please re-login!";
                UpdateKey("");
            }
        }else{
            LOG(WARNING) << "Could not open saved key!";
            UpdateKey("");
        }
    }else UpdateKey("");
}



