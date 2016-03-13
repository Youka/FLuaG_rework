/*
Project: FLuaG
File: png.cpp

Copyright (c) 2015-2016, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "libs.h"
#include "../utils/lua.h"
#include <sstream>
#include <fstream>
#include <png.h>
#include <memory>
#include <cassert>

static int png_decode(std::istream& in, lua_State* L){
	// Check PNG signature
	static const unsigned PNG_SIG_BYTES = 8;
	unsigned char png_sig[PNG_SIG_BYTES];
	if(!in.read(reinterpret_cast<char*>(png_sig), PNG_SIG_BYTES))
		return luaL_error(L, "Couldn't read signature from file!");
	if(!png_check_sig(png_sig, PNG_SIG_BYTES))
		return luaL_error(L, "File signature isn't PNG!");
	// Create PNG structures
	png_infop png_info = nullptr;
	const std::unique_ptr<png_struct,std::function<void(png_structp)>> png(png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr), [&png_info](png_structp png){png_destroy_read_struct(&png, &png_info, nullptr);});
	if(!png || !(png_info = png_create_info_struct(png.get())))
		return luaL_error(L, "Couldn't create PNG structures!");
	// Set PNG error "handler"
	if(setjmp(png_jmpbuf(png.get())))
		return luaL_error(L, "PNG read error occured!");
	// Set PNG source reader
	png_set_read_fn(png.get(), &in, [](png_structp png, png_bytep out, png_size_t out_size){
		if(!static_cast<std::istream*>(png_get_io_ptr(png))->read(reinterpret_cast<char*>(out), out_size))
			longjmp(png_jmpbuf(png), 1);
	});
	png_set_sig_bytes(png.get(), PNG_SIG_BYTES);
	// Read PNG header informations
	png_read_info(png.get(), png_info);
	png_uint_32 width, height;
	int color_type;
	if(!png_get_IHDR(png.get(), png_info, &width, &height, nullptr, &color_type, nullptr, nullptr, nullptr))
		return luaL_error(L, "Couldn't read PNG header!");
	// Set PNG image transformations
	png_set_expand(png.get());			// Converts PALETTE->RGB24, GREY?->GREY8, RNG_CHUNK->ALPHA
	png_set_strip_16(png.get());			// Converts RGB48->RGB24, GREY16->GREY8
	png_set_gray_to_rgb(png.get());			// Converts GREY->RGB
	png_read_update_info(png.get(), png_info);	// Update header to new format after conversions
	// Read PNG image
	const png_size_t rowbytes = png_get_rowbytes(png.get(), png_info);
	std::string data(height * rowbytes, '\0');
	png_bytep pdata = reinterpret_cast<png_bytep>(const_cast<char*>(data.data()));
	for(const png_bytep data_end = pdata + height * rowbytes; pdata != data_end; pdata += rowbytes)
		png_read_row(png.get(), pdata, nullptr);
	// Send PNG data to Lua
	lua_createtable(L, 0, 4);
	lua_pushinteger(L, width); lua_setfield(L, -2, "width");
	lua_pushinteger(L, height); lua_setfield(L, -2, "height");
	lua_pushstring(L, color_type & PNG_COLOR_MASK_ALPHA ? "rgba" : "rgb"); lua_setfield(L, -2, "type");
	lua_pushlstring(L, data.data(), data.length()); lua_setfield(L, -2, "data");
	return 1;
}

static int png_encode(std::ostream& out, lua_State* L){
	// Get arguments
	luaL_checktype(L, 1, LUA_TTABLE);
	lua_getfield(L, 1, "width");
	lua_getfield(L, 1, "height");
	lua_getfield(L, 1, "type");
	lua_getfield(L, 1, "data");
	const int width = luaL_checkinteger(L, -4), height = luaL_checkinteger(L, -3);
	const std::string type(luaL_checkstring(L, -2)),
			data(luaL_checkstring(L, -1), lua_rawlen(L, -1));
	lua_pop(L, 4);
	// Check arguments
	if(width < 0 || height < 0)
		return luaL_error(L, "Invalid dimension!");
	if(type != "rgb" && type != "rgba")
		return luaL_error(L, "Invalid type!");
	const bool has_alpha = type == "rgba";
	if(data.length() != static_cast<size_t>(width * height * (has_alpha ? 4 : 3)))
		return luaL_error(L, "Invalid data size!");
	// Create PNG structures
	png_infop png_info = nullptr;
	const std::unique_ptr<png_struct,std::function<void(png_structp)>> png(png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr), [&png_info](png_structp png){png_destroy_write_struct(&png, &png_info);});
	if(!png || !(png_info = png_create_info_struct(png.get())))
		return luaL_error(L, "Couldn't create PNG structures!");
	// Set PNG error "handler"
	if(setjmp(png_jmpbuf(png.get())))
		return luaL_error(L, "PNG write error occured!");
	// Set PNG target writer
	png_set_write_fn(png.get(), &out, [](png_structp png, png_bytep in, png_size_t in_size){
		if(!static_cast<std::ostream*>(png_get_io_ptr(png))->write(reinterpret_cast<char*>(in), in_size))
			longjmp(png_jmpbuf(png), 1);
	}, nullptr);
	// Write PNG header informations
	png_set_IHDR(png.get(), png_info, width, height, 8, has_alpha ? PNG_COLOR_TYPE_RGBA : PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png.get(), png_info);
	// Write PNG image
	png_bytep pdata = reinterpret_cast<png_bytep>(const_cast<char*>(data.data()));
	const png_size_t rowbytes = png_get_rowbytes(png.get(), png_info);
	assert(rowbytes == static_cast<png_size_t>(width * (has_alpha ? 4 : 3)));
	for(const png_bytep data_end = pdata + height * rowbytes; pdata != data_end; pdata += rowbytes)
		png_write_row(png.get(), pdata);
	png_write_end(png.get(), png_info);
	// No errors occured
	return 0;
}

// General functions
static int png_read(lua_State* L){
	std::istringstream in(std::string(luaL_checkstring(L, 1), lua_rawlen(L, 1)));
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
	lua_remove(L, 1); // Set data table to stack bottom
	png_encode(out, L);
	return 0;
}

static int png_version(lua_State* L){
	lua_pushstring(L, PNG_HEADER_VERSION_STRING);
	return 1;
}

int luaopen_png(lua_State* L){
	static const luaL_Reg l[] = {
		{"read", png_read},
		{"readfile", png_read_file},
		{"write", png_write},
		{"writefile", png_write_file},
		{"version", png_version},
		{NULL, NULL}
	};
	luaL_newlib(L, l);
	return 1;
}
