/*
Project: FLuaG
File: module.cpp

Copyright (c) 2015-2016, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "module.hpp"
#ifdef _WIN32
	#include <libloaderapi.h>
	#include "textconv.hpp"
#else
	#include <dlfcn.h>
#endif

namespace Module{
	std::string dir() noexcept{
		// Result buffer
		std::string path;
		// Get this DLL filename into the buffer
		union{
			std::string(*func)();
			void* obj;
		}cast_wrapper;
		cast_wrapper.func = dir;	// Prevents "warning: ISO C++ forbids casting between pointer-to-function and pointer-to-object"
#ifdef _WIN32
		HMODULE module;
		if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, reinterpret_cast<LPCWSTR>(cast_wrapper.obj), &module)){
			wchar_t filenamew[MAX_PATH];
			if(GetModuleFileNameW(module, filenamew, sizeof(filenamew)/sizeof(filenamew[0])))
				path = Utf8::from_utf16(filenamew);
		}
#else
		Dl_info dli;
		if(dladdr(cast_wrapper.obj, &dli))
			path = dli.dli_fname;
#endif
		// Shorten filename to directory
		if(!path.empty()){
			const std::string::size_type separator = path.find_last_of("\\/");
			path.resize(separator != std::string::npos ? separator+1 : 0);
		}
		// Return directory or empty string on failure
		return path;
	}
}
