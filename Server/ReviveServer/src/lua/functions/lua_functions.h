#pragma once
extern "C" {
#include "include\lua.h"
#include "include\lauxlib.h"
#include "include\lualib.h"
}
#include<chrono>


int API_get_x(lua_State* L);
int API_get_y(lua_State* L);
int API_get_z(lua_State* L);
int API_test_lua(lua_State* L);

int API_attack(lua_State* L);
int API_move(lua_State* L);