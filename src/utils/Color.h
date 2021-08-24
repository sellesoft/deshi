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
#define PackColorU32(R,G,B,A) PackU32(A,B,G,R)

enum Colors_ : u32{
    Color_NONE  = PackColorU32(  0,  0,  0,  0),
    Color_White = PackColorU32(255,255,255,255),
    Color_Black = PackColorU32(  0,  0,  0,255),
    Color_Grey  = PackColorU32(128,128,128,255),
    Color_Red     = PackColorU32(255,  0,  0,255),
    Color_Green   = PackColorU32(  0,255,  0,255),
    Color_Blue    = PackColorU32(  0,  0,255,255),
    Color_Yellow  = PackColorU32(255,255,  0,255),
    Color_Cyan    = PackColorU32(  0,255,255,255),
    Color_Magenta = PackColorU32(255,  0,255,255),
    Color_LightGrey    = PackColorU32(192,192,192,255),
    Color_LightRed     = PackColorU32(255,128,128,255),
    Color_LightGreen   = PackColorU32(128,255,128,255),
    Color_LightBlue    = PackColorU32(128,128,255,255),
    Color_LightYellow  = PackColorU32(245,234, 97,255),
    Color_LightCyan    = PackColorU32(224,255,255,255),
    Color_LightMagenta = PackColorU32(204,153,204,255),
    Color_DarkGrey    = PackColorU32( 64, 64, 64,255),
    Color_DarkRed     = PackColorU32(128,  0,  0,255),
    Color_DarkGreen   = PackColorU32(  0,128,  0,255),
    Color_DarkBlue    = PackColorU32(  0,  0,128,255),
    Color_DarkYellow  = PackColorU32(128,128,  0,255),
    Color_DarkCyan    = PackColorU32(  0,128,128,255),
    Color_DarkMagenta = PackColorU32(128,  0,128,255),
    Color_VeryDarkGrey    = PackColorU32( 32, 32, 32,255),
    Color_VeryDarkRed     = PackColorU32( 64,  0,  0,255),
    Color_VeryDarkGreen   = PackColorU32(  0, 64,  0,255),
    Color_VeryDarkBlue    = PackColorU32(  0,  0, 64,255),
    Color_VeryDarkYellow  = PackColorU32( 64, 64,  0,255),
    Color_VeryDarkCyan    = PackColorU32(  0, 64, 64,255),
    Color_VeryDarkMagenta = PackColorU32( 64,  0, 64,255),
    
    Color_Clear = Color_NONE, Color_Transparent = Color_NONE, Color_Blank = Color_NONE,
};

struct color{
    union{
        u32 rgba;
        struct{
            u8 r;
            u8 g;
            u8 b;
            u8 a;
        };
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

///////////////////////
//// @constructors ////
///////////////////////
inline color::color(){
    r = 0; g = 0; b = 0; a = 0;
};

inline color::color(u8 _r, u8 _g, u8 _b, u8 _a){
    r = _r; g = _g; b = _b; a = _a;
}

inline color::color(u32 rgba){
    rgba = rgba;
}

////////////////////
//// @operators ////
////////////////////
inline void color::operator*=(float rhs){
    r *= rhs; g *= rhs; b *= rhs;
}

inline color color::operator* (color rhs) const{
    return color(r * rhs.r, g * rhs.g, b * rhs.b);
}

inline bool color::operator==(color rhs) const{
    return rgba == rhs.rgba;
}

inline color color::operator* (float rhs) const{
    return color(r*rhs, g*rhs, b*rhs, a);
}

inline color color::operator/ (float rhs) const{
    return color(r / rhs, g / rhs, b / rhs, a);
}

///////////////////////////
//// @static functions ////
///////////////////////////
inline color color::FloatsToColor(f32 r, f32 g, f32 b, f32 a){
    return color((u8)(r / 255.0f),
                 (u8)(g / 255.0f),
                 (u8)(b / 255.0f),
                 (u8)(a / 255.0f));
}

inline void color::FillFloat3FromU32(f32* floats, u32 color){
    *(floats+0) = (f32)((color >> COLORU32_RSHIFT) & 0xFF) / 255.0f;
    *(floats+1) = (f32)((color >> COLORU32_GSHIFT) & 0xFF) / 255.0f;
    *(floats+2) = (f32)((color >> COLORU32_BSHIFT) & 0xFF) / 255.0f;
}

inline void color::FillFloat4FromU32(f32* floats, u32 color){
    *(floats+0) = (f32)((color >> COLORU32_RSHIFT) & 0xFF) / 255.0f;
    *(floats+1) = (f32)((color >> COLORU32_GSHIFT) & 0xFF) / 255.0f;
    *(floats+2) = (f32)((color >> COLORU32_BSHIFT) & 0xFF) / 255.0f;
    *(floats+3) = (f32)((color >> COLORU32_ASHIFT) & 0xFF) / 255.0f;
}
#endif //DESHI_COLOR_H