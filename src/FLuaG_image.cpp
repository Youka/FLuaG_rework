/*
Project: FLuaG
File: FLuaG_image.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "FLuaG.hpp"

#define LSTATE this->L.get()

// Unique name for Lua metatable
#define LUA_IMAGE_DATA "FLuaG image data"

// Data container for Lua userdata
struct ImageData{
	unsigned char* data;
	unsigned rowsize, stride;
	unsigned short height;
};

// Metatable methods
static int image_get_data(lua_State* L){
	ImageData* udata = reinterpret_cast<ImageData*>(luaL_checkudata(L, 1, LUA_IMAGE_DATA));

	// TODO: convert image and send as string to Lua

	return 1;
}

static int image_set_data(lua_State* L){
	ImageData* udata = reinterpret_cast<ImageData*>(luaL_checkudata(L, 1, LUA_IMAGE_DATA));
	size_t data_len;
	const char* data = luaL_checklstring(L, 2, &data_len);

	// TODO: convert image and save to userdata

	return 0;
}

namespace FLuaG{
	void Script::lua_pushimage(unsigned char* image_data, unsigned stride){
		// Fetch/create Lua image data metatable
		if(luaL_newmetatable(LSTATE, LUA_IMAGE_DATA)){
			lua_pushvalue(LSTATE, -1); lua_setfield(LSTATE, -2, "__index");
			lua_pushcfunction(LSTATE, image_get_data); lua_setfield(LSTATE, -2, "GetData");
			lua_pushcfunction(LSTATE, image_set_data); lua_setfield(LSTATE, -2, "SetData");

			// TODO: add image methods

		}
		// Create & push image data as Lua userdata
		ImageData* udata = reinterpret_cast<ImageData*>(lua_newuserdata(LSTATE, sizeof(ImageData)));
		udata->data = image_data;
		udata->rowsize = this->image_rowsize;
		udata->stride = stride;
		udata->height = this->image_height;
		// Bind metatable to userdata
		lua_setmetatable(LSTATE, -2);
	}
}
