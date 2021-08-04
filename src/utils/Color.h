#pragma once
#ifndef DESHI_COLOR_H
#define DESHI_COLOR_H

#include "../defines.h"
#include <string>

#define RANDCOLOR Color(rand() % 255 + 1, rand() % 255 + 1, rand() % 255 + 1)
#define COLORU32_RMASK 0x000000FF
#define COLORU32_GMASK 0x0000FF00
#define COLORU32_BMASK 0x00FF0000
#define COLORU32_AMASK 0xFF000000
#define COLORU32_RSHIFT 0
#define COLORU32_GSHIFT 8
#define COLORU32_BSHIFT 16
#define COLORU32_ASHIFT 24
#define PACKCOLORU32(R,G,B,A) (((u32)(A)<<COLORU32_ASHIFT) | ((u32)(B)<<COLORU32_BSHIFT) | ((u32)(G)<<COLORU32_GSHIFT) | ((u32)(R)<<COLORU32_RSHIFT))

struct Color {
	u8 r, g, b, a;
	
	static Color BLANK, WHITE, BLACK,
	LIGHT_GREY,    GREY,    DARK_GREY,    VERY_DARK_GREY,
	LIGHT_RED,     RED,     DARK_RED,     VERY_DARK_RED,
	LIGHT_YELLOW,  YELLOW,  DARK_YELLOW,  VERY_DARK_YELLOW,
	LIGHT_GREEN,   GREEN,   DARK_GREEN,   VERY_DARK_GREEN,
	LIGHT_CYAN,    CYAN,    DARK_CYAN,    VERY_DARK_CYAN,
	LIGHT_BLUE,    BLUE,    DARK_BLUE,    VERY_DARK_BLUE,
	LIGHT_MAGENTA, MAGENTA, DARK_MAGENTA, VERY_DARK_MAGENTA;
	
	Color() {
		r = 0; g = 0; b = 0; a = 0;
	};
	
	Color(u8 r, u8 g, u8 b) {
		this->r = r;
		this->g = g;
		this->b = b;
		a = 255;
	}
	
	Color(u8 r, u8 g, u8 b, u8 a) {
		this->r = r;
		this->g = g;
		this->b = b;
		this->a = a;
	}
	
	//hex to rgb
	Color(int hex) {
		r = ((hex >> 16) & 0xFF);
		g = ((hex >> 8) & 0xFF);
		b = ((hex)     & 0xFF);
		a = 255;
	}
	
	//TODO(sushi, Col) implement more operators for colors maybe
	bool operator == (Color ri) {
		if (r == ri.r && g == ri.g && b == ri.b) return true;
		return false;
	}
	
	//kind of useless
	Color operator * (Color rhs) {
		return Color(r * rhs.r, g * rhs.g, b * rhs.b);
	}
	
	Color operator * (float rhs) {
		return Color(r*rhs, g*rhs, b*rhs, a);
	}
	
	Color operator / (float rhs) {
		return Color(r / rhs, g / rhs, b / rhs, a);
	}
	
	void operator *= (float rhs) {
		this->r *= rhs; this->g *= rhs; this->b *= rhs; this->a *= rhs;
	}
	
	Color operator * (float& rhs) const {
		return Color(r * rhs, g * rhs, b * rhs);
	}
	
	Color rinvert() { //fancy name huh
		return Color(255 - r, 255 - b, 255 - g, a);
	}
	
	void invert() {
		r = 255 - r; b = 255 - b; g = 255 - g;
	}
	
	void randcol() {
		this->r = rand() % 255 + 1;
		this->g = rand() % 255 + 1;
		this->b = rand() % 255 + 1;
	}
	
	u32 R8G8B8A8_UNORM(){
		return ((u32)a << 24) | ((u32)b << 16) | ((u32)g << 8) | ((u32)r << 0);
	}
	
	void FillFloat3(f32* floats){
		*(floats+0) = (f32)r / 255.0f;
		*(floats+1) = (f32)g / 255.0f;
		*(floats+2) = (f32)b / 255.0f;
	}
	
	void FillFloat4(f32* floats){
		*(floats+0) = (f32)r / 255.0f;
		*(floats+1) = (f32)g / 255.0f;
		*(floats+2) = (f32)b / 255.0f;
		*(floats+3) = (f32)a / 255.0f;
	}
	
	Color getrandcol();
	
	std::string str() {
		return "C{" + std::to_string(r) + ", " + std::to_string(g) + ", " + std::to_string(b) + ", " + std::to_string(a) + "}";
	}
	
	static Color FloatsToColor(f32 r, f32 g, f32 b, f32 a){
		return Color((u8)(r / 255.0f),
					 (u8)(g / 255.0f),
					 (u8)(b / 255.0f),
					 (u8)(a / 255.0f));
	}
	
	static u32 PackColorU32(const Color& color){
		return ((u32)color.a << 24) | ((u32)color.b << 16) | ((u32)color.g << 8) | ((u32)color.r << 0);
	}
	
	static void FillFloat3FromU32(f32* floats, u32 color){
		*(floats+0) = (f32)((color >> COLORU32_RSHIFT) & 0xFF) / 255.0f;
		*(floats+1) = (f32)((color >> COLORU32_GSHIFT) & 0xFF) / 255.0f;
		*(floats+2) = (f32)((color >> COLORU32_BSHIFT) & 0xFF) / 255.0f;
	}
	
	static void FillFloat4FromU32(f32* floats, u32 color){
		*(floats+0) = (f32)((color >> COLORU32_RSHIFT) & 0xFF) / 255.0f;
		*(floats+1) = (f32)((color >> COLORU32_GSHIFT) & 0xFF) / 255.0f;
		*(floats+2) = (f32)((color >> COLORU32_BSHIFT) & 0xFF) / 255.0f;
		*(floats+3) = (f32)((color >> COLORU32_ASHIFT) & 0xFF) / 255.0f;
	}
};

//// Static Constants ////

//copied from pge see licence.txt so we dont get in twoubal!!!
inline Color
Color::BLANK(0,0,0,0), Color::WHITE(255,255,255), Color::BLACK(0,0,0),
Color::LIGHT_GREY   (230,230,230),Color::GREY   (192,192,192),Color::DARK_GREY   (128,128,128),Color::VERY_DARK_GREY   (64,64,64),
Color::LIGHT_RED    (255,204,203),Color::RED    (255,  0,  0),Color::DARK_RED    (128,  0,  0),Color::VERY_DARK_RED    (64, 0, 0),
Color::LIGHT_YELLOW (245,234, 97),Color::YELLOW (255,255,  0),Color::DARK_YELLOW (128,128,  0),Color::VERY_DARK_YELLOW (64,64, 0),
Color::LIGHT_GREEN  (183,223,137),Color::GREEN  (  0,255,  0),Color::DARK_GREEN  (  0,128,  0),Color::VERY_DARK_GREEN  ( 0,64, 0),
Color::LIGHT_CYAN   (224,255,255),Color::CYAN   (  0,255,255),Color::DARK_CYAN   (  0,128,128),Color::VERY_DARK_CYAN   ( 0,64,64),
Color::LIGHT_BLUE   ( 81,114,172),Color::BLUE   (  0,  0,255),Color::DARK_BLUE   (  0,  0,128),Color::VERY_DARK_BLUE   ( 0, 0,64),
Color::LIGHT_MAGENTA(204,153,204),Color::MAGENTA(255,  0,255),Color::DARK_MAGENTA(128,  0,128),Color::VERY_DARK_MAGENTA(64, 0,64);

//// Functions ////

inline Color Color::getrandcol() {
	return Color(rand() % 255 + 1, rand() % 255 + 1, rand() % 255 + 1);
}

#endif //DESHI_COLOR_H