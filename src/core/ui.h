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
	UIStyleVar_WindowPadding,	 // default vec2(10, 10)      spacing between every item and the edges of the window
	UIStyleVar_ItemSpacing,      // default vec2(1, 1)	      spacing between items within a window
	UIStyleVar_WindowBorderSize, // default 1                 border size in pixels                
	UIStyleVar_TitleBarHeight,	 // default font.height * 1.5                                        
	UIStyleVar_TitleTextAlign,   // default vec2(0, 0.5)	  how title text is aligned in title bar 
	UIStyleVar_ScrollAmount,     // default vec2(5, 5)		  amount to scroll in pixels             
	UIStyleVar_Font,			 // default "gohufont-11.bdf" currently not changable, as we dont support loading multiple fonts yet
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

//global styling
struct UIStyle {
	vec2  windowPadding;
	vec2  itemSpacing;
	float windowBorderSize;
	float titleBarHeight;
	vec2  titleTextAlign;
	vec2  scrollAmount;
	Font* font; //this is a pointer until I fix font to not store so much shit
	Color colors[UIStyleCol_COUNT];
} style;

enum UITextFlags_ {
	UITextFlags_None = 0,
	UITextFlags_NoWrap = 1 << 0,
	//UITextFlags_PositionAbsolute = 
}; typedef u32 UITextFlags;

enum UIWindowFlags_ {
	UIWindowFlags_None             = 0,
	UIWindowFlags_NoResize         = 1 << 0,
	UIWindowFlags_NoMove           = 1 << 1,
	UIWindowFlags_NoTitleBar       = 1 << 2,
	UIWindowFlags_NoBorder         = 1 << 3,
	UIWindowFlags_NoBackground     = 1 << 4,
	UIWindowFlags_NoScroll         = 1 << 5,
	UIWindowFlags_FocusOnHover     = 1 << 6,
	UIWindowFlags_NoFocus          = 1 << 7,
	UIWindowFlags_NoMinimize       = 1 << 8,
	UIWindowFlags_NoMinimizeButton = 1 << 9,

	UIWindowFlags_Invisible    = UIWindowFlags_NoMove | UIWindowFlags_NoTitleBar | UIWindowFlags_NoResize | UIWindowFlags_NoBackground
}; typedef u32 UIWindowFlags;

enum UIDrawType : u32 {
	UIDrawType_Rectangle,
	UIDrawType_Line,
	UIDrawType_Text
};

//draw commands store what kind of command it is, and info relative to that command
//this is to be stored on an array on UIWindow and determines what elements it draws when
//we do the rendering pass
struct UIDrawCmd {
	UIDrawType type;

	//all draw commands have a position, this is also considered the start of a line cmd
	vec2 position;

	//all draw commands have a color
	Color color;

	//rectangles have dimensions
	vec2 dimensions;
	//lines have a second position
	vec2 position2;

	//line thickness
	float thickness;
	
	//for use by text draw call
	string text;

	vec2 scissorOffset = vec2(0, 0);
	vec2 scissorExtent = vec2(-1,0);

};

//A window is meant to be a way to easily position widgets relative to a parent
struct UIWindow {
	string name;
	
	union {
		vec2 position;
		struct {
			float x;
			float y;
		};
	};

	union {
		vec2 dimensions;
		struct {
			float width;
			float height;
		};
	};

	union {
		vec2 scroll;
		struct {
			float scx;
			float scy;
		};
	};

	vec2 maxScroll;

	//interior window cursor that's relative to its upper left corner
	//if the window has a titlebar then the cursor's origin does not include the title bar
	//TODO(sushi, Ui) maybe make a window flag to change this
	union {
		vec2 cursor;
		struct {
			float curx;
			float cury;
		};
	};

	UIWindowFlags flags;

	//the difference between these two is that baseDrawCmds holds the commands for drawing the 
	//base of the window, eg the background, title, border, etc.
	//this way I can regenerate the window's base properties if I need to
	//TODO(sushi, Ui) maybe implement this ^
	array<UIDrawCmd> baseDrawCmds;
	array<UIDrawCmd> drawCmds;

	bool hovered = false;
	bool titleHovered = false;

	bool minimized = false;
	bool hidden = false;


	UIWindow() {};

	//I have to do this because I'm using an anonymous struct inside a union and C++ sucks
	//actually i think its literally just cause im using a union, C++ blows 
	UIWindow(const UIWindow& cop) {
		name = cop.name;
		position = cop.position;
		dimensions = cop.dimensions;
		scroll = cop.scroll;
		maxScroll = cop.maxScroll;
		cursor = cop.cursor;
		flags = cop.flags;
		drawCmds = cop.drawCmds;
		baseDrawCmds = cop.baseDrawCmds;
		hovered = cop.hovered;
		titleHovered = cop.titleHovered;
		minimized = cop.minimized;
		hidden = cop.hidden;
	}

	UIWindow& operator= (const UIWindow& cop) {
		name = cop.name; //inst 136
		position = cop.position;
		dimensions = cop.dimensions;
		scroll = cop.scroll;
		maxScroll = cop.maxScroll;
		cursor = cop.cursor;
		flags = cop.flags;
		drawCmds = cop.drawCmds; //inst 139, 142, 145, 148, 151, 154, 157, 160 valid
		baseDrawCmds = cop.baseDrawCmds;
		hovered = cop.hovered;
		titleHovered = cop.titleHovered;
		minimized = cop.minimized;
		hidden = cop.hidden;
		return *this;
	}


};


//functions in this namespace are Immediate Mode, so they only last 1 frame
//UI was designed almost entirely after ImGui in order to allow us to use it like you would ImGui
//but without all the stuff from ImGui we don't really need in an engine
//most of the code is written using ImGui as reference however some design is different and I may
//come back here and write out what is and isnt
namespace UI {

	//helpers
	static vec2 CalcTextSize(string text);

	//primitives
	static void RectFilled(f32 x, f32 y, f32 width, f32 height, Color color = Color::WHITE);

	static void Line(f32 x1, f32 y1, f32 x2, f32 y2, float thickness = 1, Color color = Color::WHITE);
	static void Line(vec2 start, vec2 end, float thickness = 1, Color color = Color::WHITE);

	static void Text(string text, UITextFlags flags = 0);
	static void Text(string text, vec2 pos, UITextFlags flags = 0);
	static void Text(string text, Color color, UITextFlags flags = 0);
	static void Text(string text, vec2 pos, Color color, UITextFlags flags = 0);

	//widgets
	static bool Button(string text);
	static bool Button(string text, vec2 pos);
	static bool Button(string text, Color color);
	static bool Button(string text, vec2 pos, Color color);


	//windows
	static void BeginWindow(string name, vec2 pos, vec2 dimensions, UIWindowFlags flags = 0);
	static void EndWindow();
	static void SetNextWindowPos(vec2 pos);
	static void SetNextWindowPos(float x, float y);
	static void SetNextWindowSize(vec2 size);		 //when you set a windows size through this you aren't supposed to take into account the titlebar!
	static void SetNextWindowSize(float x, float y); //when you set a windows size through this you aren't supposed to take into account the titlebar!
	static void SetWindowName(string name);
	static bool IsWinHovered();
	static void ShowDebugWindowOf(string name);

	//push/pop functions
	static void PushColor(UIStyleCol idx, Color color);
	static void PushVar(UIStyleVar idx, float style);
	static void PushVar(UIStyleVar idx, vec2 style);

	static void PopColor(u32 count = 1);
	static void PopVar(u32 count = 1);

	static void Init();
	static void Update();

}; //namespace UI

#endif //DESHI_UI_H