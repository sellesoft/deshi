#pragma once
#ifndef DESHI_FONT_H
#define DESHI_FONT_H

struct Font{
	u32 width;
	u32 height;
	u32 font_size;
	
	u32 char_count;
	
	string name;
	string weight;
	
	vec2 dpi;
	vec4 bbx;
	
    u16* encodings;
	u8*  texture;
};

#endif //DESHI_FONT_H
