/*
Project: FLuaG
File: avisynth.cpp

Copyright (c) 2015-2016, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

// Link Avisynth statically
#define AVSC_NO_DECLSPEC
#include <windows.h>
#include "frameservers/avisynth_c.h"

#include <config.h>
#include "../main/FLuaG.hpp"
#include "../utils/textconv.hpp"
#include "../utils/log.hpp"
#include <cassert>

namespace AVS{
	// Avisynth library handle (defined in plugin initialization)
	AVS_Library* avs_library = nullptr;

	// Filter finished
	void AVSC_CC free_filter(AVS_FilterInfo* filter_info) noexcept{
		LOG("Free Avisynth filter instance...");
		delete static_cast<FLuaG::Script*>(filter_info->user_data);
		LOG("Avisynth filter instance freed!");
	}

	// Frame filtering
	AVS_VideoFrame* AVSC_CC get_frame(AVS_FilterInfo* filter_info, int n) noexcept{
		LOG("Process frame in Avisynth filter...");
		// Get current frame
		AVS_VideoFrame* frame = avs_library->avs_get_frame(filter_info->child, n);
		assert(avs_get_row_size(frame) == filter_info->vi.width * (avs_is_rgb32(&filter_info->vi) ? 4 : 3) && avs_get_height(frame) == filter_info->vi.height);
		// Make frame writable
		avs_library->avs_make_writable(filter_info->env, &frame);
		// Render on frame
		try{
			static_cast<FLuaG::Script*>(filter_info->user_data)->ProcessFrame(avs_get_write_ptr(frame), avs_get_pitch(frame), n * (filter_info->vi.fps_denominator * 1000.0 / filter_info->vi.fps_numerator));
		}catch(const FLuaG::exception& e){
			filter_info->error = avs_library->avs_save_string(filter_info->env, e.what(), -1);
			// Because the AVS C API error handling is broken
			MessageBoxW(NULL, Utf8::to_utf16(e.what()).c_str(), L"FLuaG error", MB_OK | MB_ICONERROR);
		}
		LOG("Finished frame processing of Avisynth filter!");
		// Pass frame further in processing chain
		return frame;
	}

	// Filter call
	AVS_Value AVSC_CC apply_filter(AVS_ScriptEnvironment* env, AVS_Value args, void*) noexcept{
		LOG("Apply Avisynth filter function...");
		// Extract clip
		AVS_FilterInfo* filter_info;
		std::unique_ptr<AVS_Clip, void(*)(AVS_Clip*)> clip(avs_library->avs_new_c_filter(env, &filter_info, avs_array_elt(args, 0), 1), [](AVS_Clip* clip){avs_library->avs_release_clip(clip);});
		// Check clip/video
		AVS_VideoInfo* vinfo = &filter_info->vi;
		if(!avs_has_video(vinfo))	// Clip must have a video stream
			return avs_new_value_error("Video required!");
		else if(!avs_is_rgb(vinfo))	// Video must store colors in RGB24 or RGBA32 format
			return avs_new_value_error("Video colorspace must be RGB!");
		// Exract further filter arguments
		const char* filename = avs_as_string(avs_array_elt(args, 1)),
			*userdata = avs_array_size(args) > 2 ? avs_as_string(avs_array_elt(args, 2)) : nullptr;
		// Set userdata/script to clip
		try{
			std::unique_ptr<FLuaG::Script> F(new FLuaG::Script());
			assert(vinfo->width >= 0 && vinfo->height >= 0 && vinfo->num_frames >= 0);
			F->SetVideo({
				static_cast<unsigned short>(vinfo->width),
				static_cast<unsigned short>(vinfo->height),
				static_cast<bool>(avs_is_rgb32(vinfo)),
				static_cast<double>(vinfo->fps_numerator) / vinfo->fps_denominator,
				static_cast<unsigned long>(vinfo->num_frames)
			});
			if(userdata)
				F->SetUserdata(userdata);
			F->LoadFile(filename);
			filter_info->user_data = F.release();
		}catch(const std::bad_alloc){
			return avs_new_value_error("Not enough memory!");
		}catch(const FLuaG::exception& e){
			return avs_new_value_error(avs_library->avs_save_string(env, e.what(), -1));
		}
		// Set filter callbacks to clip
		filter_info->get_frame = get_frame;
		filter_info->free_filter = free_filter;
		// Return filtered clip
		AVS_Value out_val;
		avs_library->avs_set_to_clip(&out_val, clip.get());
		LOG("Avisynth filter function applied!");
		return out_val;
	}
}

// Avisynth plugin entry point
AVSC_EXPORT const char* avisynth_c_plugin_init(AVS_ScriptEnvironment* env){
	LOG("Initialize Avisynth plugin...");
	// Avisynth library available and valid version?
	if((AVS::avs_library || (AVS::avs_library = avs_load_library())) && !AVS::avs_library->avs_check_version(env, AVISYNTH_INTERFACE_VERSION))
		// Register function to Avisynth scripting environment
		AVS::avs_library->avs_add_function(env, PROJECT_NAME, "cs[userdata]s", AVS::apply_filter, nullptr);
	LOG("Avisynth plugin initialized!");
	// Return plugin description
	return PROJECT_DESCRIPTION;
}
