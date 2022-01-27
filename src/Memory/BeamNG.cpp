///
/// Created by Anonymous275 on 1/21/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#include "Memory/Patterns.h"
#include "Memory/BeamNG.h"
#include "Memory/Memory.h"

uint32_t BeamNG::GetTickCount_D() {
    if(GEState != nullptr){
        lua_get_field(GEState, -10002, "print");
        lua_push_fstring(GEState, "Helloooooo");
        lua_p_call(GEState, 1, 0, 0);
    }
    return Memory::GetTickCount();
}

int BeamNG::lua_open_jit_D(lua_State* State) {
    Memory::Print("Got lua State");
    GEState = State;
    OpenJITDetour->Detach();
    int r = lua_open_jit(State);
    OpenJITDetour->Attach();
    return r;
}

void BeamNG::EntryPoint() {
    Memory::Print("PID : " + std::to_string(Memory::GetPID()));
    GameModule = "BeamNG.drive.x64.exe";
    DllModule = "libbeamng.x64.dll";
    GEState = nullptr;
    GameBaseAddr = Memory::GetModuleBase(GameModule);
    DllBaseAddr = Memory::GetModuleBase(DllModule);
    GetTickCount = reinterpret_cast<def::GetTickCount>(Memory::FindPattern(GameModule, Patterns::GetTickCount[0],Patterns::GetTickCount[1]));
    lua_open_jit = reinterpret_cast<def::lua_open_jit>(Memory::FindPattern(GameModule, Patterns::open_jit[0], Patterns::open_jit[1]));
    lua_push_fstring = reinterpret_cast<def::lua_push_fstring>(Memory::FindPattern(GameModule, Patterns::push_fstring[0], Patterns::push_fstring[1]));
    lua_get_field = reinterpret_cast<def::lua_get_field>(Memory::FindPattern(GameModule, Patterns::get_field[0], Patterns::get_field[1]));
    lua_p_call = reinterpret_cast<def::lua_p_call>(Memory::FindPattern(GameModule, Patterns::p_call[0], Patterns::p_call[1]));
    TickCountDetour = std::make_unique<Detours>((void*)GetTickCount, (void*)GetTickCount_D);
    TickCountDetour->Attach();
    OpenJITDetour = std::make_unique<Detours>((void*)lua_open_jit, (void*)lua_open_jit_D);
    OpenJITDetour->Attach();
}

std::unique_ptr<Detours> BeamNG::TickCountDetour;
std::unique_ptr<Detours> BeamNG::OpenJITDetour;
uint64_t BeamNG::GameBaseAddr;
uint64_t BeamNG::DllBaseAddr;
def::GetTickCount BeamNG::GetTickCount;
def::lua_open_jit BeamNG::lua_open_jit;
def::lua_push_fstring BeamNG::lua_push_fstring;
def::lua_get_field BeamNG::lua_get_field;
def::lua_p_call BeamNG::lua_p_call;
const char* BeamNG::GameModule;
const char* BeamNG::DllModule;
lua_State* BeamNG::GEState;
