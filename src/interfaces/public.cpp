/*
Project: FLuaG
File: public.c

Copyright (c) 2015-2016, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "public.h"
#include "../main/FLuaG.hpp"
#include <config.h>
#include <cstring>

fluag_h fluag_create(void){
	try{
		return new FLuaG::Script();
	}catch(const std::bad_alloc){
		return 0;
	}
}

int fluag_load_file(fluag_h F, const char* filename, char* warning){
	if(F)
		try{
			static_cast<FLuaG::Script*>(F)->LoadFile(filename);
		}catch(const FLuaG::exception& e){
			if(warning)
				strncpy(warning, e.what(), FLUAG_WARNING_LENGTH-1)[FLUAG_WARNING_LENGTH-1] = '\0';
			return 0;
		}
	return 1;
}

int fluag_load_script(fluag_h F, const char* script, char* warning){
	if(F)
		try{
			static_cast<FLuaG::Script*>(F)->LoadScript(script);
		}catch(const FLuaG::exception& e){
			if(warning)
				strncpy(warning, e.what(), FLUAG_WARNING_LENGTH-1)[FLUAG_WARNING_LENGTH-1] = '\0';
			return 0;
		}
	return 1;
}

void fluag_set_video(fluag_h F, unsigned short width, unsigned short height, char has_alpha, double fps, unsigned long frames){
	if(F)
		static_cast<FLuaG::Script*>(F)->SetVideo({width, height, static_cast<bool>(has_alpha), fps, frames});
}

void fluag_set_userdata(fluag_h F, const char* userdata){
	if(F)
		static_cast<FLuaG::Script*>(F)->SetUserdata(userdata);
}

int fluag_process_frame(fluag_h F, unsigned char* image_data, unsigned stride, unsigned long ms, char* warning){
	if(F)
		try{
			static_cast<FLuaG::Script*>(F)->ProcessFrame(image_data, stride, ms);
		}catch(const FLuaG::exception& e){
			if(warning)
				strncpy(warning, e.what(), FLUAG_WARNING_LENGTH-1)[FLUAG_WARNING_LENGTH-1] = '\0';
			return 0;
		}
	return 1;
}

void fluag_destroy(fluag_h F){
	if(F)
		delete static_cast<FLuaG::Script*>(F);
}

const char* fluag_get_version(void){
	return PROJECT_VERSION_STRING;
}
