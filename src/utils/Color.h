#pragma once

struct Color {
	
	uint8 r, g, b, a;
	
	static const Color
		GREY,    DARK_GREY,    VERY_DARK_GREY,
	RED,     DARK_RED,     VERY_DARK_RED,
	YELLOW,  DARK_YELLOW,  VERY_DARK_YELLOW,
	GREEN,   DARK_GREEN,   VERY_DARK_GREEN,
	CYAN,    DARK_CYAN,    VERY_DARK_CYAN,
	BLUE,    DARK_BLUE,    VERY_DARK_BLUE,
	MAGENTA, DARK_MAGENTA, VERY_DARK_MAGENTA,
	WHITE,   BLACK,        BLANK;
	
	Color() {
		r = 0; g = 0; b = 0; a = 0;
	};
	
	Color(uint8 r, uint8 g, uint8 b) {
		this->r = r;
		this->g = g;
		this->b = b;
		a = 255;
	}
	
	Color(uint8 r, uint8 g, uint8 b, uint8 a) {
		this->r = r;
		this->g = g;
		this->b = b;
		this->a = a;
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
	Color getrandcol();
};

//// Static Constants ////

//copied from pge see licence.txt so we dont get in twoubal!!!
inline const Color
Color::GREY(192, 192, 192),  Color::DARK_GREY(128, 128, 128),  Color::VERY_DARK_GREY(64, 64, 64),
Color::RED(255, 0, 0),       Color::DARK_RED(128, 0, 0),       Color::VERY_DARK_RED(64, 0, 0),
Color::YELLOW(255, 255, 0),  Color::DARK_YELLOW(128, 128, 0),  Color::VERY_DARK_YELLOW(64, 64, 0),
Color::GREEN(0, 255, 0),     Color::DARK_GREEN(0, 128, 0),     Color::VERY_DARK_GREEN(0, 64, 0),
Color::CYAN(0, 255, 255),    Color::DARK_CYAN(0, 128, 128),    Color::VERY_DARK_CYAN(0, 64, 64),
Color::BLUE(0, 0, 255),      Color::DARK_BLUE(0, 0, 128),      Color::VERY_DARK_BLUE(0, 0, 64),
Color::MAGENTA(255, 0, 255), Color::DARK_MAGENTA(128, 0, 128), Color::VERY_DARK_MAGENTA(64, 0, 64),
Color::WHITE(255, 255, 255), Color::BLACK(0, 0, 0),            Color::BLANK(0, 0, 0, 0);

//// Functions ////

inline Color Color::getrandcol() {
	return Color(rand() % 255 + 1, rand() % 255 + 1, rand() % 255 + 1);
}