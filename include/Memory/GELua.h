///
/// Created by Anonymous275 on 1/30/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#pragma once
#include "Definitions.h"

class GELua {
public:
    static void FindAddresses();
    static inline def::GEUpdate GEUpdate;
    static inline def::lua_settop lua_settop;
    static inline def::GetTickCount GetTickCount;
    static inline def::lua_open_jit lua_open_jit;
    static inline def::lua_push_fstring lua_push_fstring;
    static inline def::lua_get_field lua_get_field;
    static inline def::lua_p_call lua_p_call;
    static inline def::lua_createtable lua_createtable;
    static inline def::lua_pushcclosure lua_pushcclosure;
    static inline def::lua_setfield lua_setfield;
    static inline def::lua_settable lua_settable;
    static inline def::lua_tolstring lua_tolstring;
    static inline lua_State* State;
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
