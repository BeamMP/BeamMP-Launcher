///
/// Created by Anonymous275 on 7/19/2020
///
#include "Security/Enc.h"
#include <Windows.h>
#include "Startup.h"
#include "Logger.h"
#include <iostream>
#include <thread>
unsigned long GamePID = 0;
std::string QueryKey(HKEY hKey,int ID);
void DeleteKey(){
    HKEY hKey;
    LPCTSTR sk = Sec("Software\\BeamNG\\BeamNG.drive");
    RegOpenKeyEx(HKEY_CURRENT_USER, sk, 0, KEY_ALL_ACCESS, &hKey);
    RegDeleteValueA(hKey, Sec("userpath_override"));
}
std::string Write(const std::string&Path){
    HKEY hKey;
    LPCTSTR sk = Sec("Software\\BeamNG\\BeamNG.drive");
    LONG openRes = RegOpenKeyEx(HKEY_CURRENT_USER, sk, 0, KEY_ALL_ACCESS, &hKey);
    if (openRes != ERROR_SUCCESS){
        fatal(Sec("Please launch the game at least once"));
    }
    std::string Query = QueryKey(hKey,4);
    LONG setRes = RegSetValueEx(hKey, Sec("userpath_override"), 0, REG_SZ, (LPBYTE)Path.c_str(), DWORD(Path.size()));
    if(setRes != ERROR_SUCCESS){
        fatal(Sec("Failed to launch the game")); //not fatal later
    }
    RegCloseKey(hKey);
    return Query;
}
void RollBack(const std::string&Val,int T){
    std::this_thread::sleep_for(std::chrono::seconds(T));
    if(!Val.empty()){
        if(Write(Val) == Val)DeleteKey();
    }else DeleteKey();
}
std::string Restore;
void OnExit(){
    RollBack(Restore,0);
}
void StartGame(std::string Dir,std::string Current){
    Current = Current.substr(0,Current.find_last_of('\\')) + Sec("\\BeamNG\\");
    BOOL bSuccess = FALSE;
    PROCESS_INFORMATION pi;
    STARTUPINFO si = {0};
    si.cb = sizeof(si);
    std::string BaseDir = Dir + Sec("\\Bin64");
    Dir += Sec(R"(\Bin64\BeamNG.drive.x64.exe)");
    bSuccess = CreateProcessA(Dir.c_str(), nullptr, nullptr, nullptr, TRUE, 0, nullptr, BaseDir.c_str(), &si, &pi);
    if (bSuccess){
        info(Sec("Game Launched!"));
        GamePID = pi.dwProcessId;
        Restore = Write(Current);
        atexit(OnExit);
        std::thread RB(RollBack,Restore,7);
        RB.detach();
        WaitForSingleObject(pi.hProcess, INFINITE);
        error(Sec("Game Closed! launcher closing soon"));
        RollBack(Restore,0);
    }else{
        error(Sec("Failed to Launch the game! launcher closing soon"));
        RollBack(Write(Current),0);
    }
    std::this_thread::sleep_for(std::chrono::seconds(5));
    exit(2);
}
void InitGame(const std::string& Dir,const std::string&Current){
    if(!Dev){
        std::thread Game(StartGame, Dir, Current);
        Game.detach();
    }
}
