///
/// Created by Anonymous275 on 5/2/2020
///
#include <windows.h>
#include <iostream>
#include <thread>
#include <string>

std::string QueryKey(HKEY hKey,int ID);
void SystemExec(const std::string&cmd);
void Exit(const std::string& Msg);

std::string Write(const std::string&Path){
    HKEY hKey;
    LPCTSTR sk = TEXT("Software\\BeamNG\\BeamNG.drive");
    LONG openRes = RegOpenKeyEx(HKEY_CURRENT_USER, sk, 0, KEY_ALL_ACCESS, &hKey);
    if (openRes != ERROR_SUCCESS)Exit("Error! Please launch the game at least once");
    std::string Query = QueryKey(hKey,4);
    LONG setRes = RegSetValueEx(hKey, TEXT("userpath_override"), 0, REG_SZ, (LPBYTE)Path.c_str(), Path.size());
    if(setRes != ERROR_SUCCESS)Exit("Error! Failed to launch the game code 1");
    RegCloseKey(hKey);
    return Query;
}
void DeleteKey(){
    HKEY hKey;
    LPCTSTR sk = TEXT("Software\\BeamNG\\BeamNG.drive");
    RegOpenKeyEx(HKEY_CURRENT_USER, sk, 0, KEY_ALL_ACCESS, &hKey);
    RegDeleteValueA(hKey, TEXT("userpath_override"));
}
void RollBack(const std::string&Val){
    std::this_thread::sleep_for(std::chrono::seconds(7));
    if(!Val.empty())Write(Val);
    else DeleteKey();
}
void StartGame(const std::string&ExeDir,const std::string&Current){
    std::cout << "Game Launched!\n";
    std::thread RB(RollBack,Write(Current));
    RB.detach();
    SystemExec(ExeDir + " -nocrashreport");
    Exit("Game Closed!");
}