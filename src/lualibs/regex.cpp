/*
Project: FLuaG
File: regex.cpp

Copyright (c) 2015-2016, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "libs.h"
#include "../utils/lua.h"
#include <boost/regex.hpp>

// Unique name for Lua metatable
#define LUA_REGEX "regex"

// Userdata container
struct RegexArgs{
	const boost::regex expr;
	const boost::regex_constants::match_flag_type flag;
};

// Regex metatable methods
static int regex_free(lua_State* L) noexcept{
	delete *static_cast<RegexArgs**>(luaL_checkudata(L, 1, LUA_REGEX));
	return 0;
}

static int regex_replace(lua_State* L) noexcept{
	const RegexArgs* args = *static_cast<RegexArgs**>(luaL_checkudata(L, 1, LUA_REGEX));
	try{
		lua_pushstring(L, boost::regex_replace(std::string(luaL_checkstring(L, 2)), args->expr, std::string(luaL_checkstring(L, 3)), args->flag).c_str());
	}catch(const boost::regex_error& e){
		return luaL_error(L, e.what());
	}
	return 1;
}

static int regex_match(lua_State* L) noexcept{
	const RegexArgs* args = *static_cast<RegexArgs**>(luaL_checkudata(L, 1, LUA_REGEX));
	const std::string str = luaL_checkstring(L, 2);
	boost::sregex_iterator it(str.cbegin(), str.cend(), args->expr, args->flag), it_end;
	lua_createtable(L, std::distance(it, it_end), 0);
	for(int i = 1; it != it_end; ++it, ++i){
		const boost::smatch& matches = *it;
		lua_createtable(L, matches.size(), 0);
		for(size_t sub_i = 1; sub_i <= matches.size(); ++sub_i){
			lua_createtable(L, 0, 2);
			lua_pushinteger(L, 1+matches.position(sub_i-1)); lua_setfield(L, -2, "position");
			lua_pushstring(L, matches.str(sub_i-1).c_str()); lua_setfield(L, -2, "string");
			lua_rawseti(L, -2, sub_i);
		}
		lua_rawseti(L, -2, i);
	}
	return 1;
}

// Main functions
static int regex_create(lua_State* L) noexcept{
	// Get main argument
	const char* expr = luaL_checkstring(L, 1);
	// Get optional arguments
	boost::regex_constants::syntax_option_type syntax = boost::regex_constants::ECMAScript;
	boost::regex_constants::match_flag_type flag = boost::regex_constants::match_default;
	if(lua_istable(L, 2)){
		const size_t n = lua_rawlen(L, 2);
		for(size_t i = 1; i <= n; ++i){
			lua_rawgeti(L, 2, i);
			if(lua_isstring(L, -1)){
				const std::string option = lua_tostring(L, -1);
				if(option == "icase")
					syntax |= boost::regex_constants::icase;
				else if(option == "nosubs")
					syntax |= boost::regex_constants::nosubs;
				else if(option == "optimize")
					syntax |= boost::regex_constants::optimize;
				else if(option == "collate")
					syntax |= boost::regex_constants::collate;
				else if(option == "noempty")
					flag |= boost::regex_constants::match_not_null;
			}
		}
		lua_pop(L, n);
	}
	// Create regex userdata
	try{
		boost::regex reg(expr, syntax);
		*static_cast<RegexArgs**>(lua_newuserdata(L, sizeof(RegexArgs*))) = new RegexArgs{std::move(reg), flag};
	}catch(const boost::regex_error& e){
		return luaL_error(L, e.what());
	}
	if(luaL_newmetatable(L, LUA_REGEX)){
		static const luaL_Reg l[] = {
			{"__gc", regex_free},
			{"replace", regex_replace},
			{"match", regex_match},
			{NULL, NULL}
		};
		luaL_setfuncs(L, l, 0);
		lua_pushvalue(L, -1); lua_setfield(L, -2, "__index");
	}
	lua_setmetatable(L, -2);
	return 1;
}

int luaopen_regex(lua_State* L)/* No exception specifier because of C declaration */{
	lua_pushcfunction(L, regex_create);
	return 1;
}
