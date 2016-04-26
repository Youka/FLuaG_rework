/*
Project: FLuaG
File: FLuaG_image.cpp

Copyright (c) 2015-2016, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "FLuaG.hpp"
#include "../utils/lua.h"
#include "../utils/imageop.hpp"

// Unique name for Lua metatable
#define LUA_IMAGE_DATA "FLuaG_image_data"

// Data container for Lua userdata
struct ImageData{
	std::weak_ptr<unsigned char> data;
	unsigned rowsize;
	int stride;
	unsigned short height;
};

// Metatable methods
static int image_data_delete(lua_State* L) noexcept{
	delete *static_cast<ImageData**>(luaL_checkudata(L, 1, LUA_IMAGE_DATA));
	return 0;
}

static int image_data_size(lua_State* L) noexcept{
	const ImageData* udata = *static_cast<ImageData**>(luaL_checkudata(L, 1, LUA_IMAGE_DATA));
	lua_pushinteger(L, udata->height * udata->rowsize);
	return 1;
}

static int image_data_access(lua_State* L) noexcept{
	// Get arguments
	ImageData* udata = *static_cast<ImageData**>(luaL_checkudata(L, 1, LUA_IMAGE_DATA));
	size_t data_len;
	const unsigned char* data = reinterpret_cast<const unsigned char*>(luaL_optlstring(L, 2, nullptr, &data_len));
	// Check data are still alive
	if(udata->data.expired())
		return luaL_error(L, "Data are already dead!");
	// Choose operation
	const unsigned long image_size = udata->height * udata->rowsize;
	if(data){
		// Check argument
		if(data_len != image_size)
			return luaL_error(L, "Data size isn't equal to expected image size!");
		// Copy data
		ImageOp::copy(data, udata->data.lock().get(), udata->height, udata->rowsize, ::abs(udata->stride), udata->stride < 0);
		return 0;
	}else{
		// Copy data
		if(static_cast<int>(udata->rowsize) == udata->stride)
			lua_pushlstring(L, reinterpret_cast<char*>(udata->data.lock().get()), image_size);
		else{
			std::unique_ptr<unsigned char> buf(new unsigned char[image_size]);
			ImageOp::copy(udata->data.lock().get(), buf.get(), udata->height, ::abs(udata->stride), udata->rowsize, udata->stride < 0);
			lua_pushlstring(L, reinterpret_cast<char*>(buf.get()), image_size);
		}
		return 1;
	}
}

#define LSTATE this->L.get()

namespace FLuaG{
	void Script::lua_pushimage(std::weak_ptr<unsigned char> image_data, const int stride) const noexcept{
		// Create & push image data as Lua userdata
		*static_cast<ImageData**>(lua_newuserdata(LSTATE, sizeof(ImageData*))) = new ImageData{image_data, this->image_rowsize, stride, this->image_height};
		// Fetch/create Lua image data metatable
		if(luaL_newmetatable(LSTATE, LUA_IMAGE_DATA)){
			static const luaL_Reg l[] = {
				{"__gc", image_data_delete},
				{"__len", image_data_size},
				{"__call", image_data_access},
				{NULL, NULL}
			};
			luaL_setfuncs(LSTATE, l, 0);
		}
		// Bind metatable to userdata
		lua_setmetatable(LSTATE, -2);
	}
}
