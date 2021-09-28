#pragma once
#ifndef DESHI_FONT_H
#define DESHI_FONT_H

#include "../defines.h"

enum FontType{
	FontType_NONE,
	FontType_BDF,
	FontType_TTF,
	FontType_COUNT
};

struct Font{
	Type type;
	u32  idx;
	char name[DESHI_NAME_SIZE];
	char weight[DESHI_NAME_SIZE];
	u32  width;
	u32  height;
	u32  count;
	u32   ttf_size;
	void* ttf_bake;
};

#endif //DESHI_FONT_H
