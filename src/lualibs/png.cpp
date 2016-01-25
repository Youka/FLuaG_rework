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
#include <memory>

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
			png_longjmp(png, 1);
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
	png_set_bgr(png.get());				// Converts RGB->BGR
	png_read_update_info(png.get(), png_info);	// Update header to new format after conversions
	// Read PNG image
	const png_size_t rowbytes = png_get_rowbytes(png.get(), png_info);
	std::string data(height * rowbytes, '\0');
	const std::unique_ptr<png_bytep[]> rows(new png_bytep[height]);
	rows[0] = reinterpret_cast<png_bytep>(const_cast<char*>(data.data()));
	for(png_uint_32 y = 1; y < height; ++y)
		rows[y] = rows[y-1] + rowbytes;
	png_read_image(png.get(), rows.get());
	// Send PNG data to Lua
	lua_createtable(L, 0, 4);
	lua_pushinteger(L, width); lua_setfield(L, -2, "width");
	lua_pushinteger(L, height); lua_setfield(L, -2, "height");
	lua_pushstring(L, color_type & PNG_COLOR_MASK_ALPHA ? "bgra" : "bgr"); lua_setfield(L, -2, "type");
	lua_pushlstring(L, data.data(), data.length()); lua_setfield(L, -2, "data");
	return 1;
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

static int png_version(lua_State* L){
	lua_pushstring(L, PNG_HEADER_VERSION_STRING);
	return 1;
}

int luaopen_png(lua_State* L){
	lua_createtable(L, 0, 5);
	lua_pushcfunction(L, png_read); lua_setfield(L, -2, "read");
	lua_pushcfunction(L, png_read_file); lua_setfield(L, -2, "readfile");
	lua_pushcfunction(L, png_write); lua_setfield(L, -2, "write");
	lua_pushcfunction(L, png_write_file); lua_setfield(L, -2, "writefile");
	lua_pushcfunction(L, png_version); lua_setfield(L, -2, "version");
	return 1;
}
