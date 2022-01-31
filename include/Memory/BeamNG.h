///
/// Created by Anonymous275 on 1/21/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#pragma once
#include "Memory/Detours.h"
#include "Memory/GELua.h"
#include "Memory/IPC.h"
#include <memory>
#include <string>

class BeamNG {
public:
    static void EntryPoint();
    static void SendIPC(const std::string& Data);
private:
    static std::unique_ptr<Detours> TickCountDetour;
    static std::unique_ptr<Detours> OpenJITDetour;
    static std::unique_ptr<IPC> IPCFromLauncher;
    static std::unique_ptr<IPC> IPCToLauncher;
    static int lua_open_jit_D(lua_State* State);
    static void RegisterGEFunctions();
    static uint32_t GetTickCount_D();
    static uint64_t GameBaseAddr;
    static uint64_t DllBaseAddr;
};
