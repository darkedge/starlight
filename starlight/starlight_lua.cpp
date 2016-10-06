#include "starlight_lua.h"

#include "starlight_game.h"
#include "starlight_log.h"

#include <stdio.h>
#include <string.h>

extern "C"
{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

// Don't really want to keep track of this variable
static GameInfo* luaGameInfo;

int KeyDown(lua_State* L) {
    assert(luaGameInfo);
    const char* str = lua_tostring(L, -1);
    int down = luaGameInfo->controls->GetKey(str) ? 1 : 0;
    lua_pushboolean(L, down);
    return 1;
}

int KeyPressed(lua_State* L) {
    assert(luaGameInfo);
    const char* str = lua_tolstring(L, 1, NULL);
    int down = luaGameInfo->controls->GetKeyDown(str) ? 1 : 0;
    lua_pushboolean(L, down);
    return 1;
}

int Forward(lua_State* L) {
    return 0;
}

int Right(lua_State* L) {
    return 0;
}

int GetPosition(lua_State* L) {
    assert(luaGameInfo);
    Vectormath::Aos::Vector3 v = luaGameInfo->player.GetPosition();

    lua_createtable(L, 0, 4);

    lua_pushstring(L, "x");
    lua_pushnumber(L, v.getX());
    lua_settable(L, -3);  /* 3rd element from the stack top */

    lua_pushstring(L, "y");
    lua_pushnumber(L, v.getY());
    lua_settable(L, -3);

    lua_pushstring(L, "z");
    lua_pushnumber(L, v.getZ());
    lua_settable(L, -3);

    luaL_getmetatable(L, "Vector");
    lua_setmetatable(L, -2);

    return 1;
}

int SetPosition(lua_State* L) {
    assert(luaGameInfo);

    // Pop vector
    lua_pushstring(L, "x");
    lua_gettable(L, -2);
    if (!lua_isnumber(L, -1)) {
        logger::LogInfo("could not find x");
    }
    float x = (float) lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_pushstring(L, "y");
    lua_gettable(L, -2);
    if (!lua_isnumber(L, -1)) {
        logger::LogInfo("could not find y");
    }
    float y = (float) lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_pushstring(L, "z");
    lua_gettable(L, -2);
    if (!lua_isnumber(L, -1)) {
        logger::LogInfo("could not find z");
    }
    float z = (float) lua_tonumber(L, -1);
    lua_pop(L, 1);

    luaGameInfo->player.SetPosition(x, y, z);

    return 0;
}

// Our interface
static const luaL_Reg mylib [] = {
    {"C_Forward", Forward},
    {"C_Right", Right},
    {"KeyDown", KeyDown},
    {"KeyPressed", KeyPressed},
    {"GetPosition", GetPosition},
    {"SetPosition", SetPosition},
    {NULL, NULL}
};

static lua_State* L;

void SetLuaGameInfo(GameInfo* gameInfo) {
    luaGameInfo = gameInfo;
}

void luaCreateVM() {
    L = lua_open();

    luaL_openlibs(L);

    const luaL_Reg* lib = mylib;
    for (; lib->func; lib++) {
        lua_pushcfunction(L, lib->func);
        lua_setglobal(L, lib->name);
    }
}

void luaReload() {
    //luaL_loadfile(L, "starlight.lbc"); // bytecode
    luaL_loadfile(L, "../starlight/starlight.lua"); // ascii
    lua_pcall(L, 0, 0, 0);

    logger::LogInfo("Loaded Lua code.");
}

void luaDestroyVM() {
	lua_close(L);
    L = NULL;
}

void luaUpdate(GameInfo* gameInfo) {
    assert(L);
    
    // Set game globals
    // There should be a global game table instead of loose global vars
    lua_pushnumber(L, gameInfo->deltaTime);
    lua_setglobal(L, "g_deltaTime");

    // Call MoveCamera
    lua_getglobal(L, "MoveCamera");
    if (lua_pcall(L, 0, 0, 0)) {
        logger::LogInfo(lua_tostring(L, -1));
    }

    ImGui::Begin("Lua");
    ImGui::Checkbox("Reload on Save", &gameInfo->reloadOnSave);
    if (ImGui::Button("Manual Reload")) {
        luaReload();
    }
    ImGui::End();
}
