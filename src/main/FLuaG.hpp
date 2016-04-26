/*
Project: FLuaG
File: FLuaG.hpp

Copyright (c) 2015-2016, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include <exception>
#include <string>
#include <memory>
#include <lua.hpp>
#ifdef FLUAG_FORCE_SINGLE_THREAD
	#include <thread>
	#include <future>
#endif

namespace FLuaG{
	// Simple local exception
	class exception : public std::exception{
		private:
			const std::string message;
		public:
			exception(const std::string& message) noexcept : message(message){}
			exception(std::string&& message) noexcept : message(std::move(message)){}
			const char* what() const throw() override{return message.c_str();}
	};

	// Video header informations storage
	struct VideoHeader{
		// Frame dimension expected as positive and in realistic range
		unsigned short width, height;
		// Color type RGB with alpha?
		bool has_alpha;
		// Frames-per-second on constant frame durations
		double fps;
		// Number of available frames
		unsigned long frames;
	};

	// Main class
	class Script{
		private:
			// Lua state
			using lua_ptr = std::unique_ptr<lua_State, void(*)(lua_State*)>;
			lua_ptr L = lua_ptr(luaL_newstate(), [](lua_State* L){lua_close(L);});
			// Video informations required by ProcessFrame function
			unsigned short image_height = 0;
			unsigned image_rowsize = 0;
			// Userdata required by LoadFile function
			std::string userdata;
			// Image data to Lua object
			void lua_pushimage(std::weak_ptr<unsigned char> image_data, const int stride) const noexcept;
#ifdef FLUAG_FORCE_SINGLE_THREAD
			std::promise<bool> main_prom;
			std::future<bool> main_fut = main_prom.get_future();
			std::promise<int> Lrun_prom;
			std::future<int> Lrun_fut = Lrun_prom.get_future();
			std::thread Lrun = std::thread([this](void){
				int arg;
				while((arg = this->Lrun_fut.get()) >= 0){
					this->Lrun_prom = std::promise<int>();
					this->Lrun_fut = this->Lrun_prom.get_future();
					const int call_err = lua_pcall(this->L.get(), arg, 0, 0);
					lua_gc(this->L.get(), LUA_GCCOLLECT, 0);	// Doesn't remove error message on stack top
					this->main_prom.set_value(call_err);
				}
				this->L.reset();	// Garbage collection runs Lua too (__gc metamethods)
			});
#endif
		public:
			// Ctor
			Script();
			Script(const std::string& filename);
			Script(const std::string& filename, const VideoHeader header, const std::string& userdata);
			// Dtor
#ifdef FLUAG_FORCE_SINGLE_THREAD
			~Script();
#endif
			// No copy
			Script(const Script&) = delete;
			Script& operator=(const Script&) = delete;
			// Move
			Script(Script&& other) noexcept;
			Script& operator=(Script&& other) noexcept;
			// Setters
			void SetVideo(const VideoHeader header) noexcept;
			void SetUserdata(const std::string& userdata) noexcept;
			void LoadFile(const std::string& filename);
			void LoadScript(const std::string& script);
			// Processing
			void ProcessFrame(unsigned char* image_data, const int stride, const unsigned long ms);
	};
}
