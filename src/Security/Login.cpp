///
/// Created by Anonymous275 on 11/26/2020
///

#include "Curl/http.h"
#include "Logger.h"
#include <fstream>
#include <thread>
#include <filesystem>

//check file if not present flag for login to the core network
//to then get user and pass
//if present use to contact the backend to refresh and get a public key for servers
//public keys are one time use for a random server

using namespace std::filesystem;

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

/// "username":"password"
/// "Guest":"Name"
/// "pk":"private_key"


void CheckLocalKey(){
    if(exists("key") && file_size("key") < 100){
        std::ifstream Key("key");
        if(Key.is_open()) {
            auto Size = file_size("key");
            std::string Buffer(Size, 0);
            Key.read(&Buffer[0], Size);
            Key.close();

            std::cout << "Key : " << Buffer << std::endl;
            Buffer = PostHTTP("https://auth.beammp.com/userlogin", R"({"username":"Anonymous275", "password":""})");
            std::cout << "Ret : " << Buffer << std::endl;
            if (Buffer == "-1" || Buffer.find('{') == -1) {
                fatal("Cannot connect to authentication servers please try again later!");
            }
        }else{
            warn("Could not open saved key!");
            UpdateKey(nullptr);
            AskUser();
        }
    }else{
        UpdateKey(nullptr);
        AskUser();
        std::cout << "No valid Key i am sad now" << std::endl;
    }
    system("pause");

}
