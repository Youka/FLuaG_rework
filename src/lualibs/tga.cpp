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
#include <sstream>
#include <fstream>
#include <vector>
#include <array>

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

	developer_area (variable)	// TGA V2.0 extension, fully optional
*/
// Runlength Encoding: First byte = control byte, followed by pixel value. Control byte value = 1 + first 7 bits. 8th bit set = repeat counter = repeat of pixel, else data counter = offset pixels to next control byte.

// TGA en-/decoding
template<typename COLOR, typename DATA, typename INDICES>
static bool colormap_to_pixels(const INDICES& indices, const DATA& colormap, DATA& pixels){
	const COLOR* pcolormap = reinterpret_cast<COLOR*>(const_cast<char*>(colormap.data()));
	const unsigned pcolormap_size = colormap.size() / sizeof(COLOR);
	COLOR* ppixels = reinterpret_cast<COLOR*>(const_cast<char*>(pixels.data()));
	for(const auto index : indices){
		if(index >= pcolormap_size)
			return false;
		*ppixels++ = pcolormap[index];
	}
	return true;
}

template<typename DATA, typename INDICES>
static bool colormapx_to_pixels(const INDICES& indices, const DATA& colormap, DATA& pixels, const unsigned char color_map_entry_size){
	switch(color_map_entry_size){
		case 16: return colormap_to_pixels<short>(indices, colormap, pixels);
		case 24: return colormap_to_pixels<std::array<char,3>>(indices, colormap, pixels);
		case 32: return colormap_to_pixels<int>(indices, colormap, pixels);
	}
	return false;
}

static int tga_decode(std::istream& in, lua_State* L){
	// Read TGA header from stream
	TGAHeader tga_header;
	if(!in.read(reinterpret_cast<char*>(&tga_header), sizeof(tga_header)))
		return luaL_error(L, "Couldn't read header!");
	// Validate TGA header contents
	if((tga_header.image_type > 3 && tga_header.image_type < 9) || tga_header.image_type > 11)
		return luaL_error(L, "Invalid image type!");
	if(tga_header.image_type == 1 || tga_header.image_type == 9){	// Colormap used
		if(!tga_header.color_map_type)
			return luaL_error(L, "Colormap type without colormap!");
		if(tga_header.color_map_first_index >= tga_header.color_map_length)
			return luaL_error(L, "First colormap entry is out of range!");
		if(tga_header.color_map_length == 0)
			return luaL_error(L, "Empty colormap!");
		if(tga_header.color_map_entry_size != 16 && tga_header.color_map_entry_size != 24 && tga_header.color_map_entry_size != 32)
			return luaL_error(L, "Colormap entry size must be 16, 24 or 32!");
	}
	if((tga_header.image_type == 2 || tga_header.image_type == 10) && tga_header.pixel_depth != 24 && tga_header.pixel_depth != 32)
		return luaL_error(L, "RGB just with 8-bit channels supported!");
	if(tga_header.pixel_depth != 1 && tga_header.pixel_depth != 8 && tga_header.pixel_depth != 16 && tga_header.pixel_depth != 24 && tga_header.pixel_depth != 32)
		return luaL_error(L, "Pixel depth must be 1, 8, 16, 24 or 32!");
	// Read image id from stream
	std::string image_id(tga_header.image_id_length, '\0');
	if(!in.read(const_cast<char*>(image_id.data()), image_id.length()))
		return luaL_error(L, "Couldn't read image id!");
	// Read colormap from stream
	std::string colormap;
	if(tga_header.color_map_type){
		colormap.resize((tga_header.color_map_entry_size >> 3) * (tga_header.color_map_length - tga_header.color_map_first_index));
		if(!in.ignore(tga_header.color_map_first_index * (tga_header.color_map_entry_size >> 3)) || !in.read(const_cast<char*>(colormap.data()), colormap.length()))
			return luaL_error(L, "Couldn't read colormap!");
	}
	// Read & decode pixels from stream
	std::string pixels;
	const unsigned image_size = tga_header.image_width * tga_header.image_height;
	switch(tga_header.image_type){
		// No data
		case 0:
			break;
		// Uncompressed colormap
		case 1:
			pixels.resize(image_size * (tga_header.color_map_entry_size >> 3));
			switch(tga_header.pixel_depth){
				case 1:

					// TODO

					break;
				case 8:{
						std::vector<unsigned char> indices(image_size);
						if(!in.read(reinterpret_cast<char*>(indices.data()), indices.size()))
							return luaL_error(L, "Couldn't read uncompressed colormap data!");
						if(!colormapx_to_pixels(indices, colormap, pixels, tga_header.color_map_entry_size))
							return luaL_error(L, "Colormap access out-of-bounds!");
					}break;
				case 16:{
						std::vector<unsigned short> indices(image_size);
						if(!in.read(reinterpret_cast<char*>(indices.data()), indices.size() << 1))
							return luaL_error(L, "Couldn't read uncompressed colormap data!");
						if(!colormapx_to_pixels(indices, colormap, pixels, tga_header.color_map_entry_size))
							return luaL_error(L, "Colormap access out-of-bounds!");
					}break;
				case 24:

					// TODO

					break;
				case 32:{
						std::vector<unsigned> indices(image_size);
						if(!in.read(reinterpret_cast<char*>(indices.data()), indices.size() << 2))
							return luaL_error(L, "Couldn't read uncompressed colormap data!");
						if(!colormapx_to_pixels(indices, colormap, pixels, tga_header.color_map_entry_size))
							return luaL_error(L, "Colormap access out-of-bounds!");
					}break;
			}
			break;
		// Uncompressed RGB
		case 2:
			pixels.resize(image_size * (tga_header.pixel_depth >> 3));
			if(!in.read(const_cast<char*>(pixels.data()), pixels.length()))
				return luaL_error(L, "Couldn't read uncompressed RGB data!");
			break;
		// Uncompressed greyscale
		case 3:
			pixels.resize(tga_header.pixel_depth == 1 ? (image_size & 0x7 ? (image_size >> 3) + 1 : image_size >> 3) : image_size * (tga_header.pixel_depth >> 3));
			if(!in.read(const_cast<char*>(pixels.data()), pixels.length()))
				return luaL_error(L, "Couldn't read uncompressed greyscale data!");
			break;
		// Runlength encoded colormap
		case 9:

			// TODO

			break;
		// Runlength encoded RGB
		case 10:

			// TODO

			break;
		// Runlength encoded greyscale
		case 11:

			// TODO

			break;
	}
	// Send TGA data to Lua

	// TODO

	return 0;
}

static void tga_encode(std::ostream& out, lua_State* L){

	// TODO

}

// General functions
static int tga_read(lua_State* L){
	size_t len;
	std::istringstream in(std::string(luaL_checklstring(L, 1, &len), len));
	return tga_decode(in, L);
}

static int tga_read_file(lua_State* L){
	std::ifstream in(luaL_checkstring(L, 1), std::ios_base::binary);
	if(!in)
		return luaL_error(L, "Couldn't open input file!");
	return tga_decode(in, L);
}

static int tga_write(lua_State* L){
	std::ostringstream out;
	tga_encode(out, L);
	const std::string out_str = out.str();
	lua_pushlstring(L, out_str.data(), out_str.length());
	return 1;
}

static int tga_write_file(lua_State* L){
	std::ofstream out(luaL_checkstring(L, 1), std::ios_base::binary);
	if(!out)
		return luaL_error(L, "Couldn't open output file!");
	tga_encode(out, L);
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
