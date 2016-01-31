/*
Project: FLuaG
File: textconv.hpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include <string>
#include <windef.h>
#include <Stringapiset.h>

namespace Utf8{
	// Utf-8 to utf-16 string conversion (native windows)
	inline std::wstring to_utf16(const std::string& s){
		std::wstring ws(MultiByteToWideChar(CP_UTF8, 0x0, s.data(), s.length(), NULL, 0), L'\0');
		MultiByteToWideChar(CP_UTF8, 0x0, s.data(), s.length(), const_cast<wchar_t*>(ws.data()), ws.length());
		return ws;
	}

	// Utf-16 to utf-8 string conversion (native windows)
	inline std::string from_utf16(const std::wstring& ws){
		std::string s(WideCharToMultiByte(CP_UTF8, 0x0, ws.data(), ws.length(), NULL, 0, NULL, NULL), '\0');
		WideCharToMultiByte(CP_UTF8, 0x0, ws.data(), ws.length(), const_cast<char*>(s.data()), s.length(), NULL, NULL);
		return s;
	}
}
