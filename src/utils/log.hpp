/*
Project: FLuaG
File: log.hpp

Copyright (c) 2015-2016, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#pragma once

#ifdef LOG_ENABLED
#include <sstream>

namespace Logging{
	void log(const std::string&);
	template<typename ...Args>
	inline void logn(const Args&... args){
		std::ostringstream ostr;
		const char dummy[]{'\0'/* Surpress 'zero-size' error */, (ostr << args, '\0')...};
		static_cast<void>(dummy);	// Surpress 'unused' warning
		log(ostr.str());
	}
}

	#define LOG(...) Logging::logn(__FILE__, ',', __LINE__, ':', __VA_ARGS__)
#else
	#define LOG(...) static_cast<void>(0)
#endif

