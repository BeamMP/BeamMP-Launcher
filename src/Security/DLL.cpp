///
/// Created by Anonymous275 on 11/13/2020
///
#include "Network/network.h"
#include "Security/Enc.h"
#include <windows.h>
#include "Logger.h"
#include <psapi.h>
#include <string>
#include <thread>


DWORD getParentPID(DWORD pid);
HANDLE getProcess(DWORD pid, LPSTR fname, DWORD sz);

void Kill(){
    static bool Run = false;
    if(!Run)Run = true;
    else return;
    while(Run){
        std::this_thread::sleep_for(std::chrono::seconds(2));
        NetReset();
        #ifdef DEBUG
            debug(Sec("Attention! NetReset Check!"));
        #endif
    }
}

void FindDLL(char* args[]){
    static auto argv = args;
    HANDLE hProcess = GetCurrentProcess();
    std::string Parent(MAX_PATH,0);
    DWORD ppid = getParentPID(GetCurrentProcessId());
    HANDLE Process = getProcess(ppid, &Parent[0], MAX_PATH);
    if(Process == nullptr){
        HMODULE hMods[1024];
        DWORD cbNeeded;
        unsigned int i;
        if(K32EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)){
            for ( i = 1; i < (cbNeeded / sizeof(HMODULE)); i++ ){
                TCHAR szModName[MAX_PATH];
                if (K32GetModuleFileNameExA(hProcess, hMods[i], szModName, sizeof(szModName) / sizeof(TCHAR))){
                    std::string Name(szModName),PName(argv[0]);
                    Name = Name.substr(0,Name.rfind(Sec("\\")));
                    PName = PName.substr(0,PName.rfind(Sec("\\")));
                    if(Name == PName){
                        std::thread t1(Kill);
                        t1.detach();
                    }
                }
                ZeroMemory(szModName,MAX_PATH);
            }
        }
    }
    CloseHandle(hProcess);
}