///
/// Created by Anonymous275 on 1/27/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#pragma once
#include <cstdint>

typedef struct lua_State lua_State;
typedef int (*lua_CFunction)(lua_State*);
extern int lua_gettop(lua_State* L);
namespace def {
   typedef int (*GEUpdate)(void* Param1, void* Param2, void* Param3,
                           void* Param4);
   typedef uint32_t (*GetTickCount)();
   typedef int (*lua_open_jit)(lua_State* L);
   typedef void (*lua_get_field)(lua_State* L, int idx, const char* k);
   typedef const char* (*lua_push_fstring)(lua_State* L, const char* fmt, ...);
   typedef int (*lua_p_call)(lua_State* L, int arg, int res, int err);
   typedef void (*lua_pushcclosure)(lua_State* L, lua_CFunction fn, int n);
   typedef int (*lua_settop)(lua_State* L, int idx);
   typedef void (*lua_settable)(lua_State* L, int idx);
   typedef void (*lua_createtable)(lua_State* L, int narray, int nrec);
   typedef void (*lua_setfield)(lua_State* L, int idx, const char* k);
   typedef const char* (*lua_tolstring)(lua_State* L, int idx, size_t* len);
}
