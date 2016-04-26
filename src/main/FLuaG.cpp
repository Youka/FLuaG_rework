/*
Project: FLuaG
File: FLuaG.cpp

Copyright (c) 2015-2016, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "FLuaG.hpp"
#include "../lualibs/libs.h"
#include "../utils/lua.h"
#include "../utils/module.hpp"
#include "../utils/log.hpp"

#define LSTATE this->L.get()

namespace FLuaG{
	Script::Script(){
		LOG("Default construct script...");
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
				static const luaL_Reg l[] = {
					{"mathx", luaopen_mathx},
					{"tablex", luaopen_tablex},
					{"regex", luaopen_regex},
					{"geometry", luaopen_geometry},
					{"filesystem", luaopen_filesystem},
					{"png", luaopen_png},
					{"tgl", luaopen_tgl},
					{"font", luaopen_font},
					{"utf8x", luaopen_utf8x},
					{NULL, NULL}
				};
				luaL_setfuncs(LSTATE, l, 0);
			}
			lua_pop(LSTATE, 1);
		}
		lua_pop(LSTATE, 1);
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
		LOG("Script default constructed!");
	}

	Script::Script(const std::string& filename) : Script(){
		LOG("File construct script...");
		this->LoadFile(filename);
		LOG("Script constructed with file!");
	}

	Script::Script(const std::string& filename, const VideoHeader header, const std::string& userdata) : Script(){
		LOG("File construct script with header & userdata...");
		this->LoadFile(filename);
		this->SetVideo(header);
		this->SetUserdata(userdata);
		LOG("Script constructed with file, header & userdata!");
	}

	Script::Script(Script&& other) noexcept : Script(){
		LOG("Move construct script...");
		*this = std::move(other);
		LOG("Script move constructed!");
	}

	Script& Script::operator=(Script&& other) noexcept{
		LOG("Move assign script...");
		this->L.swap(other.L);
		this->image_rowsize = other.image_rowsize;
		other.image_rowsize = 0;
		this->image_height = other.image_height;
		other.image_height = 0;
		this->userdata.swap(other.userdata);
#ifdef FLUAG_FORCE_SINGLE_THREAD
		this->call_context.swap(other.call_context);
#endif
		LOG("Script move assigned!");
		return *this;
	}

	void Script::SetVideo(const VideoHeader header) noexcept{
		LOG("Set video informations to script...");
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
		LOG("Script got video informations set!");
	}

	void Script::SetUserdata(const std::string& userdata) noexcept{
		LOG("Set userdata of script...");
		this->userdata = userdata;
		LOG("Script got userdata set!");
	}

	void Script::LoadFile(const std::string& filename){
		LOG("Load file into script...");
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
        const int err =
#ifdef FLUAG_FORCE_SINGLE_THREAD
			(*this->call_context)([this]() -> int{return lua_pcall(LSTATE, this->userdata.empty() ? 0 : 1, 0, 0);});
#else
			lua_pcall(LSTATE, this->userdata.empty() ? 0 : 1, 0, 0);
#endif
		if(err){
			const std::string err(lua_tostring(LSTATE, -1));
			lua_pop(LSTATE, 1);
			throw exception(std::move(err));
		}
#ifdef FLUAG_FORCE_SINGLE_THREAD
		(*this->call_context)([this]() -> int{lua_gc(this->L.get(), LUA_GCCOLLECT, 0); return 0;});
#else
		lua_gc(this->L.get(), LUA_GCCOLLECT, 0);
#endif
		LOG("Script loaded file!");
	}

	void Script::LoadScript(const std::string& script){
		LOG("Load string into script...");
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
        const int err =
#ifdef FLUAG_FORCE_SINGLE_THREAD
			(*this->call_context)([this]() -> int{return lua_pcall(LSTATE, this->userdata.empty() ? 0 : 1, 0, 0);});
#else
			lua_pcall(LSTATE, this->userdata.empty() ? 0 : 1, 0, 0);
#endif
		if(err){
			const std::string err(lua_tostring(LSTATE, -1));
			lua_pop(LSTATE, 1);
			throw exception(std::move(err));
		}
#ifdef FLUAG_FORCE_SINGLE_THREAD
		(*this->call_context)([this]() -> int{lua_gc(this->L.get(), LUA_GCCOLLECT, 0); return 0;});
#else
		lua_gc(this->L.get(), LUA_GCCOLLECT, 0);
#endif
		LOG("Script loaded string!");
	}

	void Script::ProcessFrame(unsigned char* image_data, const int stride, const unsigned long ms){
		LOG("Process frame by script...");
		// Check for valid stride
		if(static_cast<unsigned>(::abs(stride)) < this->image_rowsize)
			throw exception("Image stride cannot be smaller than rowsize!");
		// Look for function to call
		lua_getglobal(LSTATE, "GetFrame");
		if(!lua_isfunction(LSTATE, -1)){
			lua_pop(LSTATE, 1);
			throw exception("'GetFrame' function is missing");
		}
		// Push arguments and call function
		const std::shared_ptr<unsigned char> image_data_shared(image_data, [](unsigned char*){});	// Equip pointer with a reference count without deleter (=no ownership)
		this->lua_pushimage(image_data_shared, stride);
		lua_pushinteger(LSTATE, ms);
        const int err =
#ifdef FLUAG_FORCE_SINGLE_THREAD
			(*this->call_context)([this]() -> int{return lua_pcall(LSTATE, 2, 0, 0);});
#else
			lua_pcall(LSTATE, this->userdata.empty() ? 0 : 1, 0, 0);
#endif
		if(err){
			const std::string err(lua_tostring(LSTATE, -1));
			lua_pop(LSTATE, 1);
			throw exception(std::move(err));
		}
#ifdef FLUAG_FORCE_SINGLE_THREAD
		(*this->call_context)([this]() -> int{lua_gc(this->L.get(), LUA_GCCOLLECT, 0); return 0;});
#else
		lua_gc(this->L.get(), LUA_GCCOLLECT, 0);
#endif
		LOG("Script processed frame successfully!");
	}
}
