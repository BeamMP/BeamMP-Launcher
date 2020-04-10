////
//// Created by Anonymous275 on 3/3/2020.
////

#include <iostream>
#include <string>
#include <vector>
#include <direct.h>
#include <fstream>
#include <urlmon.h>

#pragma comment(lib, "urlmon.lib")

std::string HTTP_REQUEST(const std::string&url,int port);
std::vector<std::string> Discord_Main();
std::vector<std::string> Check();
std::string getHardwareID();
void CheckForUpdates(const std::string& CV);
void ProxyStart();

void Download(const std::string& URL,const std::string& path){
    URLDownloadToFileA(nullptr, URL.c_str(), path.c_str(), 0, nullptr);
}
void SystemExec(const std::string& cmd){
    system(cmd.c_str());
}
void WinExec(const std::string& cmd){
    WinExec(cmd.c_str(), SW_HIDE);
}

void Exit(const std::string& Msg){
    std::cout << Msg << std::endl;
    std::cout << "Press Enter to continue . . .";
    std::cin.ignore();
    exit(-1);
}

std::string CheckDir(char*dir, std::string ver){
    system(("title BeamMP Launcher v" + ver).c_str());
    char*temp;size_t len;
    _dupenv_s(&temp, &len,"APPDATA");
    std::string DN = "BeamMP-Launcher.exe",CDir = dir, AD = temp,FN = CDir.substr(CDir.find_last_of('\\')+1,CDir.size());
    AD += "\\BeamMP-Launcher";

    if(FN != DN){
        SystemExec("rename \""+ FN +"\" " + DN + ">nul");
    }

    if(CDir.substr(0,CDir.find_last_of('\\')) != AD){
        _mkdir(AD.c_str());
        SystemExec(R"(move "BeamMP-Launcher.exe" ")" + AD + "\">nul");
    }

    SetCurrentDirectoryA(AD.c_str());
    SystemExec("rename *.exe " + DN + ">nul");

    SystemExec(R"(powershell "$s=(New-Object -COM WScript.Shell).CreateShortcut('%userprofile%\Desktop\BeamMP-Launcher.lnk');$s.TargetPath=')"+AD+"\\"+DN+"';$s.Save()\"");
    CreateDirectoryA("BeamNG",nullptr);
    CreateDirectoryA("BeamNG\\mods",nullptr);
    SetFileAttributesA("BeamNG",2|4);
    return AD + "\\BeamNG";
}

std::string CheckVer(const std::string &path){
    std::string vec,temp,Path = path.substr(0,path.find_last_of('\\')) + "\\integrity.json";
    std::ifstream f(Path.c_str(), std::ios::binary);
    f.seekg(0, std::ios_base::end);
    std::streampos fileSize = f.tellg();
    vec.resize(size_t(fileSize) + 1);
    f.seekg(0, std::ios_base::beg);
    f.read(&vec[0], fileSize);
    f.close();
    vec = vec.substr(vec.find_last_of("version"),vec.find_last_of('"'));
    for(const char &a : vec){
        if(isdigit(a) || a == '.')temp+=a;
    }
    return temp;
}

int main(int argc, char* argv[])
{
    std::string ver = "0.21", Path = CheckDir(argv[0],ver),HTTP_Result;
    CheckForUpdates(ver); //Update Check

    //Security
    std::vector<std::string> Data = Check();
    std::string GamePath = Data.at(2);
    std::cout << "You own BeamNG on this machine!" << std::endl;
    std::cout << "Game Ver : " << CheckVer(GamePath) << std::endl;


    std::cout << "Name : " << Discord_Main().at(0) << std::endl;
    std::cout << "Discriminator : " << Discord_Main().at(1) << std::endl;
    std::cout << "Unique ID : " << Discord_Main().at(2) << std::endl;

    std::cout << "HWID : " << getHardwareID() << std::endl;

    std::string ExeDir = GamePath.substr(0,GamePath.find_last_of('\\')) + "\\Bin64\\BeamNG.drive.x64.exe";
    Download("https://beamng-mp.com/builds/client?did="+Discord_Main().at(2),Path + R"(\mods\BeamMP.zip)");
    HTTP_Result = HTTP_REQUEST("https://beamng-mp.com/entitlement?did="+Discord_Main().at(2),443);
    std::cout << "you have : " << HTTP_Result << std::endl;

    /*if(HTTP_Result.find("[\"MDEV\"]") != std::string::npos){
        WinExec(ExeDir + " -cefdev -console -nocrashreport -userpath " + Path);
    }else{
        WinExec(ExeDir + " -nocrashreport -userpath " + Path);
    }*/
    //std::cout << "Game Launched!\n";

    ///HTTP REQUEST FOR SERVER LIST
    ///Mods

    ProxyStart(); //Proxy main start

    Exit("");
    return 0;
}