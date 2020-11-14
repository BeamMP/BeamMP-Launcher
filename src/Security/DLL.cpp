///
/// Created by Anonymous275 on 11/13/2020
///
#include "Network/network.h"
#include "Security/Enc.h"
#include <windows.h>
#include <Logger.h>
#include <psapi.h>
#include <thread>

void Kill(){
    static bool Run = false;
    if(!Run)Run = true;
    else return;
    while(Run){
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        ClosePublic();
        #ifdef DEBUG
            debug("NetReset Check!");
        #endif
    }
}

void FindDLL(const std::string& Name) {
    static std::string PName = LocalEnc(Name.substr(0,Name.rfind(Sec("\\"))));
    static bool Running = false;
    if(Running)return;
    HANDLE hProcess = GetCurrentProcess();
    HMODULE hMods[1024];
    DWORD cbNeeded;
    unsigned int i;
    TCHAR szModName[MAX_PATH];
    if (K32EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
        for (i = 1; i < (cbNeeded / sizeof(HMODULE)); i++) {
            if (K32GetModuleFileNameExA(hProcess, hMods[i], szModName, sizeof(szModName) / sizeof(TCHAR))) {
                std::string MName(szModName);
                MName = MName.substr(0, MName.rfind(Sec("\\")));
                if (MName == LocalDec(PName)) {
                    Running = true;
                    std::thread t1(Kill);
                    t1.detach();
                }
            }
            ZeroMemory(szModName, MAX_PATH);
        }
    }
    CloseHandle(hProcess);
}