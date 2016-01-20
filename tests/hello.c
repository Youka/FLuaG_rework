// DLL functions
#include "dl.h"

// Program entry
int main(const int argc, const char** argv){
	// Check for input
	if(argc != 2)
		return 1;
	// Open plugin handle
	DLL_HANDLE plugin = DLL_OPEN(argv[1]);
	if(!plugin)
		return 2;
	// Get plugin functions
	typedef void* fluag_h;
	fluag_h(*fluag_create)(void);
	int(*fluag_load_script)(fluag_h, const char*, char*);
	void(*fluag_set_userdata)(fluag_h, const char*);
	void(*fluag_destroy)(fluag_h);
	const char*(*fluag_get_version)(void);
#ifdef __GNUC__
	#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
#endif
	if(!(
		DLL_ASSIGN_PROC(plugin, fluag_create) &&
		DLL_ASSIGN_PROC(plugin, fluag_load_script) &&
		DLL_ASSIGN_PROC(plugin, fluag_set_userdata) &&
		DLL_ASSIGN_PROC(plugin, fluag_destroy) &&
		DLL_ASSIGN_PROC(plugin, fluag_get_version)
	)){
		DLL_CLOSE(plugin);
		return 3;
	}
	// Create FLuaG instance
	fluag_h f = fluag_create();
	if(!f){
		DLL_CLOSE(plugin);
		return 4;
	}
	// Set userdata to instance
	fluag_set_userdata(f, fluag_get_version());
	// Load script to instance
	if(!fluag_load_script(f, "print(string.format('Hello from FLuaG v%s!', ...))", 0)){
		fluag_destroy(f);
		DLL_CLOSE(plugin);
		return 5;
	}
	// Destroy instance
	fluag_destroy(f);
	// Close plugin handle
	DLL_CLOSE(plugin);
	// Successful program end
	return 0;
}
