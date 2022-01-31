///
/// Created by Anonymous275 on 1/30/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#pragma once
#include "Definitions.h"

class GELua {
public:
    static void FindAddresses();
    static def::GetTickCount GetTickCount;
    static def::lua_open_jit lua_open_jit;
    static def::lua_push_fstring lua_push_fstring;
    static def::lua_get_field lua_get_field;
    static def::lua_p_call lua_p_call;
    static def::lua_createtable lua_createtable;
    static def::lua_pushcclosure lua_pushcclosure;
    static def::lua_setfield lua_setfield;
    static def::lua_settable lua_settable;
    static def::lua_tolstring lua_tolstring;
    static lua_State* State;
};

namespace GELuaTable {
    inline void Begin(lua_State* L) {
        GELua::lua_createtable(L, 0, 0);
    }
    inline void End(lua_State* L, const char* name) {
        GELua::lua_setfield(L, -10002, name);
    }
    inline void BeginEntry(lua_State* L, const char* name) {
        GELua::lua_push_fstring(L, "%s", name);
    }
    inline void EndEntry(lua_State* L) {
        GELua::lua_settable(L, -3);
    }
    inline void InsertFunction(lua_State* L, const char* name, lua_CFunction func) {
        BeginEntry(L, name);
        GELua::lua_pushcclosure(L, func, 0);
        EndEntry(L);
    }
}
