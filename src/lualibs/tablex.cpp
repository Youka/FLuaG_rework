/*
Project: FLuaG
File: tablex.cpp

Copyright (c) 2015-2016, Christoph "Youka" Spanknebel

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

static int table_copy(lua_State* L) noexcept{
	// Check main argument
	luaL_checktype(L, 1, LUA_TTABLE);
	// Get optional argument
	const int depth = luaL_optinteger(L, 2, 0);
	// Remove non-main (=unnecessary) arguments
	lua_settop(L, 1);
	// Copy recursive & generate new table
	std::function<void(const int)> copy;
	copy = [&copy,L](const int depth){
		// Create output table
		lua_createtable(L, lua_rawlen(L, -1), 0);
		// Iterate through input table elements
		lua_pushnil(L);
		while(lua_next(L, -3)){
			// Duplicate key for next iteration pass
			lua_pushvalue(L, -2);
			lua_insert(L, -2);
			// Set key+value to output table
			if(depth > 0 && lua_istable(L, -1))
				copy(depth-1);
			lua_rawset(L, -4);
		}
		// Finally, set input metatable to output
		if(lua_getmetatable(L, -2))
			lua_setmetatable(L, -2);
		// Remove input table
		lua_remove(L, -2);
	};
	copy(depth);
	// Return table result of copy
	return 1;
}

static int table_tostring(lua_State* L) noexcept{
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

static int table_allocate(lua_State* L) noexcept{
	lua_createtable(L, luaL_checkinteger(L, 1), luaL_checkinteger(L, 2));
	return 1;
}

static int table_compare(lua_State* L) noexcept{
	// Check arguments
	luaL_checktype(L, 1, LUA_TTABLE);
	luaL_checktype(L, 2, LUA_TTABLE);
	luaL_argcheck(L, lua_isnoneornil(L, 3) || lua_isfunction(L, 3), 3, "optional function expected");
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

static int table_count(lua_State* L) noexcept{
	// Check arguments
	luaL_checktype(L, 1, LUA_TTABLE);
	luaL_argcheck(L, !lua_isnoneornil(L, 2), 2, "none or nil not accepted");
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

static int table_find(lua_State* L) noexcept{
	// Get/check argument
	luaL_checktype(L, 1, LUA_TTABLE);
	luaL_argcheck(L, !lua_isnoneornil(L, 2), 2, "none or nil not accepted");
	int i = luaL_optinteger(L, 3, 1);
        // Find table entry
	if(i >= 1){
		if(lua_isfunction(L, 2))
			for(const int n = lua_rawlen(L, 1); i <= n; ++i){
				lua_pushvalue(L, 2);
				lua_rawgeti(L, 1, i);
				lua_call(L, 1, 1);
				if(lua_toboolean(L, -1)){
					lua_pushinteger(L, i);
					return 1;
				}
				lua_pop(L, 1);
			}
		else
			for(const int n = lua_rawlen(L, 1); i <= n; ++i){
				lua_rawgeti(L, 1, i);
				if(lua_equal(L, 2, -1)){
					lua_pushinteger(L, i);
					return 1;
				}
				lua_pop(L, 1);
			}
	}
	return 0;
}

static int table_fill(lua_State* L) noexcept{
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

static int table_reverse(lua_State* L) noexcept{
	luaL_checktype(L, 1, LUA_TTABLE);
	const size_t n = lua_rawlen(L, 1),
		n2 = n >> 1;
	for(size_t i = 1; i <= n2; ++i){
		lua_rawgeti(L, 1, i); lua_rawgeti(L, 1, n-i+1);
		lua_rawseti(L, 1, i); lua_rawseti(L, 1, n-i+1);
	}
	return 0;
}

static int table_rotate(lua_State* L) noexcept{
	luaL_checktype(L, 1, LUA_TTABLE);
        int axis = luaL_checkinteger(L, 2);
	const size_t n = lua_rawlen(L, 1);
	if(axis > 1 && static_cast<size_t>(axis) <= n)
		for(size_t first = 1, next = axis; first != next;){
			lua_rawgeti(L, 1, first); lua_rawgeti(L, 1, next);
			lua_rawseti(L, 1, first++); lua_rawseti(L, 1, next++);
			if(next > n)
				next = axis;
			else if(first == static_cast<size_t>(axis))
				axis = next;
		}
	return 0;
}

static int table_accumulate(lua_State* L) noexcept{
	// Check arguments
	luaL_checktype(L, 1, LUA_TTABLE);
	luaL_argcheck(L, !lua_isnoneornil(L, 2), 2, "none or nil not accepted");
	luaL_checktype(L, 3, LUA_TFUNCTION);
	// Accumulate values
	const size_t n = lua_rawlen(L, 1);
	for(size_t i = 1; i <= n; ++i){
		lua_pushvalue(L, 3);
		lua_pushvalue(L, 2);
		lua_rawgeti(L, 1, i);
		lua_call(L, 2, 1);
		lua_replace(L, 2);
	}
	lua_pop(L, 1);
	return 1;
}

static int table_insertn(lua_State* L) noexcept{
	// Check arguments
	luaL_checktype(L, 1, LUA_TTABLE);
	const int i = luaL_checkinteger(L, 2);
	// Insert values
	const int top = lua_gettop(L);
	if(i >= 1 && top > 2){
		const size_t inserts = top - 2;
		for(size_t j = lua_rawlen(L, 1); j >= static_cast<size_t>(i); --j){
			lua_rawgeti(L, 1, j);
			lua_rawseti(L, 1, j+inserts);
		}
		for(size_t j = i+inserts-1; j >= static_cast<size_t>(i); --j)
			lua_rawseti(L, 1, j);
	}
	return 0;
}

static int table_removen(lua_State* L) noexcept{
	// Check arguments
	luaL_checktype(L, 1, LUA_TTABLE);
	int i = luaL_checkinteger(L, 2);
	const int n = luaL_optinteger(L, 3, 1);
	// Remove values
	if(i >= 1 && n > 0){
		const size_t tn = lua_rawlen(L, 1);
		for(size_t j = i+n; j <= tn; ++j){
			lua_rawgeti(L, 1, j);
			lua_rawseti(L, 1, i++);
		}
		while(static_cast<size_t>(i) <= tn){
			lua_pushnil(L);
			lua_rawseti(L, 1, i++);
		}
	}
	return 0;
}

static int table_move(lua_State* L) noexcept{
	// Check arguments
	luaL_checktype(L, 1, LUA_TTABLE);
	int f = luaL_checkinteger(L, 2);
	const int e = luaL_checkinteger(L, 3),
		t = luaL_checkinteger(L, 4);
	luaL_argcheck(L, lua_isnoneornil(L, 5) || lua_istable(L, 5), 5, "optional table expected");
	// Remove trash arguments and fix stack size
	lua_settop(L, 5);
	// Unpack values
	if(f == e)
		lua_rawgeti(L, 1, f);
	else if(f < e)
		while(f <= e)
			lua_rawgeti(L, 1, f++);
	else // f > e
		while(f >= e)
			lua_rawgeti(L, 1, f--);
	// Pack values
	for(int u = t+lua_gettop(L)-6, target = lua_istable(L, 5) ? 5 : 1; u >= t; --u)
		lua_rawseti(L, target, u);
	return 0;
}

int luaopen_tablex(lua_State* L)/* No exception specifier because of C declaration */{
	static const luaL_Reg l[] = {
		{"copy", table_copy},
		{"tostring", table_tostring},
		{"alloc", table_allocate},
		{"compare", table_compare},
		{"count", table_count},
		{"find", table_find},
		{"fill", table_fill},
		{"reverse", table_reverse},
		{"rotate", table_rotate},
		{"accumulate", table_accumulate},
		{"insertn", table_insertn},
		{"removen", table_removen},
		{"move", table_move},
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
