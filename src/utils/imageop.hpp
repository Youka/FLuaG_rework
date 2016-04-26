/*
Project: FLuaG
File: imageop.hpp

Copyright (c) 2015-2016, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include <memory>
#include <algorithm>

namespace ImageOp{
	// Copy data rows with different strides and optional vertical flipping
	inline void copy(const unsigned char* src_data, unsigned char* dst_data, const unsigned height, const unsigned src_stride, const unsigned dst_stride, const bool flip = false){
		if(!flip && src_stride == dst_stride)
			std::copy(src_data, src_data + height * src_stride, dst_data);
		else{
			if(flip) src_data += (height-1) * src_stride;
			const int src_stride_i = flip ? -src_stride : src_stride;
			const unsigned min_stride = std::min(src_stride, dst_stride);
			for(const unsigned char* const dst_data_end = dst_data + height * dst_stride; dst_data != dst_data_end; src_data += src_stride_i, dst_data += dst_stride)
				std::copy(src_data, src_data + min_stride, dst_data);
		}
	}

	// Interlace RGB(A) planes
	inline std::shared_ptr<unsigned char> interlace_rgb(const unsigned char* r, const unsigned char* g, const unsigned char* b, const size_t plane_size){
		const size_t data_size = plane_size * 3;
		std::shared_ptr<unsigned char> data(new unsigned char[data_size]);
		const unsigned char* const data_end = data.get() + data_size;
		for(
			unsigned char* pdata = data.get();
			pdata != data_end;
			*pdata++ = *r++, *pdata++ = *g++, *pdata++ = *b++
		);
		return data;
	}
	inline std::shared_ptr<unsigned char> interlace_rgba(const unsigned char* r, const unsigned char* g, const unsigned char* b, const unsigned char* a, const size_t plane_size){
		const size_t data_size = plane_size << 2;
		std::shared_ptr<unsigned char> data(new unsigned char[data_size]);
		const unsigned char* const data_end = data.get() + data_size;
		for(
			unsigned char* pdata = data.get();
			pdata != data_end;
			*pdata++ = *r++, *pdata++ = *g++, *pdata++ = *b++, *pdata++ = *a++
		);
		return data;
	}

	// Deinterlace RGB(A) planes
	inline void deinterlace_rgb(const unsigned char* data, unsigned char* r, unsigned char* g, unsigned char* b, const size_t plane_size) noexcept{
		for(
			const unsigned char* const data_end = data + plane_size * 3;
			data != data_end;
			*r++ = *data++, *g++ = *data++, *b++ = *data++
		);
	}
	inline void deinterlace_rgba(const unsigned char* data, unsigned char* r, unsigned char* g, unsigned char* b, unsigned char* a, const size_t plane_size) noexcept{
		for(
			const unsigned char* const data_end = data + (plane_size << 2);
			data != data_end;
			*r++ = *data++, *g++ = *data++, *b++ = *data++, *a++ = *data++
		);
	}
}
