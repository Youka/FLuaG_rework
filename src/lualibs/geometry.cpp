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
#include <vector>
#include <array>
#include <algorithm>
#include <functional>

// Helpers
struct Point2d{
	double x, y;
	Point2d operator+(const Point2d& other) const{return {this->x + other.x, this->y + other.y};};
	Point2d operator-(const Point2d& other) const{return {this->x - other.x, this->y - other.y};};
	Point2d operator*(const Point2d& other) const{return {this->x * other.x, this->y * other.y};};
	bool operator==(const Point2d& other) const{return this->x == other.x && this->y == other.y;};
	bool operator!=(const Point2d& other) const{return !(*this == other);};
};

static double normal_z(const Point2d& v1, const Point2d& v2){
	return v1.x * v2.y - v1.y * v2.x;
}
static double normal_z(const Point2d& p0, const Point2d& p1, const Point2d& p2){
	return normal_z(p1 - p0, p2 - p1);
}

static bool in_triangle(const Point2d& p, const Point2d& t1, const Point2d& t2, const Point2d& t3){
	const char t2t3p = Math::sign(normal_z(t2, t3, p));
	return Math::sign(normal_z(t1, t2, p)) == t2t3p && t2t3p == Math::sign(normal_z(t3, t1, p));
}

// General functions
static int geometry_ear_clipping(lua_State* L){
	// Check argument
	luaL_checktype(L, 1, LUA_TTABLE);
	// Get argument (table) as 2d points
	std::vector<Point2d> points(lua_rawlen(L, 1) >> 1);
	size_t i = 0;
	for(auto& point : points){
		lua_rawgeti(L, 1, ++i); lua_rawgeti(L, 1, ++i);
                point = {luaL_checknumber(L, -2), luaL_checknumber(L, -1)};
		lua_pop(L, 2);
	}
	// Buffers for output & calculations
	std::vector<std::array<Point2d,3>> triangles; triangles.reserve(points.size()-2);
	std::vector<char> directions; directions.reserve(points.size()-2);
	std::vector<Point2d> new_points; new_points.reserve(points.size()-1);
	// Evaluate triangles from points
	while(points.size() > 2){
		// Collect angles of point-to-neighbours vectors (exclude first & last point)
		directions.clear();
		double sum_directions = 0;
		for(i = 2; i < points.size(); ++i){
			const double z = normal_z(points[i-2], points[i-1], points[i]);
			directions.push_back(Math::sign(z));
			sum_directions += z;
		}
		// Polygon is convex?
		const char all_direction = Math::sign(sum_directions);
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
			points.swap(new_points);
		}
	}
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
	// Get arguments
	const std::array<Point2d,4> points{{
		{luaL_checknumber(L, 1), luaL_checknumber(L, 2)},
		{luaL_checknumber(L, 3), luaL_checknumber(L, 4)},
		{luaL_checknumber(L, 5), luaL_checknumber(L, 6)},
		{luaL_checknumber(L, 7), luaL_checknumber(L, 8)}
	}};
	const double tolerance = luaL_optnumber(L, 9, 0.1);
	if(tolerance <= 0)
		return luaL_error(L, "Tolerance must be greater zero!");
	// Helper functions
	auto curve_split = [](const std::array<Point2d,4>& points) -> std::array<Point2d,8>{
		static const Point2d half{0.5, 0.5};
		const Point2d p01 = (points[0] + points[1]) * half,
			p12 = (points[1] + points[2]) * half,
			p23 = (points[2] + points[3]) * half,
			p012 = (p01 + p12) * half,
			p123 = (p12 + p23) * half,
			p0123 = (p012 + p123) * half;
		return {points[0], p01, p012, p0123, p0123, p123, p23, points[3]};
	};
	auto curve_is_flat = [tolerance](const std::array<Point2d,4>& points){
		std::array<Point2d,3> vecs{{
			points[1] - points[0],
			points[2] - points[1],
			points[3] - points[2]
		}};
		const auto vecs_end = std::remove(vecs.begin(), vecs.end(), Point2d{0,0});
		if(std::next(vecs.cbegin(),2) <= vecs_end)
			for(auto it = std::next(vecs.cbegin()); it != vecs_end; ++it)
				if(std::abs(normal_z(*(it-1), *it)) > tolerance)
					return false;
		return true;
	};
	std::function<void(const std::array<Point2d,4>&, std::vector<Point2d>&)> curve_to_lines;
	curve_to_lines = [&curve_to_lines,&curve_split,&curve_is_flat](const std::array<Point2d,4>& points, std::vector<Point2d>& out){
		if(curve_is_flat(points))
			out.push_back(points.back());
		else{
			const auto& points2 = curve_split(points);
			curve_to_lines({points2[0], points2[1], points2[2], points2[3]}, out);
			curve_to_lines({points2[4], points2[5], points2[6], points2[7]}, out);
		}
	};
	// Convert curve to lines
	std::vector<Point2d> lines{points.front()};
	curve_to_lines(points, lines);
	// Send line points to Lua
        lua_createtable(L, lines.size() << 1, 0);
	size_t i = 0;
	for(const auto& point : lines){
		lua_pushnumber(L, point.y); lua_pushnumber(L, point.x);
		lua_rawseti(L, -3, ++i); lua_rawseti(L, -2, ++i);
	}
	return 1;
}

int luaopen_geometry(lua_State* L){
	lua_createtable(L, 0, 2);
	lua_pushcfunction(L, geometry_ear_clipping); lua_setfield(L, -2, "earclipping");
	lua_pushcfunction(L, geometry_curve_flatten); lua_setfield(L, -2, "curveflatten");

	// TODO

	return 1;
}
