///
/// Created by Anonymous275 on 3/29/2020
///

#include <iostream>

void Download(const std::string& URL,const std::string& path);
std::string HTTP_REQUEST(const std::string&url,int port);
void SystemExec(const std::string& cmd);
void WinExec(const std::string& cmd);

void CheckForUpdates(const std::string& CV){
    system ("cls");
    std::string HTTP = HTTP_REQUEST("https://beamng-mp.com/builds/launcher?version=true",443);
    HTTP = HTTP.substr(HTTP.find_last_of("ver=")+1);

    struct stat buffer{};
    if(stat ("BeamMP-Launcher.back", &buffer) == 0)remove("BeamMP-Launcher.back");
    if(HTTP > CV){
        std::cout << "Update found!" << std::endl;
        std::cout << "Updating..." << std::endl;
        SystemExec("rename BeamMP-Launcher.exe BeamMP-Launcher.back>nul");
        Download("https://beamng-mp.com/builds/launcher?download=true", "BeamMP-Launcher.exe");
        WinExec("BeamMP-Launcher.exe");
        exit(1);
    }else{
        std::cout << "Version is up to date" << std::endl;
    }
}