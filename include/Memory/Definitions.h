///
/// Created by Anonymous275 on 1/27/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#pragma once
typedef struct lua_State lua_State;

namespace def {
    typedef unsigned long (*GetTickCount)();
    typedef int (*lua_open_jit)(lua_State* L);
    typedef void (*lua_get_field)(lua_State* L, int idx, const char* k);
    typedef const char* (*lua_push_fstring)(lua_State* L, const char* fmt, ...);
    typedef int(*lua_p_call)(lua_State* L, int arg, int res, int err);
}
