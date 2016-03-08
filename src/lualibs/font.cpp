/*
Project: FLuaG
File: font.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "libs.h"
#ifdef _WIN32
	#include "../utils/lua.h"
	#include "../utils/textconv.hpp"
	#include <wingdi.h>
#else
	#include <fontconfig/fontconfig.h>
	#include <memory>
#endif

#ifdef _WIN32
static int CALLBACK enumfontcallback(const LOGFONTW* lf, const TEXTMETRICW*, const DWORD fonttype, LPARAM lParam){
	// Push font properties to Lua table
	lua_State* L = reinterpret_cast<lua_State*>(lParam);
	lua_createtable(L, 0, 4);
	lua_pushstring(L, Utf8::from_utf16(lf->lfFaceName).c_str()); lua_setfield(L, -2, "family");
	lua_pushstring(L, Utf8::from_utf16(reinterpret_cast<const ENUMLOGFONTEXW*>(lf)->elfStyle).c_str()); lua_setfield(L, -2, "style");
	lua_pushboolean(L, fonttype & TRUETYPE_FONTTYPE); lua_setfield(L, -2, "outline");
	lua_pushstring(L, Utf8::from_utf16(reinterpret_cast<const ENUMLOGFONTEXW*>(lf)->elfScript).c_str()); lua_setfield(L, -2, "script");
	lua_rawseti(L, -2, 1+lua_rawlen(L, -2));
	// Continue font enumeration until end
	return 1;
}
#endif

static int font_list(lua_State* L){
#ifdef _WIN32
	lua_newtable(L);
	const HDC hdc = CreateCompatibleDC(NULL);
	LOGFONTW lf = {0};
	lf.lfCharSet = DEFAULT_CHARSET;
	EnumFontFamiliesExW(hdc, &lf, enumfontcallback, reinterpret_cast<LPARAM>(L), 0);
	DeleteDC(hdc);
	return 1;
#else
	// Get font list from FontConfig
	FcConfig* fc = FcInitLoadConfigAndFonts();
	std::unique_ptr<FcPattern, void(*)(FcPattern*)> pattern(FcPatternCreate(), FcPatternDestroy);
	std::unique_ptr<FcObjectSet, void(*)(FcObjectSet*)> objset(FcObjectSetBuild(FC_FAMILY, FC_STYLE, FC_FILE, FC_OUTLINE, nullptr), FcObjectSetDestroy);
	std::unique_ptr<FcFontSet, void(*)(FcFontSet*)> fontlist(FcFontList(fc, pattern.get(), objset.get()), FcFontSetDestroy);
	// Send font properties to Lua
	if(fontlist){
		lua_createtable(L, fontlist->nfont, 0);
		for(int i = 0; i < fontlist->nfont; ++i){
			lua_createtable(L, 0, 4);
			const FcPattern* font = fontlist->fonts[i];
			FcChar8* sresult; FcBool bresult;
			FcPatternGetString(font, FC_FAMILY, 0, &sresult); lua_pushstring(L, reinterpret_cast<char*>(sresult)); lua_setfield(L, -2, "family");
			FcPatternGetString(font, FC_STYLE, 0, &sresult); lua_pushstring(L, reinterpret_cast<char*>(sresult)); lua_setfield(L, -2, "style");
			FcPatternGetString(font, FC_FILE, 0, &sresult); lua_pushstring(L, reinterpret_cast<char*>(sresult)); lua_setfield(L, -2, "file");
			FcPatternGetBool(font, FC_OUTLINE, 0, &bresult); lua_pushboolean(L, bresult); lua_setfield(L, -2, "outline");
			lua_rawseti(L, -2, 1+i);
		}
		return 1;
	}else
		return 0;
#endif
}

int luaopen_font(lua_State* L){
	static const luaL_Reg l[] = {
		{"list", font_list},
		{NULL, NULL}
	};
	luaL_newlib(L, l);
	return 1;
}
