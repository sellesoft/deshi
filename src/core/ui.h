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
	UIStyleVar_TitleBarHeight,	 // default font.height * 1.2                                        
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

struct UIStyle {
	vec2  windowPadding;
	vec2  itemSpacing;
	float windowBorderSize;
	float titleBarHeight;
	vec2  titleTextAlign;
	vec2  scrollAmount;
	Font* font; //this is a pointer until I fix font to not store so much shit
	Color colors[UIStyleCol_COUNT];
};

enum UITextFlags_ {
	UITextFlags_None = 0,
	UITextFlags_NoWrap = 1 << 0,
	//UITextFlags_PositionAbsolute = 
}; typedef u32 UITextFlags;

enum UIInputTextFlags_ {
	UIInputTextFlags_NONE             = 0,
	UIInputTextFlags_EnterReturnsTrue = 1 << 0,
	
}; typedef u32 UIInputTextFlags;

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
	
	UIWindowFlags_Invisible    = UIWindowFlags_NoMove | UIWindowFlags_NoTitleBar | UIWindowFlags_NoResize | UIWindowFlags_NoBackground | UIWindowFlags_NoFocus
}; typedef u32 UIWindowFlags;


struct UIInputTextState {
	u32 id;                      //id the state belongs to
	u32 cursor = 0;              //what character in the buffer the cursor is infront of, 0 being all the way to the left
	f32 cursorBlinkTime;         //time it takes for the cursor to blink
	f32 scroll;                  //scroll offset on x
	string buffer;               //internal buffer, in case user buffer disappears
	u32 selectStart;             //beginning of text selection
	u32 selectEnd;	             //end of text selection
	TIMER_START(timeSinceTyped); //timer to time how long its been since typing, for cursor
};


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

	float titleBarHeight = 0;

	//this is the state of style when EndWindow() is called for the window
	//meaning the style for elements before the last bunch could be different
	//if the user changes stuff before ending the window and therefore this should be used carefully!!
	UIStyle style;
	
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
		titleBarHeight = cop.titleBarHeight;
		style = cop.style;
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
		titleBarHeight = cop.titleBarHeight;
		style = cop.style;
		return *this;
	}
	
	
};


//functions in this namespace are Immediate Mode, so they only last 1 frame
//UI was initially designed almost entirely after ImGui in order to allow us to use it like you would ImGui
//but without all the stuff from ImGui we don't really need in an engine
//most of the code is written using ImGui as reference however some design is different and I may
//come back here and write out what is and isnt
namespace UI {
	
	//helpers
	vec2 CalcTextSize(string text);

	//primitives
	void RectFilled(f32 x, f32 y, f32 width, f32 height, Color color = Color::WHITE);
	void RectFilled(vec2 pos, vec2 dimen, Color color = Color::WHITE);

	void Line(f32 x1, f32 y1, f32 x2, f32 y2, float thickness = 1, Color color = Color::WHITE);
	void Line(vec2 start, vec2 end, float thickness = 1, Color color = Color::WHITE);

	void Text(string text, UITextFlags flags = 0);
	void Text(string text, vec2 pos, UITextFlags flags = 0);
	void Text(string text, Color color, UITextFlags flags = 0);
	void Text(string text, vec2 pos, Color color, UITextFlags flags = 0);

	//widgets
	bool Button(string text);
	bool Button(string text, vec2 pos);
	bool Button(string text, Color color);
	bool Button(string text, vec2 pos, Color color);

	void Checkbox(string label, bool* b);

	bool InputText(string label, string& buffer, u32 maxChars = -1, UIInputTextFlags flags = 0);
	bool InputText(string label, string& buffer, vec2 pos, u32 maxChars = -1, UIInputTextFlags flags = 0);


	//windows
	void BeginWindow(string name, vec2 pos, vec2 dimensions, UIWindowFlags flags = 0);
	void EndWindow();
	void SetNextWindowPos(vec2 pos);
	void SetNextWindowPos(float x, float y);
	void SetNextWindowSize(vec2 size);		 //when you set a windows size through this you aren't supposed to take into account the titlebar!
	void SetNextWindowSize(float x, float y); //when you set a windows size through this you aren't supposed to take into account the titlebar!
	void SetWindowName(string name);
	bool IsWinHovered();
	void ShowDebugWindowOf(string name);

	//push/pop functions
	void PushColor(UIStyleCol idx, Color color);
	void PushVar(UIStyleVar idx, float style);
	void PushVar(UIStyleVar idx, vec2 style);

	void PopColor(u32 count = 1);
	void PopVar(u32 count = 1);

	void Init();
	void Update();

}; //namespace UI

#endif //DESHI_UI_H