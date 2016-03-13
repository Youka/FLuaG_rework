/*
Project: FLuaG
File: utf8x.cpp

Copyright (c) 2015-2016, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "libs.h"
#include "../utils/lua.h"

// Helpers
static unsigned charsize(const unsigned char c){
	if(c < 128)
		return 1;
	else if(c < 224)
		return 2;
	else if(c < 240)
		return 3;
	return 4;
}

static bool checkchar(const unsigned char* c){
	const unsigned cn = charsize(*c);
	return *c != 0x0 && (*c <= 0x7f || (*c >= 0xc2 && *c <= 0xf4)) &&
		(cn < 2 || (c[1] >= 0x80 && c[1] <= 0xbf)) &&
		(cn < 3 || (c[2] >= 0x80 && c[2] <= 0xbf)) &&
		(cn < 4 || (c[3] >= 0x80 && c[3] <= 0xbf));
}

// General functions
static int utf8_charrange(lua_State* L){
	// Get arguments
	size_t s_len;
	const char* s = luaL_checklstring(L, 1, &s_len);
	int i = luaL_optinteger(L, 2, 1);
	// Fix position
	if(i < 0) i += s_len + 1;
	// Get charrange
	if(i >= 1 && i <= static_cast<decltype(i)>(s_len)){
		lua_pushinteger(L, charsize(s[i-1]));
		return 1;
	}
	return 0;
}

static int utf8_chars(lua_State* L){
	luaL_checktype(L, 1, LUA_TSTRING);
	lua_pushinteger(L, 0);
	lua_pushcclosure(L, [](lua_State* L){
		const int i = lua_tointeger(L, lua_upvalueindex(2));
		const char* s = lua_tostring(L, lua_upvalueindex(1)) + i;
		if(*s == '\0')
			return 0;
		if(!checkchar(reinterpret_cast<const unsigned char*>(s)))
			return luaL_error(L, "Invalid byte sequence found!");
		const unsigned cn = charsize(*s);
		lua_pushlstring(L, s, cn);
		lua_pushinteger(L, 1+i);
		lua_pushinteger(L, i+cn); lua_replace(L, lua_upvalueindex(2));
		return 2;
	}, 2);
	return 1;
}

static int utf8_len(lua_State* L){
	unsigned n = 0;
	for(const char* s = luaL_checkstring(L, 1); *s != '\0'; s += charsize(*s), ++n)
		if(!checkchar(reinterpret_cast<const unsigned char*>(s)))
			return luaL_error(L, "Invalid byte sequence found!");
	lua_pushinteger(L, n);
	return 1;
}

int luaopen_utf8x(lua_State* L){
	static const luaL_Reg l[] = {
		{"charrange", utf8_charrange},
		{"chars", utf8_chars},
		{"len", utf8_len},
		{NULL, NULL}
	};
	lua_getglobal(L, "utf8");
	if(lua_istable(L, -1)){
		luaL_setfuncs(L, l, 0);
		lua_pushstring(L, "[\\0-\\x7F\\xC2-\\xF4][\\x80-\\xBF]*"); lua_setfield(L, -2, "charpattern");
		lua_pop(L, 1);
	}else{
		lua_pop(L, 1);
		luaL_newlib(L, l);
		lua_pushstring(L, "[\\0-\\x7F\\xC2-\\xF4][\\x80-\\xBF]*"); lua_setfield(L, -2, "charpattern");
		lua_setglobal(L, "utf8");
	}
	return 0;
}
