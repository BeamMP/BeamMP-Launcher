////
//// Created by Anonymous275 on 3/3/2020.
////

#include <iostream>
#include <urlmon.h>
#include <direct.h>
#include <fstream>
#include <string>
#include <vector>
#include <thread>

#pragma comment(lib, "urlmon.lib")

std::string HTTP_REQUEST(const std::string&url,int port);
void CheckForUpdates(const std::string& CV);
std::vector<std::string> GetDiscordInfo();
std::string QueryKey(HKEY hKey,int ID);
std::vector<std::string> GlobalInfo;
std::vector<std::string> Check();
std::string getHardwareID();
extern int DEFAULT_PORT;
void Discord_Main();
bool MPDEV = false;
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

std::string CheckDir(char*dir, const std::string& ver){
    system(("title BeamMP Launcher v" + ver).c_str());
    char*temp;size_t len;
    struct stat info{};
    _dupenv_s(&temp, &len,"APPDATA");
    std::string DN = "BeamMP-Launcher.exe",CDir = dir, AD = temp,FN = CDir.substr(CDir.find_last_of('\\')+1,CDir.size());
    AD += "\\BeamMP-Launcher";
    if(FN != DN){
        if(stat(DN.c_str(),&info)==0)remove(DN.c_str());
        SystemExec("rename \""+ FN +"\" " + DN + ">nul");
    }
    if(CDir.substr(0,CDir.find_last_of('\\')) != AD){
        _mkdir(AD.c_str());
        SystemExec(R"(move "BeamMP-Launcher.exe" ")" + AD + "\">nul");
    }
    SetCurrentDirectoryA(AD.c_str());
    SystemExec("rename *.exe " + DN + ">nul");
    SystemExec(R"(powershell "$s=(New-Object -COM WScript.Shell).CreateShortcut('%userprofile%\Desktop\BeamMP-Launcher.lnk');$s.TargetPath=')"+AD+"\\"+DN+"';$s.Save()\"");
    if(stat("BeamNG",&info))SystemExec("mkdir BeamNG>nul");
    if(stat("BeamNG\\mods",&info))SystemExec("mkdir BeamNG\\mods>nul");
    if(stat("BeamNG\\settings",&info))SystemExec("mkdir BeamNG\\settings>nul");
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
void SyncResources(const std::string&IP,int Port);

std::string Write(const std::string&Path){
    HKEY hKey;
    LPCTSTR sk = TEXT("Software\\BeamNG\\BeamNG.drive");

    LONG openRes = RegOpenKeyEx(HKEY_CURRENT_USER, sk, 0, KEY_ALL_ACCESS, &hKey);

    if (openRes != ERROR_SUCCESS) {
        Exit("Error! Please launch the game at least once");
    }
    std::string Query = QueryKey(hKey,4);
    LPCTSTR value = TEXT("userpath_override");
    LONG setRes = RegSetValueEx(hKey, value, 0, REG_SZ, (LPBYTE)Path.c_str(), Path.size());

    if (setRes != ERROR_SUCCESS) {
        Exit("Error! Failed to launch the game code 1");
    }
    RegCloseKey(hKey);
    return Query;
}
void RollBack(const std::string&Val){
    std::this_thread::sleep_for(std::chrono::seconds(5));
    Write(Val);
}
void StartGame(const std::string&ExeDir,const std::string&Current){
    std::cout << "Game Launched!\n";
    std::thread RB(RollBack,Current);
    RB.detach();
    SystemExec(ExeDir + " -nocrashreport");
    Exit("Game Closed!");
}

int main(int argc, char* argv[])
{
    const unsigned long long NPos = std::string::npos;
    struct stat info{};
    if(argc > 1){
        std::string Port = argv[1];
        if(Port.find_first_not_of("0123456789") == NPos){
            DEFAULT_PORT = std::stoi(Port);
            std::cout << "Running on custom port : " << DEFAULT_PORT << std::endl;
        }
    }
    std::string ver = "0.89", Path = CheckDir(argv[0],ver),HTTP_Result;
    CheckForUpdates(ver);

    std::thread t1(Discord_Main);
    t1.detach();


    std::cout << "Connecting to discord client..." << std::endl;
    while(GlobalInfo.empty()){
        GlobalInfo = GetDiscordInfo();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }


    HTTP_Result = HTTP_REQUEST("https://beamng-mp.com/entitlement?did="+GlobalInfo.at(2),443);
    if(HTTP_Result.find("\"MDEV\"") == NPos){
        if (HTTP_Result.find("\"MOD\"") == NPos && HTTP_Result.find("\"EA\"") == NPos){
            if (HTTP_Result.find("\"SUPPORT\"") == NPos && HTTP_Result.find("\"YT\"") == NPos){
                exit(-1);
            }
        }
    }else MPDEV = true;
    //Security
    std::vector<std::string> Data = Check();
    std::string GamePath = Data.at(2);
    if(MPDEV)std::cout << "You own BeamNG on this machine!" << std::endl;
    std::cout << "Game Version : " << CheckVer(GamePath) << std::endl;
    std::string ExeDir = "\""+GamePath.substr(0,GamePath.find_last_of('\\')) + R"(\Bin64\BeamNG.drive.x64.exe")";
    std::string Settings = Path + "\\settings\\uiapps-layouts.json";
    if(stat(Settings.c_str(),&info)!=0){
       Download("https://beamng-mp.com/client-data",Settings);
       std::cout << "Downloaded default config!" << std::endl;
    }

    Download("https://beamng-mp.com/builds/client?did="+GlobalInfo.at(2),Path + R"(\mods\BeamMP.zip)");

    if(!MPDEV){
        std::thread Game(StartGame,ExeDir,Write(Path + "\\"));
        Game.detach();
    }else{
        std::cout << "Name : " << GlobalInfo.at(0) << std::endl;
        std::cout << "Discriminator : " << GlobalInfo.at(1) << std::endl;
        std::cout << "Unique ID : " << GlobalInfo.at(2) << std::endl;
        std::cout << "HWID : " << getHardwareID() << std::endl;
        std::cout << "you have : " << HTTP_Result << std::endl;
    }

    ProxyStart();
    Exit("");
    return 0;
}