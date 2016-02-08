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
#include <complex>
#include <algorithm>

static int math_trunc(lua_State* L){
	lua_pushinteger(L, std::trunc(luaL_checknumber(L, 1)));
	return 1;
}

static int math_round(lua_State* L){
	lua_pushinteger(L, std::round(luaL_checknumber(L, 1)));
	return 1;
}

static int math_trim(lua_State* L){
	const lua_Number x = luaL_checknumber(L, 1);
	lua_Number a = luaL_checknumber(L, 2),
		b = luaL_checknumber(L, 3);
	if(a > b) std::swap(a, b);
	lua_pushnumber(L, x < a ? a : (x > b ? b : x));
	return 1;
}

static int math_sign(lua_State* L){
	lua_pushinteger(L, Math::sign(luaL_checknumber(L, 1)));
	return 1;
}

static int math_hypot(lua_State* L){
	lua_pushnumber(L, std::hypot(luaL_checknumber(L, 1), luaL_checknumber(L, 2)));
	return 1;
}

static int math_asinh(lua_State* L){
	lua_pushnumber(L, std::asinh(luaL_checknumber(L, 1)));
	return 1;
}

static int math_acosh(lua_State* L){
	lua_pushnumber(L, std::acosh(luaL_checknumber(L, 1)));
	return 1;
}

static int math_atanh(lua_State* L){
	lua_pushnumber(L, std::atanh(luaL_checknumber(L, 1)));
	return 1;
}

static int math_erf(lua_State* L){
	lua_pushnumber(L, std::erf(luaL_checknumber(L, 1)));
	return 1;
}

static int math_tgamma(lua_State* L){
	lua_pushnumber(L, std::tgamma(luaL_checknumber(L, 1)));
	return 1;
}

static int math_frexp(lua_State* L){
	int exp;
	lua_pushnumber(L, std::frexp(luaL_checknumber(L, 1), &exp));
	lua_pushinteger(L, exp);
	return 2;
}

static int math_classify(lua_State* L){
	switch(std::fpclassify(luaL_checknumber(L, 1))){
		case FP_INFINITE: lua_pushstring(L, "inf"); break;
		case FP_NAN: lua_pushstring(L, "nan"); break;
		case FP_NORMAL: lua_pushstring(L, "normal"); break;
		case FP_SUBNORMAL: lua_pushstring(L, "subnormal"); break;
		case FP_ZERO: lua_pushstring(L, "zero"); break;
		default: lua_pushstring(L, "unknown"); break;
	}
	return 1;
}

#define LUA_COMPLEX "complex"
using complex = std::complex<double>;
static int math_complex_create(lua_State* L);

static int math_complex_free(lua_State* L){
	delete *static_cast<complex**>(luaL_checkudata(L, 1, LUA_COMPLEX));
	return 0;
}

static int math_complex_access(lua_State* L){
	complex* x = *static_cast<complex**>(luaL_checkudata(L, 1, LUA_COMPLEX));
	if(lua_isnumber(L, 2) || lua_isnumber(L, 3)){
		if(lua_isnumber(L, 2))
			x->real(lua_tonumber(L, 2));
		if(lua_isnumber(L, 3))
			x->imag(lua_tonumber(L, 3));
	}else{
		lua_pushnumber(L, x->real());
		lua_pushnumber(L, x->imag());
		return 2;
	}
	return 0;
}

static int math_complex_tostring(lua_State* L){
	const complex* x = *static_cast<complex**>(luaL_checkudata(L, 1, LUA_COMPLEX));
	lua_pushfstring(L, "(%f,%f)", x->real(), x->imag());
	return 1;
}

static int math_complex_add(lua_State* L){
	const complex x = **static_cast<complex**>(luaL_checkudata(L, 1, LUA_COMPLEX)) + **static_cast<complex**>(luaL_checkudata(L, 2, LUA_COMPLEX));
	lua_settop(L, 0);
	lua_pushnumber(L, x.real());
	lua_pushnumber(L, x.imag());
	return math_complex_create(L);
}

static int math_complex_sub(lua_State* L){
	const complex x = **static_cast<complex**>(luaL_checkudata(L, 1, LUA_COMPLEX)) - **static_cast<complex**>(luaL_checkudata(L, 2, LUA_COMPLEX));
	lua_settop(L, 0);
	lua_pushnumber(L, x.real());
	lua_pushnumber(L, x.imag());
	return math_complex_create(L);
}

static int math_complex_mul(lua_State* L){
	const complex x = **static_cast<complex**>(luaL_checkudata(L, 1, LUA_COMPLEX)) * **static_cast<complex**>(luaL_checkudata(L, 2, LUA_COMPLEX));
	lua_settop(L, 0);
	lua_pushnumber(L, x.real());
	lua_pushnumber(L, x.imag());
	return math_complex_create(L);
}

static int math_complex_div(lua_State* L){
	const complex x = **static_cast<complex**>(luaL_checkudata(L, 1, LUA_COMPLEX)) / **static_cast<complex**>(luaL_checkudata(L, 2, LUA_COMPLEX));
	lua_settop(L, 0);
	lua_pushnumber(L, x.real());
	lua_pushnumber(L, x.imag());
	return math_complex_create(L);
}

static int math_complex_equal(lua_State* L){
	lua_pushboolean(L, **static_cast<complex**>(luaL_checkudata(L, 1, LUA_COMPLEX)) == **static_cast<complex**>(luaL_checkudata(L, 2, LUA_COMPLEX)));
	return 1;
}

int math_complex_create(lua_State* L){	// Declared as static previously
	const lua_Number real = luaL_optnumber(L, 1, 0),
		imag = luaL_optnumber(L, 2, 0);
	*static_cast<complex**>(lua_newuserdata(L, sizeof(complex*))) = new complex(real, imag);
	if(luaL_newmetatable(L, LUA_COMPLEX)){
		static const luaL_Reg l[] = {
			{"__gc", math_complex_free},
			{"__call", math_complex_access},
			{"__tostring", math_complex_tostring},
			{"__add", math_complex_add},
			{"__sub", math_complex_sub},
			{"__mul", math_complex_mul},
			{"__div", math_complex_div},
			{"__eq", math_complex_equal},

			// TODO

			{NULL, NULL}
		};
		luaL_setfuncs(L, l, 0);
		lua_pushvalue(L, -1); lua_setfield(L, -2, "__index");
	}
	lua_setmetatable(L, -2);
	return 1;
}

int luaopen_mathx(lua_State* L){
	static const luaL_Reg l[] = {
		{"trunc", math_trunc},
		{"round", math_round},
		{"trim", math_trim},
		{"sign", math_sign},
		{"hypot", math_hypot},
		{"asinh", math_asinh},
		{"acosh", math_acosh},
		{"atanh", math_atanh},
		{"erf", math_erf},
		{"tgamma", math_tgamma},
		{"frexp", math_frexp},
		{"classify", math_classify},
		{"complex", math_complex_create},
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
