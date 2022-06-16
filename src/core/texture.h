#pragma once
#ifndef DESHI_TEXTURE_H
#define DESHI_TEXTURE_H

#include "kigu/common.h"
#include "kigu/unicode.h"

enum ImageFormat_{ //NOTE value = bytes per pixel
	ImageFormat_BW   = 1,
	ImageFormat_BWA  = 2,
	ImageFormat_RGB  = 3,
	ImageFormat_RGBA = 4,
}; typedef u32 ImageFormat;
global const str8 ImageFormatStrings[] = {
	STR8("BW"), STR8("BWA"), STR8("RGB"), STR8("RGBA")
};

enum TextureFilter_{
	TextureFilter_Nearest, //selects single value
	TextureFilter_Linear,  //combines nearby pixels with linear weight
	TextureFilter_Cubic,   //combines even more pixels with Catmull-Rom weights
	TextureFilter_COUNT,
}; typedef Type TextureFilter;
global const str8 TextureFilterStrings[] = {
	STR8("Nearest"), STR8("Linear"), STR8("Cubic")
};

enum TextureAddressMode_{ //what happens when uv values are beyond 0..1
	TextureAddressMode_Repeat,             //uv values loop around (1.1 = .1)
	TextureAddressMode_MirroredRepeat,     //uv values loop around but mirrored (1.1 = .9)
	TextureAddressMode_ClampToEdge,        //uv values are the edge value (1.1 = 1)
	TextureAddressMode_ClampToWhite,       //uv values are white
	TextureAddressMode_ClampToBlack,       //uv values are black
	TextureAddressMode_ClampToTransparent, //uv values are transparent
	TextureAddressMode_COUNT,
}; typedef u32 TextureAddressMode;
global const str8 TextureAddressModeStrings[] = {
	STR8("Repeat"), STR8("Mirrored Repeat"), STR8("Clamp To Edge"), STR8("Clamp To White"), STR8("Clamp To Black"), STR8("Clamp To Transparent")
};

enum TextureType_{
	TextureType_1D,
	TextureType_2D,
	TextureType_3D,
	TextureType_Cube,
	TextureType_Array_1D,
	TextureType_Array_2D,
	TextureType_Array_Cube,
	TextureType_COUNT
}; typedef u32 TextureType;
global const str8 TextureTypeStrings[] = {
	STR8("1D"), STR8("2D"), STR8("3D"), STR8("Cube"), STR8("1D Array"), STR8("2D Array"), STR8("Cube Array"),
};

struct Texture{
	char name[64]; //NOTE(delle) includes the extension
	u32  idx;
	int  width;
	int  height;
	int  depth;
	int  mipmaps;
	u8*  pixels;
	b32  loaded;
	ImageFormat   format;
	TextureType   type;
	TextureFilter filter;
	TextureAddressMode uvMode;
};

#endif //DESHI_TEXTURE_H
