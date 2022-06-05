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
typedef struct{
	f32 x0, y0, u0, v0; // top-left
	f32 x1, y1, u1, v1; // bottom-right
} aligned_quad;

typedef struct{
	unsigned short x0, y0, x1, y1; // coordinates of bbox in bitmap
	f32 xoff, yoff, xadvance;
	f32 xoff2, yoff2;
} packed_char;

typedef struct{
	f32 font_size;
	s32 first_codepoint;  // if non-zero, then the chars are continuous, and this is the first codepoints
	s32* array_of_unicode_codepoints;       // if non-zero, then this is an array of unicode codepoints
	s32 num_chars;
	packed_char* chardata_for_range; // output
	unsigned char h_oversample, v_oversample; // don't set these, they're used internally
} pack_range;

struct Font{
	Type type;
	u32  idx;
	str8 name;
	char weight[64];
	u32  max_width;
	u32  max_height;
	u32  count;
	u32  ttf_size[2];
	u32  num_ranges;
	f32  uv_yoffset;   //the y offset of UV since we are now packing a white square into every font
	f32  ascent;       //the highest point above baseline a glyph reaches
	f32  descent;      //the lowest point below baseline a glyph reaches
	f32  line_gap;     //the recommended 
	f32  aspect_ratio; //max character height / max character width
	Texture* tex;
	pack_range* ranges; //stbtt_pack_range
};

//TODO(delle) an overload for specifying range if you know where you're working
global_ packed_char*
font_packed_char(Font* font, u32 codepoint){
	forI(font->num_ranges){
		if(   (codepoint >= font->ranges[i].first_codepoint)
		   && (codepoint <  font->ranges[i].first_codepoint + font->ranges[i].num_chars)){
			return font->ranges[i].chardata_for_range + (codepoint - font->ranges[i].first_codepoint);
		}
	}
	Assert(!"The requested codepoint was not found in any of the ranges. TODO better error handling here.");
	return 0;
}

global_ aligned_quad
font_aligned_quad(Font* font, u32 codepoint, vec2* pos, vec2 scale){
	packed_char* pc = font_packed_char(font, codepoint);
	if(pc){
		aligned_quad q;
		q.x0 = pos->x + pc->xoff * scale.x;
		q.y0 = pos->y + (pc->yoff + font->ascent) * scale.y;
		q.x1 = pos->x + (pc->xoff2 - pc->xoff) * scale.x;
		q.y1 = pos->y + (pc->yoff2 + font->ascent) * scale.y;
		q.u0 = ((f32)pc->x0 / font->ttf_size[0]); //NOTE(sushi) we could maybe store the UV values normalized instead of doing this everytime
		q.v0 = ((f32)pc->y0 / font->ttf_size[1]) + font->uv_yoffset;
		q.u1 = ((f32)pc->x1 / font->ttf_size[0]);
		q.v1 = ((f32)pc->y1 / font->ttf_size[1]) + font->uv_yoffset;
		pos->x += pc->xadvance * scale.x;
		return q;
	}
	Assert(!"The requested codepoint was not found in any of the ranges. TODO better error handling here.");
	return aligned_quad{};
}


#endif //DESHI_FONT_H
