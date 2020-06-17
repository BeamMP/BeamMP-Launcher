///
/// Created by Anonymous275 on 5/5/2020
///

#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <vector>
#include <psapi.h>

class Memory
{
public:
    DWORD PID = 0;
    int GetProcessId(const std::string& processName);
    long long ReadLong(HANDLE processHandle, long long address);
    long long ReadPointerLong(HANDLE processHandle, long long startAddress, std::vector<int> offsets);
    long long GetPointerAddressLong(HANDLE processHandle, long long startAddress, std::vector<int> offsets);
    long long GetModuleBase(HANDLE processHandle, const std::string&sModuleName);
    BOOL SetPrivilege(HANDLE hToken, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege);
    BOOL GetDebugPrivileges();
    int ReadInt(HANDLE processHandle, long long address);
    int GetPointerAddress(HANDLE processHandle, long long startAddress, int offsets[], int offsetCount);
    int ReadPointerInt(HANDLE processHandle, long long startAddress, std::vector<int> offsets);
    float ReadFloat(HANDLE processHandle, long long address);
    void WriteFloat(HANDLE processHandle, long long address,float Value);
    double ReadDouble(HANDLE processHandle, long long address);
    float ReadPointerFloat(HANDLE processHandle, long long startAddress, std::vector<int> offsets);
    double ReadPointerDouble(HANDLE processHandle, long long startAddress, int offsets[], int offsetCount);
    std::string ReadText(HANDLE processHandle, long long address);
    std::string ReadPointerText(HANDLE processHandle, long long startAddress, std::vector<int> offsets);
};