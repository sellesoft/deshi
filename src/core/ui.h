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

	be careful of non-integer positions as they influence other positions as floats even though
	they get converted to integer positions in the end.

be careful of pushing vars before creating child windows for the window internals, as it will also affect the window itself

metrics window: (partially in order, since some later ones remove the need for previous ones)
move window debug visual header with the other headers rather than with window vars
combine window items and children then move them under windows header (so moving thru windows/items is seamless)

//sushi: selected item is difficult since items disppear every frame
selected item rather than selected window (with contextual vars rather than just window vars)

simulate hovered/clicked/toggled on items

*/

#include "font.h"
#include "input.h"
#include "render.h"
#include "memory.h"
#include "kigu/color.h"
#include "kigu/common.h"
#include "kigu/map.h"
#include "math/math.h"

struct UIDrawCmd;
struct UIItem;
struct UIWindow;

enum UIStyleVar : u32 {
	UIStyleVar_WindowMargins,             // default vec2(10, 10)      spacing between every item and the edges of the window
	UIStyleVar_ItemSpacing,               // default vec2(1, 1)        spacing between items within a window
	UIStyleVar_WindowBorderSize,          // default 1                 border size in pixels
	UIStyleVar_ButtonBorderSize,          // default 1
	UIStyleVar_TitleBarHeight,            // default font.height * 1.2                                        
	UIStyleVar_TitleTextAlign,            // default vec2(0, 0.5)      how title text is aligned in title bar 
	UIStyleVar_ScrollAmount,              // default vec2(5, 5)        amount to scroll in pixels             
	UIStyleVar_CheckboxSize,              // default vec2(10, 10)      
	UIStyleVar_CheckboxFillPadding,       // default 2                 how far from the edge a checkbox's true filling is padding
	UIStyleVar_InputTextTextAlign,        // default vec2(0,   0.5)    how text is aligned within InputText boxes
	UIStyleVar_ButtonTextAlign,           // default vec2(0.5, 0.5)    how text is aligned within buttons
	UIStyleVar_HeaderTextAlign,           // default vec2(0,   0.5)
	UIStyleVar_SelectableTextAlign,       // default vec2(0.5, 0.5)
	UIStyleVar_TabTextAlign,              // default vec2(0.5, 0.5)
	UIStyleVar_ButtonHeightRelToFont,     // default 1.3                height of header box relative to the font height
	UIStyleVar_HeaderHeightRelToFont,     // default 1.3
	UIStyleVar_InputTextHeightRelToFont,  // default 1.3
	UIStyleVar_CheckboxHeightRelToFont,   // default 1.3
	UIStyleVar_SelectableHeightRelToFont, // default 1.3 
	UIStyleVar_TabHeightRelToFont,        // default 1.3 
	UIStyleVar_RowItemAlign,              // default vec2(0.5, 0.5)    determines how rows align their items within their cells
	UIStyleVar_RowCellPadding,            // default vec2(10, 10)      the amount of pixels to pad items within cells from the edges of the cell
	UIStyleVar_ScrollBarYWidth,           // default 5
	UIStyleVar_ScrollBarXHeight,          // default 5
	UIStyleVar_IndentAmount,              // default 8
	UIStyleVar_TabSpacing,                // default 5                 the spacing between tabs
	UIStyleVar_FontHeight,                // default font->height      height of font in pixels
	UIStyleVar_WindowSnappingTolerance,   // default vec2(5, 5)        how close a window has to be to another window's edge to snap to it.
	UIStyleVar_Font,                      // default "gohufont-11.bdf" 
	UIStyleVar_COUNT
};

global str8 styleVarStr[] = {
	str8_lit("WindowPadding"),
	str8_lit("ItemSpacing"),
	str8_lit("WindowBorderSize"),
	str8_lit("ButtonBorderSize"),
	str8_lit("TitleBarHeight"),
	str8_lit("TitleTextAlign"),
	str8_lit("ScrollAmount"),
	str8_lit("CheckboxSize"),
	str8_lit("CheckboxFillPadding"),
	str8_lit("InputTextTextAlign"),
	str8_lit("ButtonTextAlign"),
	str8_lit("HeaderTextAlign"),
	str8_lit("SelectableTextAlign"),
	str8_lit("TabTextAlign"),
	str8_lit("ButtonHeightRelToFont"),
	str8_lit("HeaderHeightRelToFont"),
	str8_lit("InputTextHeightRelToFont"),
	str8_lit("CheckboxHeightRelToFont"),
	str8_lit("SelectableHeightRelToFont"),
	str8_lit("TabHeightRelToFont"),
	str8_lit("RowItemAlign"),
	str8_lit("RowCellPadding"),
	str8_lit("ScrollBarYWidth"),
	str8_lit("ScrollBarXHeight"),
	str8_lit("IndentAmount"),
	str8_lit("TabSpacing"),
	str8_lit("FontHeight"),
	str8_lit("Font"),
	str8_lit("COUNT")
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
	
	UIStyleCol_TabBar,
	UIStyleCol_TabBg,
	UIStyleCol_TabBgActive,
	UIStyleCol_TabBgHovered,
	UIStyleCol_TabBorder,
	
	UIStyleCol_TitleBg,
	UIStyleCol_TitleBgHovered,
	UIStyleCol_TitleBgActive,
	UIStyleCol_COUNT
};

struct UIStyle {
	vec2 windowMargins;
	vec2 itemSpacing;
	f32  windowBorderSize;
	f32  buttonBorderSize;
	f32  titleBarHeight;
	vec2 titleTextAlign;
	vec2 scrollAmount;
	vec2 checkboxSize;
	f32  checkboxFillPadding;
	vec2 inputTextTextAlign;
	vec2 buttonTextAlign;
	vec2 headerTextAlign;
	vec2 selectableTextAlign;
	vec2 tabTextAlign;
	f32  buttonHeightRelToFont;
	f32  headerHeightRelToFont;
	f32  inputTextHeightRelToFont;
	f32  checkboxHeightRelToFont;
	f32  selectableHeightRelToFont;
	f32  tabHeightRelToFont;
	vec2 rowItemAlign;
	vec2 rowCellPadding;
	f32  scrollBarYWidth;
	f32  scrollBarXHeight;
	f32  indentAmount;
	f32  tabSpacing;
	f32  fontHeight;
	vec2 windowSnappingTolerance;
	//special vars that have special push/pop functions
	vec2  globalScale;
	Font* font;
	color colors[UIStyleCol_COUNT];
};

//TODO implement this or remove it 
struct UIContextInfo {
	UIWindow* hovered_window = 0;
	UIWindow* focused_window = 0;
	UIItem* last_placed_item = 0;
};

enum UITextFlags_ {
	UITextFlags_None = 0,
	UITextFlags_NoWrap = 1 << 0,
}; typedef u32 UITextFlags;

enum UIWindowFlags_ {
	UIWindowFlags_None                   = 0,
	UIWindowFlags_NoResize               = 1 << 0, //prevents the user from resizing with the edges 
	UIWindowFlags_NoMove                 = 1 << 1, //prevents the user from moving the window
	//TODO UIWindowFlags_NoTitleBar             = 1 << 2,
	UIWindowFlags_NoBorder               = 1 << 3, //doesnt draw a border
	UIWindowFlags_NoBackground           = 1 << 4, //doesnt draw a background
	UIWindowFlags_NoScrollBarX           = 1 << 5, //doesnt draw a horizonal scrollbar, but can still scroll
	UIWindowFlags_NoScrollBarY           = 1 << 6, //doesnt draw a vertical scrollbar, but can still scroll
	UIWindowFlags_NoScrollBars           = UIWindowFlags_NoScrollBarX | UIWindowFlags_NoScrollBarY, 
	UIWindowFlags_NoScrollX              = 1 << 7 | UIWindowFlags_NoScrollBarX, 
	UIWindowFlags_NoScrollY              = 1 << 8 | UIWindowFlags_NoScrollBarY,
	UIWindowFlags_NoScroll               = UIWindowFlags_NoScrollX | UIWindowFlags_NoScrollY,
	UIWindowFlags_NoFocus                = 1 << 9, 
	UIWindowFlags_FocusOnHover           = 1 << 10,
	//TODO UIWindowFlags_NoMinimize             = 1 << 11,
	//TODO UIWindowFlags_NoMinimizeButton       = 1 << 12,
	UIWindowFlags_DontSetGlobalHoverFlag = 1 << 13,
	UIWindowFlags_FitAllElements         = 1 << 14, //attempts to fit the window's size to all called elements
	UIWindowFlags_SnapToOtherWindows     = 1 << 15, //makes this window snap to the edges of other windows
	
	UIWindowFlags_NoInteract = UIWindowFlags_NoMove | UIWindowFlags_NoFocus | UIWindowFlags_NoResize | UIWindowFlags_DontSetGlobalHoverFlag | UIWindowFlags_NoScroll, 
	UIWindowFlags_Invisible  = UIWindowFlags_NoMove | UIWindowFlags_NoResize | UIWindowFlags_NoBackground | UIWindowFlags_NoFocus //| UIWindowFlags_NoTitleBar
}; typedef u32 UIWindowFlags;

enum UIInputTextFlags_ {
	UIInputTextFlags_NONE                  = 0,
	UIInputTextFlags_EnterReturnsTrue      = 1 << 0,
	UIInputTextFlags_AnyChangeReturnsTrue  = 1 << 1,
	UIInputTextFlags_CallbackTab           = 1 << 2,  //calls the given callback function when tab is pressed
	UIInputTextFlags_CallbackEnter         = 1 << 3,  //calls the given callback function when enter is pressed
	UIInputTextFlags_CallbackAlways        = 1 << 4,  //calls the given callback function when any key is pressed
	UIInputTextFlags_CallbackUpDown        = 1 << 5,  //calls the given callback function when up or down are pressed
	UIInputTextFlags_NoBackground          = 1 << 6,  
	UIInputTextFlags_FitSizeToText         = 1 << 7, 
	UIInputTextFlags_SetCursorToEndOnEnter = 1 << 8, 
	UIInputTextFlags_Numerical             = 1 << 9,  //only allows input of [0-9|.]
	UIInputTextFlags_NoEdit                = 1 << 10, //prevents editing of the buffer
	
}; typedef u32 UIInputTextFlags;

struct UIInputTextCallbackData {
	UIInputTextFlags eventFlag; //the flag that caused the call back
	UIInputTextFlags flags;     //the flags that the input text item has
	void* userData;             //custom user data
	
	u32      character;         //character that was input | r
	KeyCode  eventKey;          //key pressed on callback  | r
	u8*      buffer;            //buffer pointer           | r/w
	size_t   bufferSize;        //                         | r
	u32      cursorPos;         //cursor position (bytes)  | r/w
	u32      selectionStart;    //                         | r/w -- == selection end when no selection
	u32      selectionEnd;      //                         | r/w
};
typedef u32 (*UIInputTextCallback)(UIInputTextCallbackData* data);

struct UIInputTextState {
	u32 id;                       //id the state belongs to
	u32 cursor = 0;               //what character in the buffer the cursor is infront of, 0 being all the way to the left (byte offset)
	f32 cursorBlinkTime;          //time it takes for the cursor to blink
	f32 scroll;                   //scroll offset on x
	u32 selectStart;              //beginning of text selection (byte offset)
	u32 selectEnd;                //end of text selection (byte offset)
	UIInputTextCallback callback;
	Stopwatch timeSinceTyped;     //timer to time how long its been since typing, for cursor
};


enum UITabBarFlags_ {
	UITabBarFlags_NONE = 0,
	UITabBarFlags_NoRightIndent = 1 << 0,
	UITabBarFlags_NoLeftIndent  = 1 << 1,
	UITabBarFlags_NoIndent      = UITabBarFlags_NoRightIndent | UITabBarFlags_NoLeftIndent,
}; typedef u32 UITabBarFlags;

struct UITab {
	f32    width = 0;
	f32   height = 0;
	UIItem* item = 0;
};

struct UITabBar {
	UITabBarFlags flags;
	map<str8, UITab> tabs;
	u32  selected = 0;
	f32 tabHeight = 0;
	u32   xoffset = 0; //how far along the bar we've placed tabs 
	UIItem*  item = 0; //item representing this tabbar
};

enum UISliderFlags_ {
	UISliderFlags_NONE     = 0,
	//TODO UISliderFlags_Vertical = 1,
}; typedef u32 UISliderFlags;

enum UIImageFlags_ {
	UIImageFlags_NONE   = 0,
	//TODO UIImageFlags_RestrictAspectRatio = 1 << 0,
	//TODO UIImageFlags_Invert = 1 << 1,
	UIImageFlags_FlipX  = 1 << 2,
	UIImageFlags_FlipY  = 1 << 3,
	
}; typedef u32 UIImageFlags;

enum UIButtonFlags_ {
	UIButtonFlags_NONE = 0,
	UIButtonFlags_ReturnTrueOnHold    	       = 1 << 0,
	UIButtonFlags_ReturnTrueOnRelease 	       = 1 << 1,
	UIButtonFlags_ReturnTrueOnReleaseIfHovered = 1 << 2,
	UIButtonFlags_NoBorder            	       = 1 << 3,
	
}; typedef u32 UIButtonFlags;

enum UIHeaderFlags_ {
	UIHeaderFlags_NONE          = 0,
	UIHeaderFlags_NoIndentLeft  = 1 << 0,
	UIHeaderFlags_NoIndentRight = 1 << 1,
	UIHeaderFlags_NoIndent      = UIHeaderFlags_NoIndentLeft | UIHeaderFlags_NoIndentRight,
	UIHeaderFlags_NoBorder      = 1 << 2,
}; typedef u32 UIHeaderFlags;

struct UIHeader{
	b32 open = false;
	UIHeaderFlags flags;
};

enum UIMenuFlags_ {
	UIMenuFlags_NONE                   = 0,
	UIMenuFlags_AppearAtMouse          = 1 << 0, //makes the menu's initial position where the mouse is when called
	UIMenuFlags_AutoFitElements        = 1 << 1, //the menu will autofit the width and height of all contained items. you must have at least one item that is manually sized for this to work properly eg. you can't use vec2(MAX_F32,MAX_F32) to size all of the items. this flag is default when no size argument is given
	UIMenuFlags_KeepOpenOnScroll       = 1 << 2, //keeps the menu open if the user scrolls 
	UIMenuFlags_MoveWithScroll         = 1 << 3, //follows the scrolling of the parent window
	UIMenuFlags_KeepOpenOnClick        = 1 << 4, //keeps the menu open when an element it holds is clicked
	UIMenuFlags_KeepOpenOnOutsideClick = 1 << 5, //keeps the menu open even when the user clicks outside of it 
}; typedef u32 UIMenuFlags;

struct UIMenu {
	UIMenuFlags flags;
	vec2 pos;
	vec2 size;
};

enum UIDrawType : u32 {
	UIDrawType_Triangle,
	UIDrawType_FilledTriangle,
	UIDrawType_Rectangle,
	UIDrawType_FilledRectangle,
	UIDrawType_Line,
	UIDrawType_Circle,
	UIDrawType_FilledCircle,
	UIDrawType_Text,
	UIDrawType_WText,
	UIDrawType_Image,
};

global str8 UIDrawTypeStrs[] = {
	str8_lit("Triangle"),
	str8_lit("FilledTriangle"),
	str8_lit("Rectangle"),
	str8_lit("FilledRectangle"),
	str8_lit("Line"),
	str8_lit("Circle"),
	str8_lit("CircleFilled"),
	str8_lit("Text"),
	str8_lit("WText"),
	str8_lit("Image"),
};

#define UIDRAWCMD_MAX_VERTICES 0x3FF
#define UIDRAWCMD_MAX_INDICES UIDRAWCMD_MAX_VERTICES * 3

//draw commands store what kind of command it is, and info relative to that command
//this is to be stored on an array on UIWindow and determines what elements it draws when
//we do the rendering pass
struct UIDrawCmd {
	UIDrawType type;
	
	//because of how much drawCmds move around, we have to store these things on the heap
	Vertex2* vertices = (Vertex2*)memtalloc(UIDRAWCMD_MAX_VERTICES * sizeof(Vertex2));
	u32*     indices = (u32*)memtalloc(UIDRAWCMD_MAX_INDICES * u32size);
	RenderDrawCounts counts{}; 
	
	Texture* tex = 0;
	
	//the surface this drawCmd draws to in the renderer
	u32 render_surface_target_idx = 0;
	
	//determines if the drawCmd should be considered when using UIWindowFlag_FitAllElements
	b32 trackedForMinSize = 1;
	
	vec2 scissorOffset = vec2::ZERO;
	vec2 scissorExtent = vec2::ZERO;
	b32  useWindowScissor = true;
	b32  overrideScissorRules = false;
	
	//for matching draw cmds in debug
	u32 hash = 0;
	
	UIItem* parent = 0;
};

//attempt to uniquely hash a UIDrawCmd for debug purposes
template<>
struct hash<UIDrawCmd> {
	inline u32 operator()(const UIDrawCmd& s) {
		u32 seed = 2166134;
		forI(s.counts.vertices){
			seed ^= u32(s.vertices[i].pos.x) << u32(s.vertices[i].pos.y); 
		}
		seed ^= (u32)s.counts.vertices | (u32)s.counts.indices << (u32)(u64)s.tex | (u32)s.useWindowScissor | (u32)(u64)s.parent
			<<u32(s.scissorOffset.x)|u32(s.scissorExtent.x) << u32(s.scissorOffset.y)|u32(s.scissorExtent.y);
		return seed;
	}
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
	UIItemType_TabBar,    // BeginTabBar()
	UIItemType_Tab,       // BeginTab()
	UIItemType_Menu,      // BeginMenu()
	UIItemType_COUNT
};

global str8 UIItemTypeStrs[] = {
	str8_lit("PreItems"),
	str8_lit("PostItems"),
	str8_lit("Custom"),
	str8_lit("Abstract"),
	str8_lit("ChildWin"),
	str8_lit("PopOutWindow"),
	str8_lit("Text"),
	str8_lit("InputText"),
	str8_lit("Button"),
	str8_lit("Checkbox"),
	str8_lit("DropDown"),
	str8_lit("Slider"),
	str8_lit("Header"),
	str8_lit("Selectable"),
	str8_lit("Combo"),
	str8_lit("Image"),
	str8_lit("Separator"),
	str8_lit("TabBar"),
	str8_lit("Tab"),
};

//an item such as a button, checkbox, or input text
// this is meant to group draw commands and provide a bounding box for them, using a position
// and overall size. an items position is relative to the window it was created in and all of its
// drawcall positions are relative to itself
// 
// it also keeps track of certain things when it was created such as where the cursor 
// was before it moved it and all the style options it used to create itself.
// this is useful for when we have to look back at previous items to position a new one
struct UIWindow;
struct UIItem {
	UIItemType type;
	vec2       initialCurPos; //cursor position before this item moved it 
	UIStyle    style;         //style at the time of making the item
	
	Flags flags; 
	
	vec2 position; //relative to the window its being held in
	vec2 size;
	
	//all draw command positions are relative to the items position
	array<UIDrawCmd> drawCmds;
	
	//this is only used when the item is a child window
	UIWindow* child = 0;
	
	//set false when you dont want the item to affect scrolling or fitting all elements
	b32 trackedForMinSize = 1;
	b32 visible = 1; //TODO check items for visibility when they're created so we can opt drawing them
	
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
	str8 name;
	UIWindowType type;
	
	union {
		vec2 position;
		struct { f32 x, y; };
	};
	//TODO differenciate between client and non client area like in win32
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
	
	u32 layer = 5;
	
	//a collection of child windows
	UIWindow* parent = 0;
	map<str8, UIWindow*> children;
	pair<UIWindow*, UIItem*> hoveredChild;
	
	
	vec2 minSizeForFit;
	
	vec2 visibleRegionStart;
	vec2 visibleRegionSize;
	
	//the state of style when Begin and End are called
	UIStyle style;
	
	
	//debug information for use with metrics
	//#ifdef BUILD_INTERNAL
	f32 render_time = 0;
	f32 creation_time = 0;
	u32 items_count = 0;
	
	
	//#endif
	
	UIWindow() {};
	
};

enum UIRowFlags_ {
	UIRowFlags_NONE = 0,
	//TODO UIRowFlags_FitWidthOfArea         = 1 << 0,  
	//TODO UIRowFlags_DrawCellBackground     = 1 << 1,
	UIRowFlags_AutoSize               = 1 << 2, 
	//TODO UIRowFlags_CellBorderTop          = 1 << 3,
	//TODO UIRowFlags_CellBorderBottom       = 1 << 4,
	//TODO UIRowFlags_CellBorderLeft         = 1 << 5,
	//TODO UIRowFlags_CellBorderRight        = 1 << 6,
	//TODO UIRowFlags_CallBorderFull         = UIRowFlags_CellBorderTop | UIRowFlags_CellBorderBottom | UIRowFlags_CellBorderLeft | UIRowFlags_CellBorderRight,
}; typedef u32 UIRowFlags;

struct UIColumn {
	f32  width = 0;
	b32  relative_width = 0;
	f32  max_width = 0;
	b32  reeval_width = 0;
	vec2 alignment = vec2(-1,-1);
	array<UIItem*> items;
};

struct UIRow {
	UIRowFlags flags = 0;
	
	str8 label;
	
	f32 left_edge = 0;
	f32 right_edge = 0;
	
	f32 height = 0;
	f32 width = 0; 
	f32 xoffset = 0;
	f32 yoffset = 0; //used when using Row to make several rows
	
	f32 max_height = 0;
	f32 max_height_frame = 0;
	
	b32 reeval_height = 0;
	
	//the position of the row to base offsets of items off of.
	vec2 position;
	
	u32 item_count = 0;
	array<UIColumn> columns;
};

struct UIStats {
	u32 vertices  = 0;
	u32 indices   = 0;
	
	u32 draw_cmds = 0;
	u32 items     = 0;
	u32 windows   = 0;
};

namespace UI {
	
	//// helpers ////
	
	//calculates the given text size as it would appear onscreen with the current font pointed to by the style var
	vec2      CalcTextSize(str8 text);
	
	//returns a reference to the global ui style var. beware manually modifying this, you should use the Push/Pop system instead
	UIStyle&  GetStyle();
	
	//returns a pointer to the current working window (eg. the window who's begin was last called, if no begins have been called this returns the base window)
	UIWindow* GetWindow();
	
	//returns a pointer to the last placed item
	UIItem*   GetLastItem(u32 layeroffset = 0);
	
	//returns the position of the last placed item
	vec2      GetLastItemPos();
	
	//returns the size of the last placed item
	vec2      GetLastItemSize();
	
	//returns the position of the last placed item in screen space
	vec2      GetLastItemScreenPos();
	
	//returns the remaining space left in a window. TODO test this function
	vec2      GetWindowRemainingSpace();
	
	//returns where ui will automatically place the next item.
	vec2      GetWinCursor();
	
	//returns the center layer between 0 and UI_LAYERS
	u32       GetCenterLayer();
	
	//returns the current working layer
	u32       GetCurrentLayer();
	
	//returns UI_LAYERS, the highest layer something may be on
	u32       GetTopMostLayer();
	
	//returns the x coordinate (in window space) of the right side of the window, taking into account the borders
	//bordered area is the area that is inside the borders of the window, meaning this area includes the scroll bar and titlebar
	f32       GetBorderedRight();
	
	//returns the x coordinate (in window space) of the left side of the window, taking into account the borders 
	//bordered area is the area that is inside the borders of the window, meaning this area includes the scroll bar and titlebar
	f32       GetBorderedLeft();  
	
	//returns the y coordinate (in window space) of the top of the window, taking into account the borders
	//bordered area is the area that is inside the borders of the window, meaning this area includes the scroll bar and titlebar
	f32       GetBorderedTop();   
	
	//returns the y coordinate (in window space) of the bottom of the window, taking into account the borders
	//bordered area is the area that is inside the borders of the window, meaning this area includes the scroll bar and titlebar
	f32       GetBorderedBottom();
	
	//returns the x coordinate (in window space) of the right side of the window, taking into account the margin
	//margined area is the area inside the window that items automatically align themselves within. 
	f32       GetMarginedRight();
	
	//returns the x coordinate (in window space) of the left side of the window, taking into account the margin
	//margined area is the area inside the window that items automatically align themselves within. 
	f32       GetMarginedLeft();
	
	//returns the y coordinate (in window space) of the top of the window, taking into account the margin
	//margined area is the area inside the window that items automatically align themselves within. 
	f32       GetMarginedTop();
	
	//returns the y coordinate (in window space) of the bottom of the window, taking into account the margin
	//margined area is the area inside the window that items automatically align themselves within. 
	f32       GetMarginedBottom();
	
	//returns the x coordinate (in window space) of the right side of the window, taking into account the windows decorations (border, titlebar, scrollbar, etc.)
	//client area is the area unobstructed by window decorations. so the area that excludes the titlebar, scrollbar, and borders.
	f32       GetClientRight();
	
	//returns the x coordinate (in window space) of the left side of the window, taking into account the windows decorations (border, titlebar, scrollbar, etc.)
	//client area is the area unobstructed by window decorations. so the area that excludes the titlebar, scrollbar, and borders.
	f32       GetClientLeft();
	
	//returns the y coordinate (in window space) of the top of the window, taking into account the windows decorations (border, titlebar, scrollbar, etc.)
	//client area is the area unobstructed by window decorations. so the area that excludes the titlebar, scrollbar, and borders.
	f32       GetClientTop();
	
	//returns the y coordinate (in window space) of the bottom of the window, taking into account the windows decorations (border, titlebar, scrollbar, etc.)
	//client area is the area unobstructed by window decorations. so the area that excludes the titlebar, scrollbar, and borders.
	f32       GetClientBottom();
	
	//returns a pair of vec2s, the first being the position in window space and second the area. 
	//bordered area is the area that is inside the borders of the window, meaning this area includes the scroll bar and titlebar
	pair<vec2, vec2> GetBorderedArea();
	
	//returns a pair of vec2s, the first being the position in window space and second the area. 
	//margined area is the area inside the window that items automatically align themselves within. 
	//its size is controlled by the style var UIStyleVar_WindowMargins.
	pair<vec2, vec2> GetMarginedArea();
	
	//returns a pair of vec2s, the first being the position in window space and second the area. 
	//client area is the area unobstructed by window decorations. so the area that excludes the titlebar, scrollbar, and borders.
	pair<vec2, vec2> GetClientArea();
	
	//returns a UIStats object containing information about UI
	UIStats GetStats();
	
	//returns a pointer to an InputText's state by label
	UIInputTextState* GetInputTextState(str8 label);
	
	
	f32 GetRightIndent();
	f32 GetLeftIndent();
	
	//// control functions ////
	
	//positions the window's cursor to be on the same level as the last placed item. if you wish to have more advanced functionality with this see BeginRow
	void SameLine();
	
	//manaully set the windows cursor to a specified position. 
	//after the next item is placed the cursor goes back to default placement behavoir, this means that it will return to the MarginedLeft part of the window, below the y level of the last placed item
	//TODO better name than SetWinCursor that doesnt conflict with win32's SetCursor
	void SetWinCursor(vec2 pos);
	//dont know why this doesnt work 
	//void SetWinCursor(s32 x, s32 y) {SetWinCursor(vec2(x,y));}
	
	//manually set the cursor's X position
	void SetWinCursorX(f32 x);
	//manually set the cursors Y position
	void SetWinCursorY(f32 y);
	//sets the window's scroll values
	//use MAX_F32 to set it to max scroll
	void SetScroll(vec2 scroll); 
	//sets the window's X scroll
	//use MAX_F32 to set it to max X scroll
	void SetScrollX(f32 scroll); 
	//sets the window's Y scroll
	//use MAX_F32 to set it to max Y scroll
	void SetScrollY(f32 scroll); 
	//sets the next item to be 'active'
	//TODO I think I need to make this work with things other than InputText
	//InputText: sets keyboard focus (maybe make this its own function?)
	void SetNextItemActive();
	//sets the size of the next item
	void SetNextItemSize(vec2 size);
	FORCE_INLINE void SetNextItemSize(f32 x, f32 y){ SetNextItemSize(vec2{x,y}); }
	//sets the `width` of the next item
	void SetNextItemWidth(f32 width);
	//sets the `height` of the next item
	void SetNextItemHeight(f32 height);
	//offsets the margin's position (see GetMarginedArea for what this area is)
	void SetMarginPositionOffset(vec2 offset);
	//offsets the margin's size (see GetMarginedArea for what this area is)
	void SetMarginSizeOffset(vec2 offset);
	//tells UI that this item does not affect the windows size when the flag UIWindowFlags_FitAllElements is used
	void SetNextItemMinSizeIgnored();
	//sets UI's internal input state to PreventInputs, this prevents any window from receiving any input, such as scrolling, dragging, resizing, and any action that could be taken on any item. this is undone using SetAllowInputs
	void SetPreventInputs();
	//sets UI's internal unput state to AllowInputs. undoing SetPreventInputs
	void SetAllowInputs();
	
	
	//// @rows ////
	//  a row is a collection of columns used to align a number of items nicely
	//  you are required to specify the width of each column when using Row, as it removes the complicated
	//  nature of having to figure this out after the fact. while row width is not much of a problem, row height is
	//  so you are required to define a static height upon calling the function
	//
	//  NOTE primitives/abstracts are not considered in rows
	void BeginRow(str8 label, u32 columns, f32 rowHeight, UIRowFlags flags = 0);
	void EndRow();
	//takes an array of size equal to the number of columns specified and sets their widths to the specified sizes in pixels
	void RowSetupColumnWidths(array<f32> widths);
	//sets one columns width in pixels
	void RowSetupColumnWidth(u32 column, f32 width);
	//sets a columns width relative to the size of the item it holds. 
	//for example 1 will make the columns width 30 when it holds an item of width 30, 2 will make the columns width 60
	void RowSetupRelativeColumnWidth(u32 column, f32 width);
	//takes an array of size equal to the number of columns specified and sets their widths relative to the item they hold 
	void RowSetupRelativeColumnWidths(array<f32> widths);
	//TODO
	void RowFitBetweenEdges(array<f32> ratios, f32 left_edge, f32 right_edge);
	//sets individual item alignments for each column
	void RowSetupColumnAlignments(array<vec2> alignments);
	
	
	//// drawing ////
	void Rect(vec2 pos, vec2 dimen, color color = Color_White);
	void RectFilled(vec2 pos, vec2 dimen, color color = Color_White);
	void Line(vec2 start, vec2 end, f32 thickness = 1, color color = Color_White);
	void Circle(vec2 pos, f32 radius, f32 thickness = 1, u32 subdivisions = 30, color color = Color_White);
	void CircleFilled(vec2 pos, f32 radius, u32 subdivisions = 30, color color = Color_White);
	
	
	//// text ////
	
	//wraps by default
	void Text(str8 text, UITextFlags flags = 0);
	void Text(str8 text, vec2 pos, UITextFlags flags = 0);
	void TextF(str8 fmt, ...);
	
	
	//// items ////
	
	//a simple clickable button. the condition it returns true for depends on the flag given, if no behavoir flag is given it returns true on click by default
	b32 Button(str8 text, UIButtonFlags flags = 0);
	b32 Button(str8 text, vec2 pos, UIButtonFlags flags = 0);
	
	//a checkbox with a label. you must provide the boolean it changes
	void Checkbox(str8 label, b32* b);
	
	//TODO 
	b32 BeginCombo(str8 label, str8 preview_val);
	b32 BeginCombo(str8 label, str8 preview_val, vec2 pos);
	void EndCombo();
	
	//a selectable that takes in a boolean that determines if its selected or not. it returns true if it has been clicked on
	b32 Selectable(str8 label, b32 selected); 
	b32 Selectable(str8 label, vec2 pos, b32 selected);
	
	//a collapsable header that groups items and lets you hide them. by default it indents the content, but this can be disabled with flags
	//by default the header spans the whole width of the window
	//TODO different header styles, including one that is empty and takes a clickable area for custom graphix
	b32  BeginHeader(str8 label, UIHeaderFlags flags = 0);
	void EndHeader();
	
	//begins the bar a tab sits on. this must be called before BeginTab
	void BeginTabBar(str8 label, UITabBarFlags flags = 0);
	//begins a tab, this must be called after BeginTabBar
	b32  BeginTab(str8 label);
	//ends a tab, this must be called if BeginTab is called, calling it otherwise will assert
	void EndTab(); 
	//this must be called if BeginTabBar is called, calling it otherwise will assert
	void EndTabBar();
	
	//a slider that modifies a given value using the mouse
	void Slider(str8 label, f32* val, f32 val_min, f32 val_max, UISliderFlags flags = 0);
	
	//displays an image. set the size using SetNextItemSize
	void Image(Texture* image, vec2 pos, f32 alpha = 1, UIImageFlags flags = 0);
	void Image(Texture* image, f32 alpha = 1, UIImageFlags flags = 0);
	
	//makes a horizontal rule across the window
	//TODO vertical separators
	//TODO text in a separator 
	//text------------
	//-----text-------
	//with arbitrary placing like this
	void Separator(f32 height);
	
	//these overloads are kind of silly change them eventually
	//InputText takes in a buffer and modifies it according to input and works much like ImGui's InputText
	//However there are overloads that will return it's UIInputTextState, allowing you to directly r/w some internal information of the
	//InputText item. This should only be used if you have a good reason to!
	b32 InputText(str8 label, u8* buffer, u32 buffSize, str8 preview = str8{}, UIInputTextFlags flags = 0);
	b32 InputText(str8 label, u8* buffer, u32 buffSize, UIInputTextCallback callbackFunc, str8 preview = str8{}, UIInputTextFlags flags = 0);
	b32 InputText(str8 label, u8* buffer, u32 buffSize, vec2 pos, str8 preview = str8{}, UIInputTextFlags flags = 0);
	b32 InputText(str8 label, u8* buffer, u32 buffSize, vec2 pos, UIInputTextCallback callbackFunc, str8 preview = str8{}, UIInputTextFlags flags = 0);
	
	//begins a menu, which is just a wrapper around BeginPopOut
	//its purpose is to provide an easy way to have lists of options such as in a context menu as well
	//as provide a way to autosize the popout
	//TODO void BeginMenu(vec2 pos, UIMenuFlags flags); //default autosize overload
	//TODO void BeginMenu(vec2 pos, vec2 size, UIMenuFlags flags);
	//TODO void EndMenu();
	
	//API for making custom items externally
	//TODO decide if it would be better to just expose the internal functions instead
	//TODO reformat this eventually since im currently making it to suit a specific purpose and am not considering polishing it atm
	//TODO write up examples/a guide on how to use this
	//some knowledge about how UI works internally is somewhat required to work this
	UIItem* BeginCustomItem();
	void    CustomItem_AdvanceCursor(UIItem* item, b32 move_cursor = 1);
	void    EndCustomItem();
	
	//TODO decide if we should just expose the internal drawing commands 
	void CustomItem_DCMakeLine(UIDrawCmd& drawCmd, vec2 start, vec2 end, f32 thickness, color color);
	void CustomItem_DCMakeFilledTriangle(UIDrawCmd& drawCmd, vec2 p1, vec2 p2, vec2 p3, color color);
	void CustomItem_DCMakeTriangle(UIDrawCmd& drawCmd, vec2 p1, vec2 p2, vec2 p3, f32 thickness, color color);
	void CustomItem_DCMakeFilledRect(UIDrawCmd& drawCmd, vec2 pos, vec2 size, color color);
	void CustomItem_DCMakeRect(UIDrawCmd& drawCmd, vec2 pos, vec2 size, f32 thickness, color color);
	void CustomItem_DCMakeCircle(UIDrawCmd& drawCmd, vec2 pos, f32 radius, u32 subdivisions, f32 thickness, color color);
	void CustomItem_DCMakeFilledCircle(Vertex2* putverts, u32* putindices, vec2 offsets, vec2 pos, f32 radius, u32 subdivisions_int, color color);
	void CustomItem_DCMakeFilledCircle(UIDrawCmd& drawCmd, vec2 pos, f32 radius, u32 subdivisions_int, color color);
	void CustomItem_DCMakeText(UIDrawCmd& drawCmd, str8 text, vec2 pos, color color, vec2 scale);
	void CustomItem_DCMakeTexture(UIDrawCmd& drawCmd, Texture* texture, vec2 p0, vec2 p1, vec2 p2, vec2 p3, f32 alpha, b32 flipx, b32 flipy);
	void CustomItem_AddDrawCmd(UIItem* item, UIDrawCmd& drawCmd);
	
	//returns if the last placed item is hovered or not
	b32 IsLastItemHovered();
	
	//allows adding flags to an item so you dont have to keep specifying them everytime you make one.
	//you are required to either remove all the added flags or reset a modified items flags by the time UI::Update is called
	void AddItemFlags(UIItemType type, Flags flags);
	void RemoveItemFlags(UIItemType type, Flags flags);
	void ResetItemFlags(UIItemType type);
	
	
	//// push/pop ////
	void PushColor(UIStyleCol idx, color color);
	void PushVar(UIStyleVar idx, f32 style);
	void PushVar(UIStyleVar idx, vec2 style);
	void PushFont(Font* font);
	void PushScale(vec2 scale);
	void PushLayer(u32 layer); //NOTE default layer is 5
	void PushWindowLayer(u32 layer);
	void PushLeftIndent(f32 indent);
	void PushRightIndent(f32 indent);
	void PushDrawTarget(u32 idx);
	void PushDrawTarget(Window* idx);
	
	void PopColor(u32 count = 1);
	void PopVar(u32 count = 1);
	void PopFont(u32 count = 1);
	void PopScale(u32 count = 1);
	void PopLayer(u32 count = 1);
	void PopWindowLayer(u32 count = 1);
	void PopLeftIndent(u32 count = 1);
	void PopRightIndent(u32 count = 1);
	void PopDrawTarget(u32 count = 1);
	
	//// windows ////
	void Begin(str8 name, UIWindowFlags flags = 0);
	void Begin(str8 name, vec2 pos, vec2 dimensions, UIWindowFlags flags = 0);
	void BeginChild(str8 name, vec2 dimensions, UIWindowFlags flags = 0);
	void BeginChild(str8 name, vec2 pos, vec2 dimensions, UIWindowFlags flags = 0);
	void BeginPopOut(str8 name, vec2 pos, vec2 dimensions, UIWindowFlags flags = 0);
	void End();
	void EndChild();
	void EndPopOut();
	//continues a given window. to access child windows use '/'
	//for example: 
	//parent/child
	//parent/child/childofchild 
	void Continue(str8 name);
	void EndContinue();
	void SetNextWindowPos(vec2 pos);
	void SetNextWindowPos(f32 x, f32 y);
	void SetNextWindowSize(vec2 size);	 
	void SetNextWindowSize(f32 x, f32 y);  
	void SetWindowName(str8 name);
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
	void DrawDebugTriangle(vec2 p0, vec2 p1, vec2 p2, color color = Color_Red);
	
}; //namespace UI

#endif //DESHI_UI_H