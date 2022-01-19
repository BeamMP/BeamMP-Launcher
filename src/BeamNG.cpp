///
/// Created by Anonymous275 on 6/17/21
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#define WIN32_LEAN_AND_MEAN
#include <string>
#include "BeamNG.h"
#undef UNICODE
#include <windows.h>
#include <tlhelp32.h>

uint32_t BeamNG::GetProcessID() {
    SetLastError(0);
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    HANDLE Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if(Process32First(Snapshot, &pe32)) {
        do{
            if(std::string("BeamNG.drive.x64.exe") == pe32.szExeFile)break;
        }while(Process32Next(Snapshot, &pe32));
    }

    if(Snapshot != INVALID_HANDLE_VALUE) {
        CloseHandle(Snapshot);
    }

    if(GetLastError() != 0)return 0;
    return pe32.th32ProcessID;
}

size_t BeamNG::GetModuleBase(const char* Name) {
    return (size_t)GetModuleHandleA(Name);
}
