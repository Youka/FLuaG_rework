#pragma once

#ifdef _WIN32
	#include <windows.h>
	#define DLL_HANDLE HMODULE
	#define DLL_OPEN LoadLibrary
	#define DLL_GET_PROC GetProcAddress
	#define DLL_CLOSE FreeLibrary
#else
	#include <dlfcn.h>
	#define DLL_HANDLE void*
	#define DLL_OPEN(libname) dlopen(libname, RTLD_LAZY)
	#define DLL_GET_PROC dlsym
	#define DLL_CLOSE dlclose
#endif
#define DLL_ASSIGN_PROC(dll_handle, funcname) (funcname = DLL_GET_PROC(dll_handle, #funcname))
