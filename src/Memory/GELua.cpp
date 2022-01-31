///
/// Created by Anonymous275 on 1/30/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#include "Memory/Patterns.h"
#include "Memory/Memory.h"
#include "Memory/GELua.h"

const char* GameModule = "BeamNG.drive.x64.exe";
const char* DllModule = "libbeamng.x64.dll";

void GELua::FindAddresses() {
    GELua::State = nullptr;
    GetTickCount = reinterpret_cast<def::GetTickCount>(Memory::FindPattern(GameModule, Patterns::GetTickCount));
    lua_open_jit = reinterpret_cast<def::lua_open_jit>(Memory::FindPattern(GameModule, Patterns::open_jit));
    lua_push_fstring = reinterpret_cast<def::lua_push_fstring>(Memory::FindPattern(GameModule, Patterns::push_fstring));
    lua_get_field = reinterpret_cast<def::lua_get_field>(Memory::FindPattern(GameModule, Patterns::get_field));
    lua_p_call = reinterpret_cast<def::lua_p_call>(Memory::FindPattern(GameModule, Patterns::p_call));
    lua_createtable = reinterpret_cast<def::lua_createtable>(Memory::FindPattern(GameModule, Patterns::lua_createtable));
    lua_pushcclosure = reinterpret_cast<def::lua_pushcclosure>(Memory::FindPattern(GameModule, Patterns::lua_pushcclosure));
    lua_setfield = reinterpret_cast<def::lua_setfield>(Memory::FindPattern(GameModule, Patterns::lua_setfield));
    lua_settable = reinterpret_cast<def::lua_settable>(Memory::FindPattern(GameModule, Patterns::lua_settable));
    lua_tolstring = reinterpret_cast<def::lua_tolstring>(Memory::FindPattern(GameModule, Patterns::lua_tolstring));
}


def::GetTickCount GELua::GetTickCount;
def::lua_open_jit GELua::lua_open_jit;
def::lua_push_fstring GELua::lua_push_fstring;
def::lua_get_field GELua::lua_get_field;
def::lua_p_call GELua::lua_p_call;
def::lua_createtable GELua::lua_createtable;
def::lua_pushcclosure GELua::lua_pushcclosure;
def::lua_setfield GELua::lua_setfield;
def::lua_settable GELua::lua_settable;
def::lua_tolstring GELua::lua_tolstring;
lua_State* GELua::State;
