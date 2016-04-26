/*
Project: FLuaG
File: public.h

Copyright (c) 2015-2016, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#pragma once

#ifdef __cplusplus
	#define EXTERN_C extern "C"
#else
	#define EXTERN_C
#endif

#ifdef _WIN32
	#ifdef fluag_EXPORTS	/* Set by CMake for shared libraries (libname_EXPORTS) */
		#define FLUAG_EXPORT EXTERN_C __declspec(dllexport)
	#else
		#define FLUAG_EXPORT EXTERN_C __declspec(dllimport)
	#endif
#else
	#define FLUAG_EXPORT EXTERN_C
#endif

/** FLuaG script handle type */
typedef void* fluag_h;

/** Maximal length for output error */
#define FLUAG_ERROR_LENGTH 256

/**
Create FLuaG script handle.

@return Script handle or zero
*/
FLUAG_EXPORT fluag_h fluag_create(void);

/**
Load file into FLuaG script.

@param F Script handle
@param filename Path to file for loading
@param err Error string storage, can be zero
@return 1 if success, 0 if error (see err)
*/
FLUAG_EXPORT int fluag_load_file(fluag_h F, const char* const filename, char* err);

/**
Load script into FLuaG script.

@param F Script handle
@param script Script content as string
@param err Error string storage, can be zero
@return 1 if success, 0 if error (see err)
*/
FLUAG_EXPORT int fluag_load_script(fluag_h F, const char* const script, char* err);

/**
Set video informations into FLuaG script.

@param F Script handle
@param width Video width
@param height Video height
@param has_alpha Video has alpha channel?
@param fps Video frames-per-second
@param frames Video frames number
*/
FLUAG_EXPORT void fluag_set_video(fluag_h F, const unsigned short width, const unsigned short height, const char has_alpha, const double fps, const unsigned long frames);

/**
Set userdata into FLuaG script.

@param F Script handle
@param userdata Userdata string
*/
FLUAG_EXPORT void fluag_set_userdata(fluag_h F, const char* const userdata);

/**
Send frame into FLuaG script.

@param F Script handle
@param image_data Pixels data of image
@param stride Image row size in bytes (pixels + padding)
@param ms Image/frame time in milliseconds
@param err Error string storage, can be zero
@return 1 if success, 0 if error (see err)
*/
FLUAG_EXPORT int fluag_process_frame(fluag_h F, unsigned char* image_data, const unsigned stride, const unsigned long ms, char* err);

/**
Destroy FLuaG script handle.

@param F Script handle
*/
FLUAG_EXPORT void fluag_destroy(fluag_h F);

/**
Get FLuaG version.

@return Version string
*/
FLUAG_EXPORT const char* fluag_get_version(void);
