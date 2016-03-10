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
