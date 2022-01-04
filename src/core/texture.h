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

enum TextureFilter_{
	TextureFilter_Nearest, //selects single value
	TextureFilter_Linear,  //combines nearby pixels with linear weight
	TextureFilter_Cubic,   //combines even more pixels with Catmull-Rom weights
	TextureFilter_COUNT,
}; typedef Type TextureFilter;
global_ const char* TextureFilterStrings[] = {
	"Nearest", "Linear", "Cubic"
};

enum TextureAddressMode_{
	TextureAddressMode_Repeat,
	TextureAddressMode_MirroredRepeat,
	TextureAddressMode_ClampToEdge,
	TextureAddressMode_ClampToWhite,
	TextureAddressMode_ClampToBlack,
	TextureAddressMode_ClampToTransparent,
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
