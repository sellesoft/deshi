#pragma once
#ifndef DESHI_FONT_H
#define DESHI_FONT_H

#include "texture.h"
#include "kigu/common.h"
#include "math/Math.h"

//note, to calculate the width of a char from a specified height do
//   height / aspect_ratio / max_width

enum FontType{
	FontType_NONE,
	FontType_BDF,
	FontType_TTF,
	FontType_COUNT
};

//a bunch of structs copied from stbtt, so we can use them locally w/o having to include imgui's stb
typedef struct {
	f32 x0, y0, s0, t0; // top-left
	f32 x1, y1, s1, t1; // bottom-right
} aligned_quad;

typedef struct {
	unsigned short x0, y0, x1, y1; // coordinates of bbox in bitmap
	f32 xoff, yoff, xadvance;
	f32 xoff2, yoff2;
} packedchar;

typedef struct {
	f32 font_size;
	s32 firstcodepoint;  // if non-zero, then the chars are continuous, and this is the first codepoints
	s32* array_of_unicode_codepoints;       // if non-zero, then this is an array of unicode codepoints
	s32 num_chars;
	packedchar* chardata_for_range; // output
	unsigned char h_oversample, v_oversample; // don't set these, they're used internally
} pack_range;

//pack_context isnt really necessary as its really just something used u32ernally by stbtt

struct Font{
	Type     type;
	u32      idx;
	Texture* tex;
	char     name[64];
	char     weight[64];
	u32      max_width;
	u32      max_height;
	u32      rendered_resolution;
	u32      count;
	u32      ttf_size[2];
	u32      num_ranges;
	void*    ttf_pack_context;      //stbtt_pack_context
	pack_range* ttf_pack_ranges; //stbtt_pack_range
	
	//the y offset of UV since we are now packing a white square into every font.
	f32 uvOffset;
	
	f32 ascent; //the highest point above baseline a glyph reaches
	f32 decent; //the lowest point below baseline a glyph reaches
	f32 line_gap; //the recommended 
	
	f32 aspect_ratio; //max character height / max character width
	
	aligned_quad GetPackedQuad(s32 charidx, vec2* pos, vec2 scale = vec2::ONE);
	packedchar* GetPackedChar(s32 charidx);
	
	f32 WidthOfString(const char* str, f32 scale = 1);
	
	vec2 ScaleFromPixelHeight(u32 height);
	
	
};


//same as stbtt's but we check our ranges here so we dont have to do that anywhere else
//TODO an overload for specifying range if you know where you're working
//     eg. different text modes for InputText like alpha, numeric, etc..
inline aligned_quad Font::
GetPackedQuad(s32 charidx, vec2* pos, vec2 scale) {
	f32 ipw = 1.0f / ttf_size[0], iph = 1.0f / ttf_size[1];
	
	packedchar* b = nullptr;
	
	//determine what range the req character is in
	forI(num_ranges) {
		if (charidx >= ttf_pack_ranges[i].firstcodepoint && charidx <= ttf_pack_ranges[i].firstcodepoint + ttf_pack_ranges[i].num_chars) {
			b = ttf_pack_ranges[i].chardata_for_range + (charidx - ttf_pack_ranges[i].firstcodepoint);
			break;
		}
	}
	
	if (b) {
		aligned_quad q;
		
		q.x0 = pos->x + b->xoff * scale.x;
		q.y0 = pos->y + (b->yoff + ascent) * scale.y;
		q.x1 = pos->x + (b->xoff2 - b->xoff) * scale.x;
		q.y1 = pos->y + (b->yoff2 + ascent) * scale.y;
		
		q.s0 = b->x0 * ipw; //NOTE, we could maybe store the UV values noramalized instead of doing this everytime
		q.t0 = b->y0 * iph;
		q.s1 = b->x1 * ipw;
		q.t1 = b->y1 * iph;
		
		pos->x += b->xadvance * scale.x;
		
		return q;
	}
	Assert(0, "The req character was not found in any of the ranges. TODO better error handling here.");
	return aligned_quad{};
}

inline packedchar* Font::
GetPackedChar(s32 charidx) {
	//determine what range the req character is in
	forI(num_ranges)
		if (charidx >= ttf_pack_ranges[i].firstcodepoint && charidx <= ttf_pack_ranges[i].firstcodepoint + ttf_pack_ranges[i].num_chars)
		return ttf_pack_ranges[i].chardata_for_range + (charidx - ttf_pack_ranges[i].firstcodepoint);
	Assert(0, "The req character was not found in any of the ranges. TODO better error handling here.");
	return 0;
}

inline f32 Font::
WidthOfString(const char* str, f32 scale) {
	f32 ret = 0;
	forI(strlen(str)) ret += GetPackedChar(str[i])->xadvance * scale;
	return ret;
}


#endif //DESHI_FONT_H
