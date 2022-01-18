///
/// Created by Anonymous275 on 1/17/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#include "Launcher.h"
#include "Logger.h"
#include <fstream>
#include "Http.h"
#include "Json.h"

void UpdateKey(const char* newKey){
    if(newKey){
        std::ofstream Key("key");
        if(Key.is_open()){
            Key << newKey;
            Key.close();
        }else LOG(FATAL) << "Cannot write to disk!";
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
        UpdateKey(nullptr);
        return "";
    }
    LOG(INFO) << "Attempting to authenticate...";
    std::string Buffer = HTTP::Post("https://auth.beammp.com/userlogin", fields);
    Json::Document d;
    d.Parse(Buffer.c_str());

    if(Buffer == "-1"){
        return GetFail("Failed to communicate with the auth system!");
    }

    if (Buffer.at(0) != '{' || d.HasParseError()) {
        LOG(ERROR) << Buffer;
        return GetFail("Invalid answer from authentication servers, please try again later!");
    }

    if(!d["success"].IsNull() && d["success"].GetBool()){
        LoginAuth = true;
        if(!d["private_key"].IsNull()){
            UpdateKey(d["private_key"].GetString());
        }
        if(!d["public_key"].IsNull()){
            PublicKey = d["public_key"].GetString();
        }
        LOG(INFO) << "Authentication successful!";
    }else LOG(WARNING) << "Authentication failed!";

    if(!d["message"].IsNull()) {
        d.RemoveMember("private_key");
        d.RemoveMember("public_key");
        Json::StringBuffer buffer;
        Json::Writer<rapidjson::StringBuffer> writer(buffer);
        d.Accept(writer);
        return buffer.GetString();
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

            Json::Document d;
            d.Parse(Buffer.c_str());
            if (Buffer == "-1" || Buffer.at(0) != '{' || d.HasParseError()) {
                LOG(ERROR) << Buffer;
                LOG(FATAL) << "Invalid answer from authentication servers, please try again later!";
            }
            if(d["success"].GetBool()){
                LoginAuth = true;
                UpdateKey(d["private_key"].GetString());
                PublicKey = d["public_key"].GetString();
                UserRole = d["role"].GetString();
                //info(Role);
            }else{
                LOG(WARNING) << "Auto-Authentication unsuccessful please re-login!";
                UpdateKey(nullptr);
            }
        }else{
            LOG(WARNING) << "Could not open saved key!";
            UpdateKey(nullptr);
        }
    }else UpdateKey(nullptr);
}



