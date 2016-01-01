/*
Project: FLuaG
File: vapoursynth.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "interfaces/VapourSynth.h"

#include <config.h>
#include "FLuaG.hpp"
#include <memory>

namespace VS{
	// Filter instance data
	struct InstanceData{
		std::unique_ptr<VSNodeRef, std::function<void(VSNodeRef*)>> node;
		const VSVideoInfo *vi;
		FLuaG::Script F;
	};

	// Filter initialization
	void VS_CC init_filter(VSMap*, VSMap*, void** inst_data, VSNode* node, VSCore*, const VSAPI* vsapi){
		// Set output clip informations
		vsapi->setVideoInfo(reinterpret_cast<InstanceData*>(*inst_data)->vi, 1, node);
	}

	// Frame filtering
	const VSFrameRef* VS_CC get_frame(int n, int activationReason, void** inst_data, void**, VSFrameContext* frame_ctx, VSCore* core, const VSAPI* vsapi){
		InstanceData* data = reinterpret_cast<InstanceData*>(*inst_data);
		// Frame creation
		if(activationReason == arInitial)
			// Request needed input frames
			vsapi->requestFrameFilter(n, data->node.get(), frame_ctx);
		// Frame processing
		else if (activationReason == arAllFramesReady){
			// Create new frame
			const VSFrameRef* src = vsapi->getFrameFilter(n, data->node.get(), frame_ctx);
			std::unique_ptr<VSFrameRef, std::function<void(VSFrameRef*)>> dst(vsapi->copyFrame(src, core), [vsapi](VSFrameRef* frame){vsapi->freeFrame(frame);});
			vsapi->freeFrame(src);
			// Render on frame
			try{
				data->F.ProcessFrame(vsapi->getWritePtr(dst.get(), 0), vsapi->getStride(dst.get(), 0), n * (data->vi->fpsDen * 1000.0 / data->vi->fpsNum));
				// Return new frame
				return dst.release();
			}catch(FLuaG::exception e){
				vsapi->setFilterError(e.what(), frame_ctx);
			}
		}
		return nullptr;
	}

	// Filter destruction
	void VS_CC free_filter(void* inst_data, VSCore*, const VSAPI*){
		delete reinterpret_cast<InstanceData*>(inst_data);
	}

	// Filter creation
	void VS_CC apply_filter(const VSMap* in, VSMap* out, void*, VSCore* core, const VSAPI* vsapi){
		// Allocate instance data storage & extract clip into it
		std::unique_ptr<InstanceData> inst_data(new InstanceData{{vsapi->propGetNode(in, "clip", 0, nullptr), [vsapi](VSNodeRef* node){vsapi->freeNode(node);}}});
		inst_data->vi = vsapi->getVideoInfo(inst_data->node.get());
		// Check clip/video
		if(inst_data->vi->width == 0 || inst_data->vi->height == 0){
			vsapi->setError(out, "Video required!");
			return;
		}
		if(inst_data->vi->format->id != pfRGB24 && inst_data->vi->format->id != pfCompatBGR32){
			vsapi->setError(out, "Video colorspace must be RGB!");
			return;
		}
		// Exract further filter arguments
		const char* filename = vsapi->propGetData(in, "script", 0, nullptr),
			*userdata = vsapi->propGetData(in, "userdata", 0, nullptr);
		// Set userdata/script to clip
		try{
			inst_data->F.SetVideo({
				inst_data->vi->format->id == pfRGB24 ? FLuaG::VideoHeader::ColorType::BGR : FLuaG::VideoHeader::ColorType::BGRA,
				static_cast<unsigned short>(inst_data->vi->width),
				static_cast<unsigned short>(inst_data->vi->height),
				static_cast<double>(inst_data->vi->fpsNum) / inst_data->vi->fpsDen,
				static_cast<unsigned long>(inst_data->vi->numFrames)
			});
			if(userdata)
				inst_data->F.SetUserdata(userdata);
			inst_data->F.LoadFile(filename);
			// Create filter object and pass to frameserver
			vsapi->createFilter(in, out, PROJECT_NAME, init_filter, get_frame, free_filter, fmParallel, 0, inst_data.release(), core);
		}catch(FLuaG::exception e){
			vsapi->setError(out, e.what());
		}
	}
}

// Vapoursynth plugin entry point
VS_EXTERNAL_API(void) VapourSynthPluginInit(VSConfigPlugin config_func, VSRegisterFunction reg_func, VSPlugin* plugin){
	// Write filter information to Vapoursynth configuration (identifier, namespace, description, vs version, is read-only, plugin storage)
	config_func("youka.graphics.fluag", "graphics", PROJECT_DESCRIPTION, VAPOURSYNTH_API_VERSION, 1, plugin);
	// Register filter to Vapoursynth with configuration in plugin storage (filter name, arguments, filter creation function, userdata, plugin storage)
	reg_func(PROJECT_NAME, "clip:clip;script:data;userdata:data:opt;", VS::apply_filter, nullptr, plugin);
}
