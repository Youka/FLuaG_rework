/*
Project: FLuaG
File: FLuaG.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "FLuaG.hpp"

#define LSTATE this->L.get()

namespace FLuaG{
	Script::Script(){
		// Check Lua state allocation (unsafe C alloc)
		if(!this->L)
			throw std::bad_alloc();
		// Open Lua standard libraries
		luaL_openlibs(LSTATE);

		// TODO: setup Lua libraries

	}


	Script::Script(const std::string& filename) throw(exception) : Script(){
		this->LoadFile(filename);
	}

	Script::Script(const std::string& filename, const VideoHeader header, const std::string& userdata) throw(exception) : Script(){
		this->LoadFile(filename);
		this->SetVideo(header);
		this->SetUserdata(userdata);
	}

	Script::Script(Script&& other) : Script(){
		*this = std::move(other);
	}

	Script& Script::operator=(Script&& other){
		lua_State* old_L = this->L.release();
		this->L.reset(other.L.release());
		other.L.reset(old_L);
		return *this;
	}

	void Script::LoadFile(const std::string& filename) throw(exception){
		if(luaL_dofile(LSTATE, filename.c_str())){
			const std::string err(lua_tostring(LSTATE, -1));
			lua_pop(LSTATE, 1);
			throw exception(std::move(err));
		}
	}

	void Script::SetVideo(const VideoHeader header){
		// Create table for video informations
		lua_createtable(LSTATE, 0, 6);
		// Fill table with VideoHeader content
		switch(header.color_type){
			case VideoHeader::ColorType::RGB: lua_pushstring(LSTATE, "rgb"); break;
			case VideoHeader::ColorType::BGR: lua_pushstring(LSTATE, "bgr"); break;
			case VideoHeader::ColorType::RGBA: lua_pushstring(LSTATE, "rgba"); break;
			case VideoHeader::ColorType::BGRA: lua_pushstring(LSTATE, "bgra"); break;
		}lua_setfield(LSTATE, -2, "color_type");
		lua_pushinteger(LSTATE, header.width); lua_setfield(LSTATE, -2, "width");
		lua_pushinteger(LSTATE, header.height); lua_setfield(LSTATE, -2, "height");
		lua_pushnumber(LSTATE, header.fps); lua_setfield(LSTATE, -2, "fps");
		lua_pushinteger(LSTATE, header.frames); lua_setfield(LSTATE, -2, "frames");
		// Calculate and add+save image size
		this->image_size = header.width * header.height;
		switch(header.color_type){
			case VideoHeader::ColorType::RGB:
			case VideoHeader::ColorType::BGR: this->image_size += this->image_size << 1 /* x3 */; break;
			case VideoHeader::ColorType::RGBA:
			case VideoHeader::ColorType::BGRA: this->image_size <<= 2; break;
		}
		lua_pushinteger(LSTATE, this->image_size); lua_setfield(LSTATE, -2, "frame_size");
		// Set table to Lua environment/global space
		lua_setglobal(LSTATE, "_VIDEO");
	}

	void Script::SetUserdata(const std::string& userdata){
		lua_pushstring(LSTATE, userdata.c_str());
		lua_setglobal(LSTATE, "arg");
	}

	void Script::ProcessFrame(unsigned char* image_data, unsigned stride, unsigned long ms) throw(exception){
		lua_getglobal(LSTATE, "GetFrame");
		if(!lua_isfunction(LSTATE, -1)){
			lua_pop(LSTATE, 1);
			throw exception("'GetFrame' function is missing");
		}

		// TODO: create frame object

		lua_pushnil(LSTATE);
		lua_pushinteger(LSTATE, ms);
		if(lua_pcall(LSTATE, 2, 0, 0)){
			const std::string err(lua_tostring(LSTATE, -1));
			lua_pop(LSTATE, 1);
			throw exception(std::move(err));
		}
	}
}
