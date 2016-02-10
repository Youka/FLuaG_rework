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
static void lua_pushcomplex(lua_State* L, const complex& c);

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
	lua_pushcomplex(L, **static_cast<complex**>(luaL_checkudata(L, 1, LUA_COMPLEX)) + **static_cast<complex**>(luaL_checkudata(L, 2, LUA_COMPLEX)));
	return 1;
}

static int math_complex_sub(lua_State* L){
	lua_pushcomplex(L, **static_cast<complex**>(luaL_checkudata(L, 1, LUA_COMPLEX)) - **static_cast<complex**>(luaL_checkudata(L, 2, LUA_COMPLEX)));
	return 1;
}

static int math_complex_mul(lua_State* L){
	lua_pushcomplex(L, **static_cast<complex**>(luaL_checkudata(L, 1, LUA_COMPLEX)) * **static_cast<complex**>(luaL_checkudata(L, 2, LUA_COMPLEX)));
	return 1;
}

static int math_complex_div(lua_State* L){
	lua_pushcomplex(L, **static_cast<complex**>(luaL_checkudata(L, 1, LUA_COMPLEX)) / **static_cast<complex**>(luaL_checkudata(L, 2, LUA_COMPLEX)));
	return 1;
}

static int math_complex_equal(lua_State* L){
	lua_pushboolean(L, **static_cast<complex**>(luaL_checkudata(L, 1, LUA_COMPLEX)) == **static_cast<complex**>(luaL_checkudata(L, 2, LUA_COMPLEX)));
	return 1;
}

static int math_complex_pow(lua_State* L){
	lua_pushcomplex(L, std::pow(**static_cast<complex**>(luaL_checkudata(L, 1, LUA_COMPLEX)), **static_cast<complex**>(luaL_checkudata(L, 2, LUA_COMPLEX))));
	return 1;
}

static int math_complex_abs(lua_State* L){
	lua_pushnumber(L, std::abs(**static_cast<complex**>(luaL_checkudata(L, 1, LUA_COMPLEX))));
	return 1;
}

static int math_complex_arg(lua_State* L){
	lua_pushnumber(L, std::arg(**static_cast<complex**>(luaL_checkudata(L, 1, LUA_COMPLEX))));
	return 1;
}

static int math_complex_norm(lua_State* L){
	lua_pushnumber(L, std::norm(**static_cast<complex**>(luaL_checkudata(L, 1, LUA_COMPLEX))));
	return 1;
}

static int math_complex_conj(lua_State* L){
	lua_pushcomplex(L, std::conj(**static_cast<complex**>(luaL_checkudata(L, 1, LUA_COMPLEX))));
	return 1;
}

static int math_complex_exp(lua_State* L){
	lua_pushcomplex(L, std::exp(**static_cast<complex**>(luaL_checkudata(L, 1, LUA_COMPLEX))));
	return 1;
}

static int math_complex_log(lua_State* L){
	lua_pushcomplex(L, std::log(**static_cast<complex**>(luaL_checkudata(L, 1, LUA_COMPLEX))));
	return 1;
}

static int math_complex_sqrt(lua_State* L){
	lua_pushcomplex(L, std::sqrt(**static_cast<complex**>(luaL_checkudata(L, 1, LUA_COMPLEX))));
	return 1;
}

static int math_complex_sin(lua_State* L){
	lua_pushcomplex(L, std::sin(**static_cast<complex**>(luaL_checkudata(L, 1, LUA_COMPLEX))));
	return 1;
}

static int math_complex_cos(lua_State* L){
	lua_pushcomplex(L, std::cos(**static_cast<complex**>(luaL_checkudata(L, 1, LUA_COMPLEX))));
	return 1;
}

static int math_complex_tan(lua_State* L){
	lua_pushcomplex(L, std::tan(**static_cast<complex**>(luaL_checkudata(L, 1, LUA_COMPLEX))));
	return 1;
}

static int math_complex_asin(lua_State* L){
	lua_pushcomplex(L, std::asin(**static_cast<complex**>(luaL_checkudata(L, 1, LUA_COMPLEX))));
	return 1;
}

static int math_complex_acos(lua_State* L){
	lua_pushcomplex(L, std::acos(**static_cast<complex**>(luaL_checkudata(L, 1, LUA_COMPLEX))));
	return 1;
}

static int math_complex_atan(lua_State* L){
	lua_pushcomplex(L, std::atan(**static_cast<complex**>(luaL_checkudata(L, 1, LUA_COMPLEX))));
	return 1;
}

static int math_complex_sinh(lua_State* L){
	lua_pushcomplex(L, std::sinh(**static_cast<complex**>(luaL_checkudata(L, 1, LUA_COMPLEX))));
	return 1;
}

static int math_complex_cosh(lua_State* L){
	lua_pushcomplex(L, std::cosh(**static_cast<complex**>(luaL_checkudata(L, 1, LUA_COMPLEX))));
	return 1;
}

static int math_complex_tanh(lua_State* L){
	lua_pushcomplex(L, std::tanh(**static_cast<complex**>(luaL_checkudata(L, 1, LUA_COMPLEX))));
	return 1;
}

static int math_complex_asinh(lua_State* L){
	lua_pushcomplex(L, std::asinh(**static_cast<complex**>(luaL_checkudata(L, 1, LUA_COMPLEX))));
	return 1;
}

static int math_complex_acosh(lua_State* L){
	lua_pushcomplex(L, std::acosh(**static_cast<complex**>(luaL_checkudata(L, 1, LUA_COMPLEX))));
	return 1;
}

static int math_complex_atanh(lua_State* L){
	lua_pushcomplex(L, std::atanh(**static_cast<complex**>(luaL_checkudata(L, 1, LUA_COMPLEX))));
	return 1;
}

void lua_pushcomplex(lua_State* L, const complex& c){ // static definition on declaration
	*static_cast<complex**>(lua_newuserdata(L, sizeof(complex*))) = new complex(c);
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
			{"__pow", math_complex_pow},
			{"abs", math_complex_abs},
			{"arg", math_complex_arg},
			{"norm", math_complex_norm},
			{"conj", math_complex_conj},
			{"exp", math_complex_exp},
			{"log", math_complex_log},
			{"sqrt", math_complex_sqrt},
			{"sin", math_complex_sin},
			{"cos", math_complex_cos},
			{"tan", math_complex_tan},
			{"asin", math_complex_asin},
			{"acos", math_complex_acos},
			{"atan", math_complex_atan},
			{"sinh", math_complex_sinh},
			{"cosh", math_complex_cosh},
			{"tanh", math_complex_tanh},
			{"asinh", math_complex_asinh},
			{"acosh", math_complex_acosh},
			{"atanh", math_complex_atanh},
			{NULL, NULL}
		};
		luaL_setfuncs(L, l, 0);
		lua_pushvalue(L, -1); lua_setfield(L, -2, "__index");
	}
	lua_setmetatable(L, -2);
}

static int math_complex_create(lua_State* L){
	lua_pushcomplex(L, {luaL_optnumber(L, 1, 0), luaL_optnumber(L, 2, 0)});
	return 1;
}

static int math_polar(lua_State* L){
	lua_pushcomplex(L, std::polar(luaL_checknumber(L, 1), luaL_optnumber(L, 2, 0)));
	return 1;
}

static int math_gauss(lua_State* L){
	const double x = luaL_checknumber(L, 1),
		sigma = luaL_checknumber(L, 2);
	constexpr static const double sqrtpi2 = std::sqrt(2 * M_PI);
	lua_pushnumber(L, 1 / (sigma * sqrtpi2) * std::exp(-0.5 * std::pow(x/sigma, 2)));
	return 1;
}

static int math_fac(lua_State* L){
	lua_pushinteger(L, Math::fac(luaL_checkinteger(L, 1)));
	return 1;
}

static int math_bezier(lua_State* L){
	const double pct = luaL_checknumber(L, 1),
		pct_inv = 1 - pct;
	const int top = lua_gettop(L);
	switch(top-1){
		case 0: lua_pushnil(L); break;
		case 1: luaL_checktype(L, 2, LUA_TNUMBER); break;
		case 2: lua_pushnumber(L, pct_inv * luaL_checknumber(L, 2) + pct * luaL_checknumber(L, 3)); break;
		case 3: lua_pushnumber(L, pct_inv * pct_inv * luaL_checknumber(L, 2) + pct_inv * pct * luaL_checknumber(L, 3) + pct * pct * luaL_checknumber(L, 4)); break;
		case 4: lua_pushnumber(L, pct_inv * pct_inv * pct_inv * luaL_checknumber(L, 2) + pct_inv * pct_inv * pct * luaL_checknumber(L, 3) + pct_inv * pct * pct * luaL_checknumber(L, 4) + pct * pct * pct * luaL_checknumber(L, 5)); break;
		default:{
				double result = 0;
				for(int i = 0, n = top-2; i <= n; ++i)
					result += luaL_checknumber(L, 2+i) * Math::bernstein(i, n, pct);
				lua_pushnumber(L, result);
			}break;
	}
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
		{"polar", math_polar},
		{"gauss", math_gauss},
		{"fac", math_fac},
		{"bezier", math_bezier},
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
