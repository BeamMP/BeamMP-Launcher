///
/// Created by Anonymous275 on 1/30/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#include "Memory/Definitions.h"
#include "lua/lj_strscan.h"
#include "lua/lj_arch.h"
#include "lua/lj_obj.h"
#include "lua/lj_def.h"
#include "lua/lj_gc.h"
#include "lua/lj_bc.h"

LUA_API int lua_gettop(lua_State *L) {
    return (int)(L->top - L->base);
}
