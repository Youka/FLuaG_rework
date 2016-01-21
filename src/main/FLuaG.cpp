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
#include <map>
#include "../lualibs/libs.h"
#include "../utils/module.hpp"
#include <cassert>

#define LSTATE this->L.get()

namespace FLuaG{
	Script::Script(){
		// Check Lua state allocation (unsafe C alloc)
		if(!this->L)
			throw std::bad_alloc();
		// Open Lua standard libraries
		luaL_openlibs(LSTATE);
		// Preload Lua extension libraries
		lua_getglobal(LSTATE, "package");
		if(lua_istable(LSTATE, -1)){
			lua_getfield(LSTATE, -1, "preload");
			if(lua_istable(LSTATE, -1)){
				static const std::map<const char*, int(*)(lua_State*)> libs{
					{"mathx", luaopen_mathex},
					{"tablex", luaopen_tableex},
					{"regex", luaopen_regex},
					{"algorithm", luaopen_algorithm},
					{"filesystem", luaopen_filesystem},
					{"png", luaopen_png},
					{"tgl", luaopen_tgl},
					{"font", luaopen_font}
				};
				for(auto it = libs.cbegin(); it != libs.cend(); ++it){
					lua_pushcfunction(LSTATE, it->second); lua_setfield(LSTATE, -2, it->first);
				}
			}
			lua_pop(LSTATE, -1);
		}
		lua_pop(LSTATE, -1);
		// Extend Lua search paths
		std::string path = Module::dir();
		if(!path.empty()){
			path.append("?.lua;");
			lua_getglobal(LSTATE, "package");
			if(lua_istable(LSTATE, -1)){
				lua_getfield(LSTATE, -1, "path");
				if(lua_isstring(LSTATE, -1))
					path.append(lua_tostring(LSTATE, -1));
				lua_pop(LSTATE, 1);
				lua_pushstring(LSTATE, path.c_str()); lua_setfield(LSTATE, -2, "path");
			}
			lua_pop(LSTATE, 1);
		}
	}

	Script::Script(const std::string& filename) throw(exception) : Script(){
		this->LoadFile(filename);
	}

	Script::Script(const std::string& filename, const VideoHeader header, const std::string& userdata) throw(exception) : Script(){
		this->LoadFile(filename);
		this->SetVideo(header);
		this->SetUserdata(userdata);
	}

#ifdef FLUAG_FORCE_SINGLE_THREAD
	Script::~Script(){
		this->Lrun_prom.set_value(-1);
		this->Lrun.join();
	}
#endif

	Script::Script(Script&& other) : Script(){
		*this = std::move(other);
	}

	Script& Script::operator=(Script&& other){
		// Save old Lua state for swapping
		lua_State* old_L = this->L.release();
		// Receive
		this->L.reset(other.L.release());
		this->image_rowsize = other.image_rowsize;
		this->image_height = other.image_height;
		this->userdata = std::move(other.userdata);
		// (Send)
		other.L.reset(old_L);
		other.image_rowsize = other.image_height = 0;
		other.userdata.clear();
		// Return own reference
		return *this;
	}

	void Script::SetVideo(const VideoHeader header){
		// Create table for video informations
		lua_createtable(LSTATE, 0, 5);
		// Fill table with VideoHeader content
		lua_pushinteger(LSTATE, header.width); lua_setfield(LSTATE, -2, "width");
		lua_pushinteger(LSTATE, header.height); lua_setfield(LSTATE, -2, "height");
		lua_pushboolean(LSTATE, header.has_alpha); lua_setfield(LSTATE, -2, "has_alpha");
		lua_pushnumber(LSTATE, header.fps); lua_setfield(LSTATE, -2, "fps");
		lua_pushinteger(LSTATE, header.frames); lua_setfield(LSTATE, -2, "frames");
		// Set table to Lua environment/global space
		lua_setglobal(LSTATE, "_VIDEO");
		// Save video informations for ProcessFrame function call
		this->image_height = header.height;
		this->image_rowsize = header.has_alpha ? header.width << 2 : (header.width << 1) + header.width;
	}

	void Script::SetUserdata(const std::string& userdata){
		this->userdata = userdata;
	}

	void Script::LoadFile(const std::string& filename) throw(exception){
		// Load file and push as function
		if(luaL_loadfile(LSTATE, filename.c_str())){
			const std::string err(lua_tostring(LSTATE, -1));
			lua_pop(LSTATE, 1);
			throw exception(std::move(err));
		}
		// Push userdata string as function/file input
		if(!this->userdata.empty())
			lua_pushstring(LSTATE, this->userdata.c_str());
		// Call function/file
#ifdef FLUAG_FORCE_SINGLE_THREAD
		this->Lrun_prom.set_value(this->userdata.empty() ? 0 : 1);
		const bool err = this->main_fut.get();
		this->main_prom = std::promise<bool>();
		this->main_fut = this->main_prom.get_future();
		if(err){
#else
		if(lua_pcall(LSTATE, this->userdata.empty() ? 0 : 1, 0, 0)){
#endif
			const std::string err(lua_tostring(LSTATE, -1));
			lua_pop(LSTATE, 1);
			throw exception(std::move(err));
		}
	}

	void Script::LoadScript(const std::string& script) throw(exception){
		// Load script and push as function
		if(luaL_loadstring(LSTATE, script.c_str())){
			const std::string err(lua_tostring(LSTATE, -1));
			lua_pop(LSTATE, 1);
			throw exception(std::move(err));
		}
		// Push userdata string as function/script input
		if(!this->userdata.empty())
			lua_pushstring(LSTATE, this->userdata.c_str());
		// Call function/script
#ifdef FLUAG_FORCE_SINGLE_THREAD
		this->Lrun_prom.set_value(this->userdata.empty() ? 0 : 1);
		const bool err = this->main_fut.get();
		this->main_prom = std::promise<bool>();
		this->main_fut = this->main_prom.get_future();
		if(err){
#else
		if(lua_pcall(LSTATE, this->userdata.empty() ? 0 : 1, 0, 0)){
#endif
			const std::string err(lua_tostring(LSTATE, -1));
			lua_pop(LSTATE, 1);
			throw exception(std::move(err));
		}
	}

	void Script::ProcessFrame(unsigned char* image_data, unsigned stride, unsigned long ms) throw(exception){
		assert(image_data);
		// Check for valid stride
		if(stride < this->image_rowsize)
			throw exception("Image stride cannot be smaller than rowsize!");
		// Look for function to call
		lua_getglobal(LSTATE, "GetFrame");
		if(!lua_isfunction(LSTATE, -1)){
			lua_pop(LSTATE, 1);
			throw exception("'GetFrame' function is missing");
		}
		// Push arguments and call function
		std::shared_ptr<unsigned char> image_data_shared(image_data, [](unsigned char*){});	// Equip pointer with a reference count without deleter (=no ownership)
		this->lua_pushimage(image_data_shared, stride);
		lua_pushinteger(LSTATE, ms);
#ifdef FLUAG_FORCE_SINGLE_THREAD
		this->Lrun_prom.set_value(2);
		const bool err = this->main_fut.get();
		this->main_prom = std::promise<bool>();
		this->main_fut = this->main_prom.get_future();
		if(err){
#else
		if(lua_pcall(LSTATE, 2, 0, 0)){
#endif
			const std::string err(lua_tostring(LSTATE, -1));
			lua_pop(LSTATE, 1);
			throw exception(std::move(err));
		}
	}
}
