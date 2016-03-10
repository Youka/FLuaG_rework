/*
Project: FLuaG
File: font.hpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include <exception>
#include <string>
#include <vector>
#ifdef _WIN32
	#include "../utils/textconv.hpp"
	#include <wingdi.h>
#else
	#include <fontconfig/fontconfig.h>
	#include <memory>
	#include <pango/pangocairo.h>
#endif
#include <cassert>

#define FONT_UPSCALE 64.0

namespace Font{
	// Simple local exception
	class exception : public std::exception{
		private:
			const std::string message;
		public:
			exception(const std::string& message) : message(message){}
			exception(std::string&& message) : message(std::move(message)){}
			const char* what() const throw() override{return message.c_str();}
	};

	// Get list of installed fonts
	struct ListEntry{
		std::string family, style, file, script;
		bool outline;
	};
#ifdef _WIN32
	static int CALLBACK enumfontcallback(const LOGFONTW* lf, const TEXTMETRICW*, const DWORD fonttype, LPARAM lParam){
		// Add font properties to list output
		reinterpret_cast<std::vector<ListEntry>*>(lParam)->push_back({
			Utf8::from_utf16(lf->lfFaceName),
			Utf8::from_utf16(reinterpret_cast<const ENUMLOGFONTEXW*>(lf)->elfStyle),
			"",
			Utf8::from_utf16(reinterpret_cast<const ENUMLOGFONTEXW*>(lf)->elfScript),
			static_cast<bool>(fonttype & TRUETYPE_FONTTYPE)
		});
		// Continue font enumeration until end
		return 1;
	}
#endif
	inline std::vector<ListEntry> list() throw(exception){
		std::vector<ListEntry> result;
#ifdef _WIN32
		const HDC hdc = CreateCompatibleDC(NULL);
		if(!hdc)
			throw exception("Couldn't create device context!");
		LOGFONTW lf = {0};
		lf.lfCharSet = DEFAULT_CHARSET;
		EnumFontFamiliesExW(hdc, &lf, enumfontcallback, reinterpret_cast<LPARAM>(&result), 0);
		DeleteDC(hdc);
#else
		// Get font list from FontConfig
		FcConfig* fc = FcInitLoadConfigAndFonts();
		std::unique_ptr<FcPattern, void(*)(FcPattern*)> pattern(FcPatternCreate(), FcPatternDestroy);
		std::unique_ptr<FcObjectSet, void(*)(FcObjectSet*)> objset(FcObjectSetBuild(FC_FAMILY, FC_STYLE, FC_FILE, FC_OUTLINE, nullptr), FcObjectSetDestroy);
		std::unique_ptr<FcFontSet, void(*)(FcFontSet*)> fontlist(FcFontList(fc, pattern.get(), objset.get()), FcFontSetDestroy);
		if(!fontlist)
			throw exception("Couldn't create font list!");
		// Add font list to output
		result.reserve(fontlist->nfont);
		for(int i = 0; i < fontlist->nfont; ++i){
			const FcPattern* font = fontlist->fonts[i];
			FcChar8* sresult; FcBool bresult;
			ListEntry entry;
			FcPatternGetString(font, FC_FAMILY, 0, &sresult); entry.family = reinterpret_cast<char*>(sresult);
			FcPatternGetString(font, FC_STYLE, 0, &sresult); entry.style = reinterpret_cast<char*>(sresult);
			FcPatternGetString(font, FC_FILE, 0, &sresult); entry.file = reinterpret_cast<char*>(sresult);
			FcPatternGetBool(font, FC_OUTLINE, 0, &bresult); entry.outline = bresult;
			result.push_back(std::move(entry));
		}
#endif
		return result;
	}

	// Native font class
	class Font{
		private:
			// Attributes
#ifdef _WIN32
			HDC dc;
			HGDIOBJ old_font;
			double spacing;
#else
			cairo_t* ctx;
			PangoLayout* layout;
#endif
			// Helpers
			void copy(const Font& other){
#ifdef _WIN32
				if(!other.dc){
					this->dc = NULL;
					this->old_font = NULL;
					this->spacing = 0;
				}else{
					this->dc = CreateCompatibleDC(NULL);
					SetMapMode(this->dc, GetMapMode(other.dc));
					SetBkMode(this->dc, GetBkMode(other.dc));
					SetTextAlign(this->dc, GetTextAlign(other.dc));
					LOGFONTW lf;	// I trust in Spongebob that it has 4-byte boundary like required by GetObject
					GetObjectW(GetCurrentObject(other.dc, OBJ_FONT), sizeof(lf), &lf);
					this->old_font = SelectObject(this->dc, CreateFontIndirectW(&lf));
					this->spacing = other.spacing;
				}
#else
				if(!other.ctx){
					this->ctx = nullptr;
					this->layout = nullptr;
				}else{
					this->layout = pango_cairo_create_layout(this->ctx = cairo_create(cairo_image_surface_create(CAIRO_FORMAT_A1, 1, 1)));
					pango_layout_set_font_description(this->layout, pango_layout_get_font_description(other.layout)),
					pango_layout_set_attributes(this->layout, pango_layout_get_attributes(other.layout)),
					pango_layout_set_auto_dir(this->layout, pango_layout_get_auto_dir(other.layout));
				}
#endif
			}
			void move(Font&& other){
#ifdef _WIN32
				if(!other.dc){
					this->dc = NULL;
					this->old_font = NULL;
					this->spacing = 0;
				}else{
					this->dc = other.dc;
					other.dc = NULL;
					this->old_font = other.old_font;
					other.old_font = NULL;
					this->spacing = other.spacing;
					other.spacing = 0;
				}
#else
				if(!other.ctx){
					this->ctx = nullptr;
					this->layout = nullptr;
				}else{
					this->ctx = other.ctx;
					other.ctx = nullptr;
					this->layout = other.layout;
					other.layout = nullptr;
				}
#endif
			}
		public:
			// Rule-of-five
			Font() :
#ifdef _WIN32
				dc(NULL), old_font(NULL), spacing(0)
#else
				ctx(nullptr), layout(nullptr)
#endif
				{}
			Font(const std::string& family, float size = 12, bool bold = false, bool italic = false, bool underline = false, bool strikeout = false, double spacing = 0.0, bool rtl = false) throw(exception)
#ifdef _WIN32
				: Font(Utf8::to_utf16(family), size, bold, italic, underline, strikeout, spacing, rtl){
#else
			{
				// Check parameters
				if(size < 0)
					throw exception("Size must be bigger zero!");
				// Create context+layout
				cairo_surface_t* surf = cairo_image_surface_create(CAIRO_FORMAT_A1, 1, 1);
				if(!surf)
					throw exception("Couldn't create cairo surface!");
				if(!(this->ctx = cairo_create(surf))){
					cairo_surface_destroy(surf);
					throw exception("Couldn't create cairo context!");
				}
				if(!(this->layout = pango_cairo_create_layout(this->ctx))){
					cairo_destroy(this->ctx);
					cairo_surface_destroy(surf);
					throw exception("Couldn't create pango layout!");
				}
				// Set layout properties
				PangoFontDescription *font = pango_font_description_new();
				pango_font_description_set_family(font, family.c_str());
				pango_font_description_set_weight(font, bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
				pango_font_description_set_style(font, italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);
				pango_font_description_set_absolute_size(font, size * PANGO_SCALE * FONT_UPSCALE);
				pango_layout_set_font_description(this->layout, font);
				pango_font_description_free(font);
				PangoAttrList* attr_list = pango_attr_list_new();
				pango_attr_list_insert(attr_list, pango_attr_underline_new(underline ? PANGO_UNDERLINE_SINGLE : PANGO_UNDERLINE_NONE));
				pango_attr_list_insert(attr_list, pango_attr_strikethrough_new(strikeout));
				pango_attr_list_insert(attr_list, pango_attr_letter_spacing_new(spacing * PANGO_SCALE * FONT_UPSCALE));
				pango_layout_set_attributes(this->layout, attr_list);
				pango_attr_list_unref(attr_list);
				pango_layout_set_auto_dir(this->layout, rtl);
#endif
			}
#ifdef _WIN32
			Font(const std::wstring& family, float size = 12, bool bold = false, bool italic = false, bool underline = false, bool strikeout = false, double spacing = 0.0, bool rtl = false) throw(exception)
				: spacing(spacing){
				// Check parameters
				if(family.length() > 31)	// See LOGFONT limitation
					throw exception("Family length exceeds 31!");
				if(size < 0)
					throw exception("Size must be bigger zero!");
				// Create context
				if(!(this->dc = CreateCompatibleDC(NULL)))
					throw exception("Couldn't create device context!");
				SetMapMode(this->dc, MM_TEXT);
				SetBkMode(this->dc, TRANSPARENT);
				if(rtl)
					SetTextAlign(this->dc, TA_RTLREADING);
				// Create font
				LOGFONTW lf = {0};
				lf.lfHeight = size * FONT_UPSCALE;
				lf.lfWeight = bold ? FW_BOLD : FW_NORMAL;
				lf.lfItalic = italic;
				lf.lfUnderline = underline;
				lf.lfStrikeOut = strikeout;
				lf.lfCharSet = DEFAULT_CHARSET;
				lf.lfOutPrecision = OUT_TT_PRECIS;
				lf.lfQuality = ANTIALIASED_QUALITY;
				lf.lfFaceName[family.copy(lf.lfFaceName, 31)] = L'\0';
				HFONT font = CreateFontIndirectW(&lf);
				if(!font){
					DeleteDC(this->dc);
					throw exception("Couldn't create font!");
				}
				// Set font to context
				this->old_font = SelectObject(this->dc, font);
			}
#endif
			~Font(){
#ifdef _WIN32
				if(this->dc){
					DeleteObject(SelectObject(this->dc, this->old_font));
					DeleteDC(this->dc);
				}
#else
				if(this->ctx){
					g_object_unref(this->layout);
					cairo_surface_t* surf = cairo_get_target(this->ctx);
					cairo_destroy(this->ctx);
					cairo_surface_destroy(surf);
				}
#endif
			}
			Font(const Font& other){
				this->copy(other);
			}
			Font& operator=(const Font& other){
				this->~Font();
				this->copy(other);
				return *this;
			}
			Font(Font&& other){
				this->move(std::forward<Font>(other));
			}
			Font& operator=(Font&& other){
				this->~Font();
				this->move(std::forward<Font>(other));
				return *this;
			}
			// Getters
			operator bool() const{
#ifdef _WIN32
				return this->dc;
#else
				return this->ctx;
#endif
			}
			std::string get_family() const throw(exception){
#ifdef _WIN32
				return Utf8::from_utf16(this->get_family_unicode());
#else
				if(!this->ctx)
					throw exception("Invalid state!");
				return pango_font_description_get_family(pango_layout_get_font_description(this->layout));
#endif
			}
#ifdef _WIN32
			std::wstring get_family_unicode() const throw(exception){
				if(!this->dc)
					throw exception("Invalid state!");
				LOGFONTW lf;
				GetObjectW(GetCurrentObject(this->dc, OBJ_FONT), sizeof(lf), &lf);
				return lf.lfFaceName;
			}
#endif
			float get_size() const throw(exception){
#ifdef _WIN32
				if(!this->dc)
					throw exception("Invalid state!");
				LOGFONTW lf;
				GetObjectW(GetCurrentObject(this->dc, OBJ_FONT), sizeof(lf), &lf);
				return static_cast<float>(lf.lfHeight) / FONT_UPSCALE;
#else
				if(!this->ctx)
					throw exception("Invalid state!");
				return static_cast<float>(pango_font_description_get_size(pango_layout_get_font_description(this->layout))) / FONT_UPSCALE / PANGO_SCALE;
#endif
			}
			bool get_bold() const throw(exception){
#ifdef _WIN32
				if(!this->dc)
					throw exception("Invalid state!");
				LOGFONTW lf;
				GetObjectW(GetCurrentObject(this->dc, OBJ_FONT), sizeof(lf), &lf);
				return lf.lfWeight == FW_BOLD;
#else
				if(!this->ctx)
					throw exception("Invalid state!");
				return pango_font_description_get_weight(pango_layout_get_font_description(this->layout)) == PANGO_WEIGHT_BOLD;
#endif
			}
			bool get_italic() const throw(exception){
#ifdef _WIN32
				if(!this->dc)
					throw exception("Invalid state!");
				LOGFONTW lf;
				GetObjectW(GetCurrentObject(this->dc, OBJ_FONT), sizeof(lf), &lf);
				return lf.lfItalic;
#else
				if(!this->ctx)
					throw exception("Invalid state!");
				return pango_font_description_get_style(pango_layout_get_font_description(this->layout)) == PANGO_STYLE_ITALIC;
#endif
			}
			bool get_underline() const throw(exception){
#ifdef _WIN32
				if(!this->dc)
					throw exception("Invalid state!");
				LOGFONTW lf;
				GetObjectW(GetCurrentObject(this->dc, OBJ_FONT), sizeof(lf), &lf);
				return lf.lfUnderline;
#else
				if(!this->ctx)
					throw exception("Invalid state!");
				std::unique_ptr<PangoAttrIterator, void(*)(PangoAttrIterator*)> attr_list_iter(pango_attr_list_get_iterator(pango_layout_get_attributes(this->layout)), pango_attr_iterator_destroy);
				return reinterpret_cast<PangoAttrInt*>(pango_attr_iterator_get(attr_list_iter.get(), PANGO_ATTR_UNDERLINE))->value == PANGO_UNDERLINE_SINGLE;
#endif
			}
			bool get_strikeout() const throw(exception){
#ifdef _WIN32
				if(!this->dc)
					throw exception("Invalid state!");
				LOGFONTW lf;
				GetObjectW(GetCurrentObject(this->dc, OBJ_FONT), sizeof(lf), &lf);
				return lf.lfStrikeOut;
#else
				if(!this->ctx)
					throw exception("Invalid state!");
				std::unique_ptr<PangoAttrIterator, void(*)(PangoAttrIterator*)> attr_list_iter(pango_attr_list_get_iterator(pango_layout_get_attributes(this->layout)), pango_attr_iterator_destroy);
				return reinterpret_cast<PangoAttrInt*>(pango_attr_iterator_get(attr_list_iter.get(), PANGO_ATTR_STRIKETHROUGH))->value;
#endif
			}
			double get_spacing() const throw(exception){
#ifdef _WIN32
				if(!this->dc)
					throw exception("Invalid state!");
				return this->spacing;
#else
				if(!this->ctx)
					throw exception("Invalid state!");
				std::unique_ptr<PangoAttrIterator, void(*)(PangoAttrIterator*)> attr_list_iter(pango_attr_list_get_iterator(pango_layout_get_attributes(this->layout)), pango_attr_iterator_destroy);
				return static_cast<double>(reinterpret_cast<PangoAttrInt*>(pango_attr_iterator_get(attr_list_iter.get(), PANGO_ATTR_LETTER_SPACING))->value) / FONT_UPSCALE / PANGO_SCALE;
#endif
			}
			bool get_rtl() const throw(exception){
#ifdef _WIN32
				if(!this->dc)
					throw exception("Invalid state!");
				return GetTextAlign(this->dc) == TA_RTLREADING;
#else
				if(!this->ctx)
					throw exception("Invalid state!");
				return pango_layout_get_auto_dir(this->layout);
#endif
			}
			// Font/Text informations
			struct Metrics{
				double height, ascent, descent, internal_leading, external_leading;
			};
			Metrics metrics() const throw(exception){
#ifdef _WIN32
				if(!this->dc)
					throw exception("Invalid state!");
				TEXTMETRICW metrics;
				GetTextMetricsW(this->dc, &metrics);
				return {
					static_cast<double>(metrics.tmHeight) / FONT_UPSCALE,
					static_cast<double>(metrics.tmAscent) / FONT_UPSCALE,
					static_cast<double>(metrics.tmDescent) / FONT_UPSCALE,
					static_cast<double>(metrics.tmInternalLeading) / FONT_UPSCALE,
					static_cast<double>(metrics.tmExternalLeading) / FONT_UPSCALE
				};
#else
				if(!this->ctx)
					throw exception("Invalid state!");
				Metrics result;
				std::unique_ptr<PangoFontMetrics, void(*)(PangoFontMetrics*)> metrics(pango_context_get_metrics(pango_layout_get_context(this->layout), pango_layout_get_font_description(this->layout), nullptr), pango_font_metrics_unref);
				result.ascent = static_cast<double>(pango_font_metrics_get_ascent(metrics.get())) / FONT_UPSCALE / PANGO_SCALE;
				result.descent = static_cast<double>(pango_font_metrics_get_descent(metrics.get())) / FONT_UPSCALE / PANGO_SCALE;
				result.height = result.ascent + result.descent;
				result.internal_leading = 0; // HEIGHT - ASCENT - DESCENT
				result.external_leading = static_cast<double>(pango_layout_get_spacing(this->layout)) / FONT_UPSCALE / PANGO_SCALE;
				return result;
#endif
			}
			double text_width(const std::string& text) const throw(exception){
#ifdef _WIN32
				return this->text_width(Utf8::to_utf16(text));
#else
				if(!this->ctx)
					throw exception("Invalid state!");
				pango_layout_set_text(this->layout, text.data(), text.length());
				PangoRectangle rect;
				pango_layout_get_pixel_extents(this->layout, nullptr, &rect);
				return static_cast<double>(rect.width) / FONT_UPSCALE;
#endif
			}
#ifdef _WIN32
			double text_width(const std::wstring& text) const throw(exception){
				if(!this->dc)
					throw exception("Invalid state!");
				SIZE sz;
				GetTextExtentPoint32W(this->dc, text.data(), text.length(), &sz);
				return static_cast<double>(sz.cx) / FONT_UPSCALE + text.length() * this->spacing;
			}
#endif
			// Text-to-vectors conversion
			struct PathSegment{
				enum class Type{MOVE, LINE, CURVE, CLOSE} type;
				double x, y;
			};
			std::vector<PathSegment> text_path(const std::string& text) const throw(exception){
#ifdef _WIN32
				return this->text_path(Utf8::to_utf16(text));
#else
				// Check valid state/cairo context
				if(!this->ctx)
					throw exception("Invalid state!");
				// Add text path to context
				pango_layout_set_text(this->layout, text.data(), text.length());
				cairo_save(this->ctx);
				cairo_scale(ctx, 1.0 / FONT_UPSCALE, 1.0 / FONT_UPSCALE);
				pango_cairo_layout_path(this->ctx, this->layout);
				cairo_restore(this->ctx);
				// Get path points
				std::unique_ptr<cairo_path_t, void(*)(cairo_path_t*)> path(cairo_copy_path(this->ctx), cairo_path_destroy);
				if(path->status != CAIRO_STATUS_SUCCESS){
					cairo_new_path(this->ctx);
					throw exception("Couldn't get cairo path!");
				}
				// Pack points for output
				std::vector<PathSegment> result;
				result.reserve(path->num_data >> 1);	// Make a memory guess by expecting all segments are moves/lines
				for(cairo_path_data_t* pdata = path->data, *data_end = pdata + path->num_data; pdata != data_end; pdata += pdata->header.length){
					assert(pdata < data_end);
					switch(pdata->header.type){
						case CAIRO_PATH_MOVE_TO:
							result.push_back({PathSegment::Type::MOVE, pdata[1].point.x, pdata[1].point.y});
							break;
						case CAIRO_PATH_LINE_TO:
							result.push_back({PathSegment::Type::LINE, pdata[1].point.x, pdata[1].point.y});
							break;
						case CAIRO_PATH_CURVE_TO:
							result.push_back({PathSegment::Type::CURVE, pdata[1].point.x, pdata[1].point.y});
							result.push_back({PathSegment::Type::CURVE, pdata[2].point.x, pdata[2].point.y});
							result.push_back({PathSegment::Type::CURVE, pdata[3].point.x, pdata[3].point.y});
							break;
						case CAIRO_PATH_CLOSE_PATH:
							result.push_back({PathSegment::Type::CLOSE});
							break;
					}
				}
				// Clear context from path...
				cairo_new_path(this->ctx);
				// ...and return collected points
				return result;
#endif
			}
#ifdef _WIN32
			std::vector<PathSegment> text_path(const std::wstring& text) const throw(exception){
				// Check valid state/device context
				if(!this->dc)
					throw exception("Invalid state!");
				// Check valid text length
				if(text.length() > 8192)	// See ExtTextOut limitation
					throw exception("Text length mustn't exceed 8192!");
				// Get character widths
				std::vector<int> xdist;
				if(this->spacing != 0.0){
					xdist.reserve(text.length());
					const int scaled_spacing = this->spacing * FONT_UPSCALE;
					for(const char c : text){
						INT width;
						GetCharWidth32W(this->dc, c, c, &width);
						xdist.push_back(width + scaled_spacing);
					}
				}
				// Add text path to context
				BeginPath(this->dc);
				ExtTextOutW(this->dc, 0, 0, 0x0, NULL, text.data(), text.length(), xdist.empty() ? NULL : xdist.data());
				EndPath(this->dc);
				// Collect path points
				std::vector<PathSegment> result;
				const int points_n = GetPath(this->dc, NULL, NULL, 0);
				if(points_n){
					std::vector<POINT> points;
					std::vector<BYTE> types;
					points.reserve(points_n);
					types.reserve(points_n);
					result.reserve(points_n);
					GetPath(this->dc, points.data(), types.data(), points_n);
					// Pack points in output
					for(int point_i = 0; point_i < points_n; ++point_i){
						switch(types[point_i]){
							case PT_MOVETO:
								result.push_back({PathSegment::Type::MOVE, static_cast<double>(points[point_i].x) / FONT_UPSCALE, static_cast<double>(points[point_i].y) / FONT_UPSCALE});
								break;
							case PT_LINETO:
							case PT_LINETO|PT_CLOSEFIGURE:
								result.push_back({PathSegment::Type::LINE, static_cast<double>(points[point_i].x) / FONT_UPSCALE, static_cast<double>(points[point_i].y) / FONT_UPSCALE});
								break;
							case PT_BEZIERTO:
							case PT_BEZIERTO|PT_CLOSEFIGURE:
								result.push_back({PathSegment::Type::CURVE, static_cast<double>(points[point_i].x) / FONT_UPSCALE, static_cast<double>(points[point_i].y) / FONT_UPSCALE});
								break;
						}
						if(types[point_i]&PT_CLOSEFIGURE)
							result.push_back({PathSegment::Type::CLOSE});
					}
				}
				// Clear context from path...
				AbortPath(this->dc);
				// ...and return collected points
				return result;
			}
#endif
	};
}
