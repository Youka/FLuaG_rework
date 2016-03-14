/*
Project: FLuaG
File: vapoursynth.c

Copyright (c) 2015-2016, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

/* Vapoursynth C API header */
#include "../src/interfaces/frameservers/VapourSynth.h"
/* Standard libraries headers */
#include <stdio.h>

/* System shared library extension */
#ifdef _WIN32
	#define SHARED_LIB_EXT ".dll"
#else
	#define SHARED_LIB_EXT ".so"
#endif

/* Program entry */
int main(const int argc, const char** argv){
	/* Declarations */
	const VSAPI* vsapi;
	VSCore* core;
	VSPlugin* plugin_std, *plugin_fluag;
	VSMap* in, *out;
	const char* error;
	VSNodeRef* node;
	int i, n;
	char frame_error[128];
	const VSFrameRef* frame;
	/* Check command line arguments */
	if(argc != 2){
		fputs("Script expected!", stderr);
		return 1;
	}
	/* Get global Vapoursynth API instance */
	vsapi = getVapourSynthAPI(VAPOURSYNTH_API_VERSION);
	if(!vsapi){
		fputs("Couldn't create Vapoursynth API!", stderr);
		return 2;
	}
	/* Create Vapoursynth core instance */
	core = vsapi->createCore(0);
	/* Get 'std' plugin */
	plugin_std = vsapi->getPluginById("com.vapoursynth.std", core);
	if(!plugin_std){
		vsapi->freeCore(core);
		fputs("Couldn't get 'std' plugin!", stderr);
		return 3;
	}
	/* Load FLuaG plugin */
	in = vsapi->createMap();
	vsapi->propSetData(in, "path", "libfluag"SHARED_LIB_EXT, -1, paReplace);
	out = vsapi->invoke(plugin_std, "LoadPlugin", in);
	error = vsapi->getError(out);
	if(error){
		vsapi->freeMap(in);
		vsapi->freeCore(core);
		fputs(error, stderr);
		vsapi->freeMap(out);
		return 4;
	}
	/* Get FLuaG plugin */
	plugin_fluag = vsapi->getPluginById("youka.graphics.fluag", core);
	if(!plugin_fluag){
		vsapi->freeMap(out);
		vsapi->freeMap(in);
		vsapi->freeCore(core);
		fputs("Couldn't get FLuaG plugin!", stderr);
		return 5;
	}
	/* Create blank (yellow) clip */
	vsapi->clearMap(in);
	vsapi->propSetFloat(in, "color", 255, paReplace);
	vsapi->propSetFloat(in, "color", 255, paAppend);
	vsapi->propSetFloat(in, "color", 0, paAppend);
	vsapi->freeMap(out);
	out = vsapi->invoke(plugin_std, "BlankClip", in);
	error = vsapi->getError(out);
	if(error){
		vsapi->freeMap(in);
		vsapi->freeCore(core);
		fputs(error, stderr);
		vsapi->freeMap(out);
		return 6;
	}
	/* Call FLuaG */
	vsapi->propSetData(out, "script", argv[1], -1, paReplace);
	vsapi->freeMap(in);
	in = vsapi->invoke(plugin_fluag, "FLuaG", out);
	error = vsapi->getError(in);
	if(error){
		vsapi->freeMap(out);
		vsapi->freeCore(core);
		fputs(error, stderr);
		vsapi->freeMap(in);
		return 7;
	}
	/* Trigger frames processing */
	node = vsapi->propGetNode(in, "clip", 0, 0);
	for(i = 0, n = vsapi->getVideoInfo(node)->numFrames; i < n; ++i){
		frame = vsapi->getFrame(i, node, frame_error, sizeof(frame_error));
		if(!frame){
			vsapi->freeNode(node);
			vsapi->freeMap(out);
			vsapi->freeMap(in);
			vsapi->freeCore(core);
			fputs(frame_error, stderr);
			return 8;
		}
		vsapi->freeFrame(frame);
	}
	/* Free resources */
	vsapi->freeNode(node);
	vsapi->freeMap(out);
	vsapi->freeMap(in);
	vsapi->freeCore(core);
	/* Successful program end */
	return 0;
}
