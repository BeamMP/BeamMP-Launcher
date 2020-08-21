///
/// Created by Anonymous275 on 5/5/2020
///

#include "Memory.hpp"
#include <iostream>
#include <utility>


int Memory::GetProcessId(const std::string& processName) {
    SetLastError(0);
    PROCESSENTRY32 pe32;
    HANDLE hSnapshot = nullptr;
    GetLastError();
    pe32.dwSize = sizeof(PROCESSENTRY32);
    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if(Process32First(hSnapshot,&pe32)) {
        do {
            if(processName == pe32.szExeFile)
                break;
        } while(Process32Next(hSnapshot, &pe32));
    }

    if(hSnapshot != INVALID_HANDLE_VALUE)
        CloseHandle(hSnapshot);
    int err = GetLastError();

    if (err != 0)
        return 0;
    return pe32.th32ProcessID;
}

long long Memory::GetModuleBase(HANDLE processHandle, const std::string &sModuleName){
    HMODULE *hModules = nullptr;
    char szBuf[50];
    DWORD cModules;
    long long dwBase = -1;

    EnumProcessModulesEx(processHandle, hModules, 0, &cModules,LIST_MODULES_ALL);
    hModules = new HMODULE[cModules/sizeof(HMODULE)];

    if(EnumProcessModulesEx(processHandle, hModules, cModules/sizeof(HMODULE), &cModules,LIST_MODULES_ALL)) {
        for(size_t i = 0; i < cModules/sizeof(HMODULE); i++) {
            if(GetModuleBaseName(processHandle, hModules[i], szBuf, sizeof(szBuf))) {
                if(sModuleName == szBuf) {
                    dwBase = (long long)hModules[i];
                    break;
                }
            }
        }
    }

    delete[] hModules;
    return dwBase;
}

void PrintAllBases(HANDLE processHandle){
    HMODULE *hModules = nullptr;
    char szBuf[50];
    DWORD cModules;
    EnumProcessModulesEx(processHandle, hModules, 0, &cModules,LIST_MODULES_ALL);
    hModules = new HMODULE[cModules/sizeof(HMODULE)];
    if(EnumProcessModulesEx(processHandle, hModules, cModules/sizeof(HMODULE), &cModules,LIST_MODULES_ALL)) {
        for(size_t i = 0; i < cModules/sizeof(HMODULE); i++) {
            if(GetModuleBaseName(processHandle, hModules[i], szBuf, sizeof(szBuf))) {
                if(hModules[i] != nullptr){
                    std::cout << szBuf << " : " << hModules[i] << std::endl;
                }
            }
        }
    }
    delete[] hModules;
}

BOOL Memory::SetPrivilege(HANDLE hToken, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege){
    TOKEN_PRIVILEGES tp;
    LUID luid;

    if (!LookupPrivilegeValue(nullptr, lpszPrivilege, &luid)) {
        //printf("LookupPrivilegeValue error: %u\n", GetLastError() );
        return FALSE;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    if (bEnablePrivilege)
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    else
        tp.Privileges[0].Attributes = 0;

    if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), (PTOKEN_PRIVILEGES) nullptr, (PDWORD) nullptr)) {
        //printf("AdjustTokenPrivileges error: %u\n", GetLastError() );
        return FALSE;
    }

    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
        //printf("The token does not have the specified privilege. \n");
        return FALSE;
    }

    return TRUE;
}
BOOL Memory::GetDebugPrivileges() {
    HANDLE hToken = nullptr;
    if(!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
        return FALSE; //std::cout << "OpenProcessToken() failed, error\n>> " << GetLastError() << std::endl;
    //else std::cout << "OpenProcessToken() is OK, got the handle!" << std::endl;

    if(!SetPrivilege(hToken, SE_DEBUG_NAME, TRUE))
        return FALSE; //std::cout << "Failed to enable privilege, error:\n>> " << GetLastError() << std::endl;

    return TRUE;
}
int Memory::ReadInt(HANDLE processHandle, long long address) {
    if (address == -1)
        return -1;
    int buffer = 0;
    SIZE_T NumberOfBytesToRead = sizeof(buffer); //this is equal to 4
    SIZE_T NumberOfBytesActuallyRead;
    BOOL success = ReadProcessMemory(processHandle, (LPCVOID)address, &buffer, NumberOfBytesToRead, &NumberOfBytesActuallyRead);
    if (!success || NumberOfBytesActuallyRead != NumberOfBytesToRead) {
        //std::cout << "Memory Error!" << std::endl;
        return -1;
    }
    return buffer;
}
long long Memory::ReadLong(HANDLE processHandle, long long address) {
    if (address == -1)
        return -1;
    long long buffer = 0;
    SIZE_T NumberOfBytesToRead = sizeof(buffer);
    SIZE_T NumberOfBytesActuallyRead;
    BOOL success = ReadProcessMemory(processHandle, (LPCVOID)address, &buffer, NumberOfBytesToRead, &NumberOfBytesActuallyRead);
    if (!success || NumberOfBytesActuallyRead != NumberOfBytesToRead) {
        //std::cout << "Memory Error!" << std::endl;
        return -1;
    }
    return buffer;
}
int Memory::GetPointerAddress(HANDLE processHandle, long long startAddress, int offsets[], int offsetCount) {
    if (startAddress == -1)
        return -1;
    int ptr = ReadInt(processHandle, startAddress);
    for (int i=0; i<offsetCount-1; i++) {
        ptr+=offsets[i];
        ptr = ReadInt(processHandle, ptr);
    }
    ptr+=offsets[offsetCount-1];
    return ptr;
}
long long Memory::GetPointerAddressLong(HANDLE processHandle, long long startAddress, std::vector<int> offsets) {
    if (startAddress == -1)
        return -1;
    long long ptr = ReadLong(processHandle, startAddress);
    for (int i=0; i< offsets.size()-1; i++) {
        ptr+=offsets[i];
        ptr = ReadLong(processHandle, ptr);
    }
    ptr+=offsets[offsets.size()-1];
    return ptr;
}
int Memory::ReadPointerInt(HANDLE processHandle, long long startAddress, std::vector<int> offsets) {
    if (startAddress == -1)
        return -1;
    return ReadInt(processHandle, GetPointerAddressLong(processHandle, startAddress, std::move(offsets)));
}
long long Memory::ReadPointerLong(HANDLE processHandle, long long startAddress, std::vector<int> offsets) {
    if (startAddress == -1)
        return -1;
    return ReadLong(processHandle, GetPointerAddressLong(processHandle, startAddress, std::move(offsets)));
}

float Memory::ReadFloat(HANDLE processHandle, long long address) {
    if (address == -1)
        return -1;
    float buffer = 0.0;
    SIZE_T NumberOfBytesToRead = sizeof(buffer); //this is equal to 4
    SIZE_T NumberOfBytesActuallyRead;
    BOOL success = ReadProcessMemory(processHandle, (LPCVOID)address, &buffer, NumberOfBytesToRead, &NumberOfBytesActuallyRead);
    if (!success || NumberOfBytesActuallyRead != NumberOfBytesToRead)
        return -1;
    return buffer;
}
void Memory::WriteFloat(HANDLE processHandle, long long address,float Value) {
    if (address == -1)
        return;
    SIZE_T NumberOfBytesToWrite = sizeof(Value); //this is equal to 4
    SIZE_T NumberOfBytesWritten;
    BOOL Write = WriteProcessMemory(processHandle, LPVOID(address), &Value, NumberOfBytesToWrite, &NumberOfBytesWritten);
}
double Memory::ReadDouble(HANDLE processHandle, long long address) {
    if (address == -1)
        return -1;
    double buffer = 0.0;
    SIZE_T NumberOfBytesToRead = sizeof(buffer); //this is equal to 8
    SIZE_T NumberOfBytesActuallyRead;
    BOOL success = ReadProcessMemory(processHandle, (LPCVOID)address, &buffer, NumberOfBytesToRead, &NumberOfBytesActuallyRead);
    if (!success || NumberOfBytesActuallyRead != NumberOfBytesToRead)
        return -1;
    return buffer;
}
float Memory::ReadPointerFloat(HANDLE processHandle, long long startAddress, std::vector<int> offsets) {
    if (startAddress == -1)
        return -1;
    return ReadFloat(processHandle, GetPointerAddressLong(processHandle, startAddress, std::move(offsets)));
}
double Memory::ReadPointerDouble(HANDLE processHandle, long long startAddress, int offsets[], int offsetCount) {
    if (startAddress == -1)
        return -1;
    return ReadDouble(processHandle, GetPointerAddress(processHandle, startAddress, offsets, offsetCount));
}
std::string Memory::ReadText(HANDLE processHandle, long long address) {
    if (address == -1)
        return "-1";
    char buffer = 1;
    char* stringToRead = new char[128];
    SIZE_T NumberOfBytesToRead = sizeof(buffer);
    SIZE_T NumberOfBytesActuallyRead;
    int i = 0;
    while (buffer != 0) {
        BOOL success = ReadProcessMemory(processHandle, (LPCVOID)address, &buffer, NumberOfBytesToRead, &NumberOfBytesActuallyRead);
        if (!success || NumberOfBytesActuallyRead != NumberOfBytesToRead)
            return "-1";
        stringToRead[i] = buffer;
        i++;
        address++;
    }
    return stringToRead;
}
std::string Memory::ReadPointerText(HANDLE processHandle, long long startAddress, std::vector<int> offsets) {
    if (startAddress == -1)
        return "-1";
    return ReadText(processHandle, GetPointerAddressLong(processHandle, startAddress, std::move(offsets)));
}