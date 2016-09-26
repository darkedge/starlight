#include "starlight_lua.h"

#include "starlight_log.h"

#include <stdio.h>
#include <string.h>

extern "C"
{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
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

    luaL_loadfile(L, "starlight.lbc");
    lua_pcall(L, 0, 0, 0);


    lua_getglobal(L, "f");
    if (!lua_isfunction(L, -1)) {
        logger::LogInfo("f is not a function");
    }

    if (lua_pcall(L, 0, 0, 0)) {
        char buf[256] = {};
        sprintf(buf,"error running function: %s", lua_tostring(L, -1));
        logger::LogInfo(buf);
    }
}

void slDestroyLuaVM() {
	lua_close(L);
}
