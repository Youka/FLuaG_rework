/*
Project: FLuaG
File: png.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "libs.h"
#include <sstream>
#include <fstream>
#include <png.h>

static int png_decode(std::istream& in, lua_State* L){

	// TODO

	return 0;
}

static void png_encode(std::ostream& out, lua_State* L){

	// TODO

}

// General functions
static int png_read(lua_State* L){
	size_t len;
	std::istringstream in(std::string(luaL_checklstring(L, 1, &len), len));
	return png_decode(in, L);
}

static int png_read_file(lua_State* L){
	std::ifstream in(luaL_checkstring(L, 1), std::ios_base::binary);
	if(!in)
		return luaL_error(L, "Couldn't open input file!");
	return png_decode(in, L);
}

static int png_write(lua_State* L){
	std::ostringstream out;
	png_encode(out, L);
	const std::string out_str = out.str();
	lua_pushlstring(L, out_str.data(), out_str.length());
	return 1;
}

static int png_write_file(lua_State* L){
	std::ofstream out(luaL_checkstring(L, 1), std::ios_base::binary);
	if(!out)
		return luaL_error(L, "Couldn't open output file!");
	png_encode(out, L);
	return 0;
}

int luaopen_png(lua_State* L){
	lua_createtable(L, 0, 4);
	lua_pushcfunction(L, png_read); lua_setfield(L, -2, "read");
	lua_pushcfunction(L, png_read_file); lua_setfield(L, -2, "readfile");
	lua_pushcfunction(L, png_write); lua_setfield(L, -2, "write");
	lua_pushcfunction(L, png_write_file); lua_setfield(L, -2, "writefile");
	return 1;
}
