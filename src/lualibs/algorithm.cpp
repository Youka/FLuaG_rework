/*
Project: FLuaG
File: algorithm.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "libs.h"
#include "../utils/lua.h"
#include <vector>
#include <array>
#include <algorithm>
#include <functional>

static int algorithm_ear_clipping(lua_State* L){
	// Check argument
	luaL_checktype(L, 1, LUA_TTABLE);
	// Get argument (table) as 2d points
	struct Point2d{double x, y;};
	std::vector<Point2d> points(lua_rawlen(L, 1) >> 1);
	size_t i = 0;
	for(auto& point : points){
		lua_rawgeti(L, 1, ++i);
		lua_rawgeti(L, 1, ++i);
                point = {luaL_checknumber(L, -2), luaL_checknumber(L, -1)};
		lua_pop(L, 2);
	}
	// Buffers for output & calculations
	std::vector<std::array<Point2d,3>> triangles; triangles.reserve(points.size()-2);
	std::vector<char> directions; directions.reserve(points.size()-2);
	std::vector<Point2d> new_points; new_points.reserve(points.size()-1);
	// Helper functions for calculations
	auto normal_z = [](Point2d prev, Point2d cur, Point2d next){return (cur.x - prev.x) * (next.y - cur.y) - (cur.y - prev.y) * (next.x - cur.x);};
	auto sign = [](const double x){return x < 0 ? -1 : (x > 0 ? 1 : 0);};
	auto in_triangle = [&normal_z,&sign](Point2d p, Point2d t1, Point2d t2, Point2d t3){
		const char t2t3p = sign(normal_z(t2, t3, p));
		return sign(normal_z(t1, t2, p)) == t2t3p && t2t3p == sign(normal_z(t3, t1, p));
	};
	// Evaluate triangles from points
	while(points.size() > 2){
		// Collect angles of point-to-neighbours vectors (exclude first & last point)
		directions.clear();
		double sum_directions = 0;
		for(i = 2; i < points.size(); ++i){
			const double z = normal_z(points[i-2], points[i-1], points[i]);
			directions.push_back(sign(z));
			sum_directions += z;
		}
		// Polygon is convex?
		const char all_direction = sign(sum_directions);
		if(std::all_of(directions.cbegin(), directions.cend(), std::bind(std::equal_to<const char>(), all_direction, std::placeholders::_1))){
			// Just split points into triangles
			for(i = 2; i < points.size(); ++i)
				triangles.push_back({points.front(), points[i-1], points[i]});
			break;	// Finish / just 2 points left
		}else{
			// Pick ears/edge triangles from points
			new_points = {points.front()};
			for(size_t first = 0, next = 1, next_end = points.size()-1; next != next_end; ++next){
				const Point2d& t1 = points[first], t2 = points[next], t3 = points[next+1];
				const char& direction = directions[next-1];	// Remember: directions doesn't include the first point!!!
				// Point is ear without intersection
				if(direction == all_direction && std::none_of(points.cbegin(), points.cend(), std::bind(in_triangle, std::placeholders::_1, t1, t2, t3)))
					triangles.push_back({t1, t2, t3});
				// Point is valley or ear with intersection
				else if(direction != 0){
					new_points.push_back(t2);
					first = next;
				}
				// Point is on vector...
			}
			new_points.push_back(points.back());
			points = new_points;
		}
	}
	// Send triangles to Lua
	lua_createtable(L, triangles.size(), 0);
	i = 0;
	for(const auto& triangle : triangles){
		lua_createtable(L, 6, 0);
		lua_pushnumber(L, triangle[0].x);
		lua_pushnumber(L, triangle[0].y);
		lua_pushnumber(L, triangle[1].x);
		lua_pushnumber(L, triangle[1].y);
		lua_pushnumber(L, triangle[2].x);
		lua_pushnumber(L, triangle[2].y);
		lua_rawseti(L, -7, 6);
		lua_rawseti(L, -6, 5);
		lua_rawseti(L, -5, 4);
		lua_rawseti(L, -4, 3);
		lua_rawseti(L, -3, 2);
		lua_rawseti(L, -2, 1);
		lua_rawseti(L, -2, ++i);
	}
	return 1;
}

int luaopen_algorithm(lua_State* L){
	lua_createtable(L, 0, 1);
	lua_pushcfunction(L, algorithm_ear_clipping); lua_setfield(L, -2, "earclipping");

	// TODO

	return 1;
}
