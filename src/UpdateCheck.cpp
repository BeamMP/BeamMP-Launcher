///
/// Created by Anonymous275 on 3/29/2020
///

#include <iostream>

void Download(const std::string& URL,const std::string& path);
std::string HTTP_REQUEST(const std::string&url,int port);
std::string HTA(const std::string& hex);
void SystemExec(const std::string& cmd);
void WinExec(const std::string& cmd);
void Exit(const std::string& Msg);

void CheckForUpdates(const std::string& CV){
    system ("cls");
    std::string link = "https://beamng-mp.com/builds/launcher?version=true";
    std::string HTTP = HTTP_REQUEST(link,443);
    link = "https://beamng-mp.com/builds/launcher?download=true";
    if(HTTP.find_first_of("0123456789") == std::string::npos){
        Exit("Primary Servers Offline! sorry for the inconvenience!");
    }
    struct stat buffer{};
    if(stat ("BeamMP-Launcher.back", &buffer) == 0)remove("BeamMP-Launcher.back");
    if(HTTP > CV){
        std::cout << "Update found!" << std::endl;
        std::cout << "Updating..." << std::endl;
        SystemExec("rename BeamMP-Launcher.exe BeamMP-Launcher.back>nul");
        Download(link, "BeamMP-Launcher.exe");
        WinExec("BeamMP-Launcher.exe");
        exit(1);
    }else{
        std::cout << "Version is up to date" << std::endl;
    }
}