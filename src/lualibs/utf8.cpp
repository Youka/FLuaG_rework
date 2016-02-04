/*
Project: FLuaG
File: utf8.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "libs.h"
#include <vector>
#include <sstream>

// Helpers
static unsigned codepoint_size(const unsigned char c){
	if(c < 128)
		return 1;
	else if(c < 224)
		return 2;
	else if(c < 240)
		return 3;
	return 4;
}

static bool check_codepoint(const unsigned cp){
	const unsigned char* pcp = reinterpret_cast<const unsigned char*>(&cp);
	const unsigned cps = codepoint_size(*pcp);
        return !(
		(cps > 3 ? pcp[3] < 0x80 || pcp[3] > 0xbf : pcp[3] != 0) ||
		(cps > 2 ? pcp[2] < 0x80 || pcp[2] > 0xbf : pcp[2] != 0) ||
		(cps > 1 ? pcp[1] < 0x80 || pcp[1] > 0xbf : pcp[1] != 0) ||
		(*pcp > 0x7f && *pcp < 0xc2) || *pcp > 0xf4
	);
}

struct Codepoints{
	bool error;
	struct Codepoint{
		size_t byte_pos;
		unsigned value;
	};
	std::vector<Codepoint> items;
};
Codepoints codepoints(const std::string& s){
	// Output buffer
	Codepoints result{false};
	result.items.reserve(s.length() << 2);
	// Go through bytes
	unsigned cps;
	for(const unsigned char* pchar = reinterpret_cast<const unsigned char*>(s.data()), *pchar_end = pchar+s.length(); pchar != pchar_end; pchar += cps){
		cps = codepoint_size(*pchar);
		// Check valid byte sequence
		const size_t byte_pos = pchar - reinterpret_cast<const unsigned char*>(s.data());
		if(pchar+cps > pchar_end){
			result.error = true;
			result.items.push_back({byte_pos, 0});
			break;
		}
		// Add codepoint to output
		switch(cps){
			case 1: result.items.push_back({byte_pos, *pchar}); break;
			case 2: result.items.push_back({byte_pos, *reinterpret_cast<const unsigned short*>(pchar)}); break;
			case 3: result.items.push_back({byte_pos, *pchar + static_cast<const unsigned>(*reinterpret_cast<const unsigned short*>(pchar+1) << 8)}); break;
			case 4: result.items.push_back({byte_pos, *reinterpret_cast<const unsigned*>(pchar)}); break;
		}
		if(!check_codepoint(result.items.back().value)){
			result.error = true;
			break;
		}
	}
	// Return valid or invalid output
	return result;
}

// General functions
static int utf8_char(lua_State* L){
	std::ostringstream ss;
	const int n = lua_gettop(L);
	for(int i = 1; i <= n; ++i){
		const lua_Integer cp = luaL_checkinteger(L, i);
		if(!check_codepoint(cp))
			return luaL_argerror(L, i, "Invalid codepoint!");
		const char* pcp = reinterpret_cast<const char*>(&cp);
		ss.write(pcp, codepoint_size(*pcp));
	}
	lua_pushstring(L, ss.str().c_str());
	return 1;
}

static int utf8_codepoint(lua_State* L){
	// Get arguments
	const std::string s(luaL_checkstring(L, 1));
	const int i = luaL_optinteger(L, 2, 1),
		j = luaL_optinteger(L, 3, i);
	// Check arguments
	if(i < 1 || j < i)
		return luaL_error(L, "Invalid byte position!");
	// Extract codepoints
	const int oldtop = lua_gettop(L);
	if(i <= static_cast<int>(s.length())){
		auto cps = codepoints(s.substr(i-1, j-i+1));
		if(cps.error)
			return luaL_error(L, "Invalid byte sequence found!");
		for(auto cp : cps.items)
			lua_pushinteger(L, cp.value);
	}
	return lua_gettop(L) - oldtop;
}

static int utf8_codes(lua_State* L){

	// TODO

	return 0;
}

static int utf8_len(lua_State* L){

	// TODO

	return 0;
}

static int utf8_offset(lua_State* L){

	// TODO

	return 0;
}

int luaopen_utf8(lua_State* L){
	lua_createtable(L, 0, 6);
	lua_pushcfunction(L, utf8_char); lua_setfield(L, -2, "char");
	lua_pushstring(L, "[\\0-\\x7F\\xC2-\\xF4][\\x80-\\xBF]*"); lua_setfield(L, -2, "charpattern");
	lua_pushcfunction(L, utf8_codepoint); lua_setfield(L, -2, "codepoint");
	lua_pushcfunction(L, utf8_codes); lua_setfield(L, -2, "codes");
	lua_pushcfunction(L, utf8_len); lua_setfield(L, -2, "len");
	lua_pushcfunction(L, utf8_offset); lua_setfield(L, -2, "offset");
	return 1;
}
