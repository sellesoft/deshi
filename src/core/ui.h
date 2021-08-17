#pragma once
#ifndef DESHI_UI_H
#define DESHI_UI_H

#include "renderer.h"
#include "../defines.h"
#include "../math/VectorMatrix.h"
#include "../utils/color.h"
#include "../utils/string.h"
#include "../utils/font.h"

enum UIStyleVar : u32 {
	UIStyleVar_WindowPadding,	    // default vec2(10, 10)      spacing between every item and the edges of the window
	UIStyleVar_ItemSpacing,         // default vec2(1, 1)	     spacing between items within a window
	UIStyleVar_WindowBorderSize,    // default 1                 border size in pixels                
	UIStyleVar_TitleBarHeight,	    // default font.height * 1.2                                        
	UIStyleVar_TitleTextAlign,      // default vec2(0, 0.5)  	 how title text is aligned in title bar 
	UIStyleVar_ScrollAmount,        // default vec2(5, 5)		 amount to scroll in pixels             
	UIStyleVar_CheckboxSize,        // default vec2(10, 10)      
	UIStyleVar_CheckboxFillPadding, // default 2                 how far from the edge a checkbox's true filling is padding
	UIStyleVar_InputTextTextAlign,  // default vec2(0, 0.5)      how text is aligned within InputText boxes
	UIStyleVar_Font,			    // default "gohufont-11.bdf" currently not changable, as we dont support loading multiple fonts yet
	UIStyleVar_COUNT
};

global_ const char* styleVarStr[] = {
	"UIStyleVar_WindowPadding",
	"UIStyleVar_ItemSpacing",
	"UIStyleVar_WindowBorderSize",
	"UIStyleVar_TitleBarHeight",
	"UIStyleVar_TitleTextAlign",
	"UIStyleVar_ScrollAmount",
	"UIStyleVar_CheckboxSize",
	"UIStyleVar_CheckboxFillPadding",
	"UIStyleVar_InputTextTextAlign",
	"UIStyleVar_Font",
	"UIStyleVar_COUNT"
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
	vec2  checkboxSize;
	float checkboxFillPadding;
	vec2  inputTextTextAlign;
	Font* font; //this is a pointer until I fix font to not store so much shit
	color colors[UIStyleCol_COUNT];
};

enum UITextFlags_ {
	UITextFlags_None = 0,
	UITextFlags_NoWrap = 1 << 0,
	//UITextFlags_PositionAbsolute = 
}; typedef u32 UITextFlags;

enum UIWindowFlags_ {
	UIWindowFlags_None                   = 0,
	UIWindowFlags_NoResize               = 1 << 0,
	UIWindowFlags_NoMove                 = 1 << 1,
	UIWindowFlags_NoTitleBar             = 1 << 2,
	UIWindowFlags_NoBorder               = 1 << 3,
	UIWindowFlags_NoBackground           = 1 << 4,
	UIWindowFlags_NoScrollX              = 1 << 5,
	UIWindowFlags_NoScrollY              = 1 << 6,
	UIWindowFlags_NoScroll               = UIWindowFlags_NoScrollX | UIWindowFlags_NoScrollY,
	UIWindowFlags_NoFocus                = 1 << 7,
	UIWindowFlags_FocusOnHover           = 1 << 8,
	UIWindowFlags_NoMinimize             = 1 << 9,
	UIWindowFlags_NoMinimizeButton       = 1 << 10,
	UIWindowFlags_DontSetGlobalHoverFlag = 1 << 11,
	UIWindowFlags_FitAllElements         = 1 << 12, //attempts to fit the window's size to all called elements
	
	UIWindowFlags_Invisible    = UIWindowFlags_NoMove | UIWindowFlags_NoTitleBar | UIWindowFlags_NoResize | UIWindowFlags_NoBackground | UIWindowFlags_NoFocus
}; typedef u32 UIWindowFlags;


enum UIInputTextFlags_ {
	UIInputTextFlags_NONE                  = 0,
	UIInputTextFlags_EnterReturnsTrue      = 1 << 0,
	UIInputTextFlags_AnyChangeReturnsTrue  = 1 << 1,
	UIInputTextFlags_CallbackTab           = 1 << 2,
	UIInputTextFlags_CallbackEnter         = 1 << 3,
	UIInputTextFlags_CallbackAlways        = 1 << 4,
	UIInputTextFlags_CallbackUpDown        = 1 << 5,
	UIInputTextFlags_NoBackground          = 1 << 6,
	UIInputTextFlags_SetCursorToEndOnEnter = 1 << 7,
    
}; typedef u32 UIInputTextFlags;

struct UIInputTextCallbackData {
	UIInputTextFlags eventFlag; //the flag that caused the call back
	UIInputTextFlags flags;    //the flags that the input text item has
	void* userData;           //custom user data
    
	u8       character;        //character that was input  | r
	Key::Key eventKey;         //key pressed on callback   | r
	string*  buffer;           //buffer pointer			   | r/w
	u32      cursorPos;		   //cursor position		   | r/w
	u32      selectionStart;   //                          | r/w -- == selection end when no selection
	u32      selectionEnd;     //                          | r/w
};
typedef u32 (*UIInputTextCallback)(UIInputTextCallbackData* data);

struct UIInputTextState {
	u32    id;                    //id the state belongs to
	u32    cursor = 0;            //what character in the buffer the cursor is infront of, 0 being all the way to the left
	f32    cursorBlinkTime;       //time it takes for the cursor to blink
	f32    scroll;                //scroll offset on x
	string buffer;                //internal buffer, in case user buffer disappears
	u32    selectStart;           //beginning of text selection
	u32    selectEnd;	          //end of text selection
	UIInputTextCallback callback;
	TIMER_START(timeSinceTyped);  //timer to time how long its been since typing, for cursor
};

enum UIDrawType : u32 {
	UIDrawType_Rectangle,
	UIDrawType_FilledRectangle,
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
	color color;
	
	//rectangles have dimensions
	vec2 dimensions;

	//lines have a second position
	vec2 position2;
	
	//line thickness
	float thickness;
	
	//for use by text draw call
	string text;
    
	//determines if the drawCmd should be considered when using UIWindowFlag_FitAllElements
	bool trackedForFit = 1;
	
	vec2 scissorOffset = vec2(0, 0);
	vec2 scissorExtent = vec2(-1,0);
};

enum UIItemType : u32 {
	UIItemType_Base,      //base window draw commands
	UIItemType_Abstract,  //any single drawcall such as a line, rectangle, circle, etc
	UIItemType_Text,      //Text()
	UIItemType_InputText, //InputText()
	UIItemType_Button,    //Button()
	UIItemType_Checkbox,  //Checkbox()
};

//an item such as a button, checkbox, or input text
//this is meant to group draw commands and provide a bounding box for them, using a position
//and overall size. 
//this does have a drawback, in our final drawing loop we have to add one more for loop to loop
//over all items and then their draw calls.
//however this method of storing things shoudl help with positioning items and such later on
struct UIItem {
	//these 3 elements can always be initalized by simply doing
	//UIItem item{ UIItemType_TYPE, curwin->cursor, style };
	//when you create the item in the cpp
	UIItemType type;
	vec2       initialCurPos; //cursor position before this item moved it 
	UIStyle    style;         //style at the time of making the item

	
	vec2 position; //relative to the window its being held in
	vec2 size;

	//all draw command positions are relative to the items position
	array<UIDrawCmd> drawCmds;
};

//A window is meant to be a way to easily position widgets relative to a parent
struct UIWindow {
	string name;
	
	union {
		vec2 position;
		struct { float x; float y; };
	};
	
	union {
		vec2 dimensions;
		struct { float width; float height; };
	};
	
	union {
		vec2 scroll;
		struct { float scx; float scy; };
	};
	
	vec2 maxScroll;
	
	//interior window cursor that's relative to its upper left corner
	union {
		vec2 cursor;
		struct { float curx; float cury; };
	};
	
	UIWindowFlags flags;
	

	//base items are always drawn before items and is just a way to defer drawing 
	//base window stuff to EndWindow(), so we can do dynamic sizing
	array<UIItem> items;
	array<UIItem> baseItems;
	
	UIItem* hoveredItem = 0;
	
	
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

};


//functions in this namespace are Immediate Mode, so they only last 1 frame
//UI was initially designed almost entirely after ImGui in order to allow us to use it like you would ImGui
//but without all the stuff from ImGui we don't really need in an engine
//most of the code is written using ImGui as reference however some design is different and I may
//come back here and write out what is and isnt
namespace UI {
	
	//helpers
	vec2    CalcTextSize(string text);
	void    SetNextItemActive();
	UIStyle GetStyle();
	void    SameLine();
	vec2    GetLastItemPos();
	vec2    GetLastItemSize();


    
	//primitives
	void Rect(vec2 pos, vec2 dimen, color color = color::WHITE);
	void RectFilled(vec2 pos, vec2 dimen, color color = color::WHITE);
    
	void Line(vec2 start, vec2 end, float thickness = 1, color color = color::WHITE);
    
	void Text(string text, UITextFlags flags = 0);
	void Text(string text, vec2 pos, UITextFlags flags = 0);
	void Text(string text, color color, UITextFlags flags = 0);
	void Text(string text, vec2 pos, color color, UITextFlags flags = 0);
    
	//widgets
	//NOTE: I probably should make a SetNextItemPos as well, but I like being able to do posiiton inline, not sure yet
	void SetNextItemSize(vec2 size);
    
	bool Button(string text);
	bool Button(string text, vec2 pos);
	bool Button(string text, color color);
	bool Button(string text, vec2 pos, color color);
    
	void Checkbox(string label, bool* b);
    
	//these overloads are kind of silly change them eventually
	//InputText takes in a buffer and modifies it according to input and works much like ImGui's InputText
	//However there are overloads that will return it's UIInputTextState, allowing you to directly r/w some internal information of the
	//InputText item. This should only be used if you have a good reason to!
	bool InputText(string label, string& buffer, u32 maxChars, UIInputTextFlags flags = 0);
	bool InputText(string label, string& buffer, u32 maxChars, UIInputTextCallback callbackFunc, UIInputTextFlags flags = 0);
	bool InputText(string label, string& buffer, u32 maxChars, UIInputTextState*& getInputTextState, UIInputTextFlags flags = 0);
	bool InputText(string label, string& buffer, u32 maxChars, vec2 pos, UIInputTextFlags flags = 0);
	bool InputText(string label, string& buffer, u32 maxChars, vec2 pos, UIInputTextCallback callbackFunc, UIInputTextFlags flags = 0);
	bool InputText(string label, string& buffer, u32 maxChars, vec2 pos, UIInputTextState*& getInputTextState, UIInputTextFlags flags = 0);

    
	//windows
	void BeginWindow(string name, vec2 pos, vec2 dimensions, UIWindowFlags flags = 0);
	void EndWindow();
	void SetNextWindowPos(vec2 pos);
	void SetNextWindowPos(float x, float y);
	void SetNextWindowSize(vec2 size);		  //when you set a windows size through this you aren't supposed to take into account the titlebar!
	void SetNextWindowSize(float x, float y); //when you set a windows size through this you aren't supposed to take into account the titlebar!
	void SetWindowName(string name);
	bool IsWinHovered();
	bool AnyWinHovered();
	void ShowDebugWindowOf(string name);
    
	//push/pop functions
	void PushColor(UIStyleCol idx, color color);
	void PushVar(UIStyleVar idx, float style);
	void PushVar(UIStyleVar idx, vec2 style);
    
	void PopColor(u32 count = 1);
	void PopVar(u32 count = 1);
    
	void Init();
	void Update();
    
}; //namespace UI

#endif //DESHI_UI_H