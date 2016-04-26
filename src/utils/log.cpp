/*
Project: FLuaG
File: log.cpp

Copyright (c) 2015-2016, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#ifdef LOG_ENABLED
#include "log.hpp"
#include <exception>
#include <mutex>
#include <memory>
#include <fstream>
#include <iostream>

namespace Logging{
	// Simple local exception for log access failure
	class exception : public std::exception{
		private:
			const std::string message;
		public:
			exception(const std::string& message) : message(message){}
			exception(std::string&& message) : message(std::move(message)){}
			const char* what() const noexcept override{return message.c_str();}
	};

	// Write single C string into log
	void log(const std::string& s){
		// Lock for thread-safety
		static std::mutex mut;
		const std::unique_lock<std::mutex> lock(mut);
		// Initialize logging target
		static std::unique_ptr<std::ostream> stream;
		if(!stream){
			const char* const log_target = getenv("LOG_TARGET");
			if(log_target){
				stream.reset(new std::ofstream(log_target));
				if(!stream->good())
					throw exception("Couldn't access log target \"" + std::string(log_target) + "\"!");
				stream->rdbuf()->pubsetbuf(nullptr, 0);
			}else
				stream.reset(new std::ostream(std::cerr.rdbuf()));
		}
		// Write to log
		(*stream) << s << '\n';
	}
}
#endif
