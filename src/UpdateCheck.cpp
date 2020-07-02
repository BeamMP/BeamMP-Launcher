///
/// Created by Anonymous275 on 3/29/2020
///

#include <iostream>
#include <thread>
int Download(const std::string& URL,const std::string& path);
std::string HTTP_REQUEST(const std::string&url,int port);
std::string HTA(const std::string& hex);
void SystemExec(const std::string& cmd);
void WinExec(const std::string& cmd);
void Exit(const std::string& Msg);
void ReLaunch(int argc,char*args[]);
void CheckForUpdates(int argc,char*args[],const std::string& CV){
    std::string link = "https://beamng-mp.com/builds/launcher?version=true";
    std::string HTTP = HTTP_REQUEST(link,443);
    link = "https://beamng-mp.com/builds/launcher?download=true";
    if(HTTP.find_first_of("0123456789") == std::string::npos){
        Exit("Primary Servers Offline! sorry for the inconvenience!");
    }
    struct stat buffer{};
    if(stat ("BeamMP-Launcher.back", &buffer) == 0)remove("BeamMP-Launcher.back");
    if(HTTP > CV){
        system("cls");
        std::cout << "Update found!" << std::endl;
        std::cout << "Updating..." << std::endl;
        SystemExec("rename BeamMP-Launcher.exe BeamMP-Launcher.back>nul");
        if(int i = Download(link, "BeamMP-Launcher.exe") != -1){
            std::cout << "Launcher Update failed! trying again... code : " << i << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(2));
            if(int i2 = Download(link, "BeamMP-Launcher.exe") != -1){
                std::cout << "Launcher Update failed! code : " << i2 << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(5));
                ReLaunch(argc,args);
            }

        }
        WinExec("BeamMP-Launcher.exe");
        exit(1);
    }else{
        std::cout << "Version is up to date" << std::endl;
    }
}