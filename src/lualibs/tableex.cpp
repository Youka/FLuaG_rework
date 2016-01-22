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
#include "../utils/lua.h"
#include <functional>
#include <sstream>

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
			if(deep && lua_istable(L, -1))
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
	// Check argument
	luaL_checktype(L, 1, LUA_TTABLE);
	// Remove unnecessary arguments
	const int args_n = lua_gettop(L);
	if(args_n > 1)
		lua_pop(L, args_n - 1);
	// Output buffer
	std::ostringstream buf;
	buf << std::boolalpha << std::showbase;
	// Build table string in buffer
	std::function<void(const unsigned)> build;
	build = [&build,L,&buf](const unsigned indent){
		// Indentation strings
		const std::string indent_str(indent, '\t'), indent_more_str(indent+1, '\t');
		// Write table header
		if(buf.tellp())
			buf << '\n';
		buf << indent_str << '{';
		// Iterate through table elements
		lua_pushnil(L);
		while(lua_next(L, -2)){
			// Write key
			buf << '\n' << indent_more_str << '[';
			switch(lua_type(L, -2)){
				case LUA_TBOOLEAN:
					buf << static_cast<bool>(lua_toboolean(L, -2));
					break;
				case LUA_TNUMBER:
					buf << std::dec << lua_tonumber(L, -2);
					break;
                                case LUA_TSTRING:
                                	buf << '"' << lua_tostring(L, -2) << '"';
					break;
				case LUA_TTABLE:
				case LUA_TFUNCTION:
				case LUA_TLIGHTUSERDATA:
				case LUA_TUSERDATA:
				case LUA_TTHREAD:
					switch(lua_type(L, -2)){
						case LUA_TTABLE: buf << "table:"; break;
						case LUA_TFUNCTION: buf << "function:"; break;
						case LUA_TLIGHTUSERDATA: buf << "lightuserdata:"; break;
						case LUA_TUSERDATA: buf << "userdata:"; break;
						case LUA_TTHREAD: buf << "thread:"; break;
					}
					buf << std::hex << lua_topointer(L, -2);
					break;
			}
			buf << "] = ";
			// Write value
			switch(lua_type(L, -1)){
				case LUA_TBOOLEAN:
					buf << static_cast<bool>(lua_toboolean(L, -1));
					break;
				case LUA_TNUMBER:
					buf << std::dec << lua_tonumber(L, -1);
					break;
                                case LUA_TSTRING:
                                	buf << '"' << lua_tostring(L, -1) << '"';
					break;
				case LUA_TTABLE:
					build(indent+1);
					break;
				case LUA_TFUNCTION:
				case LUA_TLIGHTUSERDATA:
				case LUA_TUSERDATA:
				case LUA_TTHREAD:
					switch(lua_type(L, -1)){
						case LUA_TFUNCTION: buf << "function:"; break;
						case LUA_TLIGHTUSERDATA: buf << "lightuserdata:"; break;
						case LUA_TUSERDATA: buf << "userdata:"; break;
						case LUA_TTHREAD: buf << "thread:"; break;
					}
					buf << std::hex << lua_topointer(L, -1);
					break;
			}
			buf << ',';
			// Remove value, keep key for next iteration pass
			lua_pop(L, 1);
		}
		// Write table footer
		buf << '\n' << indent_str << '}';
	};
	build(0);
	// Return buffer string to Lua
	lua_pushstring(L, buf.str().c_str());
	return 1;
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
