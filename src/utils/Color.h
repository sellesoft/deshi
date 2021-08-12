#pragma once
#ifndef DESHI_COLOR_H
#define DESHI_COLOR_H

#include "string.h"
#include "../defines.h"

#define COLORU32_RMASK 0x000000FF
#define COLORU32_GMASK 0x0000FF00
#define COLORU32_BMASK 0x00FF0000
#define COLORU32_AMASK 0xFF000000
#define COLORU32_RSHIFT 0
#define COLORU32_GSHIFT 8
#define COLORU32_BSHIFT 16
#define COLORU32_ASHIFT 24
#define PACKCOLORU32(R,G,B,A) (((u32)(A)<<COLORU32_ASHIFT) | ((u32)(B)<<COLORU32_BSHIFT) | ((u32)(G)<<COLORU32_GSHIFT) | ((u32)(R)<<COLORU32_RSHIFT))

struct color{
	u8 r, g, b, a;
	
	static color BLANK, WHITE, BLACK,
	LIGHT_GREY,    GREY,    DARK_GREY,    VERY_DARK_GREY,
	LIGHT_RED,     RED,     DARK_RED,     VERY_DARK_RED,
	LIGHT_YELLOW,  YELLOW,  DARK_YELLOW,  VERY_DARK_YELLOW,
	LIGHT_GREEN,   GREEN,   DARK_GREEN,   VERY_DARK_GREEN,
	LIGHT_CYAN,    CYAN,    DARK_CYAN,    VERY_DARK_CYAN,
	LIGHT_BLUE,    BLUE,    DARK_BLUE,    VERY_DARK_BLUE,
	LIGHT_MAGENTA, MAGENTA, DARK_MAGENTA, VERY_DARK_MAGENTA;
	
    color();
    color(u8 r, u8 g, u8 b);
    color(u8 r, u8 g, u8 b, u8 a);
    color(u32 hex);
    
    void  operator*=(float rhs);
    bool  operator==(color ri) const;
    color operator* (color rhs) const;
    color operator* (float rhs) const;
    color operator/ (float rhs) const;
    
    color  getrandcol() const;
    void   invert();
    color  rinvert() const;
    void   randcol();
    u32    R8G8B8A8_UNORM() const;
    void   FillFloat3(f32* floats) const;
    void   FillFloat4(f32* floats) const;
    string str() const;
    
    static u32   PackColorU32(const color& color);
    static color FloatsToColor(f32 r, f32 g, f32 b, f32 a);
    static void  FillFloat3FromU32(f32* floats, u32 color);
    static void  FillFloat4FromU32(f32* floats, u32 color);
};

////////////////////
//// @constants ////
////////////////////
inline color
color::BLANK(0,0,0,0), color::WHITE(255,255,255), color::BLACK(0,0,0),
color::LIGHT_GREY   (230,230,230),color::GREY   (192,192,192),color::DARK_GREY   (128,128,128),color::VERY_DARK_GREY   (64,64,64),
color::LIGHT_RED    (255,204,203),color::RED    (255,  0,  0),color::DARK_RED    (128,  0,  0),color::VERY_DARK_RED    (64, 0, 0),
color::LIGHT_YELLOW (245,234, 97),color::YELLOW (255,255,  0),color::DARK_YELLOW (128,128,  0),color::VERY_DARK_YELLOW (64,64, 0),
color::LIGHT_GREEN  (183,223,137),color::GREEN  (  0,255,  0),color::DARK_GREEN  (  0,128,  0),color::VERY_DARK_GREEN  ( 0,64, 0),
color::LIGHT_CYAN   (224,255,255),color::CYAN   (  0,255,255),color::DARK_CYAN   (  0,128,128),color::VERY_DARK_CYAN   ( 0,64,64),
color::LIGHT_BLUE   ( 81,114,172),color::BLUE   (  0,  0,255),color::DARK_BLUE   (  0,  0,128),color::VERY_DARK_BLUE   ( 0, 0,64),
color::LIGHT_MAGENTA(204,153,204),color::MAGENTA(255,  0,255),color::DARK_MAGENTA(128,  0,128),color::VERY_DARK_MAGENTA(64, 0,64);

///////////////////////
//// @constructors ////
///////////////////////
inline color::color(){
    r = 0; g = 0; b = 0; a = 0;
};

inline color::color(u8 _r, u8 _g, u8 _b){
    r = _r; g = _g; b = _b; a = 255;
}

inline color::color(u8 _r, u8 _g, u8 _b, u8 _a){
    r = _r; g = _g; b = _b; a = _a;
}

inline color::color(u32 hex){
    r = ((hex >> 16) & 0xFF);
    g = ((hex >> 8) & 0xFF);
    b = ((hex)     & 0xFF);
    a = 255;
}

////////////////////
//// @operators ////
////////////////////
inline void color::operator*=(float rhs){
    r *= rhs; g *= rhs; b *= rhs; a *= rhs;
}

inline color color::operator* (color rhs) const{
    return color(r * rhs.r, g * rhs.g, b * rhs.b);
}

inline bool color::operator==(color ri) const{
    if (r == ri.r && g == ri.g && b == ri.b) return true;
    return false;
}

inline color color::operator* (float rhs) const{
    return color(r*rhs, g*rhs, b*rhs, a);
}

inline color color::operator/ (float rhs) const{
    return color(r / rhs, g / rhs, b / rhs, a);
}

////////////////////
//// @functions ////
////////////////////
inline color color::getrandcol() const{
	return color(rand()%255 + 1, rand()%255 + 1, rand()%255 + 1);
}

inline color color::rinvert() const{ //fancy name huh
    return color(255 - r, 255 - b, 255 - g, a);
}

inline void color::invert(){
    r = 255 - r; b = 255 - b; g = 255 - g;
}

inline void color::randcol(){
    this->r = rand() % 255 + 1;
    this->g = rand() % 255 + 1;
    this->b = rand() % 255 + 1;
}

inline u32 color::R8G8B8A8_UNORM() const{
    return ((u32)a << 24) | ((u32)b << 16) | ((u32)g << 8) | ((u32)r << 0);
}

inline void color::FillFloat3(f32* floats) const{
    *(floats+0) = (f32)r / 255.0f;
    *(floats+1) = (f32)g / 255.0f;
    *(floats+2) = (f32)b / 255.0f;
}

inline void color::FillFloat4(f32* floats) const{
    *(floats+0) = (f32)r / 255.0f;
    *(floats+1) = (f32)g / 255.0f;
    *(floats+2) = (f32)b / 255.0f;
    *(floats+3) = (f32)a / 255.0f;
}

inline string color::str() const{
    return "C{" + string::toStr(r) + ", " + string::toStr(g) + ", " + string::toStr(b) + ", " + string::toStr(a) + "}";
}

///////////////////////////
//// @static functions ////
///////////////////////////
inline u32 color::PackColorU32(const color& color){
    return ((u32)color.a << 24) | ((u32)color.b << 16) | ((u32)color.g << 8) | ((u32)color.r << 0);
}

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