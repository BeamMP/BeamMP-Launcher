///
/// Created by Anonymous275 on 1/21/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#define WIN32_LEAN_AND_MEAN
#undef UNICODE
#include "Memory/Memory.h"
#include "Memory/BeamNG.h"
#include <psapi.h>
#include <string>
#include <tlhelp32.h>

uint32_t Memory::GetBeamNGPID(const std::set<uint32_t>& BL) {
    SetLastError(0);
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    HANDLE Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (Process32First(Snapshot, &pe32)) {
        do {
            if (std::string("BeamNG.drive.x64.exe") == pe32.szExeFile
                && BL.find(pe32.th32ProcessID) == BL.end()
                && BL.find(pe32.th32ParentProcessID) == BL.end()) {
                break;
            }
        } while (Process32Next(Snapshot, &pe32));
    }

    if (Snapshot != INVALID_HANDLE_VALUE) {
        CloseHandle(Snapshot);
    }

    if (GetLastError() != 0)
        return 0;
    return pe32.th32ProcessID;
}

uint64_t Memory::GetModuleBase(const char* Name) {
    return (uint64_t)GetModuleHandleA(Name);
}

uint32_t Memory::GetPID() {
    return GetCurrentProcessId();
}

uint64_t Memory::FindPattern(const char* module, const char* Pattern[]) {
    MODULEINFO mInfo { nullptr };
    GetModuleInformation(GetCurrentProcess(), GetModuleHandleA(module), &mInfo, sizeof(MODULEINFO));
    auto base = uint64_t(mInfo.lpBaseOfDll);
    auto size = uint32_t(mInfo.SizeOfImage);
    auto len = strlen(Pattern[1]);
    for (auto i = 0; i < size - len; i++) {
        bool found = true;
        for (auto j = 0; j < len && found; j++) {
            found &= Pattern[1][j] == '?' || Pattern[0][j] == *(char*)(base + i + j);
        }
        if (found) {
            return base + i;
        }
    }
    return 0;
}

void* operator new(size_t size) {
    return GlobalAlloc(GPTR, size);
}

void* operator new[](size_t size) {
    return GlobalAlloc(GPTR, size);
}

void operator delete(void* p) noexcept {
    GlobalFree(p);
}

void operator delete[](void* p) noexcept {
    GlobalFree(p);
}

typedef struct BASE_RELOCATION_ENTRY {
    USHORT Offset : 12;
    USHORT Type : 4;
} BASE_RELOCATION_ENTRY, *PBASE_RELOCATION_ENTRY;

void Memory::Inject(uint32_t PID) {
    PVOID imageBase = GetModuleHandle(nullptr);
    auto dosHeader = (PIMAGE_DOS_HEADER)imageBase;
    auto ntHeader = (PIMAGE_NT_HEADERS)((DWORD_PTR)imageBase + dosHeader->e_lfanew);

    PVOID localImage = VirtualAlloc(nullptr, ntHeader->OptionalHeader.SizeOfImage, MEM_COMMIT, PAGE_READWRITE);
    memcpy(localImage, imageBase, ntHeader->OptionalHeader.SizeOfImage);

    HANDLE targetProcess = OpenProcess(MAXIMUM_ALLOWED, FALSE, PID);
    PVOID targetImage = VirtualAllocEx(targetProcess, nullptr, ntHeader->OptionalHeader.SizeOfImage, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

    DWORD_PTR deltaImageBase = DWORD_PTR(targetImage) - DWORD_PTR(imageBase);

    auto relocationTable = (PIMAGE_BASE_RELOCATION)((DWORD_PTR)localImage + ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
    PDWORD_PTR patchedAddress;
    while (relocationTable->SizeOfBlock > 0) {
        DWORD relocationEntriesCount = (relocationTable->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(USHORT);
        auto relocationRVA = (PBASE_RELOCATION_ENTRY)(relocationTable + 1);
        for (uint32_t i = 0; i < relocationEntriesCount; i++) {
            if (relocationRVA[i].Offset) {
                patchedAddress = PDWORD_PTR(DWORD_PTR(localImage) + relocationTable->VirtualAddress + relocationRVA[i].Offset);
                *patchedAddress += deltaImageBase;
            }
        }
        relocationTable = PIMAGE_BASE_RELOCATION(DWORD_PTR(relocationTable) + relocationTable->SizeOfBlock);
    }
    WriteProcessMemory(targetProcess, targetImage, localImage, ntHeader->OptionalHeader.SizeOfImage, nullptr);
    CreateRemoteThread(targetProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)((DWORD_PTR)EntryPoint + deltaImageBase), nullptr, 0, nullptr);
    CloseHandle(targetProcess);
}

void Memory::Print(const std::string& msg) {
    HANDLE stdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (stdOut != nullptr && stdOut != INVALID_HANDLE_VALUE) {
        DWORD written = 0;
        WriteConsoleA(stdOut, "[BeamMP] ", 9, &written, nullptr);
        WriteConsoleA(stdOut, msg.c_str(), DWORD(msg.size()), &written, nullptr);
        WriteConsoleA(stdOut, "\n", 1, &written, nullptr);
    }
}

uint32_t Memory::EntryPoint() {
    AllocConsole();
    SetConsoleTitleA("BeamMP Console");
    BeamNG::EntryPoint();
    return 0;
}

uint32_t Memory::GetTickCount() {
    return ::GetTickCount();
}
