#pragma once
#ifndef DESHI_COLOR_H
#define DESHI_COLOR_H

#include "../defines.h"

#define COLORU32_RMASK 0x000000FF
#define COLORU32_GMASK 0x0000FF00
#define COLORU32_BMASK 0x00FF0000
#define COLORU32_AMASK 0xFF000000
#define COLORU32_RSHIFT 0
#define COLORU32_GSHIFT 8
#define COLORU32_BSHIFT 16
#define COLORU32_ASHIFT 24
#define PackColorU32(R,G,B,A) PackU32(R,G,B,A)

#define randcolor color(rand() % 255, rand() % 255, rand() % 255)

struct color{
	union{
		u32 rgba = 0;
		struct{ u8 r, g, b, a; };
	};
	
	color();
	color(u8 r, u8 g, u8 b, u8 a = 255);
	color(u32 rgba);
	
	void  operator*=(float rhs);
	bool  operator==(color rhs) const;
	color operator* (color rhs) const;
	color operator* (float rhs) const;
	color operator/ (float rhs) const;
	
	static color FloatsToColor(f32 r, f32 g, f32 b, f32 a);
	static void  FillFloat3FromU32(f32* floats, u32 color);
	static void  FillFloat4FromU32(f32* floats, u32 color);
};

////////////////////
//// @constants ////
////////////////////
inline global_ color Color_NONE            = color(  0,  0,  0,  0);
inline global_ color Color_White           = color(255,255,255,255);
inline global_ color Color_Black           = color(  0,  0,  0,255);
inline global_ color Color_Grey            = color(128,128,128,255);
inline global_ color Color_Red             = color(255,  0,  0,255);
inline global_ color Color_Green           = color(  0,255,  0,255);
inline global_ color Color_Blue            = color(  0,  0,255,255);
inline global_ color Color_Yellow          = color(255,255,  0,255);
inline global_ color Color_Cyan            = color(  0,255,255,255);
inline global_ color Color_Magenta         = color(255,  0,255,255);
inline global_ color Color_LightGrey       = color(192,192,192,255);
inline global_ color Color_LightRed        = color(255,128,128,255);
inline global_ color Color_LightGreen      = color(128,255,128,255);
inline global_ color Color_LightBlue       = color(128,128,255,255);
inline global_ color Color_LightYellow     = color(245,234, 97,255);
inline global_ color Color_LightCyan       = color(224,255,255,255);
inline global_ color Color_LightMagenta    = color(204,153,204,255);
inline global_ color Color_DarkGrey        = color( 64, 64, 64,255);
inline global_ color Color_DarkRed         = color(128,  0,  0,255);
inline global_ color Color_DarkGreen       = color(  0,128,  0,255);
inline global_ color Color_DarkBlue        = color(  0,  0,128,255);
inline global_ color Color_DarkYellow      = color(128,128,  0,255);
inline global_ color Color_DarkCyan        = color(  0,128,128,255);
inline global_ color Color_DarkMagenta     = color(128,  0,128,255);
inline global_ color Color_VeryDarkGrey    = color( 32, 32, 32,255);
inline global_ color Color_VeryDarkRed     = color( 64,  0,  0,255);
inline global_ color Color_VeryDarkGreen   = color(  0, 64,  0,255);
inline global_ color Color_VeryDarkBlue    = color(  0,  0, 64,255);
inline global_ color Color_VeryDarkYellow  = color( 64, 64,  0,255);
inline global_ color Color_VeryDarkCyan    = color(  0, 64, 64,255);
inline global_ color Color_VeryDarkMagenta = color( 64,  0, 64,255);
inline global_ color Color_Clear       = Color_NONE;
inline global_ color Color_Transparent = Color_NONE; 
inline global_ color Color_Blank       = Color_NONE;
inline global_ color Color_Gray         = Color_Grey; 
inline global_ color Color_LightGray    = Color_LightGrey; 
inline global_ color Color_DarkGray     = Color_DarkGrey;
inline global_ color Color_VeryDarkGray = Color_VeryDarkGrey;

///////////////////////
//// @constructors ////
///////////////////////
inline color::color(){DPZoneScoped;
	r = 0; g = 0; b = 0; a = 0;
};

inline color::color(u8 _r, u8 _g, u8 _b, u8 _a){DPZoneScoped;
	r = _r; g = _g; b = _b; a = _a;
}

inline color::color(u32 _rgba){DPZoneScoped;
	rgba = ByteSwap32(_rgba);
}

////////////////////
//// @operators ////
////////////////////
inline void color::operator*=(float rhs){DPZoneScoped;
	r = u8((f32)r * rhs); g = u8((f32)g * rhs); b = u8((f32)b * rhs);
}

inline color color::operator* (color rhs) const{DPZoneScoped;
	return color(r*rhs.r, g*rhs.g, b*rhs.b, a);
}

inline bool color::operator==(color rhs) const{DPZoneScoped;
	return rgba == rhs.rgba;
}

inline color color::operator* (float rhs) const{DPZoneScoped;
	return color(u8((f32)r * rhs), u8((f32)g * rhs), u8((f32)b * rhs), a);
}

inline color color::operator/ (float rhs) const{DPZoneScoped;
	return color(u8((f32)r / rhs), u8((f32)g / rhs), u8((f32)b / rhs), a);
}

///////////////////////////
//// @static functions ////
///////////////////////////
inline color color::FloatsToColor(f32 r, f32 g, f32 b, f32 a){DPZoneScoped;
	return color((u8)(r * 255.0f),
				 (u8)(g * 255.0f),
				 (u8)(b * 255.0f),
				 (u8)(a * 255.0f));
}

inline void color::FillFloat3FromU32(f32* floats, u32 color){DPZoneScoped;
	*(floats+0) = (f32)((color >> COLORU32_RSHIFT) & 0xFF) / 255.0f;
	*(floats+1) = (f32)((color >> COLORU32_GSHIFT) & 0xFF) / 255.0f;
	*(floats+2) = (f32)((color >> COLORU32_BSHIFT) & 0xFF) / 255.0f;
}

inline void color::FillFloat4FromU32(f32* floats, u32 color){DPZoneScoped;
	*(floats+0) = (f32)((color >> COLORU32_RSHIFT) & 0xFF) / 255.0f;
	*(floats+1) = (f32)((color >> COLORU32_GSHIFT) & 0xFF) / 255.0f;
	*(floats+2) = (f32)((color >> COLORU32_BSHIFT) & 0xFF) / 255.0f;
	*(floats+3) = (f32)((color >> COLORU32_ASHIFT) & 0xFF) / 255.0f;
}

#endif //DESHI_COLOR_H