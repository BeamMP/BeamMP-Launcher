///
/// Created by Anonymous275 on 5/2/2020
///
#include <Windows.h>
#include <iostream>
#include <thread>

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
void RollBack(const std::string&Val,int T){
    std::this_thread::sleep_for(std::chrono::seconds(T));
    if(!Val.empty())Write(Val);
    else DeleteKey();
}

void SetPID(DWORD PID);
void StartGame(const std::string&ExeDir,const std::string&Current){
    BOOL bSuccess = FALSE;
    PROCESS_INFORMATION pi;
    STARTUPINFO si = {0};
    si.cb = sizeof(si);
    std::string BaseDir = ExeDir.substr(0,ExeDir.find_last_of('\\'));
    bSuccess = CreateProcessA(ExeDir.c_str(), nullptr, nullptr, nullptr, TRUE, 0, nullptr, BaseDir.c_str(), &si, &pi);
    if (bSuccess)
    {
        std::cout << "Game Launched!\n";
        SetPID(pi.dwProcessId);
        std::thread RB(RollBack,Write(Current),7);
        RB.detach();
        WaitForSingleObject(pi.hProcess, INFINITE);
        std::cout << "\nGame Closed! launcher closing in 5 secs\n";
    }else std::cout << "\nFailed to Launch the game! launcher closing in 5 secs\n";
    RollBack(Write(Current),0);
    std::this_thread::sleep_for(std::chrono::seconds(5));
    exit(2);
}