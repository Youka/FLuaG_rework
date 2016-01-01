/*
Project: FLuaG
File: FLuaG.hpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

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

namespace FLuaG{
	// Simple local exception
	class exception : public std::exception{
		private:
			const std::string message;
		public:
			exception(const std::string& message) : message(message){}
			exception(std::string&& message) : message(std::move(message)){}
			const char* what() const throw() override{return message.c_str();}
	};

	// Video header informations storage
	struct VideoHeader{
		// Limited set of supported color types
		enum class ColorType : char{
			RGB,
			BGR,
			RGBA,
			BGRA
		}color_type;
		// Frame dimension expected as positive and in realistic range
		unsigned short width, height;
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
			// Size of image data to ProcessFrame function
			unsigned long image_size = 0;
		public:
			// Ctor
			Script();
			Script(const std::string& filename) throw(exception);
			Script(const std::string& filename, const VideoHeader header, const std::string& userdata) throw(exception);
			// No copy
			Script(const Script&) = delete;
			Script& operator=(const Script&) = delete;
			// Move
			Script(Script&& other);
			Script& operator=(Script&& other);
			// Setters
			void LoadFile(const std::string& filename) throw(exception);
			void SetVideo(const VideoHeader header);
			void SetUserdata(const std::string& userdata);
			// Processing
			void ProcessFrame(unsigned char* image_data, unsigned stride, unsigned long ms) throw(exception);
	};
}
