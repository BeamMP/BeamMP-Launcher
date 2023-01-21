///
/// Created by Anonymous275 on 1/21/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#pragma once
#include <memory>
#include <string>
#include "Memory/GELua.h"
#include "Memory/Hook.h"
#include "Memory/IPC.h"

class BeamNG {
   public:
   static void EntryPoint();
   static void SendIPC(const std::string& Data);

   private:
   static inline std::unique_ptr<Hook<def::update_function>> UpdateDetour;
   static inline std::unique_ptr<Hook<def::lua_open_jit>> OpenJITDetour;
   static inline std::unique_ptr<IPC> IPCFromLauncher;
   static inline std::unique_ptr<IPC> IPCToLauncher;
   static inline uint64_t GameBaseAddr;
   static inline uint64_t DllBaseAddr;
   static int lua_open_jit_D(lua_State* State);
   static uint64_t update_D(lua_State* State);
   static void RegisterGEFunctions();
   // static int GetTickCount_D(void* GEState, void* Param2, void* Param3, void*
   // Param4);
   static void IPCListener();
   static uint32_t IPCSender(void* LP);
};
