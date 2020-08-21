///
/// Created by Anonymous275 on 7/19/2020
///
#include "Security/Enc.h"
#include <windows.h>
#include "Security/Game.h"
#include <filesystem>
#include <RestartManager.h>
#include "Logger.h"
#include <thread>

namespace fs = std::experimental::filesystem;
void CheckDirs(){
    for (auto& p : fs::directory_iterator(Sec("BeamNG\\mods"))) {
        if(fs::is_directory(p.path()))exit(0);
    }
}

bool BeamLoad(PCWSTR pszFile){
    bool Ret = false;
    DWORD dwSession;
    WCHAR szSessionKey[CCH_RM_SESSION_KEY+1] = {0};
    DWORD dwError = RmStartSession(&dwSession, 0, szSessionKey);
    if (dwError == ERROR_SUCCESS) {
        dwError = RmRegisterResources(dwSession, 1, &pszFile,
        0, nullptr, 0, nullptr);
        if (dwError == ERROR_SUCCESS) {
            DWORD dwReason;
            UINT nProcInfoNeeded;
            UINT nProcInfo = 10;
            RM_PROCESS_INFO rgpi[10];
            dwError = RmGetList(dwSession, &nProcInfoNeeded,&nProcInfo, rgpi, &dwReason);
            if (dwError == ERROR_SUCCESS) {
                if(nProcInfo == 1){
                    std::string AppName(50,0);
                    size_t N;
                    wcstombs_s(&N,&AppName[0],50, rgpi[0].strAppName, 50);
                    if(!AppName.find(Sec("BeamNG.drive")) && GamePID == rgpi[0].Process.dwProcessId){
                        Ret = true;
                    }
                }
            }
        }
        RmEndSession(dwSession);
    }
    return Ret;
}
void ContinuousCheck(fs::file_time_type last){
    std::string path = Sec(R"(BeamNG\mods\BeamMP.zip)");
    int i = 0;
    while(fs::exists(path) && last == fs::last_write_time(path)){
        if(!BeamLoad(SecW(L"BeamNG\\mods\\BeamMP.zip"))) {
            if (i < 60)i++;
            else {
                error(Sec("Mod did not load! launcher closing soon"));
                std::this_thread::sleep_for(std::chrono::seconds(5));
                exit(0);
            }
        }else i = 0;
        CheckDirs();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    exit(0);
}

void SecureMods(){
   fs::file_time_type last = fs::last_write_time(Sec(R"(BeamNG\mods\BeamMP.zip)"));
   auto* HandleCheck = new std::thread(ContinuousCheck,last);
   HandleCheck->detach();
   delete HandleCheck;
}