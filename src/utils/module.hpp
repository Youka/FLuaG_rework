/*
Project: FLuaG
File: module.hpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include <string>
#ifdef _WIN32
	#include <libloaderapi.h>
	#include "textconv.hpp"
#else
	#include <dlfcn.h>
#endif

namespace Module{
#ifdef _WIN32	// Unix needs the following function exported for DL informations
	static
#endif
	std::string dir(){
		// Result buffer
		std::string path;
		// Get this DLL filename into the buffer
#ifdef _WIN32
		HMODULE module;
		if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, reinterpret_cast<LPCWSTR>(dir), &module)){
			wchar_t filenamew[MAX_PATH];
			if(GetModuleFileNameW(module, filenamew, sizeof(filenamew)/sizeof(filenamew[0])))
				path = Utf8::from_utf16(filenamew);
		}
#else
		Dl_info dli;
		if(dladdr(dir, &dli))
			path = dli.dli_fname;
#endif
		// Shorten filename to directory
		if(!path.empty()){
			const std::string::size_type separator = path.find_last_of("\\/");
			if(separator != std::wstring::npos)
				path.resize(separator+1);
			else
				path.clear();
		}
		// Return directory or empty string on failure
		return path;
	}
}
