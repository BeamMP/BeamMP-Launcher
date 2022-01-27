///
/// Created by Anonymous275 on 1/21/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#pragma once
#include "Memory/Detours.h"
#include "Definitions.h"
#include <cstdint>
#include <memory>

class BeamNG {
public:
    static void EntryPoint();
private:
    static std::unique_ptr<Detours> TickCountDetour;
    static std::unique_ptr<Detours> OpenJITDetour;
    static int lua_open_jit_D(lua_State* State);
    static uint32_t GetTickCount_D();
    static uint64_t GameBaseAddr;
    static uint64_t DllBaseAddr;
    static def::GetTickCount GetTickCount;
    static def::lua_open_jit lua_open_jit;
    static def::lua_push_fstring lua_push_fstring;
    static def::lua_get_field lua_get_field;
    static def::lua_p_call lua_p_call;
    static const char* GameModule;
    static const char* DllModule;
    static lua_State* GEState;
};
