#pragma once
#ifndef DESHI_TEXTURE_H
#define DESHI_TEXTURE_H

#include "../defines.h"

enum ImageFormat_{ //NOTE value = bytes per pixel
	ImageFormat_BW   = 1,
	ImageFormat_BWA  = 2,
	ImageFormat_RGB  = 3,
	ImageFormat_RGBA = 4,
}; typedef u32 ImageFormat;
global_ const char* ImageFormatStrings[] = {
	"BW", "BWA", "RGB", "RGBA"
};

enum TextureFilter_{
	TextureFilter_Nearest, //selects single value
	TextureFilter_Linear,  //combines nearby pixels with linear weight
	TextureFilter_Cubic,   //combines even more pixels with Catmull-Rom weights
	TextureFilter_COUNT,
}; typedef Type TextureFilter;
global_ const char* TextureFilterStrings[] = {
	"Nearest", "Linear", "Cubic"
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
global_ const char* TextureAddressModeStrings[] = {
	"Repeat", "Mirrored Repeat", "Clamp To Edge", "Clamp To White", "Clamp To Black", "Clamp To Transparent"
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
global_ const char* TextureTypeStrings[] = {
	"1D", "2D", "3D", "Cube", "1D Array", "2D Array", "Cube Array",
};

struct Texture{
	char name[DESHI_NAME_SIZE];
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
