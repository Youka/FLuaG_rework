/*
Project: FLuaG
File: geometry.cpp

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

static int geometry_stretch(lua_State* L){
	const Geometry::Point2d v = Geometry::stretch({luaL_checknumber(L, 1), luaL_checknumber(L,2)}, luaL_checknumber(L, 3));
	lua_pushnumber(L, v.x);
	lua_pushnumber(L, v.y);
	return 2;
}

static int geometry_line_intersect(lua_State* L){
	try{
		const Geometry::Point2d p = Geometry::line_intersect({luaL_checknumber(L, 1), luaL_checknumber(L, 2)}, {luaL_checknumber(L, 3), luaL_checknumber(L, 4)}, {luaL_checknumber(L, 5), luaL_checknumber(L, 6)}, {luaL_checknumber(L, 7), luaL_checknumber(L, 8)});
		lua_pushnumber(L, p.x);
		lua_pushnumber(L, p.y);
		return 2;
	}catch(const std::out_of_range&){
	}catch(const std::length_error&){
	}
	return 0;
}

static int geometry_on_line(lua_State* L){
	lua_pushboolean(L, Geometry::on_line({luaL_checknumber(L, 1), luaL_checknumber(L, 2)}, {luaL_checknumber(L, 3), luaL_checknumber(L, 4)}, {luaL_checknumber(L, 5), luaL_checknumber(L, 6)}));
	return 1;
}

static int geometry_in_triangle(lua_State* L){
	lua_pushboolean(L, Geometry::in_triangle({luaL_checknumber(L, 1), luaL_checknumber(L, 2)}, {luaL_checknumber(L, 3), luaL_checknumber(L, 4)}, {luaL_checknumber(L, 5), luaL_checknumber(L, 6)}, {luaL_checknumber(L, 7), luaL_checknumber(L, 8)}));
	return 1;
}

static int geometry_direction(lua_State* L){
	lua_pushinteger(L, Math::sign(lua_gettop(L) > 4 ? Geometry::normal_z({luaL_checknumber(L, 1), luaL_checknumber(L, 2)}, {luaL_checknumber(L, 3), luaL_checknumber(L, 4)}, {luaL_checknumber(L, 5), luaL_checknumber(L, 6)}) : Geometry::normal_z({luaL_checknumber(L, 1), luaL_checknumber(L, 2)}, {luaL_checknumber(L, 3), luaL_checknumber(L, 4)})));
	return 1;
}

static int geometry_bound(lua_State* L){
	luaL_checktype(L, 1, LUA_TTABLE);
	const size_t n = lua_rawlen(L, 1) & ~0x1;	// Size as multiple of 2
	if(n != 0){
		lua_rawgeti(L, 1, 1); lua_rawgeti(L, 1, 2);
		double min_x = luaL_checknumber(L, -2),
			min_y = luaL_checknumber(L, -1),
			max_x = min_x,
			max_y = min_y;
		lua_pop(L, 2);
		for(size_t i = 3; i <= n; i += 2){
			lua_rawgeti(L, 1, i); lua_rawgeti(L, 1, i+1);
			const double x = luaL_checknumber(L, -2),
				y = luaL_checknumber(L, -1);
			if(x < min_x) min_x = x;
			else if(x > max_x) max_x = x;
			if(y < min_y) min_y = y;
			else if(y > max_y) max_y = y;
			lua_pop(L, 2);
		}
		lua_pushnumber(L, min_x);
		lua_pushnumber(L, min_y);
		lua_pushnumber(L, max_x);
		lua_pushnumber(L, max_y);
		return 4;
	}
	return 0;
}

static int geometry_arc_curve(lua_State* L){
	const auto curves = Geometry::arc_to_curves({luaL_checknumber(L, 1), luaL_checknumber(L, 2)}, {luaL_checknumber(L, 3), luaL_checknumber(L, 4)}, luaL_checknumber(L, 5));
	lua_createtable(L, curves.size() << 3, 0);
	int i = 0;
	for(const auto& curve : curves){
		lua_pushnumber(L, curve[3].y); lua_pushnumber(L, curve[3].x); lua_pushnumber(L, curve[2].y); lua_pushnumber(L, curve[2].x); lua_pushnumber(L, curve[1].y); lua_pushnumber(L, curve[1].x); lua_pushnumber(L, curve[0].y); lua_pushnumber(L, curve[0].x);
		lua_rawseti(L, -9, ++i); lua_rawseti(L, -8, ++i); lua_rawseti(L, -7, ++i); lua_rawseti(L, -6, ++i); lua_rawseti(L, -5, ++i); lua_rawseti(L, -4, ++i); lua_rawseti(L, -3, ++i); lua_rawseti(L, -2, ++i);
	}
	return 1;
}

static int geometry_ear_clipping(lua_State* L){
	// Check argument
	luaL_checktype(L, 1, LUA_TTABLE);
	// Get argument (table) as 2d points
	std::vector<Geometry::Point2d> points(lua_rawlen(L, 1) >> 1);
	int i = 0;
	for(auto& point : points){
		lua_rawgeti(L, 1, ++i); lua_rawgeti(L, 1, ++i);
                point = {luaL_checknumber(L, -2), luaL_checknumber(L, -1)};
		lua_pop(L, 2);
	}
	// Convert polygon/points to triangles
	const auto triangles = Geometry::ear_clipping(points);
	// Send triangles to Lua
	lua_createtable(L, triangles.size(), 0);
	i = 0;
	for(const auto& triangle : triangles){
		lua_createtable(L, 6, 0);
		lua_pushnumber(L, triangle[0].x); lua_pushnumber(L, triangle[0].y); lua_pushnumber(L, triangle[1].x); lua_pushnumber(L, triangle[1].y); lua_pushnumber(L, triangle[2].x); lua_pushnumber(L, triangle[2].y);
		lua_rawseti(L, -7, 6); lua_rawseti(L, -6, 5); lua_rawseti(L, -5, 4); lua_rawseti(L, -4, 3); lua_rawseti(L, -3, 2); lua_rawseti(L, -2, 1);
		lua_rawseti(L, -2, ++i);
	}
	return 1;
}

static int geometry_curve_flatten(lua_State* L){
	try{
		const auto lines = Geometry::curve_flatten({{
				{luaL_checknumber(L, 1), luaL_checknumber(L, 2)},
				{luaL_checknumber(L, 3), luaL_checknumber(L, 4)},
				{luaL_checknumber(L, 5), luaL_checknumber(L, 6)},
				{luaL_checknumber(L, 7), luaL_checknumber(L, 8)}
			}},luaL_optnumber(L, 9, 0.1));
		lua_createtable(L, lines.size() << 1, 0);
		int i = 0;
		for(const auto& point : lines){
			lua_pushnumber(L, point.y); lua_pushnumber(L, point.x);
			lua_rawseti(L, -3, ++i); lua_rawseti(L, -2, ++i);
		}
		return 1;
	}catch(const std::out_of_range& e){
		return luaL_error(L, e.what());
	}
	return 0;
}

#define LUA_MATRIX "matrix"
static int geometry_matrix_create(lua_State* L){

	// TODO

	return 0;
}

int luaopen_geometry(lua_State* L){
	static const luaL_Reg l[] = {
		{"stretch", geometry_stretch},
		{"lineintersect", geometry_line_intersect},
		{"online", geometry_on_line},
		{"intriangle", geometry_in_triangle},
		{"direction", geometry_direction},
		{"bound", geometry_bound},
		{"arccurve", geometry_arc_curve},
		{"earclipping", geometry_ear_clipping},
		{"curveflatten", geometry_curve_flatten},
		{"matrix", geometry_matrix_create},
		{NULL, NULL}
	};
	luaL_newlib(L, l);
	return 1;
}
