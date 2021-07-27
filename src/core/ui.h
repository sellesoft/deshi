#pragma once
#ifndef DESHI_UI_H
#define DESHI_UI_H

#include "renderer.h"
#include "../defines.h"
#include "../math/VectorMatrix.h"
#include "../utils/Color.h"
#include "../utils/string.h"
#include "../utils/font.h"

enum UIStyleVar : u32 {
	UIStyleVar_WindowPadding,
	UIStyleVar_WindowBorderSize,
	UIStyleVar_TitleBarHeight,
	UIStyleVar_Font,
	UIStyleVar_COUNT
};

enum UIStyleCol : u32 {
	UIStyleCol_Text,
	UIStyleCol_WindowBg,
	UIStyleCol_Border,
	UIStyleCol_FrameBg,
	UIStyleCol_FrameBgActive,
	UIStyleCol_TitleBg,
	UIStyleCol_TitleBgActive,
	UIStyleCol_COUNT
};

enum UIWindowFlags_ {
	UIWindowFlags_None         = 0,
	UIWindowFlags_NoResize     = 1 << 0,
	UIWindowFlags_NoMove       = 1 << 1,
	UIWindowFlags_NoTitleBar   = 1 << 2,
	UIWindowFlags_NoBorder     = 1 << 3,
	UIWindowFlags_NoBackground = 1 << 4,
	UIWindowFlags_Invisible    = UIWindowFlags_NoMove | UIWindowFlags_NoTitleBar | UIWindowFlags_NoResize | UIWindowFlags_NoBackground
}; typedef u32 UIWindowFlags;

//A window is meant to be a way to easily position widgets relative to a parent
struct UIWindow {
	string name;
	
	vec2 position;
	vec2 dimensions;
	
	//interior window cursor that's relative to its upper left corner
	//if the window has a titlebar then the cursor's origin does not include the title bar
	//TODO(sushi, Ui) maybe make a window flag to change this
	vec2 cursor;

	UIWindowFlags flags;
};


//functions in this namespace are Immediate Mode, so they only last 1 frame
namespace UI {

	//primitives
	void RectFilled(f32 x, f32 y, f32 width, f32 height, Color color = Color::WHITE);

	void Line(f32 x1, f32 y1, f32 x2, f32 y2, float thickness = 1, Color color = Color::WHITE);
	void Line(vec2 start, vec2 end, float thickness = 1, Color color = Color::WHITE);

	void Text(string text);
	void Text(string text, vec2 pos);
	void Text(string text, Color color);
	void Text(string text, vec2 pos, Color color);

	//windows
	void BeginWindow(string name, vec2 pos, vec2 dimensions, UIWindowFlags flags = 0);
	void EndWindow();

	void Init();

}; //namespace UI

#endif //DESHI_UI_H