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
int Download(const std::string& URL,const std::string& OutFileName);
void StartGame(const std::string&ExeDir,const std::string&Current);
std::string HTTP_REQUEST(const std::string&url,int port);
void CheckForUpdates(int argc,char*args[],const std::string& CV);
char*EName = (char*)"4265616d4d502d4c61756e636865722e657865";
extern std::vector<std::string> GlobalInfo;
std::string HTA(const std::string& hex);
extern std::vector<std::string> SData;
std::string getHardwareID();
char* ver = (char*)"312e3530"; //1.50
char* patchlevel = (char*)"";
int DEFAULT_PORT = 4444;
void Discord_Main();
bool Dev = false;
void ProxyStart();
void ExitError();
void Check();

void SystemExec(const std::string& cmd){
    system(cmd.c_str());
}

void WinExec(const std::string& cmd){
    WinExec(cmd.c_str(), SW_HIDE);
}

void Exit(const std::string& Msg){
    std::cout << Msg << std::endl;
    std::cout << "Press Enter to continue...";
    std::cin.ignore();
    exit(-1);
}
void ReLaunch(int argc,char*args[]){
    std::string Arg;
    for(int c = 2; c <= argc; c++){
        Arg += " ";
        Arg += args[c-1];
    }
    system("cls");
    ShellExecute(nullptr,"runas",HTA(EName).c_str(),Arg.c_str(),nullptr,SW_SHOWNORMAL);
    exit(1);
}
std::string CheckDir(int argc,char*args[]){
    struct stat info{};
    std::string CDir = args[0];
    if(stat("BeamNG",&info)){
        SystemExec("mkdir BeamNG>nul");
        if(stat("BeamNG",&info))ReLaunch(argc,args);
    }
    if(!stat("BeamNG\\mods",&info))SystemExec("RD /S /Q BeamNG\\mods>nul");
    if(!stat("BeamNG\\mods",&info))ReLaunch(argc,args);
    SystemExec("mkdir BeamNG\\mods>nul");
    if(stat("BeamNG\\settings",&info))SystemExec("mkdir BeamNG\\settings>nul");
    return CDir.substr(0,CDir.find_last_of('\\')) + "\\BeamNG";
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

void URelaunch(int argc,char* args[]){
    std::string Arg;
    for(int c = 2; c <= argc; c++){
        Arg += " ";
        Arg += args[c-1];
    }
    ShellExecute(nullptr,"open",HTA(EName).c_str(),Arg.c_str(),nullptr,SW_SHOWNORMAL);
    exit(1);
}

void CheckName(int argc,char* args[]){
    struct stat info{};
    std::string DN = HTA(EName),CDir = args[0],FN = CDir.substr(CDir.find_last_of('\\')+1);
    if(FN != DN){
        if(stat(DN.c_str(),&info)==0)remove(DN.c_str());
        if(stat(DN.c_str(),&info)==0)ReLaunch(argc,args);
        SystemExec("rename \""+ FN +"\" " + DN + ">nul");
        URelaunch(argc,args);
    }
}

void SecurityCheck(){
    int i = 0;
    std::ifstream f(HTA(EName), std::ios::binary);
    f.seekg(0, std::ios_base::end);
    std::streampos fileSize = f.tellg();
    if(IsDebuggerPresent() || fileSize > 0x60B5F){
        i++;
        GlobalInfo.clear();
        GlobalInfo.at(13);
    }
    if(i){
        GlobalInfo.clear();
        GlobalInfo.at(13);
    }
    f.close();
}
int main(int argc, char* argv[]){
    const unsigned long long NPos = std::string::npos;
    struct stat info{};
    system("cls");
    SetConsoleTitleA(("BeamMP Launcher v" + HTA(ver) + patchlevel).c_str());
    CheckName(argc,argv);
    SecurityCheck();
    std::string link, HTTP_Result;
    std::thread t1(Discord_Main);
    t1.detach();
    std::cout << "Connecting to discord client..." << std::endl;
    while(GlobalInfo.empty())std::this_thread::sleep_for(std::chrono::milliseconds(300));
    std::cout << "Client Connected!" << std::endl;
    //https://beamng-mp.com/entitlement?did=(ID)&t=l;
    HTTP_Result = HTTP_REQUEST(HTA("68747470733a2f2f6265616d6e672d6d702e636f6d2f656e7469746c656d656e743f6469643d")+
                               HTA(GlobalInfo.at(2) + "26743d6c"),443);
    /*if (HTTP_Result.find("\"MOD\"") == NPos && HTTP_Result.find("\"EA\"") == NPos){
            if (HTTP_Result.find("\"SUPPORT\"") == NPos && HTTP_Result.find("\"YT\"") == NPos){
                exit(-1);
            }
        }*/

    SecurityCheck();
    if(HTTP_Result.find('"') == NPos && HTTP_Result != "[]"){
        std::cout << HTA("596f7520617265206e6f7420696e20746865206f6666696369616c204265616d4d5020446973636f726420706c65617365206a6f696e20616e642074727920616761696e2068747470733a2f2f646973636f72642e67672f6265616d6d70") << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
        exit(-1);
    }
    if(HTTP_Result.find(HTA("224d44455622")) != NPos)Dev = true;
    std::string Path = CheckDir(argc,argv);
    std::thread CFU(CheckForUpdates,argc,argv,HTA(ver)+patchlevel);
    CFU.join();

    if(argc > 1){
        std::string Port = argv[1];
        if(Port.find_first_not_of("0123456789") == NPos){
            if(std::stoi(Port) > 1000){
                DEFAULT_PORT = std::stoi(Port);
                std::cout << "Running on custom port : " << DEFAULT_PORT << std::endl;
            }
        }
        if(argc > 2)Dev = false;
    }

    //Security
    auto*Sec = new std::thread(Check);
    Sec->join();
    delete Sec;
    SecurityCheck();
    if(SData.size() != 3)ExitError();
    std::string GamePath = SData.at(2);
    std::cout << "Game Version : " << CheckVer(GamePath) << std::endl;
    std::string ExeDir = GamePath.substr(0,GamePath.find_last_of('\\')) + R"(\Bin64\BeamNG.drive.x64.exe)";
    std::string DUI = Path + R"(\settings\uiapps-layouts.json)";
    std::string GS = Path + R"(\settings\game-settings.ini)";
    if(stat(DUI.c_str(),&info)!=0 || stat(GS.c_str(),&info)!=0){
       link = "https://beamng-mp.com/client-ui-data";
       std::cout << "Downloading default config..." << std::endl;
       if(int i = Download(link,DUI) != -1){
            remove(DUI.c_str());
            std::cout << "Error! Failed to download code : " << i << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(3));
            ReLaunch(argc,argv);
       }
       link = "https://beamng-mp.com/client-settings-data";
       Download(link,GS);
       std::cout << "Download Complete!" << std::endl;
    }
    DUI.clear();
    GS.clear();
    if(!Dev){
        std::cout << "Downloading mod..." << std::endl;
        //https://beamng-mp.com/builds/client?did=(ID)
        link = HTA("68747470733a2f2f6265616d6e672d6d702e636f6d2f6275696c64732f636c69656e743f6469643d")
                +HTA(GlobalInfo.at(2));
        Download(link,Path + R"(\mods\BeamMP.zip)");
        std::cout << "Download Complete!" << std::endl;
        link.clear();
        std::thread Game(StartGame,ExeDir,(Path + "\\"));
        Game.detach();
    }else{
        SecurityCheck();
        std::cout << "Name : " << GlobalInfo.at(0) << std::endl;
        std::cout << "Discriminator : " << HTA(GlobalInfo.at(1)) << std::endl;
        std::cout << "Unique ID : " << HTA(GlobalInfo.at(2)) << std::endl;
        //std::cout << "HWID : " << getHardwareID() << std::endl;
        std::cout << "you have : " << HTTP_Result << std::endl;
    }

    ProxyStart();
    Exit("");
    return 0;
}