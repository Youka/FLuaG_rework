/*
Project: FLuaG
File: font.hpp

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
#include <vector>
#ifdef _WIN32
	#include "../utils/textconv.hpp"
	#include <wingdi.h>
#else
	#include <fontconfig/fontconfig.h>
	#include <memory>
#endif

namespace Font{
	// Simple local exception
	class exception : public std::exception{
		private:
			const std::string message;
		public:
			exception(const std::string& message) : message(message){}
			exception(std::string&& message) : message(std::move(message)){}
			const char* what() const throw() override{return message.c_str();}
	};

	// Get list of installed fonts
	struct ListEntry{
		std::string family, style, file, script;
		bool outline;
	};
#ifdef _WIN32
	static int CALLBACK enumfontcallback(const LOGFONTW* lf, const TEXTMETRICW*, const DWORD fonttype, LPARAM lParam){
		// Add font properties to list output
		reinterpret_cast<std::vector<ListEntry>*>(lParam)->push_back({
			Utf8::from_utf16(lf->lfFaceName),
			Utf8::from_utf16(reinterpret_cast<const ENUMLOGFONTEXW*>(lf)->elfStyle),
			"",
			Utf8::from_utf16(reinterpret_cast<const ENUMLOGFONTEXW*>(lf)->elfScript),
			static_cast<bool>(fonttype & TRUETYPE_FONTTYPE)
		});
		// Continue font enumeration until end
		return 1;
	}
#endif
	inline std::vector<ListEntry> list() throw(exception){
		std::vector<ListEntry> result;
#ifdef _WIN32
		const HDC hdc = CreateCompatibleDC(NULL);
		if(!hdc)
			throw exception("Couldn't create device context!");
		LOGFONTW lf = {0};
		lf.lfCharSet = DEFAULT_CHARSET;
		EnumFontFamiliesExW(hdc, &lf, enumfontcallback, reinterpret_cast<LPARAM>(&result), 0);
		DeleteDC(hdc);
#else
		// Get font list from FontConfig
		FcConfig* fc = FcInitLoadConfigAndFonts();
		std::unique_ptr<FcPattern, void(*)(FcPattern*)> pattern(FcPatternCreate(), FcPatternDestroy);
		std::unique_ptr<FcObjectSet, void(*)(FcObjectSet*)> objset(FcObjectSetBuild(FC_FAMILY, FC_STYLE, FC_FILE, FC_OUTLINE, nullptr), FcObjectSetDestroy);
		std::unique_ptr<FcFontSet, void(*)(FcFontSet*)> fontlist(FcFontList(fc, pattern.get(), objset.get()), FcFontSetDestroy);
		if(!fontlist)
			throw exception("Couldn't create font list!");
		// Add font list to output
		result.reserve(fontlist->nfont);
		for(int i = 0; i < fontlist->nfont; ++i){
			const FcPattern* font = fontlist->fonts[i];
			FcChar8* sresult; FcBool bresult;
			ListEntry entry;
			FcPatternGetString(font, FC_FAMILY, 0, &sresult); entry.family = reinterpret_cast<char*>(sresult);
			FcPatternGetString(font, FC_STYLE, 0, &sresult); entry.style = reinterpret_cast<char*>(sresult);
			FcPatternGetString(font, FC_FILE, 0, &sresult); entry.file = reinterpret_cast<char*>(sresult);
			FcPatternGetBool(font, FC_OUTLINE, 0, &bresult); entry.outline = bresult;
			result.push_back(std::move(entry));
		}
#endif
		return result;
	}
}
