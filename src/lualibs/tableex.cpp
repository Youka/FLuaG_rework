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
#include <functional>

// Lua compatibility macros
#if LUA_VERSION_NUM <= 501
	#define lua_rawlen lua_objlen
#endif
#define luaL_optboolean(L, arg, d) (lua_isnoneornil(L, arg) ? d : (luaL_checktype(L, arg, LUA_TBOOLEAN), lua_toboolean(L, arg)))

static int table_copy(lua_State* L){
	// Check main argument
	luaL_checktype(L, 1, LUA_TTABLE);
	// Get optional argument
	const bool deep = luaL_optboolean(L, 2, true);
	// Remove non-main (=unnecessary) arguments
	const int args_n = lua_gettop(L);
	if(args_n > 1)
		lua_pop(L, args_n - 1);
	// Copy recursive & generate new table
	std::function<void(void)> copy;
	copy = [&copy,L,deep](){
		// Create output table
		lua_createtable(L, lua_rawlen(L, -1), 0);
		// Iterate through input table elements
		lua_pushnil(L);
		while(lua_next(L, -3)){
			// Duplicate key for next iteration pass
			lua_pushvalue(L, -2);
			lua_insert(L, -2);
			// Set key+value to output table
			if(lua_istable(L, -1) && deep)
				copy();
			lua_rawset(L, -4);
		}
		// Finally, set input metatable to output
		if(lua_getmetatable(L, -2))
			lua_setmetatable(L, -2);
		// Remove input table
		lua_remove(L, -2);
	};
	copy();
	// Return table result of copy
	return 1;
}

static int table_tostring(lua_State* L){
	// Check main argument
	luaL_checktype(L, 1, LUA_TTABLE);

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
