#pragma once
#ifndef DESHI_UI_H
#define DESHI_UI_H

/*

  
  UI functions are Immediate Mode, so they only last 1 frame
  UI was initially designed almost entirely after ImGui in order to allow us to use it like you would ImGui
  but without all the stuff from ImGui we don't really need in an engine
  most of the code is written using ImGui as reference however some design is different and I may
  come back here and write out what is and isnt


  some ui design guidelines im going to collect:

	ui should never add items to a window internally, only the user adds items, and interally
	we add drawcalls to build the items that the user requests. 
	basically, never use items to build other items.

	it is VERY important that a UIItem function calls AdvanceCursor before ANY functionality involving
	the user interacting with the item visually, as well as after setting the item's size.
	for example in Button we have to make sure we call AdvanceCursor before querying if the user 
	has clicked it or not. this is because Row moves items and if you attempt to query for any 
	interaction before AdvanceCursor correctly positions the item, it will not be in the correct 
	place when the user attempts to interact with it 
	
  BIG TODOS:
  
	Tabs
	An actual table item
	Ability to set flags on Button that allow it to return true if its held, only if mouse is released over it, etc.
	The internal set up for Child Windows is incredibly messy and needs to be redone



*/

#include "renderer.h"
#include "font.h"
#include "../defines.h"
#include "../math/math.h"
#include "../utils/color.h"
#include "../utils/string.h"
#include "../utils/map.h"

enum UIStyleVar : u32 {
	UIStyleVar_WindowPadding,	        // default vec2(10, 10)      spacing between every item and the edges of the window
	UIStyleVar_ItemSpacing,             // default vec2(1, 1)	     spacing between items within a window
	UIStyleVar_WindowBorderSize,        // default 1                 border size in pixels                
	UIStyleVar_TitleBarHeight,	        // default font.height * 1.2                                        
	UIStyleVar_TitleTextAlign,          // default vec2(0, 0.5)  	 how title text is aligned in title bar 
	UIStyleVar_ScrollAmount,            // default vec2(5, 5)		 amount to scroll in pixels             
	UIStyleVar_CheckboxSize,            // default vec2(10, 10)      
	UIStyleVar_CheckboxFillPadding,     // default 2                 how far from the edge a checkbox's true filling is padding
	UIStyleVar_InputTextTextAlign,      // default vec2(0, 0.5)      how text is aligned within InputText boxes
	UIStyleVar_ButtonTextAlign,         // default vec2(0.5, 0.5)    how text is aligned within buttons
	UIStyleVar_HeaderTextAlign,         // default vec2(0.05, 0.5)
	UIStyleVar_ButtonHeightRelToFont,   // default 1.3                height of header box relative to the font height
	UIStyleVar_HeaderHeightRelToFont,   // default 1.3
	UIStyleVar_InputTextHeightRelToFont,// default 1.3
	UIStyleVar_CheckboxHeightRelToFont, // default 1.3
	UIStyleVar_RowItemAlign,            // default vec2(0.5, 0.5)    determines how rows align their items within their cells
	UIStyleVar_RowCellPadding,          // default vec2(10, 10)      the amount of pixels to pad items within cells from the edges of the cell
	UIStyleVar_ScrollBarYWidth,         // default 5
	UIStyleVar_ScrollBarXHeight,        // default 5
	UIStyleVar_IndentAmount,            // default 8
	UIStyleVar_FontHeight,              // default font->height      height of font in pixels
	UIStyleVar_Font,			        // default "gohufont-11.bdf" 
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
	"UIStyleVar_ButtonTextAlign",     
	"UIStyleVar_RowItemAlign",        
	"UIStyleVar_RowCellPadding",    
	"UIStyleVar_FontHeight",
	"UIStyleVar_Font",
	"UIStyleVar_COUNT"
};

enum UIStyleCol : u32 {
	UIStyleCol_Text,
	UIStyleCol_Separator,
	
	UIStyleCol_WindowBg,
	UIStyleCol_Border,
	
	UIStyleCol_FrameBg,
	UIStyleCol_FrameBgHovered,
	UIStyleCol_FrameBgActive,  
	
	UIStyleCol_ScrollBarBg,
	UIStyleCol_ScrollBarBgHovered,
	UIStyleCol_ScrollBarBgActive,
	UIStyleCol_ScrollBarDragger,
	UIStyleCol_ScrollBarDraggerHovered,
	UIStyleCol_ScrollBarDraggerActive,
	
	UIStyleCol_ButtonBg,
	UIStyleCol_ButtonBgActive,
	UIStyleCol_ButtonBgHovered,
	UIStyleCol_ButtonBorder,
	
	UIStyleCol_CheckboxBg,
	UIStyleCol_CheckboxBgActive,
	UIStyleCol_CheckboxBgHovered,
	UIStyleCol_CheckboxFilling,
	UIStyleCol_CheckboxBorder,
	
	UIStyleCol_HeaderBg,
	UIStyleCol_HeaderBgActive,
	UIStyleCol_HeaderBgHovered,
	UIStyleCol_HeaderButton,
	UIStyleCol_HeaderButtonActive,
	UIStyleCol_HeaderButtonHovered,
	UIStyleCol_HeaderBorder,
	
	UIStyleCol_SliderBg,
	UIStyleCol_SliderBgActive,
	UIStyleCol_SliderBgHovered,
	UIStyleCol_SliderBar,
	UIStyleCol_SliderBarActive,
	UIStyleCol_SliderBarHovered,
	UIStyleCol_SliderBorder,
	
	UIStyleCol_InputTextBg,
	UIStyleCol_InputTextBgActive,
	UIStyleCol_InputTextBgHovered,
	UIStyleCol_InputTextBorder,
	
	UIStyleCol_SelectableBg,
	UIStyleCol_SelectableBgActive,
	UIStyleCol_SelectableBgHovered,
	
	UIStyleCol_TitleBg,
	UIStyleCol_TitleBgHovered,
	UIStyleCol_TitleBgActive,
	UIStyleCol_COUNT
};

struct UIStyle {
	vec2 windowPadding;
	vec2 itemSpacing;
	f32  windowBorderSize;
	f32  titleBarHeight;
	vec2 titleTextAlign;
	vec2 scrollAmount;
	vec2 checkboxSize;
	f32  checkboxFillPadding;
	vec2 inputTextTextAlign;
	vec2 buttonTextAlign;
	vec2 headerTextAlign;
	f32  buttonHeightRelToFont;
	f32  headerHeightRelToFont;
	f32  inputTextHeightRelToFont;
	f32  checkboxHeightRelToFont;
	vec2 rowItemAlign;
	vec2 rowCellPadding;
	f32  scrollBarYWidth;
	f32  scrollBarXHeight;
	f32  indentAmount;
	f32  fontHeight;
	
	//special vars that have special push/pop functions
	vec2  globalScale;
	Font* font;
	color colors[UIStyleCol_COUNT];
};

enum UITextFlags_ {
	UITextFlags_None = 0,
	UITextFlags_NoWrap = 1 << 0,
}; typedef u32 UITextFlags;

enum UIWindowFlags_ {
	UIWindowFlags_None                   = 0,
	UIWindowFlags_NoResize               = 1 << 0,
	UIWindowFlags_NoMove                 = 1 << 1,
	UIWindowFlags_NoTitleBar             = 1 << 2,
	UIWindowFlags_NoBorder               = 1 << 3,
	UIWindowFlags_NoBackground           = 1 << 4,
	UIWindowFlags_NoScrollBarX           = 1 << 5,
	UIWindowFlags_NoScrollBarY           = 1 << 6, 
	UIWindowFlags_NoScrollBars           = UIWindowFlags_NoScrollBarX | UIWindowFlags_NoScrollBarY,
	UIWindowFlags_NoScrollX              = 1 << 7 | UIWindowFlags_NoScrollBarX,
	UIWindowFlags_NoScrollY              = 1 << 8 | UIWindowFlags_NoScrollBarY,
	UIWindowFlags_NoScroll               = UIWindowFlags_NoScrollX | UIWindowFlags_NoScrollY,
	UIWindowFlags_NoFocus                = 1 << 9,
	UIWindowFlags_FocusOnHover           = 1 << 10,
	UIWindowFlags_NoMinimize             = 1 << 11,
	UIWindowFlags_NoMinimizeButton       = 1 << 12,
	UIWindowFlags_DontSetGlobalHoverFlag = 1 << 13,
	UIWindowFlags_FitAllElements         = 1 << 14, //attempts to fit the window's size to all called elements
	
	UIWindowFlags_NoInteract = UIWindowFlags_NoMove | UIWindowFlags_NoFocus | UIWindowFlags_NoResize | UIWindowFlags_DontSetGlobalHoverFlag | UIWindowFlags_NoScroll, 
	UIWindowFlags_Invisible  = UIWindowFlags_NoMove | UIWindowFlags_NoTitleBar | UIWindowFlags_NoResize | UIWindowFlags_NoBackground | UIWindowFlags_NoFocus
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
	UIInputTextFlags_FitSizeToText         = 1 << 7,
	UIInputTextFlags_SetCursorToEndOnEnter = 1 << 8,
	UIInputTextFlags_Numerical             = 1 << 9, //only allows input of [0-9|.]
	
}; typedef u32 UIInputTextFlags;

struct UIInputTextCallbackData {
	UIInputTextFlags eventFlag; //the flag that caused the call back
	UIInputTextFlags flags;     //the flags that the input text item has
	void* userData;             //custom user data
	
	u8       character;         //character that was input  | r
	Key::Key eventKey;          //key pressed on callback   | r
	char*    buffer;            //buffer pointer            | r/w
	size_t   bufferSize;        //                          | r
	u32      cursorPos;         //cursor position		    | r/w
	u32      selectionStart;    //                          | r/w -- == selection end when no selection
	u32      selectionEnd;      //                          | r/w
};
typedef u32 (*UIInputTextCallback)(UIInputTextCallbackData* data);

struct UIInputTextState {
	u32 id;                       //id the state belongs to
	u32 cursor = 0;               //what character in the buffer the cursor is infront of, 0 being all the way to the left
	f32 cursorBlinkTime;          //time it takes for the cursor to blink
	f32 scroll;                   //scroll offset on x
	u32 selectStart;              //beginning of text selection
	u32 selectEnd;                //end of text selection
	UIInputTextCallback callback;
	TIMER_START(timeSinceTyped);  //timer to time how long its been since typing, for cursor
};

enum UISliderFlags_ {
	UISliderFlags_NONE     = 0,
	UISliderFlags_Vertical = 1,
	
	
}; typedef u32 UISliderFlags;

enum UIImageFlags_ {
	UIImageFlags_NONE = 0,
	UIImageFlags_RestrictAspectRatio = 1 << 0,
	UIImageFlags_Invert = 1 << 1,
	
}; typedef u32 UIImageFlags;

enum UIButtonFlags_ {
	UIButtonFlags_NONE = 0,
	UIButtonFlags_ReturnTrueOnHold    = 1 << 0,
	UIButtonFlags_ReturnTrueOnRelease = 1 << 1,
	
	
}; typedef u32 UIButtonFlags;

enum UIDrawType : u32 {
	UIDrawType_Rectangle,
	UIDrawType_FilledRectangle,
	UIDrawType_Line,
	UIDrawType_Circle,
	UIDrawType_CircleFilled,
	UIDrawType_Text,
	UIDrawType_WText,
	UIDrawType_Image,
};

global_ const char* UIDrawTypeStrs[] = {
	"Rectangle",
	"FilledRectangle",
	"Line",
	"Circle",
	"CircleFilled",
	"Text",
	"WText",
	"Image",
};

#define UIDRAWCMD_MAX_VERTICES 0x5FF
#define UIDRAWCMD_MAX_INDICES UIDRAWCMD_MAX_VERTICES * 3

struct UIItem;

//draw commands store what kind of command it is, and info relative to that command
//this is to be stored on an array on UIWindow and determines what elements it draws when
//we do the rendering pass
struct UIDrawCmd {
	UIDrawType type;
	
	vec2    position; //draw cmd start, line start
	vec2   position2; //line end
	vec2  dimensions; //rectangles have dimensions
	f32    thickness; //line thickness, circle radius, texture alpha
	color      color; //draw cmds have either a texture or a color
	Texture*     tex; //if texture is non-zero, we use that as its color, and thickness as its alpha
	u32 subdivisions; //circle subdivisons
	
	Vertex2 vertices[UIDRAWCMD_MAX_VERTICES];
	u32     indices[UIDRAWCMD_MAX_INDICES] = {0};
	vec2    counts; 

	//array<Vertex2> vertices;
	//array<u32>     indices;
	
	//TODO
	//eventually we could maybe store text as an int* or something, so as unicode codepoints, since in the end,
	//at least with TTF, thats how we communicate what letter we want.
	string text;
	wstring wtext;
	Font* font;
	
	//determines if the drawCmd should be considered when using UIWindowFlag_FitAllElements
	b32 trackedForMinSize = 1;
	
	vec2 scissorOffset = vec2(0, 0);
	vec2 scissorExtent = vec2(0, 0);
	b32  useWindowScissor = true;
	b32  overrideScissorRules = false;
	
	//for matching draw cmds in debug
	u32 hash = 0;
	
	UIItem* parent = 0;
};

enum UIItemType : u32 {
	UIItemType_PreItems,  // internal items drawn before user items
	UIItemType_PostItems, // ditto, but afterwards
	UIItemType_Custom,    // BeginCustomItem()
	UIItemType_Abstract,  // any single drawcall such as a line, rectangle, circle, etc
	UIItemType_Window,    // an item that holds a pointer to a window to be drawn
	UIItemType_PopOutWindow,    // an item that holds a pointer to a window to be drawn
	UIItemType_Text,      // Text()
	UIItemType_InputText, // InputText()
	UIItemType_Button,    // Button()
	UIItemType_Checkbox,  // Checkbox()
	UIItemType_DropDown,  // DropDown()
	UIItemType_Slider,    // Slider()
	UIItemType_Header,    // Header()
	UIItemType_Selectable,// Selectable()
	UIItemType_Combo,     // BeginCombo()
	UIItemType_Image,     // Image()
	UIItemType_Separator, // Separator()
};

global_ const char* UIItemTypeStrs[] = {
	"PreItems",  
	"PostItems", 
	"Custom",    
	"Abstract",  
	"ChildWin",  
	"PopOutWindow",
	"Text",      
	"InputText", 
	"Button",    
	"Checkbox",  
	"DropDown",  
	"Slider",    
	"Header",    
	"Selectable",
	"Combo",     
	"Image",     
	"Separator", 
};

//an item such as a button, checkbox, or input text
// this is meant to group draw commands and provide a bounding box for them, using a position
// and overall size. an items position is relative to the window it was created in and all of its
// drawcall positions are relative to itself
// 
// it also keeps track of certain things when it was created such as where the cursor 
// was before it moved it and all the style options it used to create itself.
// this is useful for when we have to look back at previous items to position a new one
// 
// this does have a drawback, in our final drawing loop we have to add one more for loop to loop
// over all items and then their draw calls. it also probably eats up a good bit more memory, considering
// im saving the state of style everytime you make an item, this could maybe be reduced by only storing important
// things instead
struct UIWindow;
struct UIItem {
	UIItemType type;
	vec2       initialCurPos; //cursor position before this item moved it 
	UIStyle    style;         //style at the time of making the item
	
	vec2 position; //relative to the window its being held in
	vec2 size;
	
	//all draw command positions are relative to the items position
	array<UIDrawCmd> drawCmds;
	
	//this is only used when the item is a child window
	UIWindow* child = 0;
	
	//set false when you dont want the item to affect scrolling or fitting all elements
	b32 trackedForMinSize = 1;
	
	//DEBUG info
	u32 item_idx;
	u32 item_layer;
};

#define UI_WINDOW_ITEM_LAYERS 11

//for resizing inputs, but could maybe be used for other things later
enum WinActiveSide {
	wNone,
	wLeft, wRight, wTop, wBottom,
};

enum UIWindowState_ {
	UIWSNone          = 0,
	UIWSBegan         = 1 << 0,
	UIWSEnded         = 1 << 1,
	UIWSFocused       = 1 << 2,
	UIWSHovered       = 1 << 3,
	UIWSChildHovered  = 1 << 4,
	UIWSLatch         = 1 << 5,
	UIWSBeingScrolled = 1 << 6,
	UIWSBeingResized  = 1 << 7,
	UIWSBeingDragged  = 1 << 8,
}; typedef u32 UIWindowState;

enum UIWindowType_ {
	UIWindowType_Normal,
	UIWindowType_Child,  //stays embedded within a parent window
	UIWindowType_PopOut, //is able to leave the parent windows boundries
}; typedef u32 UIWindowType;

// a window is a collection of items and items are a collection of drawcalls.
// item positions are relative to the window's upper left corner.
// drawcall positions are relative to the item's upper left corner.
struct UIWindow {
	string name;
	UIWindowType type;
	
	union {
		vec2 position;
		struct { f32 x, y; };
	};
	
	union {
		vec2 dimensions;
		struct { f32 width, height; };
	};
	
	union {
		vec2 scroll;
		struct { f32 scx, scy; };
	};
	
	vec2 maxScroll;
	
	//interior window cursor that's relative to its upper left corner
	//this places items and not draw calls
	union {
		vec2 cursor;
		struct { f32 curx, cury; };
	};
	
	UIWindowFlags flags;
	
	struct {
		UIWindowState flags = UIWSNone;
		WinActiveSide active_side = wNone;
	} win_state;
	
	//base items are always drawn before items and is just a way to defer drawing 
	//base window stuff to End(), so we can do dynamic sizing
	array<UIItem> items[UI_WINDOW_ITEM_LAYERS];
	array<UIItem> preItems;
	array<UIItem> postItems;
	array<UIItem> popOuts;
	
	u32 windowlayer = 5;
	
	//a collection of child windows
	UIWindow* parent = 0;
	map<const char*, UIWindow*> children;
	pair<UIWindow*, UIItem*> hoveredChild;
	
	
	vec2 minSizeForFit;
	
	vec2 visibleRegionStart;
	vec2 visibleRegionSize;
	
	//the state of style when Begin and End are called
	UIStyle style;
	
	
	//debug information for use with metrics
	//#ifdef DESHI_INTERNAL
	f32 render_time = 0;
	f32 creation_time = 0;
	u32 items_count = 0;
	
	
	//#endif
	
	UIWindow() {};
	
};

enum UIRowFlags_ {
	UIRowFlags_NONE = 0,
	UIRowFlags_FitWidthOfWindow   = 1 << 0,  
	UIRowFlags_DrawCellBackground = 1 << 1,
	UIRowFlags_CellBorderTop      = 1 << 2,
	UIRowFlags_CellBorderBottom   = 1 << 3,
	UIRowFlags_CellBorderLeft     = 1 << 4,
	UIRowFlags_CellBorderRight    = 1 << 5,
	UIRowFlags_CallBorderFull     = UIRowFlags_CellBorderTop | UIRowFlags_CellBorderBottom | UIRowFlags_CellBorderLeft | UIRowFlags_CellBorderRight,
	
}; typedef u32 UIRowFlags;

//for use internally only!
//maybe i should move it to the cpp file
// 
// struct for organizing items to be aligned in a row
//
// rows of items can be made by calling Row(columns, ...)
// you can use this to make tables by ensuring that each column is the same width
// for several Row() calls.
// 
// however it's primary purpose is to allow easy placing of items on the same line
// so consecutive Rows do not need to be aligned in anyway, more advanced table stuff will
// be made into an actual Table item later on
//
// Row()'s default behavoir though is to act like ImGui's same line, where it simply aligns items
// as if you were drawing them in a Row, left aligned, rather than new-lining them each time.
// we have a SameLine() as well, but this makes it easy to same line several consecutive things 
// rather than calling SameLine() everytime
// 
// the alignment of items within cells can be adjusted by changing UIStyleVar_RowItemAlign
// this also works in between items as each item stores this variable so you can have one column of items
// aligned different than the others
struct UIRow {
	UIRowFlags flags = 0;
	u32 columns = 0;
	
	f32 height = 0;
	f32 width = 0; 
	f32 xoffset = 0;
	f32 yoffset = 0; //used when using Row to make several rows
	
	//the position of the row to base offsets of items off of.
	vec2 position;
	
	
	array<UIItem*> items; 
	//the b32ean indicates whether or not the column width is relative to the size of the object
	array<pair<f32, b32>> columnWidths;
};

namespace UI {
	
	//// helpers ////
	vec2              CalcTextSize(cstring text);
	vec2              CalcTextSize(wcstring text);
	FORCE_INLINE vec2 CalcTextSize(const string& text)  { return CalcTextSize(cstring {text.str,u64(text.count)}); }
	FORCE_INLINE vec2 CalcTextSize(const wstring& text) { return CalcTextSize(wcstring{text.str,u64(text.count) }); }
	FORCE_INLINE vec2 CalcTextSize(const char* text)    { return CalcTextSize(cstring {(char*)text,u64(strlen(text))}); }
	FORCE_INLINE vec2 CalcTextSize(const wchar* text) { return CalcTextSize(wcstring{(wchar*)text,u64(wcslen(text)) }); }
	UIStyle&  GetStyle();
	UIWindow* GetWindow();
	UIItem*   GetLastItem(u32 layeroffset = 0);
	vec2      GetLastItemPos();
	vec2      GetLastItemSize();
	vec2      GetLastItemScreenPos();
	vec2      GetWindowRemainingSpace();
	vec2      GetPositionForNextItem();
	u32       GetCenterLayer();
	f32       GetBorderedRight();
	f32       GetBorderedLeft();
	f32       GetBorderedTop();
	f32       GetBorderedBottom();
	f32       GetMarginedRight();
	f32       GetMarginedLeft();
	f32       GetMarginedTop();
	f32       GetMarginedBottom();
	f32       GetScrollBaredRight();
	f32       GetScrollBaredLeft();
	f32       GetScrollBaredTop();
	f32       GetScrollBaredBottom();
	pair<vec2, vec2> GetBorderedArea();
	pair<vec2, vec2> GetMarginedArea();
	pair<vec2, vec2> GetScrollBaredArea();
	
	
	
	
	//// control functions ////
	void SameLine();
	void SetCursor(vec2 pos);
	void SetScroll(vec2 scroll); //MAX_F32 sets to max scroll
	void SetNextItemActive();
	void SetNextItemSize(vec2 size);
	void SetMarginPositionOffset(vec2 offset);
	void SetMarginSizeOffset(vec2 offset);
	void SetNextItemMinSizeIgnored();
	
	
	
	//// rows ////
	void BeginRow(u32 columns, f32 rowHeight, UIRowFlags flags = 0);
	void EndRow();
	void RowSetupColumnWidths(array<f32> widths);
	void RowSetupColumnWidth(u32 column, f32 width);
	void RowSetupRelativeColumnWidth(u32 column, f32 width);
	void RowSetupRelativeColumnWidths(array<f32> widths);
	
	//// drawing ////
	void Rect(vec2 pos, vec2 dimen, color color = Color_White);
	void RectFilled(vec2 pos, vec2 dimen, color color = Color_White);
	void Line(vec2 start, vec2 end, f32 thickness = 1, color color = Color_White);
	void Circle(vec2 pos, f32 radius, u32 subdivisions = 30, color color = Color_White);
	void CircleFilled(vec2 pos, f32 radius, u32 subdivisions = 30, color color = Color_White);
	
	
	//// text ////
	void Text(const char* text, UITextFlags flags = 0);
	void Text(const char* text, vec2 pos, UITextFlags flags = 0);
	void Text(const wchar* text, UITextFlags flags = 0);
	void Text(const wchar* text, vec2 pos, UITextFlags flags = 0);
	void TextF(const char* fmt, ...);
	
	
	//// items ////
	b32 Button(const char* text, UIButtonFlags flags = 0);
	b32 Button(const char* text, vec2 pos, UIButtonFlags flags = 0);
	
	void Checkbox(string label, b32* b);
	
	b32 BeginCombo(const char* label, const char* preview_val);
	b32 BeginCombo(const char* label, const char* preview_val, vec2 pos);
	
	void EndCombo();
	
	b32 Selectable(const char* label, b32 selected); 
	b32 Selectable(const char* label, vec2 pos, b32 selected);
	
	b32  BeginHeader(const char* label);
	void EndHeader();
	
	void Slider(const char* label, f32* val, f32 val_min, f32 val_max, UISliderFlags flags = 0);
	
	void Image(Texture* image, vec2 pos, f32 alpha = 1, UIImageFlags flags = 0);
	void Image(Texture* image, f32 alpha = 1, UIImageFlags flags = 0);
	
	void Separator(f32 height);
	
	//these overloads are kind of silly change them eventually
	//InputText takes in a buffer and modifies it according to input and works much like ImGui's InputText
	//However there are overloads that will return it's UIInputTextState, allowing you to directly r/w some internal information of the
	//InputText item. This should only be used if you have a good reason to!
	b32 InputText(const char* label, char* buffer, u32 buffSize, const char* preview = 0, UIInputTextFlags flags = 0);
	b32 InputText(const char* label, char* buffer, u32 buffSize, UIInputTextCallback callbackFunc, const char* preview = 0, UIInputTextFlags flags = 0);
	b32 InputText(const char* label, char* buffer, u32 buffSize, UIInputTextState*& getInputTextState, const char* preview = 0, UIInputTextFlags flags = 0);
	b32 InputText(const char* label, char* buffer, u32 buffSize, vec2 pos, const char* preview = 0, UIInputTextFlags flags = 0);
	b32 InputText(const char* label, char* buffer, u32 buffSize, vec2 pos, UIInputTextCallback callbackFunc, const char* preview = 0, UIInputTextFlags flags = 0);
	b32 InputText(const char* label, char* buffer, u32 buffSize, vec2 pos, UIInputTextState*& getInputTextState, const char* preview = 0, UIInputTextFlags flags = 0);
	
	UIItem* BeginCustomItem(u32 layeroffset = 0);
	void    CustomItemAdvanceCursor(UIItem* item, b32 move_cursor = 1);
	void    EndCustomItem();
	
	//// push/pop ////
	void PushColor(UIStyleCol idx, color color);
	void PushVar(UIStyleVar idx, f32 style);
	void PushVar(UIStyleVar idx, vec2 style);
	void PushFont(Font* font);
	void PushScale(vec2 scale);
	void PushLayer(u32 layer);
	void PushWindowLayer(u32 layer);
	
	void PopColor(u32 count = 1);
	void PopVar(u32 count = 1);
	void PopFont(u32 count = 1);
	void PopScale(u32 count = 1);
	void PopLayer(u32 count = 1);
	void PopWindowLayer(u32 count = 1);
	
	
	//// windows ////
	void Begin(const char* name, UIWindowFlags flags = 0, UIWindowType type = UIWindowType_Normal);
	void Begin(const char* name, vec2 pos, vec2 dimensions, UIWindowFlags flags = 0, UIWindowType type = UIWindowType_Normal);
	void BeginChild(const char* name, vec2 dimensions, UIWindowFlags flags = 0);
	void BeginChild(const char* name, vec2 pos, vec2 dimensions, UIWindowFlags flags = 0);
	void BeginPopOut(const char* name, vec2 pos, vec2 dimensions, UIWindowFlags flags = 0);
	void End();
	void EndChild();
	void EndPopOut();
	void SetNextWindowPos(vec2 pos);
	void SetNextWindowPos(f32 x, f32 y);
	void SetNextWindowSize(vec2 size);		  //when you set a windows size through this you aren't supposed to take into account the titlebar!
	void SetNextWindowSize(f32 x, f32 y); //when you set a windows size through this you aren't supposed to take into account the titlebar!
	void SetWindowName(const char* name);
	b32 IsWinHovered();
	b32 AnyWinHovered();
	void ShowMetricsWindow();
	void DemoWindow();
	
	
	//// init and update ////
	void Init();
	void Update();
	
	
	//// debug ////
	void DrawDebugRect(vec2 pos, vec2 size, color color = Color_Red);
	void DrawDebugRectFilled(vec2 pos, vec2 size, color color = Color_Red);
	void DrawDebugCircle(vec2 pos, f32 radius, color color = Color_Red);
	void DrawDebugCircleFilled(vec2 pos, f32 radius, color color = Color_Red);
	void DrawDebugLine(vec2 pos1, vec2 pos2, color color = Color_Red);
	
}; //namespace UI

#endif //DESHI_UI_H