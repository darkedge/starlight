#pragma once

void luaCreateVM();
void luaDestroyVM();

void SetLuaGameInfo(GameInfo* gameInfo);
void luaUpdate(GameInfo* gameInfo);
void luaReload();
