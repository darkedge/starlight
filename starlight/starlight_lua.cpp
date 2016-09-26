#include "starlight_lua.h"

#include "starlight_log.h"

#include <stdio.h>
#include <string.h>

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

static lua_State* L;

static const luaL_Reg lualibs[] = {
    {"", luaopen_base},
    {LUA_LOADLIBNAME, luaopen_package},
    {LUA_TABLIBNAME, luaopen_table},
    {LUA_STRLIBNAME, luaopen_string},
    {LUA_MATHLIBNAME, luaopen_math},
    {LUA_DBLIBNAME, luaopen_debug},
    {NULL, NULL},
};

void slCreateLuaVM() {
    L = lua_open();

    // luaL_openlibs(L)
    const luaL_Reg *lib = lualibs;
    for (; lib->func; lib++) {
        lua_pushcfunction(L, lib->func);
        lua_pushstring(L, lib->name);
        lua_call(L, 1, 0);
    }
}

void slDestroyLuaVM() {
	lua_close(L);
}
