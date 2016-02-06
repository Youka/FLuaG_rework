/*
Project: FLuaG
File: tablex.cpp

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
	lua_settop(L, 1);
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
	lua_settop(L, 1);
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

static int table_all_of(lua_State* L){
	// Check arguments
	luaL_checktype(L, 1, LUA_TTABLE);
	luaL_argcheck(L, !lua_isnoneornil(L, 2), 2, "none or nil not accepted");
	// Clear stack from trash arguments
	lua_settop(L, 2);
	// Table contents all of ...?
	const size_t n = lua_rawlen(L, 1);
	if(lua_isfunction(L, 2))
		for(size_t i = 1; i <= n; ++i){
			lua_pushvalue(L, 2);
			lua_rawgeti(L, 1, i);
			lua_call(L, 1, 1);
			if(!lua_toboolean(L, -1)){
				lua_pushboolean(L, false);
				return 1;
			}
			lua_pop(L, 1);
		}
	else
		for(size_t i = 1; i <= n; ++i){
			lua_rawgeti(L, 1, i);
			if(!lua_equal(L, 2, -1)){
				lua_pushboolean(L, false);
				return 1;
			}
			lua_pop(L, 1);
		}
	lua_pushboolean(L, true);
	return 1;
}

static int table_compare(lua_State* L){
	// Check arguments
	luaL_checktype(L, 1, LUA_TTABLE);
	luaL_checktype(L, 2, LUA_TTABLE);
	luaL_argcheck(L, lua_isnoneornil(L, 3) || lua_isfunction(L, 3), 3, "optional function expected");
	// Clear stack from trash arguments
	lua_settop(L, 3);
	// Compare table contents
	const size_t n = lua_rawlen(L, 1);
	if(n != lua_rawlen(L, 2)){
		lua_pushboolean(L, false);
		return 1;
	}
	if(lua_isfunction(L, 3))
		for(size_t i = 1; i <= n; ++i){
			lua_pushvalue(L, 3);
			lua_rawgeti(L, 1, i); lua_rawgeti(L, 2, i);
			lua_call(L, 2, 1);
			if(!lua_toboolean(L, -1)){
				lua_pushboolean(L, false);
				return 1;
			}
			lua_pop(L, 1);
		}
	else
		for(size_t i = 1; i <= n; ++i){
			lua_rawgeti(L, 1, i); lua_rawgeti(L, 2, i);
			if(!lua_equal(L, -2, -1)){
				lua_pushboolean(L, false);
				return 1;
			}
			lua_pop(L, 2);
		}
	lua_pushboolean(L, true);
	return 1;
}

static int table_count(lua_State* L){
	// Check arguments
	luaL_checktype(L, 1, LUA_TTABLE);
	luaL_argcheck(L, !lua_isnoneornil(L, 2), 2, "none or nil not accepted");
	// Clear stack from trash arguments
	lua_settop(L, 2);
	// Count matches
	size_t count = 0;
	const size_t n = lua_rawlen(L, 1);
	if(lua_isfunction(L, 2))
		for(size_t i = 1; i <= n; ++i){
			lua_pushvalue(L, 2);
			lua_rawgeti(L, 1, i);
			lua_call(L, 1, 1);
			if(lua_toboolean(L, -1))
				++count;
			lua_pop(L, 1);
		}
	else
		for(size_t i = 1; i <= n; ++i){
			lua_rawgeti(L, 1, i);
			if(lua_equal(L, 2, -1))
				++count;
			lua_pop(L, 1);
		}
	lua_pushinteger(L, count);
	return 1;
}

static int table_allocate(lua_State* L){
	lua_createtable(L, luaL_checkinteger(L, 1), luaL_checkinteger(L, 2));
	return 1;
}

static int table_fill(lua_State* L){
	// Get/check argument
	luaL_checktype(L, 1, LUA_TTABLE);
	luaL_argcheck(L, !lua_isnoneornil(L, 2), 2, "none or nil not accepted");
	int n = luaL_checkinteger(L, 3),
		i = luaL_optinteger(L, 4, 1);
        // Fill table range with value
	if(i >= 1 && n > 0)
                for(n += i; i < n; ++i){
			lua_pushvalue(L, 2);
			lua_rawseti(L, 1, i);
                }
	return 0;
}

int luaopen_tablex(lua_State* L){
	static const luaL_Reg l[] = {
		{"copy", table_copy},
		{"tostring", table_tostring},
		{"allof", table_all_of},
		{"compare", table_compare},
		{"count", table_count},
		{"allocate", table_allocate},
		{"fill", table_fill},
		{NULL, NULL}
	};
	lua_getglobal(L, "table");
	if(lua_istable(L, -1)){
		luaL_setfuncs(L, l, 0);
		lua_pop(L, 1);
	}else{
		lua_pop(L, 1);
		luaL_newlib(L, l);
		lua_setglobal(L, "table");
	}
	return 0;
}
