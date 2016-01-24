/*
Project: FLuaG
File: tga.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "libs.h"
#include <fstream>

// TGA format structure
#pragma pack(push, 1)
struct TGAHeader{
	unsigned char image_id_length,		// length of image_id
			color_map_type,		// 0 = no colormap; 1 = has colormap
			image_type;		// 0 = no data; 1 = uncompressed colormap; 2 = uncompressed RGB; 3 = uncompressed grayscale; 9 = compressed colormap; 10 = compressed RGB; 11 = compressed grayscale
	unsigned short color_map_first_index,	// index of first colormap entry in use
			color_map_length;	// number of color map entries
	unsigned char color_map_entry_size;	// bits per colormap entry (f.e. 16, 24, 32)
	short x_origin,				// horizontal offset of displayed image
		y_origin;			// vertical offset of displayed image
	unsigned short image_width,		// image width in pixels
			image_height;		// image height in pixels
	unsigned char pixel_depth,		// bits per pixel (f.e. 1, 8, 16, 24, 32)
			image_descriptor;	// 0-3 bits: attribute bits per pixel; 4th bit: 0 = left-to-right, 1 = right-to-left; 5th bit: 0 = bottom-up, 1 = top-down
};
#pragma pack(pop)
/*
	image_id (0-255 bytes)		// image type identification
	color_map (0-8192 bytes)	// colormap entries
	image_data (variable)		// pixels data

	developer area (variable)	// TGA V2.0 extension, fully optional
*/
// Runlength Encoding: First byte = control byte, followed by pixel value. Control byte value = 1 + first 7 bits. 8th bit set = repeat counter = repeat of pixel, else data counter = offset pixels to next control byte.

// General functions
static int tga_read(lua_State* L){

	// TODO

	return 0;
}

static int tga_read_file(lua_State* L){

	// TODO

	return 0;
}

static int tga_write(lua_State* L){

	// TODO

	return 0;
}

static int tga_write_file(lua_State* L){

	// TODO

	return 0;
}

int luaopen_tga(lua_State* L){
	lua_createtable(L, 0, 4);
	lua_pushcfunction(L, tga_read); lua_setfield(L, -2, "read");
	lua_pushcfunction(L, tga_read_file); lua_setfield(L, -2, "readfile");
	lua_pushcfunction(L, tga_write); lua_setfield(L, -2, "write");
	lua_pushcfunction(L, tga_write_file); lua_setfield(L, -2, "writefile");
	return 1;
}
