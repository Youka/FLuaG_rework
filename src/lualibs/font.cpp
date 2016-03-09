/*
Project: FLuaG
File: font.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "libs.h"
#include "../utils/lua.h"
#include "../utils/font.hpp"

static int font_list(lua_State* L){
	try{
		const auto list = Font::list();
		lua_createtable(L, list.size(), 0);
		int i = 0;
		for(const auto& entry : list){
			lua_createtable(L, 0, 5);
			lua_pushstring(L, entry.family.c_str()); lua_setfield(L, -2, "family");
			lua_pushstring(L, entry.style.c_str()); lua_setfield(L, -2, "style");
			lua_pushstring(L, entry.file.c_str()); lua_setfield(L, -2, "file");
			lua_pushstring(L, entry.script.c_str()); lua_setfield(L, -2, "script");
			lua_pushboolean(L, entry.outline); lua_setfield(L, -2, "outline");
			lua_rawseti(L, -2, ++i);
		}
	}catch(const Font::exception& e){
		return luaL_error(L, e.what());
	}
	return 1;
}

int luaopen_font(lua_State* L){
	static const luaL_Reg l[] = {
		{"list", font_list},

		// TODO: font class

		{NULL, NULL}
	};
	luaL_newlib(L, l);
	return 1;
}
