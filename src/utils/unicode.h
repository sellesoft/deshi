#pragma once
#ifndef DESHI_UNICODE_H
#define DESHI_UNICODE_H
//!ref: https://github.com/Dion-Systems/metadesk/blob/master/source/md.h
//!ref: https://github.com/Dion-Systems/metadesk/blob/master/source/md.c
//NOTE the printf defines don't work without <cstdio> included

#include "../defines.h"

enum{
	StringEncoding_ASCII,
	StringEncoding_UTF8,
	StringEncoding_UTF16,
	StringEncoding_UTF32,
}; typedef u32 StringEncoding;

struct str8{
	u8* str;
	upt count;
};
#define str8_lit(s) {(u8*)(s), sizeof(s)-1}

struct str16{
	u16* str;
	upt  count;
};
#define str16_lit(s) {(u16*)(s), sizeof(s)-1}

struct str32{
	u32* str;
	upt  count;
};
#define str32_lit(s) {(u32*)(s), sizeof(s)-1}

struct generic_string{
	StringEncoding encoding;
	union{
		struct{
			void* str;
			upt   count;
		};
		 cstring s_char;
		str8    s_u8;
		str16   s_u16;
		str32   s_u32;
	};
};

struct DecodedCodepoint{
	u32 codepoint;
	u32 advance;
};

///////////////////
//// @decoding ////
///////////////////
#define unicode_bitmask1 0x01
#define unicode_bitmask2 0x03
#define unicode_bitmask3 0x07
#define unicode_bitmask4 0x0F
#define unicode_bitmask5 0x1F
#define unicode_bitmask6 0x3F
#define unicode_bitmask7 0x7F
#define unicode_bitmask8 0xFF
#define unicode_bitmask9  0x01FF
#define unicode_bitmask10 0x03FF

global_ DecodedCodepoint
decoded_codepoint_from_utf8(u8* str, u64 max_advance){
	persist u8 utf8_class[32] = { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,2,2,2,2,3,3,4,5, };
	
	DecodedCodepoint result = {MAX_U32, 1};
	u8 byte = str[0];
	u8 byte_class = utf8_class[byte >> 3];
	switch(byte_class){
		case 1:{
			result.codepoint = byte;
		}break;
		case 2:{
			if(2 <= max_advance){
				u8 next_byte = str[1];
				if(utf8_class[next_byte >> 3] == 0){
					result.codepoint  = (byte & unicode_bitmask5) << 6;
					result.codepoint |= next_byte & unicode_bitmask6;
					result.advance = 2;
				}
			}
		}break;
		case 3:{
			if(3 <= max_advance){
				u8 next_byte[2] = {str[1], str[2]};
				if(   (utf8_class[next_byte[0] >> 3] == 0)
				   && (utf8_class[next_byte[1] >> 3] == 0)){
					result.codepoint  = (byte & unicode_bitmask4) << 12;
					result.codepoint |= (next_byte[0] & unicode_bitmask6) << 6;
					result.codepoint |=  next_byte[1] & unicode_bitmask6;
					result.advance = 3;
				}
			}
		}break;
		case 4:{
			if(4 <= max_advance){
				u8 next_byte[3] = {str[1], str[2], str[3]};
				if(   (utf8_class[next_byte[0] >> 3] == 0)
				   && (utf8_class[next_byte[1] >> 3] == 0)
				   && (utf8_class[next_byte[2] >> 3] == 0)){
					result.codepoint  = (byte & unicode_bitmask3) << 18;
					result.codepoint |= (next_byte[0] & unicode_bitmask6) << 12;
					result.codepoint |= (next_byte[1] & unicode_bitmask6) << 6;
					result.codepoint |=  next_byte[2] & unicode_bitmask6;
					result.advance = 4;
				}
			}
		}break;
	}
	return result;
}

global_ DecodedCodepoint
decoded_codepoint_from_utf16(u16* str, u64 max_advance){
	DecodedCodepoint result = {MAX_U32, 1};
	result.codepoint = str[0];
	result.advance = 1;
	if((1 < max_advance) && (0xD800 <= str[0]) && (str[0] < 0xDC00) && (0xDC00 <= str[1]) && (str[1] < 0xE000)){
        result.codepoint = ((str[0] - 0xD800) << 10) | (str[1] - 0xDC00);
        result.advance = 2;
    }
	return result;
}

global_ u32
utf8_from_codepoint(u8* out, u32 codepoint){
#define unicode_bit8 0x80
	u32 advance = 0;
	if      (codepoint <= 0x7F){
		out[0] = u8(codepoint);
		advance = 1;
	}else if(codepoint <= 0x7FF){
		out[0] = (unicode_bitmask2 << 6) | ((codepoint >> 6) & unicode_bitmask5);
		out[1] = unicode_bit8 | (codepoint & unicode_bitmask6);
		advance = 2;
	}else if(codepoint <= 0xFFFF){
		out[0] = (unicode_bitmask3 << 5) | ((codepoint >> 12) & unicode_bitmask4);
		out[1] = unicode_bit8 | ((codepoint >> 6) & unicode_bitmask6);
		out[2] = unicode_bit8 | ( codepoint       & unicode_bitmask6);
		advance = 3;
	}else if(codepoint <= 0x10FFFF){
		out[0] = (unicode_bitmask4 << 3) | ((codepoint >> 18) & unicode_bitmask3);
		out[1] = unicode_bit8 | ((codepoint >> 12) & unicode_bitmask6);
		out[2] = unicode_bit8 | ((codepoint >>  6) & unicode_bitmask6);
		out[3] = unicode_bit8 | ( codepoint        & unicode_bitmask6);
		advance = 4;
	}else{
		out[0] = '?';
		advance = 1;
	}
	return advance;
#undef unicode_bit8
}

global_ u32
utf16_from_codepoint(u16* out, u32 codepoint){
	u32 advance = 1;
	if(codepoint == MAX_U32){
		out[0] = u16('?');
	}else if(codepoint < 0x10000){
		out[0] = u16(codepoint);
	}else{
		u64 v = codepoint - 0x10000;
		out[0] = 0xD800 + (v >> 10);
		out[1] = 0xDC00 + (v & unicode_bitmask10);
		advance = 2;
	}
	return advance;
}

#undef unicode_bitmask1
#undef unicode_bitmask2
#undef unicode_bitmask3
#undef unicode_bitmask4
#undef unicode_bitmask5
#undef unicode_bitmask6
#undef unicode_bitmask7
#undef unicode_bitmask8
#undef unicode_bitmask9
#undef unicode_bitmask10

/////////////////////
//// @conversion ////
/////////////////////
global_ str8
str8_from_str16(str16 in, Allocator* allocator = stl_allocator){
	u64 space = 3*in.count;
	u8* str = (u8*)allocator->reserve((space+1)*sizeof(u8));
	allocator->commit(str, (space+1)*sizeof(u8));
	u16* ptr = in.str;
	u16* opl = ptr + in.count; //one past last
	u64 size = 0;
	DecodedCodepoint consume;
	for(;ptr < opl;){
		consume = decoded_codepoint_from_utf16(ptr, opl - ptr);
		ptr += consume.advance;
		size += utf8_from_codepoint(str + size, consume.codepoint);
	}
	str[size] = '\0';
	str = (u8*)allocator->resize(str, (size+1)*sizeof(u8));
	return str8{str, size};
}

global_ str8
str8_from_str32(str32 in, Allocator* allocator = stl_allocator){
	u64 space = 4*in.count;
	u8* str = (u8*)allocator->reserve((space+1)*sizeof(u8));
	allocator->commit(str, (space+1)*sizeof(u8));
	u32* ptr = in.str;
	u32* opl = ptr + in.count; //one past last
	u64 size = 0;
	DecodedCodepoint consume;
	for(;ptr < opl; ptr += 1){
		size += utf8_from_codepoint(str + size, *ptr);
	}
	str[size] = '\0';
	str = (u8*)allocator->resize(str, (size+1)*sizeof(u8));
	return str8{str, size};
}

global_ str16
str16_from_str8(str8 in, Allocator* allocator = stl_allocator){
	u64 space = 2*in.count;
	u16* str = (u16*)allocator->reserve((space+1)*sizeof(u16));
	allocator->commit(str, (space+1)*sizeof(u16));
	u8* ptr = in.str;
	u8* opl = ptr + in.count; //one past last
	u64 size = 0;
	DecodedCodepoint consume;
	for(;ptr < opl;){
		consume = decoded_codepoint_from_utf8(ptr, opl - ptr);
		ptr += consume.advance;
		size += utf16_from_codepoint(str + size, consume.codepoint);
	}
	str[size] = '\0';
	str = (u16*)allocator->resize(str, (size+1)*sizeof(u16));
	return str16{str, size};
}

global_ str32
str32_from_str8(str8 in, Allocator* allocator = stl_allocator){
	u64 space = in.count;
	u32* str = (u32*)allocator->reserve((space+1)*sizeof(u32));
	allocator->commit(str, (space+1)*sizeof(u32));
	u8* ptr = in.str;
	u8* opl = ptr + in.count; //one past last
	u64 size = 0;
	DecodedCodepoint consume;
	for(;ptr < opl;){
		consume = decoded_codepoint_from_utf8(ptr, opl - ptr);
		ptr += consume.advance;
		str[size] = consume.codepoint;
		size += 1;
	}
	str[size] = '\0';
	str = (u32*)allocator->resize(str, (size+1)*sizeof(u32));
	return str32{str, size};
}

/////////////////
//// @printf ////
/////////////////


#endif //DESHI_UNICODE_H
