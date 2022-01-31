///
/// Created by Anonymous275 on 1/21/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#include "Memory/BeamNG.h"
#include "Memory/Memory.h"

uint32_t BeamNG::GetTickCount_D() {
   if(GELua::State != nullptr) {
       IPCFromLauncher->try_receive();
       if(!IPCFromLauncher->receive_timed_out()) {
           if(IPCFromLauncher->msg()[0] == 'C') {
               GELua::lua_get_field(GELua::State, -10002, "handleCoreMsg");
               Memory::Print(std::string("Sending to handleCoreMsg -> ") + char(IPCFromLauncher->msg()[1]) + std::to_string(IPCFromLauncher->msg().size()) );
           } else {
               GELua::lua_get_field(GELua::State, -10002, "handleGameMsg");
               Memory::Print(std::string("Sending to handleGameMsg -> ") + char(IPCFromLauncher->msg()[1]) + std::to_string(IPCFromLauncher->msg().size()) );
           }
           GELua::lua_push_fstring(GELua::State, "%s", &IPCFromLauncher->c_str()[1]);
           GELua::lua_p_call(GELua::State, 1, 0, 0);
           IPCFromLauncher->confirm_receive();
       }
   }
   return Memory::GetTickCount();
}

int BeamNG::lua_open_jit_D(lua_State* State) {
    Memory::Print("Got lua State");
    GELua::State = State;
    RegisterGEFunctions();
    OpenJITDetour->Detach();
    int r = GELua::lua_open_jit(State);
    OpenJITDetour->Attach();
    return r;
}

void BeamNG::EntryPoint() {
    Memory::Print("PID : " + std::to_string(Memory::GetPID()));
    GELua::FindAddresses();
    /*GameBaseAddr = Memory::GetModuleBase(GameModule);
    DllBaseAddr = Memory::GetModuleBase(DllModule);*/
    TickCountDetour = std::make_unique<Detours>((void*)GELua::GetTickCount, (void*)GetTickCount_D);
    TickCountDetour->Attach();
    OpenJITDetour = std::make_unique<Detours>((void*)GELua::lua_open_jit, (void*)lua_open_jit_D);
    OpenJITDetour->Attach();
    IPCToLauncher = std::make_unique<IPC>("BeamMP_IN", "BeamMP_Sem3", "BeamMP_Sem4", 0x1900000);
    IPCFromLauncher = std::make_unique<IPC>("BeamMP_OUT", "BeamMP_Sem1", "BeamMP_Sem2", 0x1900000);
}

int Core(lua_State* L) {
    if(lua_gettop(L) == 1) {
        size_t Size;
        const char* Data = GELua::lua_tolstring(L, 1, &Size);
        Memory::Print("Core -> " + std::string(Data) + " - " + std::to_string(Size));
        std::string msg(Data, Size);
        BeamNG::SendIPC("C" + msg);
    }
    return 0;
}

int Game(lua_State* L) {
    if(lua_gettop(L) == 1) {
        size_t Size;
        const char* Data = GELua::lua_tolstring(L, 1, &Size);
        Memory::Print("Game -> " + std::string(Data) + " - " + std::to_string(Size));
        std::string msg(Data, Size);
        BeamNG::SendIPC("G" + msg);
    }
    return 0;
}

void BeamNG::RegisterGEFunctions() {
    Memory::Print("Registering GE Functions");
    GELuaTable::Begin(GELua::State);
    GELuaTable::InsertFunction(GELua::State, "Core", Core);
    GELuaTable::InsertFunction(GELua::State, "Game", Game);
    GELuaTable::End(GELua::State, "MP");
    Memory::Print("Registered!");
}

void BeamNG::SendIPC(const std::string& Data) {
    IPCToLauncher->send(Data);
}

std::unique_ptr<Detours> BeamNG::TickCountDetour;
std::unique_ptr<Detours> BeamNG::OpenJITDetour;
std::unique_ptr<IPC> BeamNG::IPCFromLauncher;
std::unique_ptr<IPC> BeamNG::IPCToLauncher;
uint64_t BeamNG::GameBaseAddr;
uint64_t BeamNG::DllBaseAddr;
