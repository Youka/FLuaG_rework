/*
Project: FLuaG
File: tableex.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "libs.h"

static int table_copy(lua_State* L){

	// TODO

	return 0;
}

static int table_tostring(lua_State* L){

	// TODO

	return 0;
}

int luaopen_tableex(lua_State* L){
	// Get 'table' table
	lua_getglobal(L, "table");
	const bool is_global = lua_istable(L, -1);
	if(!is_global){
		lua_pop(L, 1);
		lua_createtable(L, 0, 2);
	}
	// Set table functions
	lua_pushcfunction(L, table_copy); lua_setfield(L, -2, "copy");
	lua_pushcfunction(L, table_tostring); lua_setfield(L, -2, "tostring");
	// Set table to global environment
	if(is_global)
		lua_pop(L, 1);
	else
		lua_setglobal(L, "table");
	return 0;
}
