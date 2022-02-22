///
/// Created by Anonymous275 on 1/21/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///


#include "atomic_queue/atomic_queue.h"
#include "Memory/BeamNG.h"
#include "Memory/Memory.h"

atomic_queue::AtomicQueue2<std::string, 1000> AtomicQueue;

int BeamNG::lua_open_jit_D(lua_State* State) {
    Memory::Print("Got lua State");
    GELua::State = State;
    RegisterGEFunctions();
    return OpenJITDetour->Original(State);
}

void BeamNG::EntryPoint() {
    auto status = MH_Initialize();
    if(status != MH_OK)Memory::Print(std::string("MH Error -> ") + MH_StatusToString(status));
    Memory::Print("PID : " + std::to_string(Memory::GetPID()));
    GELua::FindAddresses();
    /*GameBaseAddr = Memory::GetModuleBase(GameModule);
    DllBaseAddr = Memory::GetModuleBase(DllModule);*/
    OpenJITDetour = std::make_unique<Hook<def::lua_open_jit>>(GELua::lua_open_jit, lua_open_jit_D);
    OpenJITDetour->Enable();
    IPCToLauncher = std::make_unique<IPC>("BeamMP_IN", "BeamMP_Sem3", "BeamMP_Sem4", 0x1900000);
    IPCFromLauncher = std::make_unique<IPC>("BeamMP_OUT", "BeamMP_Sem1", "BeamMP_Sem2", 0x1900000);
    IPCListener();
}

int Core(lua_State* L) {
    if(lua_gettop(L) == 1) {
        size_t Size;
        const char* Data = GELua::lua_tolstring(L, 1, &Size);
        //Memory::Print("Core -> " + std::string(Data) + " - " + std::to_string(Size));
        std::string msg(Data, Size);
        BeamNG::SendIPC("C" + msg);
    }
    return 0;
}

int Game(lua_State* L) {
    if(lua_gettop(L) == 1) {
        size_t Size;
        const char* Data = GELua::lua_tolstring(L, 1, &Size);
        //Memory::Print("Game -> " + std::string(Data) + " - " + std::to_string(Size));
        std::string msg(Data, Size);
        BeamNG::SendIPC("G" + msg);
    }
    return 0;
}

int LuaPop(lua_State* L) {
    std::string MSG;
    if (AtomicQueue.try_pop(MSG)) {
        GELua::lua_push_fstring(L, "%s", MSG.c_str());
        return 1;
    }
    return 0;
}

void BeamNG::RegisterGEFunctions() {
    Memory::Print("Registering GE Functions");
    GELuaTable::Begin(GELua::State);
    GELuaTable::InsertFunction(GELua::State, "Core", Core);
    GELuaTable::InsertFunction(GELua::State, "Game", Game);
    GELuaTable::InsertFunction(GELua::State, "try_pop", LuaPop);
    GELuaTable::End(GELua::State, "MP");
    Memory::Print("Registered!");
}

void BeamNG::SendIPC(const std::string& Data) {
    IPCToLauncher->send(Data);
}

void BeamNG::IPCListener() {
    int TimeOuts = 0;
    while(TimeOuts < 20) {
        IPCFromLauncher->receive();
        if (!IPCFromLauncher->receive_timed_out()) {
            TimeOuts = 0;
            AtomicQueue.push(IPCFromLauncher->msg());
            IPCFromLauncher->confirm_receive();
        } else TimeOuts++;
    }
    Memory::Print("IPC System shutting down");
}
