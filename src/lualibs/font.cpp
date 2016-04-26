/*
Project: FLuaG
File: font.cpp

Copyright (c) 2015-2016, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "libs.h"
#include "../utils/lua.h"
#include "../utils/font.hpp"

#define LUA_FONT "font"

static int font_list(lua_State* L) noexcept{
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

static int font_free(lua_State* L) noexcept{
	delete *static_cast<Font::Font**>(luaL_checkudata(L, 1, LUA_FONT));
	return 0;
}

static int font_data(lua_State* L) noexcept{
	const Font::Font* font = *static_cast<Font::Font**>(luaL_checkudata(L, 1, LUA_FONT));
	lua_pushstring(L, font->get_family().c_str());
	lua_pushnumber(L, font->get_size());
	lua_pushboolean(L, font->get_bold());
	lua_pushboolean(L, font->get_italic());
	lua_pushboolean(L, font->get_underline());
	lua_pushboolean(L, font->get_strikeout());
	lua_pushnumber(L, font->get_spacing());
	lua_pushboolean(L, font->get_rtl());
	return 8;
}

static int font_metrics(lua_State* L) noexcept{
	const Font::Font::Metrics metrics = (*static_cast<Font::Font**>(luaL_checkudata(L, 1, LUA_FONT)))->metrics();
	lua_pushnumber(L, metrics.height);
	lua_pushnumber(L, metrics.ascent);
	lua_pushnumber(L, metrics.descent);
	lua_pushnumber(L, metrics.internal_leading);
	lua_pushnumber(L, metrics.external_leading);
	return 5;
}

static int font_text_width(lua_State* L) noexcept{
	lua_pushnumber(L, (*static_cast<Font::Font**>(luaL_checkudata(L, 1, LUA_FONT)))->text_width(luaL_checkstring(L, 2)));
	return 1;
}

static int font_text_path(lua_State* L) noexcept{
	try{
		const std::vector<Font::Font::PathSegment> segments = (*static_cast<Font::Font**>(luaL_checkudata(L, 1, LUA_FONT)))->text_path(luaL_checkstring(L, 2));
		lua_createtable(L, segments.size() / 3, 0);	// Memory guess by expecting all segments are moves/lines
		int table_i = 0;
		for(size_t segment_i = 0; segment_i < segments.size(); ++segment_i){
			const auto& segment = segments[segment_i];
			switch(segment.type){
				using Type = Font::Font::PathSegment::Type;
				case Type::MOVE:
					lua_pushstring(L, "m"); lua_rawseti(L, -2, ++table_i);
					lua_pushnumber(L, segment.x); lua_rawseti(L, -2, ++table_i);
					lua_pushnumber(L, segment.y); lua_rawseti(L, -2, ++table_i);
					break;
				case Type::LINE:
					lua_pushstring(L, "l"); lua_rawseti(L, -2, ++table_i);
					lua_pushnumber(L, segment.x); lua_rawseti(L, -2, ++table_i);
					lua_pushnumber(L, segment.y); lua_rawseti(L, -2, ++table_i);
					break;
				case Type::CURVE:
					lua_pushstring(L, "b"); lua_rawseti(L, -2, ++table_i);
					lua_pushnumber(L, segment.x); lua_rawseti(L, -2, ++table_i);
					lua_pushnumber(L, segment.y); lua_rawseti(L, -2, ++table_i);
					assert(segment_i+2 < segments.size());
					lua_pushnumber(L, segments[segment_i+1].x); lua_rawseti(L, -2, ++table_i);
					lua_pushnumber(L, segments[segment_i+1].y); lua_rawseti(L, -2, ++table_i);
					lua_pushnumber(L, segments[segment_i+2].x); lua_rawseti(L, -2, ++table_i);
					lua_pushnumber(L, segments[segment_i+2].y); lua_rawseti(L, -2, ++table_i);
					segment_i += 2;	// Skip 2 more for next loop pass
					break;
				case Type::CLOSE:
					lua_pushstring(L, "c"); lua_rawseti(L, -2, ++table_i);
					break;
			}
		}
	}catch(const Font::exception& e){
		return luaL_error(L, e.what());
	}
	return 1;
}

static int font_create(lua_State* L) noexcept{
	try{
		Font::Font font(luaL_checkstring(L, 1), luaL_optnumber(L, 2, 12),
					luaL_optboolean(L, 3, false), luaL_optboolean(L, 4, false), luaL_optboolean(L, 5, false), luaL_optboolean(L, 6, false),
					luaL_optnumber(L, 7, 0), luaL_optboolean(L, 8, false));
		*static_cast<Font::Font**>(lua_newuserdata(L, sizeof(Font::Font*))) = new Font::Font(std::move(font));
	}catch(const Font::exception& e){
		return luaL_error(L, e.what());
	}
	if(luaL_newmetatable(L, LUA_FONT)){
		static const luaL_Reg l[] = {
			{"__gc", font_free},
			{"data", font_data},
			{"metrics", font_metrics},
			{"textwidth", font_text_width},
			{"textpath", font_text_path},
			{NULL, NULL}
		};
		luaL_setfuncs(L, l, 0);
		lua_pushvalue(L, -1); lua_setfield(L, -2, "__index");
	}
	lua_setmetatable(L, -2);
	return 1;
}

int luaopen_font(lua_State* L)/* No exception specifier because of C declaration */{
	static const luaL_Reg l[] = {
		{"list", font_list},
		{"create", font_create},
		{NULL, NULL}
	};
	luaL_newlib(L, l);
	return 1;
}
