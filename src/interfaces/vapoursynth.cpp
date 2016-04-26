/*
Project: FLuaG
File: vapoursynth.cpp

Copyright (c) 2015-2016, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "frameservers/VapourSynth.h"

#include <config.h>
#include "../main/FLuaG.hpp"
#include "../utils/imageop.hpp"
#include "../utils/log.hpp"
#include <cassert>

namespace VS{
	// Filter instance data
	struct InstanceData{
		std::unique_ptr<VSNodeRef, std::function<void(VSNodeRef*)>> node;
		const VSVideoInfo *vi;
		FLuaG::Script F;
	};

	// Filter initialization
	void VS_CC init_filter(VSMap*, VSMap*, void** inst_data, VSNode* node, VSCore*, const VSAPI* vsapi) noexcept{
		LOG("Initialize Vapoursynth filter instance...");
		// Set output clip informations
		vsapi->setVideoInfo(static_cast<InstanceData*>(*inst_data)->vi, 1, node);
		LOG("Vapoursynth filter instance initialized!");
	}

	// Frame filtering
	const VSFrameRef* VS_CC get_frame(int n, int activationReason, void** inst_data, void**, VSFrameContext* frame_ctx, VSCore* core, const VSAPI* vsapi) noexcept{
		LOG("Process frame in Vapoursynth filter...");
		InstanceData* data = static_cast<InstanceData*>(*inst_data);
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
			assert(vsapi->getFrameWidth(dst.get(), 0) == data->vi->width && vsapi->getFrameHeight(dst.get(), 0) == data->vi->height && vsapi->getFrameFormat(dst.get())->id == data->vi->format->id);
			// Merge frame planes
			const bool has_alpha = data->vi->format->id == pfCompatBGR32;
			const size_t plane_size = data->vi->width * data->vi->height;
			const std::shared_ptr<unsigned char> fdata = has_alpha ?
				ImageOp::interlace_rgba(vsapi->getReadPtr(dst.get(), 0), vsapi->getReadPtr(dst.get(), 1), vsapi->getReadPtr(dst.get(), 2), vsapi->getReadPtr(dst.get(), 3), plane_size) :
				ImageOp::interlace_rgb(vsapi->getReadPtr(dst.get(), 2), vsapi->getReadPtr(dst.get(), 1), vsapi->getReadPtr(dst.get(), 0), plane_size);
			// Render on frame
			try{
				data->F.ProcessFrame(fdata.get(), -(has_alpha ? vsapi->getStride(dst.get(), 0) << 2 : vsapi->getStride(dst.get(), 0) * 3), n * (data->vi->fpsDen * 1000.0 / data->vi->fpsNum));
				// Unmerge frame planes
				if(has_alpha)
					ImageOp::deinterlace_rgba(fdata.get(), vsapi->getWritePtr(dst.get(), 0), vsapi->getWritePtr(dst.get(), 1), vsapi->getWritePtr(dst.get(), 2), vsapi->getWritePtr(dst.get(), 3), plane_size);
				else
					ImageOp::deinterlace_rgb(fdata.get(), vsapi->getWritePtr(dst.get(), 2), vsapi->getWritePtr(dst.get(), 1), vsapi->getWritePtr(dst.get(), 0), plane_size);
				// Return new frame
				return dst.release();
			}catch(const FLuaG::exception& e){
				vsapi->setFilterError(e.what(), frame_ctx);
			}
		}
		LOG("Finished frame processing of Vapoursynth filter!");
		return nullptr;
	}

	// Filter destruction
	void VS_CC free_filter(void* inst_data, VSCore*, const VSAPI*) noexcept{
		LOG("Free Vapoursynth filter instance...");
		delete static_cast<InstanceData*>(inst_data);
		LOG("Vapoursynth filter instance freed!");
	}

	// Filter creation
	void VS_CC apply_filter(const VSMap* in, VSMap* out, void*, VSCore* core, const VSAPI* vsapi) noexcept{
		LOG("Apply Vapoursynth filter function...");
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
		int err;
		const char* filename = vsapi->propGetData(in, "script", 0, nullptr),
			*userdata = vsapi->propGetData(in, "userdata", 0, &err);    // Error pointer required to surpress error raise even with optional parameter
		// Set userdata/script to clip
		try{
			assert(inst_data->vi->width >= 0 && inst_data->vi->height >= 0 && inst_data->vi->numFrames >= 0);
			inst_data->F.SetVideo({
				static_cast<unsigned short>(inst_data->vi->width),
				static_cast<unsigned short>(inst_data->vi->height),
				inst_data->vi->format->id == pfCompatBGR32,
				static_cast<double>(inst_data->vi->fpsNum) / inst_data->vi->fpsDen,
				static_cast<unsigned long>(inst_data->vi->numFrames)
			});
			if(userdata)
				inst_data->F.SetUserdata(userdata);
			inst_data->F.LoadFile(filename);
			// Create filter object and pass to frameserver
			vsapi->createFilter(in, out, PROJECT_NAME, init_filter, get_frame, free_filter, fmParallelRequests, 0, inst_data.release(), core);
		}catch(const std::bad_alloc){
			vsapi->setError(out, "Not enough memory!");
		}catch(const FLuaG::exception& e){
			vsapi->setError(out, e.what());
		}
		LOG("Vapoursynth filter function applied!");
	}
}

// Vapoursynth plugin entry point
VS_EXTERNAL_API(void) VapourSynthPluginInit(VSConfigPlugin config_func, VSRegisterFunction reg_func, VSPlugin* plugin) noexcept{
	LOG("Initialize Vapoursynth plugin...");
	// Write filter information to Vapoursynth configuration (identifier, namespace, description, vs version, is read-only, plugin storage)
	config_func("youka.graphics.fluag", "graphics", PROJECT_DESCRIPTION, VAPOURSYNTH_API_VERSION, 1, plugin);
	// Register filter to Vapoursynth with configuration in plugin storage (filter name, arguments, filter creation function, userdata, plugin storage)
	reg_func(PROJECT_NAME, "clip:clip;script:data;userdata:data:opt;", VS::apply_filter, nullptr, plugin);
	LOG("Vapoursynth plugin initialized!");
}
