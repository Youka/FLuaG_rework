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
#include <GL/glu.h>
#include <memory>

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

static int geometry_tesselate(lua_State* L){
	// Check argument
	luaL_checktype(L, 1, LUA_TTABLE);
	// Get argument (table) as contours of 2d/fake-3d points
	std::vector<std::vector<std::array<double,3>>> contours(lua_rawlen(L, 1));
	for(size_t contour_i = 0; contour_i < contours.size(); ++contour_i){
		std::vector<std::array<double,3>>& points = contours[contour_i];
		lua_rawgeti(L, 1, 1+contour_i);
		luaL_checktype(L, -1, LUA_TTABLE);
		points.resize(lua_rawlen(L, -1) >> 1);
		int i = 0;
		for(auto& point : points){
			lua_rawgeti(L, -1, ++i); lua_rawgeti(L, -2, ++i);
			point = {luaL_checknumber(L, -2), luaL_checknumber(L, -1), 0};
			lua_pop(L, 2);
		}
		lua_pop(L, 1);
	}
	// Tesselate with GLU
	struct TessState{
		GLenum cur_type;
		std::vector<Geometry::Point2d> buffer;
		std::vector<std::array<Geometry::Point2d,3>> triangles;
	}state;
	const std::unique_ptr<GLUtesselator, void(APIENTRY *)(GLUtesselator*)> tess(gluNewTess(), gluDeleteTess);
	gluTessProperty(tess.get(), GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_ODD);
	gluTessProperty(tess.get(), GLU_TESS_BOUNDARY_ONLY, GL_FALSE);
	void (*begin_callback)(const GLenum, void*) = [](const GLenum type, void* userdata){
		static_cast<TessState*>(userdata)->cur_type = type;
	};
	gluTessCallback(tess.get(), GLU_TESS_BEGIN_DATA, reinterpret_cast<void(APIENTRY *)()>(begin_callback));
	void (*vertex_callback)(const void*, void*) = [](const void* vertex, void* userdata){
		auto& pvertex = *static_cast<const std::array<double,3>*>(vertex);
		static_cast<TessState*>(userdata)->buffer.push_back({pvertex[0], pvertex[1]});
	};
	gluTessCallback(tess.get(), GLU_TESS_VERTEX_DATA, reinterpret_cast<void(APIENTRY *)()>(vertex_callback));
	void (*end_callback)(void*) = [](void* userdata){
		TessState* state = static_cast<TessState*>(userdata);
		switch(state->cur_type){
			case GL_TRIANGLES:
				for(size_t i = 0; i+2 < state->buffer.size(); i+=3)
					state->triangles.push_back({state->buffer[i], state->buffer[i+1], state->buffer[i+2]});
				break;
			case GL_TRIANGLE_STRIP:
				for(size_t i = 2; i < state->buffer.size(); ++i)
					state->triangles.push_back({state->buffer[i-2], state->buffer[i-1], state->buffer[i]});
				break;
			case GL_TRIANGLE_FAN:
				for(size_t i = 2; i < state->buffer.size(); ++i)
					state->triangles.push_back({state->buffer.front(), state->buffer[i-1], state->buffer[i]});
				break;
		}
		state->buffer.clear();
	};
	gluTessCallback(tess.get(), GLU_TESS_END_DATA, reinterpret_cast<void(APIENTRY *)()>(end_callback));
	void (*combine_callback)(const GLdouble[3], const void*[4], const GLfloat[4], void**) = [](const GLdouble coords[3], const void*[4], const GLfloat[4], void** out){
		*out = new std::array<double,3>{coords[0], coords[1], coords[2]};	// Safe because std::array is just a wrapper around C array
	};
	gluTessCallback(tess.get(), GLU_TESS_COMBINE, reinterpret_cast<void(APIENTRY *)()>(combine_callback));
	gluTessBeginPolygon(tess.get(), &state);
	for(auto& contour : contours){
		gluTessBeginContour(tess.get());
		for(auto& points : contour)
			gluTessVertex(tess.get(), points.data(), points.data());
		gluTessEndContour(tess.get());
	}
	gluTessEndPolygon(tess.get());
	// Send triangles to Lua
	lua_createtable(L, state.triangles.size(), 0);
	int i = 0;
	for(const auto& triangle : state.triangles){
		lua_createtable(L, 6, 0);
		lua_pushnumber(L, triangle[0].x); lua_pushnumber(L, triangle[0].y); lua_pushnumber(L, triangle[1].x); lua_pushnumber(L, triangle[1].y); lua_pushnumber(L, triangle[2].x); lua_pushnumber(L, triangle[2].y);
		lua_rawseti(L, -7, 6); lua_rawseti(L, -6, 5); lua_rawseti(L, -5, 4); lua_rawseti(L, -4, 3); lua_rawseti(L, -3, 2); lua_rawseti(L, -2, 1);
		lua_rawseti(L, -2, ++i);
	}
	return 1;
}

#define LUA_MATRIX "matrix"
using Matrix = Math::Matrix4x4<double>;
static void lua_pushmatrix(lua_State* L, const Matrix& matrix);

static int geometry_matrix_free(lua_State* L){
	delete *static_cast<Matrix**>(luaL_checkudata(L, 1, LUA_MATRIX));
	return 0;
}

static int geometry_matrix_mul(lua_State* L){
	lua_pushmatrix(L, **static_cast<Matrix**>(luaL_checkudata(L, 1, LUA_MATRIX)) * **static_cast<Matrix**>(luaL_checkudata(L, 2, LUA_MATRIX)));
	return 1;
}

static int geometry_matrix_multiply(lua_State* L){
	(*static_cast<Matrix**>(luaL_checkudata(L, 1, LUA_MATRIX)))->multiply(**static_cast<Matrix**>(luaL_checkudata(L, 2, LUA_MATRIX)));
	lua_settop(L, 1);
	return 1;
}

static int geometry_matrix_translate(lua_State* L){
	(*static_cast<Matrix**>(luaL_checkudata(L, 1, LUA_MATRIX)))->translate(luaL_checknumber(L, 2), luaL_checknumber(L, 3), luaL_checknumber(L, 4));
	lua_settop(L, 1);
	return 1;
}

static int geometry_matrix_scale(lua_State* L){
	(*static_cast<Matrix**>(luaL_checkudata(L, 1, LUA_MATRIX)))->scale(luaL_checknumber(L, 2), luaL_checknumber(L, 3), luaL_checknumber(L, 4));
	lua_settop(L, 1);
	return 1;
}

static int geometry_matrix_rotate(lua_State* L){
	Matrix::Axis axis = Matrix::Axis::X;
	static const char* option_str[] = {"x", "y", "z", nullptr};
	switch(luaL_checkoption(L, 3, nullptr, option_str)){
		case 0: axis = Matrix::Axis::X; break;
		case 1: axis = Matrix::Axis::Y; break;
		case 2: axis = Matrix::Axis::Z; break;
	}
	(*static_cast<Matrix**>(luaL_checkudata(L, 1, LUA_MATRIX)))->rotate(luaL_checknumber(L, 2), axis);
	lua_settop(L, 1);
	return 1;
}

static int geometry_matrix_identity(lua_State* L){
	(*static_cast<Matrix**>(luaL_checkudata(L, 1, LUA_MATRIX)))->identity();
	lua_settop(L, 1);
	return 1;
}

static int geometry_matrix_invert(lua_State* L){
	lua_pushboolean(L, (*static_cast<Matrix**>(luaL_checkudata(L, 1, LUA_MATRIX)))->invert());
	return 1;
}

static int geometry_matrix_transform(lua_State* L){
	const auto vec = (*static_cast<Matrix**>(luaL_checkudata(L, 1, LUA_MATRIX)))->transform({luaL_checknumber(L, 2), luaL_checknumber(L, 3), luaL_optnumber(L, 4, 0), luaL_optnumber(L, 5, 1)});
	lua_pushnumber(L, vec[0]);
	lua_pushnumber(L, vec[1]);
	lua_pushnumber(L, vec[2]);
	lua_pushnumber(L, vec[3]);
	return 4;
}

static int geometry_matrix_data(lua_State* L){
	Matrix& matrix = **static_cast<Matrix**>(luaL_checkudata(L, 1, LUA_MATRIX));
	if(lua_gettop(L) == 1){
		lua_pushnumber(L, matrix[0]); lua_pushnumber(L, matrix[1]); lua_pushnumber(L, matrix[2]); lua_pushnumber(L, matrix[3]);
		lua_pushnumber(L, matrix[4]); lua_pushnumber(L, matrix[5]); lua_pushnumber(L, matrix[6]); lua_pushnumber(L, matrix[7]);
		lua_pushnumber(L, matrix[8]); lua_pushnumber(L, matrix[9]); lua_pushnumber(L, matrix[10]); lua_pushnumber(L, matrix[11]);
		lua_pushnumber(L, matrix[12]); lua_pushnumber(L, matrix[13]); lua_pushnumber(L, matrix[14]); lua_pushnumber(L, matrix[15]);
		return 16;
	}else{
		matrix[0] = luaL_checknumber(L, 2); matrix[0] = luaL_checknumber(L, 3); matrix[0] = luaL_checknumber(L, 4); matrix[0] = luaL_checknumber(L, 5);
		matrix[0] = luaL_checknumber(L, 6); matrix[0] = luaL_checknumber(L, 7); matrix[0] = luaL_checknumber(L, 8); matrix[0] = luaL_checknumber(L, 9);
		matrix[0] = luaL_checknumber(L, 10); matrix[0] = luaL_checknumber(L, 11); matrix[0] = luaL_checknumber(L, 12); matrix[0] = luaL_checknumber(L, 13);
		matrix[0] = luaL_checknumber(L, 14); matrix[0] = luaL_checknumber(L, 15); matrix[0] = luaL_checknumber(L, 16); matrix[0] = luaL_checknumber(L, 17);
		return 0;
	}
}

void lua_pushmatrix(lua_State* L, const Matrix& matrix){ // static definition on declaration
	*static_cast<Matrix**>(lua_newuserdata(L, sizeof(Matrix*))) = new Matrix(matrix);
	if(luaL_newmetatable(L, LUA_MATRIX)){
		static const luaL_Reg l[] = {
			{"__gc", geometry_matrix_free},
			{"__mul", geometry_matrix_mul},
			{"multiply", geometry_matrix_multiply},
			{"translate", geometry_matrix_translate},
			{"scale", geometry_matrix_scale},
			{"rotate", geometry_matrix_rotate},
			{"identity", geometry_matrix_identity},
			{"invert", geometry_matrix_invert},
			{"transform", geometry_matrix_transform},
			{"data", geometry_matrix_data},
			{NULL, NULL}
		};
		luaL_setfuncs(L, l, 0);
		lua_pushvalue(L, -1); lua_setfield(L, -2, "__index");
	}
	lua_setmetatable(L, -2);
}

static int geometry_matrix_create(lua_State* L){
	switch(lua_gettop(L)){
		case 0:
			lua_pushmatrix(L, Matrix());
			break;
		case 16: lua_pushmatrix(L, Matrix(luaL_checknumber(L, 1), luaL_checknumber(L, 2), luaL_checknumber(L, 3), luaL_checknumber(L, 4),
					luaL_checknumber(L, 5), luaL_checknumber(L, 6), luaL_checknumber(L, 7), luaL_checknumber(L, 8),
					luaL_checknumber(L, 9), luaL_checknumber(L, 10), luaL_checknumber(L, 11), luaL_checknumber(L, 12),
					luaL_checknumber(L, 13), luaL_checknumber(L, 14), luaL_checknumber(L, 15), luaL_checknumber(L, 16))
				);
			break;
		default: return luaL_error(L, "Invalid matrix arguments!");
	}
	return 1;
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
		{"curveflatten", geometry_curve_flatten},
		{"earclipping", geometry_ear_clipping},
		{"tesselate", geometry_tesselate},
		{"matrix", geometry_matrix_create},
		{NULL, NULL}
	};
	luaL_newlib(L, l);
	return 1;
}
