#pragma once
#include "../math/Vector4.h"

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

};

Color getrandcol() {
	return Color(rand() % 255 + 1, rand() % 255 + 1, rand() % 255 + 1);
}

//copied from pge see licence.txt so we dont get in twoubal!!!
inline static const Color
GREY(192, 192, 192),  DARK_GREY(128, 128, 128),  VERY_DARK_GREY(64, 64, 64),
RED(255, 0, 0),       DARK_RED(128, 0, 0),       VERY_DARK_RED(64, 0, 0),
YELLOW(255, 255, 0),  DARK_YELLOW(128, 128, 0),  VERY_DARK_YELLOW(64, 64, 0),
GREEN(0, 255, 0),     DARK_GREEN(0, 128, 0),     VERY_DARK_GREEN(0, 64, 0),
CYAN(0, 255, 255),    DARK_CYAN(0, 128, 128),    VERY_DARK_CYAN(0, 64, 64),
BLUE(0, 0, 255),      DARK_BLUE(0, 0, 128),      VERY_DARK_BLUE(0, 0, 64),
MAGENTA(255, 0, 255), DARK_MAGENTA(128, 0, 128), VERY_DARK_MAGENTA(64, 0, 64),
WHITE(255, 255, 255), BLACK(0, 0, 0),            BLANK(0, 0, 0, 0);