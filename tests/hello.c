/*
Project: FLuaG
File: hello.c

Copyright (c) 2015-2016, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

/* FLuaG C API header */
#include "../src/interfaces/public.h"

/* Program entry */
int main(){
	/* Create FLuaG instance */
	fluag_h f = fluag_create();
	if(!f)
		return 1;
	/* Set userdata to instance */
	fluag_set_userdata(f, fluag_get_version());
	/* Load script to instance */
	if(!fluag_load_script(f, "print(string.format('Hello from FLuaG v%s!', ...))", 0)){
		fluag_destroy(f);
		return 2;
	}
	/* Destroy instance */
	fluag_destroy(f);
	/* Successful program end */
	return 0;
}
