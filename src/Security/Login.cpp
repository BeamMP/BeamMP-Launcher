///
/// Created by Anonymous275 on 11/26/2020
///

#include "Curl/http.h"
#include <filesystem>
#include "Logger.h"
#include <fstream>
#include "Json.h"


//check file if not present flag for login to the core network
//to then get user and pass
//if present use to contact the backend to refresh and get a public key for servers
//public keys are one time use for a random server

using namespace std::filesystem;
std::string PublicKey;

void UpdateKey(const char* newKey){
    if(newKey){
        std::ofstream Key("key");
        if(Key.is_open()){
            Key << newKey;
            Key.close();
        }else fatal("Cannot write to disk!");
    }else if(exists("key")){
        remove("key");
    }
}

void AskUser(){
    //Flag Core Network Update to have a login screen
}

/// "username":"value","password":"value"
/// "Guest":"Name"
/// "pk":"private_key"

void QueryKey(){
    /*std::string Buffer = PostHTTP("https://auth.beammp.com/pkToUser", R"({"key":")"+PublicKey+"\"}");
    std::cout << Buffer << std::endl;*/
}

void CheckLocalKey(){
    if(exists("key") && file_size("key") < 100){
        std::ifstream Key("key");
        if(Key.is_open()) {
            auto Size = file_size("key");
            std::string Buffer(Size, 0);
            Key.read(&Buffer[0], Size);
            Key.close();
            Buffer = PostHTTP("https://auth.beammp.com/userlogin", R"({"pk":")"+Buffer+"\"}");
            json::Document d;
            std::cout << Buffer << std::endl;
            d.Parse(Buffer.c_str());
            if (Buffer == "-1" || Buffer.find('{') == -1 || d.HasParseError()) {
                fatal("Invalid answer from authentication servers, please try again later!");
            }
            if(d["success"].GetBool()){
                UpdateKey(d["private_key"].GetString());
                PublicKey = d["public_key"].GetString();
                QueryKey();
            }else{
                std::cout << "Well..... re-login" << std::endl;
                std::cout << Buffer << std::endl;
                //send it all to the game
            }
        }else{
            warn("Could not open saved key!");
            UpdateKey(nullptr);
            AskUser();
        }
    }else{
        UpdateKey(nullptr);
        AskUser();
    }
    system("pause");
}
