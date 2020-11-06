///
/// Created by Anonymous275 on 7/16/2020
///
#include "Discord/discord_info.h"
#include "Security/Enc.h"
#include <windows.h>
#include "Startup.h"
#include <tlhelp32.h>
#include "Logger.h"
#include <fstream>
#include <Psapi.h>
void DAS(){
#ifndef DEBUG
    int i = 0;
    std::ifstream f(GetEN(), std::ios::binary);
    f.seekg(0, std::ios_base::end);
    std::streampos fileSize = f.tellg();
    if(IsDebuggerPresent() || fileSize > 0x3D0900){
        i++;
        DAboard();
    }
    if(i)DAboard();
    f.close();
#endif
}
DWORD getParentPID(DWORD pid){
    HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe = {0};
    DWORD ppid = 0;
    pe.dwSize = sizeof(PROCESSENTRY32);
    if(Process32First(h, &pe)){
        do{
            if(pe.th32ProcessID == pid){
                ppid = pe.th32ParentProcessID;
                break;
            }
        }while(Process32Next(h, &pe));
    }
    CloseHandle(h);
    return ppid;
}

HANDLE getProcess(DWORD pid, LPSTR fname, DWORD sz) {
    HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (h) {
        GetModuleFileNameEx(h, nullptr, fname, sz);
        return h;
    }
    return nullptr;
}

void UnderSimulation(char* argv[]){
    DWORD ppid;
    std::string Parent(MAX_PATH,0);
    ppid = getParentPID(GetCurrentProcessId());
    HANDLE Process = getProcess(ppid, &Parent[0], MAX_PATH);
    std::string Code = Sec("Code ");
    if(Process == nullptr){
        error(Code+std::to_string(2));
        exit(1);
    }

    auto P = Parent.find(Sec(".exe"));
    if(P != std::string::npos)Parent.resize(P + 4);
    else return;
    std::string S1 = Sec("\\Windows\\explorer.exe");
    std::string S2 = Sec("JetBrains\\CLion");
    std::string S3 = Sec("\\Windows\\System32\\cmd.exe");
    std::string S4 = Sec("steam.exe");
    if(Parent == std::string(argv[0]))return;
    if(Parent.find(S1) == 2)return;
    if(Parent.find(S2) != std::string::npos)return;
    if(Parent.find(S3) == 2)return;
    if(Parent.find(S3) != -1)return;
    //TerminateProcess(Process, 1);
    //error(Code + std::to_string(4));
    //exit(1); //TODO look into that later
}
void SecurityCheck(char* argv[]){
    //UnderSimulation(argv);
    DAS();
}
