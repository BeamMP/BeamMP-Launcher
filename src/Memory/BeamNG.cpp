///
/// Created by Anonymous275 on 1/21/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#include "Memory/Patterns.h"
#include "Memory/BeamNG.h"
#include "Memory/Memory.h"

std::string GetHex(uint64_t num) {
    char buffer[30];
    sprintf(buffer, "%llx", num);
    return std::string{buffer};
}

void BeamNG::EntryPoint() {
    auto GameBaseAddr = Memory::GetModuleBase("BeamNG.drive.x64.exe");
    auto DllBaseAddr = Memory::GetModuleBase("libbeamng.x64.dll");
    Memory::Print("PID : " + std::to_string(Memory::GetPID()));

    auto res = Memory::FindByPattern("BeamNG.drive.x64.exe", Patterns::GetTickCount[0], Patterns::GetTickCount[1]);
    auto res2 = Memory::FindByPattern("BeamNG.drive.x64.exe", Patterns::open_jit[0], Patterns::open_jit[1]);
    auto res3 = Memory::FindByPattern("BeamNG.drive.x64.exe", Patterns::get_field[0], Patterns::get_field[1]);
    auto res4 = Memory::FindByPattern("BeamNG.drive.x64.exe", Patterns::push_fstring[0], Patterns::push_fstring[1]);
    auto res5 = Memory::FindByPattern("BeamNG.drive.x64.exe", Patterns::p_call[0], Patterns::p_call[1]);


}
