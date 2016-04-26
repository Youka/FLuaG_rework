/*
Project: FLuaG
File: standalone.c

Copyright (c) 2015-2016, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

/* FLuaG C API header */
#include "../src/interfaces/public.h"
/* Standard libraries headers */
#include <string.h>
#include <stdio.h>

/* Program entry */
int main(const int argc, const char** argv){
	/* Declarations */
	fluag_h f;
	int i;
	unsigned width, height, has_alpha, frames;
	float fps;
	char warning[FLUAG_ERROR_LENGTH];
	/* Create FLuaG instance */
	f = fluag_create();
	if(!f){
		fputs("Couldn't create FLuaG instance!", stderr);
		return 1;
	}
	/* Evaluate command line arguments and send to instance */
	for(i = 1; i < argc; ++i)
		/* Userdata */
		if(strncmp(argv[i], "--arg=", 6) == 0)
			fluag_set_userdata(f, argv[i]+6);
		/* Video informations */
		else if(strncmp(argv[i], "--video=", 8) == 0){
			if(sscanf(argv[i]+8, "%u,%u,%u,%f,%u", &width, &height, &has_alpha, &fps, &frames) != 5){
				fluag_destroy(f);
				fputs("Invalid video argument format!", stderr);
				return 2;
			}
			fluag_set_video(f, width, height, has_alpha, fps, frames);
		/* File */
		}else if(!fluag_load_file(f, argv[i], warning)){
			fluag_destroy(f);
			fputs(warning, stderr);
			return 3;
		}
	/* Destroy instance */
	fluag_destroy(f);
	/* Successful program end */
	return 0;
}
