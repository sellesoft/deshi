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
	UIStyleVar_TitleTextAlign,   //how title text is aligned in title bar, default vec2(0, 0.5)
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
//UI was designed almost entirely after ImGui in order to allow us to use it like you would ImGui
//but without all the stuff from ImGui we don't really need in an engine
//most of the code is written using ImGui as reference however some design is different and I may
//come back here and write out what is and isnt
namespace UI {

	//primitives
	static void RectFilled(f32 x, f32 y, f32 width, f32 height, Color color = Color::WHITE);

	static void Line(f32 x1, f32 y1, f32 x2, f32 y2, float thickness = 1, Color color = Color::WHITE);
	static void Line(vec2 start, vec2 end, float thickness = 1, Color color = Color::WHITE);

	static void Text(string text);
	static void Text(string text, vec2 pos);
	static void Text(string text, Color color);
	static void Text(string text, vec2 pos, Color color);

	//windows
	static void BeginWindow(string name, vec2 pos, vec2 dimensions, UIWindowFlags flags = 0);
	static void EndWindow();

	//push/pop functions
	static void PushColor(UIStyleCol idx, Color color);
	static void PushVar(UIStyleVar idx, float style);
	static void PushVar(UIStyleVar idx, vec2 style);

	static void PopColor(u32 count = 1);
	static void PopStyle(u32 count = 1);



	static void Init();
	static void Update();

}; //namespace UI

#endif //DESHI_UI_H