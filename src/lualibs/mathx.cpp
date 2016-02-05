/*
Project: FLuaG
File: mathx.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "libs.h"
#include "../utils/lua.h"
#include "../utils/math.hpp"
#include <cmath>

static int math_hypot(lua_State* L){
	lua_pushnumber(L, std::hypot(luaL_checknumber(L, 1), luaL_checknumber(L, 2)));
	return 1;
}

static int math_sign(lua_State* L){
	lua_pushinteger(L, Math::sign(luaL_checknumber(L, 1)));
	return 1;
}

int luaopen_mathx(lua_State* L){
	static const luaL_Reg l[] = {
		{"hypot", math_hypot},
		{"sign", math_sign},
		{NULL, NULL}
	};
	lua_getglobal(L, "math");
	if(lua_istable(L, -1)){
		luaL_setfuncs(L, l, 0);
		lua_pop(L, 1);
	}else{
		lua_pop(L, 1);
		luaL_newlib(L, l);
		lua_setglobal(L, "math");
	}
	return 0;
}
