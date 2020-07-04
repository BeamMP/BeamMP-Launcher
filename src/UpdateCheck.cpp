///
/// Created by Anonymous275 on 3/29/2020
///

#include <iostream>
#include <thread>
#include <string>

int Download(const std::string& URL,const std::string& path);
std::string HTTP_REQUEST(const std::string&url,int port);
void SystemExec(const std::string& cmd);
void URelaunch(int argc,char* args[]);
void ReLaunch(int argc,char*args[]);
void Exit(const std::string& Msg);
extern char*EName;
std::string hta(const std::string& hex)
{
    std::string ascii;
    for (size_t i = 0; i < hex.length(); i += 2)
    {
        std::string part = hex.substr(i, 2);
        char ch = char(stoul(part, nullptr, 16));
        ascii += ch;
    }
    return ascii;
}

void CheckForUpdates(int argc,char*args[],const std::string& CV){
    std::string link = hta("68747470733a2f2f6265616d6e672d6d702e636f6d2f6275696c64732f6c61756e636865723f76657273696f6e3d74727565");
    //https://beamng-mp.com/builds/launcher?version=true
    std::string HTTP = HTTP_REQUEST(link,443);
    if(HTTP.find_first_of("0123456789") == std::string::npos){
        Exit("Primary Servers Offline! sorry for the inconvenience!");
    }
    link = hta("68747470733a2f2f6265616d6e672d6d702e636f6d2f6275696c64732f6c61756e636865723f646f776e6c6f61643d74727565");
    //https://beamng-mp.com/builds/launcher?download=true

    struct stat buffer{};
    std::string Back = hta("4265616d4d502d4c61756e636865722e6261636b");
    //BeamMP-Launcher.back
    if(stat(Back.c_str(), &buffer) == 0)remove(Back.c_str());
    if(HTTP > CV){
        system("cls");
        std::cout << "Update found!" << std::endl;
        std::cout << "Updating..." << std::endl;
        SystemExec("rename "+hta(EName)+" "+Back+">nul");
        if(int i = Download(link, hta(EName)) != -1){
            std::cout << "Launcher Update failed! trying again... code : " << i << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(2));
            if(int i2 = Download(link, hta(EName)) != -1){
                std::cout << "Launcher Update failed! code : " << i2 << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(5));
                ReLaunch(argc,args);
            }
        }
        URelaunch(argc,args);
    }else{
        std::cout << "Version is up to date" << std::endl;
    }
}