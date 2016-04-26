/*
Project: FLuaG
File: lua.h

Copyright (c) 2015-2016, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#pragma once

#if LUA_VERSION_NUM <= 501
	#define lua_rawlen lua_objlen
	#define luaL_newlibtable(L, l) lua_createtable(L, 0, sizeof(l)/sizeof(l[0])-1)
	inline void luaL_setfuncs(lua_State* L, const luaL_Reg* l, int nup) noexcept{
		if(nup == 0)
			luaL_register(L, NULL, l);
		else{
			for(; l->name != NULL; ++l){
				for(int n = lua_gettop(L), i = n - nup + 1; i <= n; lua_pushvalue(L, i++));
				lua_pushcclosure(L, l->func, nup); lua_setfield(L, -2, l->name);
			}
			lua_pop(L, nup);
		}
	}
	#define luaL_newlib(L, l) (luaL_newlibtable(L,l), luaL_setfuncs(L,l,0))
#else
	#define lua_equal(L, i1, i2) lua_compare(L, i1, i2, LUA_OPEQ)
#endif

#define luaL_checkboolean(L, arg) (luaL_checktype(L, arg, LUA_TBOOLEAN), lua_toboolean(L, arg))
#define luaL_optboolean(L, arg, d) (lua_isnoneornil(L, arg) ? d : luaL_checkboolean(L, arg))
