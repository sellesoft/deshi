#include "ui.h"
#include "../utils/array_algorithms.h"
//color pallete 
//current palette:
//https://lospec.com/palette-list/slso8
//TODO(sushi, Ui) implement menu style file loading sort of stuff yeah
//TODO(sushi, Ui) standardize what UI element each color belongs to
local struct {
	//TODO make a nice default palette maybe
} colors;

//global styling
UIStyle style;

//for color stack, saves what element was changed and what it's old color was 
struct ColorMod {
	UIStyleCol element;
	color oldCol;
};

#define UI_LAYERS 11
static constexpr u32 CHAR_SIZE = sizeof(CHAR);
static const u32 UI_CENTER_LAYER = (u32)floor((f32)UI_LAYERS / 2.f);

//for style variable stack
struct VarMod {
	UIStyleVar var;
	f32 oldFloat[2];
	VarMod(UIStyleVar var, f32 old)  { this->var = var; oldFloat[0] = old; }
	VarMod(UIStyleVar var, vec2 old) { this->var = var; oldFloat[0] = old.x; oldFloat[1] = old.y; }
};

//number of vars that are req for variable then offset of that var in UIStyle
struct UIStyleVarType {
	u32 count;
	u32 offset;
};

local const UIStyleVarType uiStyleVarTypes[] = {
	{2, offsetof(UIStyle, windowPadding)},
	{2, offsetof(UIStyle, itemSpacing)},
	{1, offsetof(UIStyle, windowBorderSize)},
	{1, offsetof(UIStyle, titleBarHeight)},
	{2, offsetof(UIStyle, titleTextAlign)},
	{2, offsetof(UIStyle, scrollAmount)},
	{2, offsetof(UIStyle, checkboxSize)},
	{1, offsetof(UIStyle, checkboxFillPadding)},
	{2, offsetof(UIStyle, inputTextTextAlign)},
	{2, offsetof(UIStyle, buttonTextAlign)},
	{2, offsetof(UIStyle, headerTextAlign)},
	{1, offsetof(UIStyle, buttonHeightRelToFont)},
	{1, offsetof(UIStyle, headerHeightRelToFont)},
	{1, offsetof(UIStyle, inputTextHeightRelToFont)},
	{1, offsetof(UIStyle, checkboxHeightRelToFont)},
	{2, offsetof(UIStyle, rowItemAlign)},
	{2, offsetof(UIStyle, rowCellPadding)},
	{1, offsetof(UIStyle, scrollBarYWidth)},
	{1, offsetof(UIStyle, scrollBarXHeight)},
	{1, offsetof(UIStyle, indentAmount)},
	{1, offsetof(UIStyle, fontHeight)},
};

//this variable defines the space the user is working in when calling UI functions
//windows are primarily a way for the user to easily position things on screen relative to a parent
//and to make detecting where text wraps and other things easier
//by default a window that takes up the entire screen and is invisible is made on init
UIWindow* curwin; 
UIWindow* hovered;


local vec2 NextWinSize  = vec2(-1, 0);
local vec2 NextWinPos   = vec2(-1, 0);
local vec2 NextItemPos  = vec2(-1, 0);
local vec2 NextItemSize = vec2(-1, 0);

local vec2 MarginPositionOffset = vec2::ZERO;
local vec2 MarginSizeOffset = vec2::ZERO;

//window map which only stores known windows
//and their order in layers eg. when one gets clicked it gets moved to be first if its set to
local map<const char*, UIWindow*>        windows;   
local map<const char*, UIInputTextState> inputTexts;  //stores known input text labels and their state
local map<const char*, b32>              combos;      //stores known combos and if they are open or not
local map<const char*, b32>              sliders;     //stores whether a slider is being actively changed
local map<const char*, b32>              headers;     //stores whether a header is open or not
local array<UIWindow*>                   windowStack; //window stack which allow us to use windows like we do colors and styles
local array<ColorMod>                    colorStack; 
local array<VarMod>                      varStack; 
local array<vec2>                        scaleStack;  //global scales
local array<Font*>                       fontStack;
local array<u32>                         layerStack;
local array<u32>                         winLayerStack;
local array<f32>                         indentStack{ 0 }; //stores global indentations

local array<UIDrawCmd> debugCmds; //debug draw cmds that are always drawn last

local u32 initColorStackSize;
local u32 initStyleStackSize;

local u32 activeId = -1; //the id of an active widget eg. input text

//row variables
local UIRow row;

//misc state vars
local UIWindow* inputupon = 0; //set to a window who has drag, scrolling, or resizing inputs being used on it 

//global ui state flags
enum UIState_ {
	UISNone                   = 0, 
	UISGlobalHovered          = 1 << 0,
	UISRowBegan               = 1 << 1,
	UISComboBegan             = 1 << 2,
	UISCursorSet              = 1 << 3,
	UISNextItemSizeSet        = 1 << 4,
	UISNextItemActive         = 1 << 5,
	UISNextItemMinSizeIgnored = 1 << 6,
}; typedef u32 UIState;

//global ui input state
enum InputState_ {
	ISNone,
	ISScrolling,
	ISDragging,
	ISResizing,
	ISPreventInputs,
}; typedef u32 InputState;

struct {
	
	UIState flags = UISNone;
	InputState input = ISNone;
	u32 layer = UI_CENTER_LAYER;
	u32 winlayer = UI_CENTER_LAYER;
	
}ui_state;

struct {
	u32 vertices = 0;
	u32 indices = 0;
	
	u32 draw_cmds = 0;
	u32 items = 0;
}ui_stats;

//helper defines
#define StateHasFlag(flag) ((ui_state.flags) & (flag))
#define StateHasFlags(flags) (((ui_state.flags) & (flags)) == (flags))
#define StateAddFlag(flag) ui_state.flags |= flag
#define StateRemoveFlag(flag) ((ui_state.flags) &= (~(flag)))
#define StateResetFlags ui_state.flags = UISNone

#define WinBegan(win)         HasFlag(win->win_state.flags, UIWSBegan)
#define WinEnded(win)         HasFlag(win->win_state.flags, UIWSEnded)
#define WinHovered(win)       HasFlag(win->win_state.flags, UIWSHovered)
#define WinChildHovered(win)  HasFlag(win->win_state.flags, UIWSChildHovered)
#define WinFocused(win)       HasFlag(win->win_state.flags, UIWSFocused)
#define WinLatched(win)       HasFlag(win->win_state.flags, UIWSLatch)
#define WinBeingScrolled(win) HasFlag(win->win_state.flags, UIWSBeingScrolled)
#define WinBeingResized(win)  HasFlag(win->win_state.flags, UIWSBeingResized)
#define WinBeingDragged(win)  HasFlag(win->win_state.flags, UIWSBeingDragged)

#define WinSetBegan(win)         AddFlag(win->win_state.flags, UIWSBegan)
#define WinSetEnded(win)         AddFlag(win->win_state.flags, UIWSEnded)
#define WinSetHovered(win)       AddFlag(win->win_state.flags, UIWSHovered)
#define WinSetChildHovered(win)  AddFlag(win->win_state.flags, UIWSChildHovered)
#define WinSetFocused(win)       AddFlag(win->win_state.flags, UIWSFocused)
#define WinSetLatched(win)       AddFlag(win->win_state.flags, UIWSLatch)
#define WinSetBeingScrolled(win) AddFlag(win->win_state.flags, UIWSBeingScrolled)
#define WinSetBeingResized(win)  AddFlag(win->win_state.flags, UIWSBeingResized)
#define WinSetBeingDragged(win)  AddFlag(win->win_state.flags, UIWSBeingDragged)

#define WinUnSetBegan(win)         RemoveFlag(win->win_state.flags, UIWSBegan)
#define WinUnSetEnded(win)         RemoveFlag(win->win_state.flags, UIWSEnded)
#define WinUnSetHovered(win)       RemoveFlag(win->win_state.flags, UIWSHovered)
#define WinUnSetChildHovered(win)  RemoveFlag(win->win_state.flags, UIWSChildHovered)
#define WinUnSetFocused(win)       RemoveFlag(win->win_state.flags, UIWSFocused)
#define WinUnSetLatched(win)       RemoveFlag(win->win_state.flags, UIWSLatch)
#define WinUnSetBeingScrolled(win) RemoveFlag(win->win_state.flags, UIWSBeingScrolled)
#define WinUnSetBeingResized(win)  RemoveFlag(win->win_state.flags, UIWSBeingResized)
#define WinUnSetBeingDragged(win)  RemoveFlag(win->win_state.flags, UIWSBeingDragged)

#define WinStateResetFlags(win) win->win_state.flags = UIWSNone

#define NextActive StateHasFlag(UISNextItemActive)

#define WinHasFlag(flag) (curwin->flags & flag)
#define WinHasFlags(flag) ((curwin->flags & flag) == flag) 
#define DrawCmdScreenPos(pos) pos + item->position + curwin->position 
#define ItemScreenPos item->position + curwin->position 

#define globalIndent *indentStack.last
#define addGlobalIndent(val) indentStack.add(val)
#define popGlobalIndent indentStack.pop()

#define CanTakeInput      ui_state.input == ISNone
#define PreventInputs     ui_state.input = ISPreventInputs
#define AllowInputs       ui_state.input = ISNone;      inputupon = 0;
#define SetResizingInput  ui_state.input = ISResizing;  inputupon = window;
#define SetDraggingInput  ui_state.input = ISDragging;  inputupon = window;
#define SetScrollingInput ui_state.input = ISScrolling; inputupon = window; WinSetBeingScrolled(window);
#define WinResizing       ui_state.input == ISResizing
#define WinDragging       ui_state.input == ISDragging
#define WinScrolling      ui_state.input == ISScrolling

//for breaking on a window's begin or end
UIWindow* break_window_begin = 0;
UIWindow* break_window_end = 0;

//for breaking on an item from the metrics window
UIWindow* break_window_item = 0;
u32 item_idx = -1;
u32 item_layer = -1;

//for breaking on a drawCmd
u32 break_drawCmd_create_hash = -1;
u32 break_drawCmd_draw_hash = -1;

#ifdef DESHI_INTERNAL
#define BreakOnItem if(break_window_item && break_window_item == curwin && curwin->items[item_layer].count == item_idx){ DebugBreakpoint;}
#else
#define BreakOnItem
#endif

#ifdef DESHI_INTERNAL
#define BreakOnDrawCmdCreation if(break_drawCmd_create_hash == drawCmd.hash) {DebugBreakpoint;}
#define BreakOnDrawCmdDraw     if(break_drawCmd_draw_hash == drawCmd.hash) {DebugBreakpoint;}
#else
#define BreakOnDrawCmdCreation
#define BreakOnDrawCmdDraw 
#endif


//helper functions


//this calculates text taking into account newlines, BUT NOT WRAPPING
//useful for sizing a window to fit some text
vec2 UI::CalcTextSize(cstring text){
	vec2 result = vec2{0, f32(style.fontHeight)};
	f32 line_width = 0;
	switch(style.font->type){
		case FontType_BDF: case FontType_NONE:{
			while(text){
				if(*text.str == '\n'){
					result.y += style.fontHeight;
					line_width = 0;
				}
				line_width += style.font->max_width * style.fontHeight / style.font->aspect_ratio / style.font->max_width;
				if(line_width > result.x) result.x = line_width ;
				advance(&text,1);
			}
		}break;
		case FontType_TTF:{
			while(text){
				if(*text.str == '\n'){
					result.y += style.fontHeight;
					line_width = 0;
				}
				line_width += style.font->GetPackedChar(*text.str)->xadvance * style.fontHeight / style.font->aspect_ratio / style.font->max_width;
				if(line_width > result.x) result.x = line_width;
				advance(&text,1);
			}
		}break;
		default: Assert(!"unhandled font type"); break;
	}
	return result;
}

vec2 UI::CalcTextSize(wcstring text) {
	vec2 result = vec2{ 0, f32(style.fontHeight) };
	f32 line_width = 0;
	switch (style.font->type) {
		case FontType_BDF: case FontType_NONE: {
			while (text) {
				if (*text.str == '\n') {
					result.y += style.fontHeight;
					line_width = 0;
				}
				line_width += style.font->max_width;
				if (line_width > result.x) result.x = line_width;
				advance(&text, 1);
			}
		}break;
		case FontType_TTF: {
			while (text) {
				if (*text.str == '\n') {
					result.y += style.fontHeight;
					line_width = 0;
				}
				line_width += style.font->GetPackedChar(*text.str)->xadvance * style.fontHeight / style.font->aspect_ratio / style.font->max_width;
				if (line_width > result.x) result.x = line_width;
				advance(&text, 1);
			}
		}break;
		default: Assert(!"unhandled font type"); break;
	}
	return result;
}

inline b32 isItemHovered(UIItem* item) {
	return Math::PointInRectangle(DeshInput->mousePos, item->position + curwin->position, item->size * style.globalScale);
}

inline b32 isLocalAreaHovered(vec2 pos, vec2 size, UIItem* item) {
	return Math::PointInRectangle(DeshInput->mousePos, pos + item->position + curwin->position, size);
}

inline b32 isItemActive(UIItem* item) {
	return WinHovered(curwin) && CanTakeInput && isItemHovered(item);
}

//internal master cursor controller
//  this is an attempt to centralize what happens at the end of each item function
// 
//  i expect this to fall through at some point, as not all items are created equal and may need to
//  have different things happen after its creation, which could be handled as special cases within
//  the function itself.
inline void AdvanceCursor(UIItem* itemmade, b32 moveCursor = 1) {
	
	//if a row is in progress, we must reposition the item to conform to row style variables
	//this means that you MUST ensure that this happens before any interactions with the item are calculated
	//for instance in the case of Button, this must happen before you check that the user has clicked it!
	if (StateHasFlag(UISRowBegan)) {
		//abstract item types (lines, rectangles, etc.) are not row'd, for now
		if (itemmade->type != UIItemType_Abstract) {
			row.items.add(itemmade);
			
			f32 height = row.height;
			f32 width;
			//determine if the width is relative to the size of the item or not
			if (row.columnWidths[(row.items.count - 1) % row.columns].second != false)
				width = itemmade->size.x * row.columnWidths[(row.items.count - 1) % row.columns].first;
			else
				width = row.columnWidths[(row.items.count - 1) % row.columns].first;
			
			itemmade->position.y = row.position.y + (height - itemmade->size.y) * itemmade->style.rowItemAlign.y + row.yoffset;
			itemmade->position.x = row.position.x + row.xoffset + (width - itemmade->size.x) * itemmade->style.rowItemAlign.x;
			
			row.xoffset += width;
			
			//if we have finished a row, set xoffset to 0 and offset y for another row
			if (row.items.count % row.columns == 0) {
				row.xoffset = 0;
				row.yoffset += row.height;
			}
			
			//we dont need to handle moving the cursor here, because the final position of the cursor after a row is handled in EndRow()
		}
	}
	else if (moveCursor) curwin->cursor = vec2{ 0, itemmade->position.y + itemmade->size.y + style.itemSpacing.y - style.windowPadding.y + curwin->scy - style.windowBorderSize } ;
}

//returns if the window can scroll over x
inline b32 CanScrollX(UIWindow* window = curwin) {
	return !HasFlag(window->flags, UIWindowFlags_NoScrollX) && window->width < window->minSizeForFit.x;
}

inline b32 CanScrollY(UIWindow* window = curwin) {
	return !HasFlag(window->flags, UIWindowFlags_NoScrollY) && window->height < window->minSizeForFit.y;
}

//function for getting the position of a new item based on style, so the long string of additions
//is centralized for new additions
inline vec2 PositionForNewItem(UIWindow* window = curwin) {
	vec2 pos = window->cursor + (style.windowPadding + MarginPositionOffset - window->scroll) + vec2(globalIndent, 0)
		+ vec2::ONE * ((HasFlag(window->flags, UIWindowFlags_NoBorder)) ? 0 : style.windowBorderSize);
	MarginPositionOffset = vec2::ZERO;
	return pos;
}

//returns a pair representing the area of the window that is bordered
//first is the position and second is the size
inline pair<vec2, vec2> BorderedArea(UIWindow* window = curwin) {
	return make_pair(
					 vec2::ONE * style.windowBorderSize,
					 window->dimensions - vec2::ONE * 2 * style.windowBorderSize
					 );
}

//same as the bordered area, but also takes into account the margins
inline pair<vec2, vec2> MarginedArea(UIWindow* window = curwin) {
	vec2 f = vec2::ONE * style.windowBorderSize + vec2::ONE * style.windowPadding;
	vec2 s = window->dimensions - 2 * f - MarginSizeOffset;
	s.x -= (CanScrollY() ? style.scrollBarYWidth : 0);
	//s.y -= (CanScrollX() ? style.scrollBarXHeight : 0);
	MarginSizeOffset = vec2::ZERO;
	return make_pair(f, s);
}

//the bordered area taking into account the scroll bars
inline pair<vec2, vec2> ScrollBaredArea(UIWindow* window = curwin) {
	auto p = BorderedArea(window);
	p.second.x -= (CanScrollY(window) ? style.scrollBarYWidth : 0);
	p.second.y -= (CanScrollX(window) ? style.scrollBarXHeight : 0);
	return p;
}

//TODO(sushi) eventually change these to always use curwin instead of checking everytime
// probably just separate them into 2 overloaded functions each instead
FORCE_INLINE f32 BorderedRight(UIWindow* window = curwin)  { return window->dimensions.x - (window == curwin ? style.windowBorderSize : window->style.windowBorderSize); }
FORCE_INLINE f32 BorderedLeft(UIWindow* window = curwin)   { return (window == curwin ? style.windowBorderSize : window->style.windowBorderSize); }
FORCE_INLINE f32 BorderedTop(UIWindow* window = curwin)    { return (window == curwin ? style.windowBorderSize : window->style.windowBorderSize); }
FORCE_INLINE f32 BorderedBottom(UIWindow* window = curwin) { return window->dimensions.y - (window == curwin ? style.windowBorderSize : window->style.windowBorderSize); }

FORCE_INLINE f32 MarginedRight(UIWindow* window = curwin) { f32 ret = window->dimensions.x - (window == curwin ? style.windowBorderSize + style.windowPadding.x : window->style.windowBorderSize + window->style.windowPadding.x) - (CanScrollY(window) ? (window == curwin ? style.scrollBarYWidth : window->style.scrollBarYWidth) : 0) + MarginSizeOffset.x; MarginSizeOffset.x = 0; return ret; }
FORCE_INLINE f32 MarginedLeft(UIWindow* window = curwin)   { return (window == curwin ? style.windowBorderSize + style.windowPadding.x : window->style.windowBorderSize + window->style.windowPadding.x) ; }
FORCE_INLINE f32 MarginedTop(UIWindow* window = curwin)    { return (window == curwin ? style.windowBorderSize + style.windowPadding.y : window->style.windowBorderSize + window->style.windowPadding.y) ; }
FORCE_INLINE f32 MarginedBottom(UIWindow* window = curwin) { f32 ret = window->dimensions.y - (window == curwin ? style.windowBorderSize + style.windowPadding.y : window->style.windowBorderSize + window->style.windowPadding.y) - (CanScrollX(window) ? (window == curwin ? style.scrollBarXHeight : window->style.scrollBarXHeight) : 0) + MarginSizeOffset.y; MarginSizeOffset.y = 0; return ret; }

FORCE_INLINE f32 ScrollBaredRight(UIWindow* window = curwin)  { return BorderedRight(window) - (CanScrollY() ? (window == curwin ? style.scrollBarYWidth : window->style.scrollBarYWidth) : 0); }
FORCE_INLINE f32 ScrollBaredLeft(UIWindow* window = curwin)   { return BorderedLeft(window); }
FORCE_INLINE f32 ScrollBaredTop(UIWindow* window = curwin)    { return BorderedTop(window); }
FORCE_INLINE f32 ScrollBaredBottom(UIWindow* window = curwin) { return BorderedBottom(window) - (CanScrollX() ? (window == curwin ? style.scrollBarXHeight : window->style.scrollBarXHeight) : 0); }

//return the maximum width an item can be in a non-scrolled state
FORCE_INLINE f32 MaxItemWidth(UIWindow* window = curwin) {
	return MarginedRight(window) - MarginedLeft(window);
}

FORCE_INLINE b32 MouseInArea(vec2 pos, vec2 size) {
	return Math::PointInRectangle(DeshInput->mousePos, pos, size);
}

FORCE_INLINE b32 MouseInWinArea(vec2 pos, vec2 size) {
	return Math::PointInRectangle(DeshInput->mousePos - curwin->position, pos, size);
}

inline vec2 DecideItemSize(vec2 defaultSize, vec2 itemPos) {
	vec2 size;
	if (NextItemSize.x != -1) {
		if (NextItemSize.x == MAX_F32)
			size.x = MarginedRight() - itemPos.x;
		else if (NextItemSize.x == 0)
			if (defaultSize.x == MAX_F32)
				size.x = MarginedRight() - itemPos.x;
			else size.x = defaultSize.x;
		else size.x = NextItemSize.x;
		
		if (NextItemSize.y == MAX_F32)
			size.y = MarginedBottom() - itemPos.y;
		else if (NextItemSize.y == 0)
			if(defaultSize.y == MAX_F32)
				size.y = MarginedBottom() - itemPos.y;
			else size.y = defaultSize.y;
		else size.y = NextItemSize.y;
	}
	else {
		if (defaultSize.x == MAX_F32)
			size.x = MarginedRight() - itemPos.x;
		else size.x = defaultSize.x;
		
		if (defaultSize.y == MAX_F32)
			size.y = MarginedBottom() - itemPos.y;
		else size.y = defaultSize.y;
	}
	return size;
	
}

FORCE_INLINE void SetWindowCursor(CursorType curtype) {
	DeshWindow->SetCursor(curtype);
	StateAddFlag(UISCursorSet);
}

FORCE_INLINE vec2 GetTextScale() {
	return vec2::ONE* style.fontHeight / (f32)style.font->max_height * style.globalScale;
}


UIStyle& UI::GetStyle(){
	return style;
}

UIWindow* UI::GetWindow() {
	return curwin;
}

vec2 UI::GetLastItemPos() {
	//Assert(curwin->items.count, "Attempt to get last item position without creating any items!");
	return curwin->items[ui_state.layer].last->position;
}

vec2 UI::GetLastItemSize() {
	//Assert(curwin->items.count, "Attempt to get last item size without creating any items!");
	return curwin->items[ui_state.layer].last->size;
}

vec2 UI::GetLastItemScreenPos() {
	//Assert(curwin->items.count, "Attempt to get last item position without creating any items!");
	return curwin->position + curwin->items[ui_state.layer].last->position;
}

vec2 UI::GetWindowRemainingSpace() {
	return vec2(MarginedRight() - curwin->curx, MarginedBottom() - curwin->cury);
}

vec2 UI::GetPositionForNextItem() {
	return PositionForNewItem();
}

u32 UI::GetCenterLayer() {
	return UI_CENTER_LAYER;
}

f32 UI::GetBorderedRight()                { return BorderedRight(); }
f32 UI::GetBorderedLeft()                 { return BorderedLeft(); }
f32 UI::GetBorderedTop()                  { return BorderedTop(); }
f32 UI::GetBorderedBottom()               { return BorderedBottom(); }
f32 UI::GetMarginedRight()                { return MarginedRight(); }
f32 UI::GetMarginedLeft()                 { return MarginedLeft(); }
f32 UI::GetMarginedTop()                  { return MarginedTop(); }
f32 UI::GetMarginedBottom()               { return MarginedBottom(); }
f32 UI::GetScrollBaredRight()             { return ScrollBaredRight(); }
f32 UI::GetScrollBaredLeft()              { return ScrollBaredLeft(); }
f32 UI::GetScrollBaredTop()               { return ScrollBaredTop(); }
f32 UI::GetScrollBaredBottom()            { return ScrollBaredBottom(); }
pair<vec2, vec2> UI::GetBorderedArea()    { return BorderedArea(); }
pair<vec2, vec2> UI::GetMarginedArea()    { return MarginedArea(); }
pair<vec2, vec2> UI::GetScrollBaredArea() { return ScrollBaredArea(); }

//returns the cursor to the same line as the previous and moves it to the right by the 
//width of the object
void UI::SameLine() {
	//Assert(curwin->items.count, "Attempt to sameline an item creating any items!");
	if (curwin->items[ui_state.layer].last) {
		curwin->cursor.y = curwin->items[ui_state.layer].last->initialCurPos.y;
		curwin->cursor.x += curwin->items[ui_state.layer].last->initialCurPos.x + curwin->items[ui_state.layer].last->size.x + style.itemSpacing.x;
	}
}

void UI::SetScroll(vec2 scroll) {
	if (scroll.x == MAX_F32)
		curwin->scx = curwin->maxScroll.x;
	else
		curwin->scx = scroll.x;
	
	if (scroll.y == MAX_F32)
		curwin->scy = curwin->maxScroll.y;
	else
		curwin->scy = scroll.y;
}

void UI::SetNextItemActive() {
	StateAddFlag(UISNextItemActive);
}

void UI::SetNextItemSize(vec2 size) {
	NextItemSize = size;
}

void UI::SetMarginPositionOffset(vec2 offset) {
	MarginPositionOffset = offset;
}

void UI::SetMarginSizeOffset(vec2 offset){
	MarginSizeOffset = offset;
}

void UI::SetNextItemMinSizeIgnored() {
	StateAddFlag(UISNextItemMinSizeIgnored);
}

//internal last item getter, returns nullptr if there are none
FORCE_INLINE UIItem* UI::GetLastItem(u32 layeroffset) {
	return curwin->items[ui_state.layer + layeroffset].last;
}

//helper for making any new UIItem, since now we must work with item pointers internally
//this function also decides if we are working with a new item or continuing to work on a previous
inline UIItem* BeginItem(UIItemType type, u32 layeroffset = 0) {
	
	if (type == UIItemType_PreItems) {
		curwin->preItems.add(UIItem{ type, curwin->cursor, style });
		return curwin->preItems.last;
	}
	else if (type == UIItemType_PostItems) {
		curwin->postItems.add(UIItem{ type, curwin->cursor, style });
		return curwin->postItems.last;
	}
	else if (type == UIItemType_PopOutWindow) {
		curwin->popOuts.add(UIItem{ type, curwin->cursor, style });
		return curwin->popOuts.last;
	}
	else {
		curwin->items[ui_state.layer + layeroffset].add(UIItem{ type, curwin->cursor, style });
#ifdef DESHI_INTERNAL
		UI::GetLastItem(layeroffset)->item_layer = ui_state.layer + layeroffset;
		UI::GetLastItem(layeroffset)->item_idx = curwin->items[ui_state.layer + layeroffset].count;
		BreakOnItem;
#endif
	}
	if (StateHasFlag(UISNextItemMinSizeIgnored)) {
		UI::GetLastItem(layeroffset)->trackedForMinSize = 0;
		StateRemoveFlag(UISNextItemMinSizeIgnored);
	}
	
	ui_stats.items++;
	curwin->items_count++;
	return UI::GetLastItem(layeroffset);
}

inline void EndItem(UIItem* item) {
	//copy the last made item to lastitem, so we can look back at it independently of custom item nonsense
	//maybe only do this is we're making a custom item
	//lastitem = *item;
}

//this is for debugging debug cmds, all it does extra is hash the drawCmd
//so we can break on it later
inline void AddDrawCmd(UIItem* item, UIDrawCmd& drawCmd) {
	if (!drawCmd.tex) drawCmd.tex = style.font->tex;
	drawCmd.hash = hash<UIDrawCmd>{}(drawCmd);
	item->drawCmds.add(drawCmd);
	ui_stats.draw_cmds++;
	ui_stats.vertices += drawCmd.counts.x;
	ui_stats.indices += drawCmd.counts.y;
	Assert(drawCmd.counts.x < UIDRAWCMD_MAX_VERTICES);
	Assert(drawCmd.counts.y < UIDRAWCMD_MAX_INDICES);
	BreakOnDrawCmdCreation;
}

//@BeginRow
//  a row is a collection of columns used to align a number of items nicely
//  you are required to specify the width of each column when using Row, as it removes the complicated
//  nature of having to figure this out after the fact. while row width is not much of a problem, row height is
//  so you are required to define a static height upon calling the function
//

void UI::BeginRow(u32 columns, f32 rowHeight, UIRowFlags flags) {
	Assert(!StateHasFlag(UISRowBegan), "Attempted to start a new Row without finishing one already in progress!");
	//TODO(sushi) when we have more row flags, check for mutually exclusive flags here
	row.columns = columns;
	row.flags = flags;
	row.height = rowHeight;
	row.position = PositionForNewItem();
	forI(columns) row.columnWidths.add({ 0.f,false });
	StateAddFlag(UISRowBegan);
}

void UI::EndRow() {
	Assert(StateHasFlag(UISRowBegan), "Attempted to a end a row without calling BeginRow() first!");
	Assert(row.items.count % row.columns == 0, "Attempted to end a Row without giving the correct amount of items!");
	curwin->cursor = vec2{ 0, row.position.y  + row.yoffset + style.itemSpacing.y - style.windowPadding.y + curwin->scroll.y };
	row.items.clear();
	row.columnWidths.clear();
	row = UIRow{ 0 };
	StateRemoveFlag(UISRowBegan);
}

//this function sets up a static column width for a specified column that does not respect the size of the object
void UI::RowSetupColumnWidth(u32 column, f32 width) {
	Assert(StateHasFlag(UISRowBegan), "Attempted to set a column's width with no Row in progress!");
	Assert(column <= row.columns && column >= 1, "Attempted to set a column who doesn't exists width!");
	row.columnWidths[column - 1] = { width, false };
}

//this function sets up static column widths that do not respect the size of the item at all
void UI::RowSetupColumnWidths(array<f32> widths){
	Assert(StateHasFlag(UISRowBegan), "Attempted to pass column widths without first calling BeginRow()!");
	Assert(widths.count == row.columns, "Passed in the wrong amount of column widths for in progress Row");
	forI(row.columns)
		row.columnWidths[i] = { widths[i], false };
}

//see the function below for what this does
void UI::RowSetupRelativeColumnWidth(u32 column, f32 width) {
	Assert(StateHasFlag(UISRowBegan), "Attempted to set a column's width with no Row in progress!");
	Assert(column <= row.columns && column >= 1, "Attempted to set a column who doesn't exists width!");
	row.columnWidths[column - 1] = { width, true };
}

//this function sets it so that column widths are relative to the size of the item the cell holds
//meaning it should be passed something like 1.2 or 1.3, indicating that the column should have a width of 
//1.2 * width of the item
void UI::RowSetupRelativeColumnWidths(array<f32> widths) {
	Assert(StateHasFlag(UISRowBegan), "Attempted to pass column widths without first calling BeginRow()!");
	Assert(widths.count == row.columns, "Passed in the wrong amount of column widths for in progress Row");
	forI(row.columns)
		row.columnWidths[i] = { widths[i], true };
}

//4 verts, 6 indices
FORCE_INLINE vec2
MakeLine(Vertex2* putverts, u32* putindices, vec2 offsets, vec2 start, vec2 end, f32 thickness, color color) {
	Assert(putverts && putindices);
	if (color.a == 0) return vec2::ZERO;

	u32     col = color.rgba;
	Vertex2* vp = putverts + (u32)offsets.x;
	u32* ip = putindices + (u32)offsets.y;

	vec2 ott = end - start;
	vec2 norm = vec2(ott.y, -ott.x).normalized();

	ip[0] = offsets.x; ip[1] = offsets.x + 1; ip[2] = offsets.x + 2;
	ip[3] = offsets.x; ip[4] = offsets.x + 2; ip[5] = offsets.x + 3;
	vp[0].pos = { start.x,start.y }; vp[0].uv = { 0,0 }; vp[0].color = col;
	vp[1].pos = { end.x,    end.y }; vp[1].uv = { 0,0 }; vp[1].color = col;
	vp[2].pos = { end.x,    end.y }; vp[2].uv = { 0,0 }; vp[2].color = col;
	vp[3].pos = { start.x,start.y }; vp[3].uv = { 0,0 }; vp[3].color = col;

	vp[0].pos += norm * thickness / 2;
	vp[1].pos += norm * thickness / 2;
	vp[2].pos -= norm * thickness / 2;
	vp[3].pos -= norm * thickness / 2;

	return vec2(4, 6);
}

FORCE_INLINE void
MakeLine(UIDrawCmd& drawCmd, vec2 start, vec2 end, f32 thickness, color color) {
	drawCmd.counts += MakeLine(drawCmd.vertices, drawCmd.indices, drawCmd.counts, start, end, thickness, color);
	drawCmd.type = UIDrawType_Line;
}

//3 verts, 3 indices
FORCE_INLINE vec2 
MakeFilledTriangle(Vertex2* putverts, u32* putindices, vec2 offsets, vec2 p1, vec2 p2, vec2 p3, color color) {
	Assert(putverts && putindices);
	if (color.a == 0) return vec2::ZERO;
	
	u32     col = color.rgba;
	Vertex2* vp = putverts + (u32)offsets.x;
	u32*     ip = putindices + (u32)offsets.y;

	ip[0] = offsets.x; ip[1] = offsets.x + 1; ip[2] = offsets.x + 2;
	vp[0].pos = p1; vp[0].uv = { 0,0 }; vp[0].color = col;
	vp[1].pos = p2; vp[1].uv = { 0,0 }; vp[1].color = col;
	vp[2].pos = p3; vp[2].uv = { 0,0 }; vp[2].color = col;
	
	return vec2(3, 3);
}

FORCE_INLINE void
MakeFilledTriangle(UIDrawCmd& drawCmd, vec2 p1, vec2 p2, vec2 p3, color color) {
	drawCmd.counts += MakeFilledTriangle(drawCmd.vertices, drawCmd.indices, drawCmd.counts, p1, p2, p3, color);
	drawCmd.type = UIDrawType_FilledTriangle;
}

FORCE_INLINE vec2
MakeTriangle(Vertex2* putverts, u32* putindices, vec2 offsets, vec2 p0, vec2 p1, vec2 p2, f32 thickness, color color) {
	Assert(putverts && putindices);
	if (color.a == 0) return vec2::ZERO;

	u32     col = color.rgba;
	Vertex2* vp = putverts + (u32)offsets.x;
	u32*     ip = putindices + (u32)offsets.y;

	MakeLine(vp, ip, vec2::ZERO,  p0, p1, 1, color);
	MakeLine(vp, ip, vec2(4, 6),  p1, p2, 1, color);
	MakeLine(vp, ip, vec2(8, 12), p2, p0, 1, color);

	return vec2(12, 18);

	//ip[0]  = offsets.x + 0; ip[1]  = offsets.x + 1; ip[2]  = offsets.x + 3;
	//ip[3]  = offsets.x + 0; ip[4]  = offsets.x + 3; ip[5]  = offsets.x + 2;
	//ip[6]  = offsets.x + 2; ip[7]  = offsets.x + 3; ip[8]  = offsets.x + 5;
	//ip[9]  = offsets.x + 2; ip[10] = offsets.x + 5; ip[11] = offsets.x + 4;
	//ip[12] = offsets.x + 4; ip[13] = offsets.x + 5; ip[14] = offsets.x + 1;
	//ip[15] = offsets.x + 4; ip[16] = offsets.x + 1; ip[17] = offsets.x + 0;
	//
	//f32 ang1 = Math::AngBetweenVectors(p1 - p0, p2 - p0)/2;
	//f32 ang2 = Math::AngBetweenVectors(p0 - p1, p2 - p1)/2;
	//f32 ang3 = Math::AngBetweenVectors(p1 - p2, p0 - p2)/2;
	//
	//vec2 p0offset = (Math::vec2RotateByAngle(-ang1, p2 - p0).normalized() * thickness / (2 * sinf(Radians(ang1)))).clampedMag(0, thickness * 2);
	//vec2 p1offset = (Math::vec2RotateByAngle(-ang2, p2 - p1).normalized() * thickness / (2 * sinf(Radians(ang2)))).clampedMag(0, thickness * 2);
	//vec2 p2offset = (Math::vec2RotateByAngle(-ang3, p0 - p2).normalized() * thickness / (2 * sinf(Radians(ang3)))).clampedMag(0, thickness * 2);
	//       
	//vp[0].pos = p0 - p0offset; vp[0].uv = { 0,0 }; vp[0].color = col;
	//vp[1].pos = p0 + p0offset; vp[1].uv = { 0,0 }; vp[1].color = col;
	//vp[2].pos = p1 + p1offset; vp[2].uv = { 0,0 }; vp[2].color = col;
	//vp[3].pos = p1 - p1offset; vp[3].uv = { 0,0 }; vp[3].color = col;
	//vp[4].pos = p2 + p2offset; vp[4].uv = { 0,0 }; vp[4].color = col;
	//vp[5].pos = p2 - p2offset; vp[5].uv = { 0,0 }; vp[5].color = col;

	//return vec3(6, 18);
}

FORCE_INLINE void
MakeTriangle(UIDrawCmd& drawCmd, vec2 p1, vec2 p2, vec2 p3, f32 thickness, color color) {
	drawCmd.counts += MakeTriangle(drawCmd.vertices, drawCmd.indices, drawCmd.counts, p1, p2, p3, thickness, color);
	drawCmd.type = UIDrawType_Triangle;
}



//4 verts, 6 indices
FORCE_INLINE vec2
MakeFilledRect(Vertex2* putverts, u32* putindices, vec2 offsets, vec2 pos, vec2 size, color color) {
	Assert(putverts && putindices);
	if (color.a == 0) return vec2::ZERO;
	
	u32     col = color.rgba;
	Vertex2* vp = putverts + (u32)offsets.x;
	u32*     ip = putindices + (u32)offsets.y;
	
	vec2 tl = pos;
	vec2 br = pos + size;
	vec2 bl = pos + vec2(0, size.y);
	vec2 tr = pos + vec2(size.x, 0);

	ip[0] = offsets.x; ip[1] = offsets.x + 1; ip[2] = offsets.x + 2;
	ip[3] = offsets.x; ip[4] = offsets.x + 2; ip[5] = offsets.x + 3;
	vp[0].pos = tl; vp[0].uv = { 0,0 }; vp[0].color = col;
	vp[1].pos = tr; vp[1].uv = { 0,0 }; vp[1].color = col;
	vp[2].pos = br; vp[2].uv = { 0,0 }; vp[2].color = col;
	vp[3].pos = bl; vp[3].uv = { 0,0 }; vp[3].color = col;
	return vec2(4, 6);
}

FORCE_INLINE void
MakeFilledRect(UIDrawCmd& drawCmd, vec2 pos, vec2 size, color color) {
	drawCmd.counts += MakeFilledRect(drawCmd.vertices, drawCmd.indices, drawCmd.counts, pos, size, color);
	drawCmd.type = UIDrawType_FilledRectangle;
}
//8 verts, 24 indices
FORCE_INLINE vec2
MakeRect(Vertex2* putverts, u32* putindices, vec2 offsets, vec2 pos, vec2 size, f32 thickness, color color) {
	Assert(putverts && putindices);
	if (color.a == 0) return vec2::ZERO;
	
	u32     col = color.rgba;
	Vertex2* vp = putverts + (u32)offsets.x;
	u32*     ip = putindices + (u32)offsets.y;
	
	vec2 tl = pos;
	vec2 br = pos + size;
	vec2 bl = pos + vec2(0, size.y);
	vec2 tr = pos + vec2(size.x, 0);
	
	f32 sqt = sqrtf(thickness);
	vec2 tlo = sqt * vec2(-M_HALF_SQRT_TWO, -M_HALF_SQRT_TWO);
	vec2 bro = sqt * vec2( M_HALF_SQRT_TWO,  M_HALF_SQRT_TWO);
	vec2 tro = sqt * vec2( M_HALF_SQRT_TWO, -M_HALF_SQRT_TWO);
	vec2 blo = sqt * vec2(-M_HALF_SQRT_TWO,  M_HALF_SQRT_TWO);

	ip[0]  = offsets.x + 0; ip[1]  = offsets.x + 1; ip[2]  = offsets.x + 3;
	ip[3]  = offsets.x + 0; ip[4]  = offsets.x + 3; ip[5]  = offsets.x + 2;
	ip[6]  = offsets.x + 2; ip[7]  = offsets.x + 3; ip[8]  = offsets.x + 5;
	ip[9]  = offsets.x + 2; ip[10] = offsets.x + 5; ip[11] = offsets.x + 4;
	ip[12] = offsets.x + 4; ip[13] = offsets.x + 5; ip[14] = offsets.x + 7;
	ip[15] = offsets.x + 4; ip[16] = offsets.x + 7; ip[17] = offsets.x + 6;
	ip[18] = offsets.x + 6; ip[19] = offsets.x + 7; ip[20] = offsets.x + 1;
	ip[21] = offsets.x + 6; ip[22] = offsets.x + 1; ip[23] = offsets.x + 0;
	
	vp[0].pos = tl + tlo; vp[0].uv = { 0,0 }; vp[0].color = col;
	vp[1].pos = tl + bro; vp[1].uv = { 0,0 }; vp[1].color = col;
	vp[2].pos = tr + tro; vp[2].uv = { 0,0 }; vp[2].color = col;
	vp[3].pos = tr + blo; vp[3].uv = { 0,0 }; vp[3].color = col;
	vp[4].pos = br + bro; vp[4].uv = { 0,0 }; vp[4].color = col;
	vp[5].pos = br + tlo; vp[5].uv = { 0,0 }; vp[5].color = col;
	vp[6].pos = bl + blo; vp[6].uv = { 0,0 }; vp[6].color = col;
	vp[7].pos = bl + tro; vp[7].uv = { 0,0 }; vp[7].color = col;
	return vec2(8, 24);
}

FORCE_INLINE void
MakeRect(UIDrawCmd& drawCmd, vec2 pos, vec2 size, f32 thickness, color color) {
	drawCmd.counts += MakeRect(drawCmd.vertices, drawCmd.indices, drawCmd.counts, pos, size, thickness, color);
	drawCmd.type = UIDrawType_Rectangle;
}

FORCE_INLINE vec2
MakeCircle(Vertex2* putverts, u32* putindices, vec2 offsets, vec2 pos, f32 radius, u32 subdivisions_int, f32 thickness, color color) {
	Assert(putverts && putindices);
	if (color.a == 0) return vec2::ZERO;
	
	u32     col = color.rgba;
	Vertex2* vp = putverts + (u32)offsets.x;
	u32*     ip = putindices + (u32)offsets.y;

	f32 subdivisions = f32(subdivisions_int);
	u32 nuindexes = subdivisions * 6;

	//first and last point
	vec2 last = pos + vec2(radius, 0);
	vp[0].pos = last + vec2(-thickness / 2, 0);	vp[0].uv={0,0}; vp[0].color=col;
	vp[1].pos = last + vec2( thickness / 2, 0); vp[1].uv={0,0}; vp[1].color=col;
	ip[0] = offsets.x + 0; ip[1] = offsets.x + 1; ip[3] = offsets.x + 0;
	ip[nuindexes - 1] = offsets.x + 0; ip[nuindexes - 2] = ip[nuindexes - 4] = offsets.x + 1;

	for (int i = 1; i < subdivisions_int; i++) {
		f32 a1 = (f32(i) * M_2PI) / subdivisions;
		vec2 offset(radius * cosf(a1), radius * sinf(a1));
		vec2 point = pos + offset;

		u32 idx = i * 2;
		vp[idx].pos = point - offset.normalized() * thickness / 2; vp[idx].uv = { 0,0 }; vp[idx].color = col;
		vp[idx + 1].pos = point + offset.normalized() * thickness / 2; vp[idx + 1].uv = { 0,0 }; vp[idx + 1 ].color = col;
		
		u32 ipidx1 = 6 * (i - 1) + 2;
		u32 ipidx2 = 6 * i - 1;
		ip[ipidx1] = ip[ipidx1 + 2] = ip[ipidx1 + 5] = offsets.x + idx + 1;
		ip[ipidx2] = ip[ipidx2 + 1] = ip[ipidx2 + 4] = offsets.x + idx;
	}

	return vec2(2 * subdivisions, nuindexes);
}

FORCE_INLINE void
MakeCircle(UIDrawCmd& drawCmd, vec2 pos, f32 radius, u32 subdivisions, f32 thickness, color color) {
	drawCmd.counts += MakeCircle(drawCmd.vertices, drawCmd.indices, drawCmd.counts, pos, radius, subdivisions, thickness, color);
	drawCmd.type = UIDrawType_Circle;
}

FORCE_INLINE vec2 
MakeFilledCircle(Vertex2* putverts, u32* putindices, vec2 offsets, vec2 pos, f32 radius, u32 subdivisions_int, color color) {
	Assert(putverts && putindices);
	if (color.a == 0) return vec2::ZERO;
	
	u32     col = color.rgba;
	Vertex2* vp = putverts + (u32)offsets.x;
	u32*     ip = putindices + (u32)offsets.y;

	vp[0].pos = pos; vp[0].uv = { 0,0 }; vp[0].color = col;
	vp[1].pos = pos + vec2(radius, 0); vp[1].uv = { 0,0 }; vp[1].color = col;
	u32 nuindexes = 3 * subdivisions_int;

	ip[1] = offsets.x + 1;
	for (int i = 0; i < nuindexes; i += 3) ip[i] = offsets.x;

	ip[nuindexes - 1] = offsets.x + 1;

	vec2 sum;
	f32 subdivisions = f32(subdivisions_int);
	for(u32 i = 1; i < subdivisions_int; i++) {
		f32 a1 = (f32(i) * M_2PI) / subdivisions;
		vec2 offset(radius * cosf(a1), radius * sinf(a1));
		vec2 point = pos + offset;

		vp[i+1].pos = point; vp[i+1].uv = { 0,0 }; vp[i+1].color = col;

		u32 ipidx = 3 * i - 1;
		ip[ipidx] = ip[ipidx + 2] = offsets.x + i + 1;
	}

	return vec2(subdivisions + 1, nuindexes);
}

FORCE_INLINE void
MakeFilledCircle(UIDrawCmd& drawCmd, vec2 pos, f32 radius, u32 subdivisions_int, color color) {
	drawCmd.counts += MakeFilledCircle(drawCmd.vertices, drawCmd.indices, drawCmd.counts, pos, radius, subdivisions_int, color);
	drawCmd.type = UIDrawType_FilledCircle;
}

FORCE_INLINE vec2
MakeText(Vertex2* putverts, u32* putindices, vec2 offsets, cstring text, vec2 pos, color color, vec2 scale) {
	Assert(putverts && putindices);
	if (color.a == 0) return vec2::ZERO;
	
	vec2 sum;
	switch (style.font->type) {
		//// BDF (and NULL) font rendering ////
		case FontType_BDF: case FontType_NONE: {
			forI(text.count) {
				u32     col = color.rgba;
				Vertex2* vp = putverts + (u32)offsets.x + 4 * i;
				u32*     ip = putindices + (u32)offsets.y + 6 * i;
				
				f32 w = style.font->max_width * scale.x;
				f32 h = style.font->max_height * scale.y;
				f32 dy = 1.f / (f32)style.font->count;
				
				f32 idx = f32(text[i] - 32);
				f32 topoff = idx * dy;
				f32 botoff = topoff + dy;

				ip[0] = offsets.x+4*i; ip[1] = offsets.x+4*i + 1; ip[2] = offsets.x+4*i + 2;
				ip[3] = offsets.x+4*i; ip[4] = offsets.x+4*i + 2; ip[5] = offsets.x+4*i + 3;
				vp[0].pos = { pos.x + 0,pos.y + 0 }; vp[0].uv = { 0,topoff + style.font->uvOffset }; vp[0].color = col;
				vp[1].pos = { pos.x + w,pos.y + 0 }; vp[1].uv = { 1,topoff + style.font->uvOffset }; vp[1].color = col;
				vp[2].pos = { pos.x + w,pos.y + h }; vp[2].uv = { 1,botoff + style.font->uvOffset }; vp[2].color = col;
				vp[3].pos = { pos.x + 0,pos.y + h }; vp[3].uv = { 0,botoff + style.font->uvOffset }; vp[3].color = col;
				
				pos.x += style.font->max_width * scale.x;
				
			}
		}break;
		//// TTF font rendering ////
		case FontType_TTF: {
			forI(text.count) {
				u32     col = color.rgba;
				Vertex2* vp = putverts + (u32)offsets.x + 4 * i;
				u32*     ip = putindices + (u32)offsets.y + 6 * i;
				
				aligned_quad q = style.font->GetPackedQuad(text[i], &pos, scale);

				ip[0] = offsets.x+4*i; ip[1] = offsets.x+4*i + 1; ip[2] = offsets.x+4*i + 2;
				ip[3] = offsets.x+4*i; ip[4] = offsets.x+4*i + 2; ip[5] = offsets.x+4*i + 3;
				vp[0].pos = { q.x0,q.y0 }; vp[0].uv = { q.s0,q.t0 + style.font->uvOffset }; vp[0].color = col;
				vp[1].pos = { q.x1,q.y0 }; vp[1].uv = { q.s1,q.t0 + style.font->uvOffset }; vp[1].color = col;
				vp[2].pos = { q.x1,q.y1 }; vp[2].uv = { q.s1,q.t1 + style.font->uvOffset }; vp[2].color = col;
				vp[3].pos = { q.x0,q.y1 }; vp[3].uv = { q.s0,q.t1 + style.font->uvOffset }; vp[3].color = col;
				
			}break;
			default: Assert(!"unhandled font type"); break;
		}
	}
	
	return vec2(4, 6) * text.count;
}

FORCE_INLINE void
MakeText(UIDrawCmd& drawCmd, cstring text, vec2 pos, color color, vec2 scale) {
	drawCmd.counts += MakeText(drawCmd.vertices, drawCmd.indices, drawCmd.counts, text, pos, color, scale);
	drawCmd.tex = style.font->tex;
	drawCmd.type = UIDrawType_Text;
}

FORCE_INLINE vec2
MakeText(Vertex2* putverts, u32* putindices, vec2 offsets, wcstring text, vec2 pos, color color, vec2 scale) {
	Assert(putverts && putindices);
	if (color.a == 0) return vec2::ZERO;

	vec2 sum;
	switch (style.font->type) {
		//// BDF (and NULL) font rendering ////
		case FontType_BDF: case FontType_NONE: {
			forI(text.count) {
				u32     col = color.rgba;
				Vertex2* vp = putverts + (u32)offsets.x + 4 * i;
				u32* ip = putindices + (u32)offsets.y + 6 * i;

				f32 w = style.font->max_width * scale.x;
				f32 h = style.font->max_height * scale.y;
				f32 dy = 1.f / (f32)style.font->count;

				f32 idx = f32(text[i] - 32);
				f32 topoff = idx * dy;
				f32 botoff = topoff + dy;

				ip[0] = offsets.x + 4 * i; ip[1] = offsets.x + 4 * i + 1; ip[2] = offsets.x + 4 * i + 2;
				ip[3] = offsets.x + 4 * i; ip[4] = offsets.x + 4 * i + 2; ip[5] = offsets.x + 4 * i + 3;
				vp[0].pos = { pos.x + 0,pos.y + 0 }; vp[0].uv = { 0,topoff + style.font->uvOffset }; vp[0].color = col;
				vp[1].pos = { pos.x + w,pos.y + 0 }; vp[1].uv = { 1,topoff + style.font->uvOffset }; vp[1].color = col;
				vp[2].pos = { pos.x + w,pos.y + h }; vp[2].uv = { 1,botoff + style.font->uvOffset }; vp[2].color = col;
				vp[3].pos = { pos.x + 0,pos.y + h }; vp[3].uv = { 0,botoff + style.font->uvOffset }; vp[3].color = col;

				pos.x += style.font->max_width * scale.x;

			}
		}break;
			//// TTF font rendering ////
		case FontType_TTF: {
			forI(text.count) {
				u32     col = color.rgba;
				Vertex2* vp = putverts + (u32)offsets.x + 4 * i;
				u32* ip = putindices + (u32)offsets.y + 6 * i;

				aligned_quad q = style.font->GetPackedQuad(text[i], &pos, scale);

				ip[0] = offsets.x + 4 * i; ip[1] = offsets.x + 4 * i + 1; ip[2] = offsets.x + 4 * i + 2;
				ip[3] = offsets.x + 4 * i; ip[4] = offsets.x + 4 * i + 2; ip[5] = offsets.x + 4 * i + 3;
				vp[0].pos = { q.x0,q.y0 }; vp[0].uv = { q.s0,q.t0 + style.font->uvOffset }; vp[0].color = col;
				vp[1].pos = { q.x1,q.y0 }; vp[1].uv = { q.s1,q.t0 + style.font->uvOffset }; vp[1].color = col;
				vp[2].pos = { q.x1,q.y1 }; vp[2].uv = { q.s1,q.t1 + style.font->uvOffset }; vp[2].color = col;
				vp[3].pos = { q.x0,q.y1 }; vp[3].uv = { q.s0,q.t1 + style.font->uvOffset }; vp[3].color = col;

			}break;
		default: Assert(!"unhandled font type"); break;
		}
	}

	return vec2(4, 6) * text.count;
}

FORCE_INLINE void
MakeText(UIDrawCmd& drawCmd, wcstring text, vec2 pos, color color, vec2 scale) {
	drawCmd.counts += MakeText(drawCmd.vertices, drawCmd.indices, drawCmd.counts, text, pos, color, scale);
	drawCmd.type = UIDrawType_Text;
}

FORCE_INLINE vec2 
MakeTexture(Vertex2* putverts, u32* putindices, vec2 offsets, Texture* texture, vec2 p0, vec2 p1, vec2 p2, vec2 p3, f32 alpha) {
	Assert(putverts && putindices);
	if (!alpha) return vec2::ZERO;


	u32     col = PackColorU32(255,255,255,255.f * alpha);
	Vertex2* vp = putverts + (u32)offsets.x;
	u32*     ip = putindices + (u32)offsets.y;

	ip[0] = offsets.x; ip[1] = offsets.x + 1; ip[2] = offsets.x + 2;
	ip[3] = offsets.x; ip[4] = offsets.x + 2; ip[5] = offsets.x + 3;
	vp[0].pos = p0; vp[0].uv = { 0,1 }; vp[0].color = col;
	vp[1].pos = p1; vp[1].uv = { 1,1 }; vp[1].color = col;
	vp[2].pos = p2; vp[2].uv = { 1,0 }; vp[2].color = col;
	vp[3].pos = p3; vp[3].uv = { 0,0 }; vp[3].color = col;
	
	return vec2(4, 6);
}

FORCE_INLINE void
MakeTexture(UIDrawCmd& drawCmd, Texture* texture, vec2 pos, vec2 size, f32 alpha) {
	drawCmd.counts += MakeTexture(drawCmd.vertices, drawCmd.indices, drawCmd.counts, texture, pos, pos + size.ySet(0), pos + size, pos + size.xSet(0), alpha);
	drawCmd.type = UIDrawType_Image;
	drawCmd.tex = texture;
}

//internal debug drawing functions
void DebugTriangle(vec2 p0, vec2 p1, vec2 p2, color col = Color_Red) {
	UIDrawCmd dc;
	MakeTriangle(dc, p0, p1, p2, 1, col);
	debugCmds.add(dc);
}

void DebugTriangleFilled(vec2 p0, vec2 p1, vec2 p2, color col = Color_Red) {
	UIDrawCmd dc;
	MakeFilledTriangle(dc, p0, p1, p2, col);
	debugCmds.add(dc);
}

void DebugRect(vec2 pos, vec2 size, color col = Color_Red) {
	UIDrawCmd dc;
	MakeRect(dc, pos, size, 1, col);
	debugCmds.add(dc);
}

void DebugRectFilled(vec2 pos, vec2 size, color col = Color_Red) {
	UIDrawCmd dc;
	MakeFilledRect(dc, pos, size, col);
	debugCmds.add(dc);
}

void DebugCircle(vec2 pos, f32 radius, color col = Color_Red) {
	UIDrawCmd dc;
	MakeCircle(dc, pos, radius, 20, 1, col);
	debugCmds.add(dc);
}

void DebugCircleFilled(vec2 pos, f32 radius, color col = Color_Red) {
	UIDrawCmd dc;
	MakeFilledCircle(dc, pos, radius, 20, col);
	debugCmds.add(dc);
}

void DebugLine(vec2 pos, vec2 pos2, color col = Color_Red) {
	UIDrawCmd dc;
	MakeLine(dc, pos, pos2, 1, col);
	debugCmds.add(dc);
}

void DebugText(vec2 pos, char* text, color col = Color_White) {
	UIDrawCmd dc;
	MakeText(dc, cstring{ text, strlen(text) }, pos, col, vec2::ONE);
	debugCmds.add(dc);
}


//@Primitive Items


void UI::Rect(vec2 pos, vec2 dimen, color color) {
	UIItem       item{ UIItemType_Abstract, curwin->cursor, style };
	UIDrawCmd drawCmd;
	MakeRect(drawCmd, vec2::ZERO, dimen, 1, color);
	AddDrawCmd(&item, drawCmd);
	
	item.position = pos;
	item.size = dimen;
	curwin->items[ui_state.layer].add(item);
}

void UI::RectFilled(vec2 pos, vec2 dimen, color color) {
	UIItem       item{ UIItemType_Abstract, curwin->cursor, style };
	UIDrawCmd drawCmd;
	MakeFilledRect(drawCmd, pos, dimen, color);
	AddDrawCmd(&item, drawCmd);
	
	item.position = pos;
	item.size = dimen;
	curwin->items[ui_state.layer].add(item);
}


//@Line


void UI::Line(vec2 start, vec2 end, f32 thickness, color color){
	UIItem       item{ UIItemType_Abstract, curwin->cursor, style };
	UIDrawCmd drawCmd;
	MakeLine(drawCmd, start, end, thickness, color);
	AddDrawCmd(&item, drawCmd);

	item.position = vec2::ZERO;// { Min(drawCmd.position.x, drawCmd.position2.x), Min(drawCmd.position.y, drawCmd.position2.y) };
	item.    size = Max(start, end) - item.position;
	item.drawCmds.add(drawCmd);
	curwin->items[ui_state.layer].add(item);
}


//@Circle


void UI::Circle(vec2 pos, f32 radius, f32 thickness, u32 subdivisions, color color) {
	UIItem       item{ UIItemType_Abstract, curwin->cursor, style };
	UIDrawCmd drawCmd;
	MakeCircle(drawCmd, pos, radius, subdivisions, thickness, color);
	AddDrawCmd(&item, drawCmd);
	
	item.position = pos - vec2::ONE * radius;
	item.size = vec2::ONE * radius * 2;
	
	item.drawCmds.add(drawCmd);
	curwin->items[ui_state.layer].add(item);
	
}

void UI::CircleFilled(vec2 pos, f32 radius, u32 subdivisions, color color) {
	UIItem       item{ UIItemType_Abstract, curwin->cursor, style };
	UIDrawCmd drawCmd;
	MakeFilledCircle(drawCmd, pos, radius, subdivisions, color);
	AddDrawCmd(&item, drawCmd);
	
	item.position = pos - vec2::ONE * radius;
	item.size = vec2::ONE * radius * 2;
	
	item.drawCmds.add(drawCmd);
	curwin->items[ui_state.layer].add(item);
}


/////////////////////
////		   	 ////
////    Items    ////
////			 ////
/////////////////////


//@Text


//internal function for actually making and adding the drawCmd
local void TextCall(char* text, vec2 pos, color color, UIItem* item) {
	UIDrawCmd drawCmd;
	MakeText(drawCmd, cstring{ text, strlen(text) }, pos, color, GetTextScale());
	AddDrawCmd(item, drawCmd);
}

//secondary, for unicode
local void TextCall(wchar* text, vec2 pos, color color, UIItem* item) {
	UIDrawCmd drawCmd;
	MakeText(drawCmd, wcstring{ text, wcslen(text) }, pos, color, GetTextScale());
	AddDrawCmd(item, drawCmd);
}

//main function for wrapping, where position is starting position of text relative to the top left of the window
//this function also decides if text is to be wrapped or not, and if not simply calls TextEx (to clean up the Text() functions)
local void TextW(const char* in, vec2 pos, color color, b32 nowrap, b32 move_cursor = true) {
	
	using namespace UI;
	UIItem* item = BeginItem(UIItemType_Text);
	item->position = pos;
	
	if (!nowrap) {
		string text = in;
		
		//we split string by newlines and put them into here 
		//maybe make this into its own function
		array<string> newlined;
		
		u32 newline = text.findFirstChar('\n');
		if (newline != string::npos && newline != text.count - 1) {
			string remainder = text.substr(newline + 1);
			newlined.add(text.substr(0, newline - 1));
			newline = remainder.findFirstChar('\n');
			while (newline != string::npos) {
				if (!newline) {
					newlined.add("");
					remainder.erase(0);
					newline = remainder.findFirstChar('\n');
				}
				else {
					newlined.add(remainder.substr(0, newline - 1));
					remainder = remainder.substr(newline + 1);
					newline = remainder.findFirstChar('\n');
				}
			}
			newlined.add(remainder);
		}
		else {
			newlined.add(text);
		}
		vec2 workcur = vec2{ 0,0 };
		
		//TODO make this differenciate between monospace/non-monospace when i eventually add that to Font	
		switch (style.font->type) {
			
			case FontType_TTF: {
				Font* font = style.font;
				
				f32 wscale = style.fontHeight / font->aspect_ratio / font->max_width;
				f32 maxw = MarginedRight() - item->position.x;
				f32 currlinew = 0;
				
				for (string& t : newlined) {
					for (int i = 0; i < t.count; i++) {
						currlinew += font->GetPackedChar(t[i])->xadvance * wscale;
						
						if (currlinew >= maxw) {
							
							//find closest space to split by, if none we just split the word
							u32 lastspc = t.findLastChar(' ', i);
							string nustr = t.substr(0, (lastspc == string::npos) ? i - 1 : lastspc);
							TextCall(nustr.str, workcur, color, item);
							
							if (nustr.count == t.count) continue;
							
							t = t.substr(nustr.count);
							workcur.y += style.fontHeight + style.itemSpacing.y;
							item->size.x = Max(item->size.x, currlinew);

							i = 0;
							currlinew = 0;
						}
					}
					//place last bit of text that didn't need wrapped
					if (currlinew) {
						TextCall(t.str, workcur, color, item);
						workcur.y += style.fontHeight + style.itemSpacing.y;
					}
					
				}
			}break;
			
			case FontType_BDF: {
				//max characters we can place 
				u32 maxChars = u32(floor(MarginedRight() - item->position.x) / style.font->max_width);
				
				//make sure max chars never equals 0
				if (!maxChars) maxChars++;
				
				//wrap each string in newline array
				for (string& t : newlined) {
					//we need to see if the string goes beyond the width of the window and wrap if it does
					if (maxChars < t.count) {
						//if this is true we know item's total width is just maxChars times font width
						item->size.x = Max(item->size.x, maxChars * (f32)style.font->max_width);
						
						//find closest space to split by
						u32 splitat = t.findLastChar(' ', maxChars);
						string nustr = t.substr(0, (splitat == string::npos) ? maxChars - 1 : splitat);
						TextCall(nustr.str, workcur, color, item);
						
						if (nustr.count == t.count) continue;
						
						t = t.substr(nustr.count);
						workcur.y += style.fontHeight + style.itemSpacing.y;
						
						//continue to wrap if we need to
						while (t.count > maxChars) {
							splitat = t.findLastChar(' ', maxChars);
							nustr = t.substr(0, (splitat == string::npos) ? maxChars - 1 : splitat);
							TextCall(nustr.str, workcur, color, item);
							
							if (nustr.count == t.count) break;
							
							t = t.substr(nustr.count);
							workcur.y += style.fontHeight + style.itemSpacing.y;
							
							if (!strlen(t.str)) break;
						}
						//write last bit of text
						TextCall(t.str, workcur, color, item);
						workcur.y += style.fontHeight + style.itemSpacing.y;
					}
					else {
						TextCall(t.str, workcur, color, item);
						workcur.y += style.fontHeight + style.itemSpacing.y;
						item->size.x = Max(item->size.x, t.count * (f32)style.font->max_width);
					}
				}
				
				
				
			}break;
			default:Assert(!"unknown font type?");
		}
		
		if (NextItemSize.x != -1)
			item->size = NextItemSize;
		//else CalcItemSize(item);
		
		item->size.y = workcur.y;
		
		NextItemSize = vec2{ -1, 0 };
	}
	else {
		//TODO(sushi) make NoWrap also check for newlines
		
		if (NextItemSize.x != -1) item->size = NextItemSize;
		else                      item->size = UI::CalcTextSize(in);
		
		NextItemSize = vec2{ -1, 0 };
		
		TextCall((char*)in, vec2{ 0,0 }, style.colors[UIStyleCol_Text], item);
	}
	
	AdvanceCursor(item, move_cursor);
}

//second function for wrapping, using unicode
//these can probably be merged into one but i dont feel like doing that rn
local void TextW(const wchar* in, vec2 pos, color color, b32 nowrap, b32 move_cursor = true) {

	using namespace UI;
	UIItem* item = BeginItem(UIItemType_Text);
	item->position = pos;

	if (!nowrap) {
		wstring text = in;

		//we split wstring by newlines and put them into here 
		//maybe make this into its own function
		array<wstring> newlined;

		u32 newline = text.findFirstChar('\n');
		if (newline != string::npos && newline != text.count - 1) {
			wstring remainder = text.substr(newline + 1);
			newlined.add(text.substr(0, newline - 1));
			newline = remainder.findFirstChar('\n');
			while (newline != string::npos) {
				if (!newline) {
					newlined.add(L"");
					remainder.erase(0);
					newline = remainder.findFirstChar('\n');
				}
				else {
					newlined.add(remainder.substr(0, newline - 1));
					remainder = remainder.substr(newline + 1);
					newline = remainder.findFirstChar('\n');
				}
			}
			newlined.add(remainder);
		}
		else {
			newlined.add(text);
		}
		vec2 workcur = vec2{ 0,0 };

		//TODO make this differenciate between monospace/non-monospace when i eventually add that to Font	
		switch (style.font->type) {

			case FontType_TTF: {
				Font* font = style.font;

				f32 wscale = style.fontHeight / font->aspect_ratio / font->max_width;
				f32 maxw = MarginedRight() - item->position.x;
				f32 currlinew = 0;

				for (wstring& t : newlined) {
					for (int i = 0; i < t.count; i++) {
						currlinew += font->GetPackedChar(t[i])->xadvance * wscale;

						if (currlinew >= maxw) {

							//find closest space to split by, if none we just split the word
							u32 lastspc = t.findLastChar(' ', i);
							wstring nustr = t.substr(0, (lastspc == string::npos) ? i - 1 : lastspc);
							TextCall(nustr.str, workcur, color, item);

							if (nustr.count == t.count) continue;

							t = t.substr(nustr.count);
							workcur.y += style.fontHeight + style.itemSpacing.y;
							item->size.x = Max(item->size.x, currlinew);

							i = 0;
							currlinew = 0;
						}
					}
					//place last bit of text that didn't need wrapped
					if (currlinew) {
						TextCall(t.str, workcur, color, item);
						workcur.y += style.fontHeight + style.itemSpacing.y;
					}

				}
			}break;

			case FontType_BDF: {
				//max characters we can place 
				u32 maxChars = u32(floor(MarginedRight() - item->position.x) / style.font->max_width);

				//make sure max chars never equals 0
				if (!maxChars) maxChars++;

				//wrap each wstring in newline array
				for (wstring& t : newlined) {
					//we need to see if the wstring goes beyond the width of the window and wrap if it does
					if (maxChars < t.count) {
						//if this is true we know item's total width is just maxChars times font width
						item->size.x = Max(item->size.x, maxChars * (f32)style.font->max_width);

						//find closest space to split by
						u32 splitat = t.findLastChar(' ', maxChars);
						wstring nustr = t.substr(0, (splitat == string::npos) ? maxChars - 1 : splitat);
						TextCall(nustr.str, workcur, color, item);

						if (nustr.count == t.count) continue;

						t = t.substr(nustr.count);
						workcur.y += style.fontHeight + style.itemSpacing.y;

						//continue to wrap if we need to
						while (t.count > maxChars) {
							splitat = t.findLastChar(' ', maxChars);
							nustr = t.substr(0, (splitat == string::npos) ? maxChars - 1 : splitat);
							TextCall(nustr.str, workcur, color, item);

							if (nustr.count == t.count) break;

							t = t.substr(nustr.count);
							workcur.y += style.fontHeight + style.itemSpacing.y;

							if (!wcslen(t.str)) break;
						}
						//write last bit of text
						TextCall(t.str, workcur, color, item);
						workcur.y += style.fontHeight + style.itemSpacing.y;
					}
					else {
						TextCall(t.str, workcur, color, item);
						workcur.y += style.fontHeight + style.itemSpacing.y;
						item->size.x = Max(item->size.x, t.count * (f32)style.font->max_width);
					}
				}
			}break;
			default:Assert(!"unknown font type?");
		}

		if (NextItemSize.x != -1)
			item->size = NextItemSize;
		//else CalcItemSize(item);

		item->size.y = workcur.y;

		NextItemSize = vec2{ -1, 0 };
	}
	else {
		//TODO(sushi) make NoWrap also check for newlines

		if (NextItemSize.x != -1) item->size = NextItemSize;
		else                      item->size = UI::CalcTextSize(in);

		NextItemSize = vec2{ -1, 0 };

		TextCall((char*)in, vec2{ 0,0 }, style.colors[UIStyleCol_Text], item);
	}

	AdvanceCursor(item, move_cursor);
}

void UI::Text(const char* text, UITextFlags flags) {
	TextW(text, PositionForNewItem(), style.colors[UIStyleCol_Text], HasFlag(flags, UITextFlags_NoWrap));
}

void UI::Text(const char* text, vec2 pos, UITextFlags flags) {
	TextW(text, pos, style.colors[UIStyleCol_Text], HasFlag(flags, UITextFlags_NoWrap), 0);
}

void UI::Text(const wchar* text, UITextFlags flags){
	TextW(text, PositionForNewItem(), style.colors[UIStyleCol_Text], HasFlag(flags, UITextFlags_NoWrap));
}

void UI::Text(const wchar* text, vec2 pos, UITextFlags flags){
	TextW(text, pos, style.colors[UIStyleCol_Text], HasFlag(flags, UITextFlags_NoWrap), 0);
}

void UI::TextF(const char* fmt, ...) {
	string s;
	va_list argptr;
	va_start(argptr, fmt);
	s.count  = vsnprintf(nullptr, 0, fmt, argptr);
	s.str   = (char*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
	s.allocator->commit(s.str, s.count+1);
	s.space = s.count+1;
	vsnprintf(s.str, s.count+1, fmt, argptr);
	va_end(argptr);
	TextW(s.str, curwin->cursor, style.colors[UIStyleCol_Text], false);
}

//@Button

b32 UI::Button(const char* text, vec2 pos, UIButtonFlags flags) {
	UIItem* item = BeginItem(UIItemType_Button);
	item->position = pos;
	item->size = DecideItemSize(vec2(Min(MarginedRight() - item->position.x, Max(50.f, CalcTextSize(text).x * 1.1f)), style.fontHeight * style.buttonHeightRelToFont), item->position);
	AdvanceCursor(item);
	
	b32 active = WinHovered(curwin) && isItemHovered(item);

	{//background
		UIDrawCmd drawCmd;
		vec2  bgpos = vec2::ZERO;
		vec2  bgdim = item->size;
		color bgcol = style.colors[(active ? (DeshInput->LMouseDown() ? UIStyleCol_ButtonBgActive : UIStyleCol_ButtonBgHovered) : UIStyleCol_ButtonBg)];
		MakeFilledRect(drawCmd, bgpos, bgdim, bgcol);
		AddDrawCmd(item, drawCmd);
	}
	
	{//border
		UIDrawCmd drawCmd;
		vec2  borpos = vec2::ZERO;
		vec2  bordim = item->size;
		color borcol = style.colors[UIStyleCol_ButtonBorder];
		MakeRect(drawCmd, borpos, bordim, 1, borcol);
		AddDrawCmd(item, drawCmd);
	}
	
	{//text
		UIDrawCmd drawCmd;
		drawCmd.scissorOffset = vec2::ONE;
		drawCmd.scissorExtent = item->size - vec2::ONE;
		drawCmd.useWindowScissor = false;
		vec2 texpos =
			vec2((item->size.x - UI::CalcTextSize(text).x) * style.buttonTextAlign.x,
				(style.fontHeight * style.buttonHeightRelToFont - style.fontHeight) * style.buttonTextAlign.y);
		color texcol = style.colors[UIStyleCol_Text];
		MakeText(drawCmd, cstring{ (char*)text, strlen(text) }, texpos, texcol, GetTextScale());
		AddDrawCmd(item, drawCmd);
	}
	
	if (active) {
		//TODO(sushi) do this better
		if (HasFlag(flags, UIButtonFlags_ReturnTrueOnHold))
			if (DeshInput->LMouseDown()) { PreventInputs; return true; }
		else return false;
		if (HasFlag(flags, UIButtonFlags_ReturnTrueOnRelease)) {
			PreventInputs;
			if (DeshInput->LMouseReleased()) return true;
			else return false;
		}
		if (DeshInput->LMousePressed()){ 
			PreventInputs; 
			return true;
		}
	}
	return false;
}

b32 UI::Button(const char* text, UIButtonFlags flags) {
	return Button(text, PositionForNewItem(), flags);
}

//@Checkbox

void UI::Checkbox(string label, b32* b) {
	UIItem* item = BeginItem(UIItemType_Checkbox);
	
	vec2 boxpos = PositionForNewItem();
	vec2 boxsiz = vec2::ONE * style.checkboxHeightRelToFont * style.fontHeight;
	
	item->position = boxpos;
	item->size = boxsiz;
	item->size.x += style.itemSpacing.x + CalcTextSize(label).x;
	
	AdvanceCursor(item);
	
	u32 fillPadding = (u32)style.checkboxFillPadding;
	vec2 fillpos = boxsiz * vec2(fillPadding / boxsiz.x, fillPadding / boxsiz.y);
	vec2 fillsiz = boxsiz * (vec2::ONE - 2 * vec2(fillPadding / boxsiz.x, fillPadding / boxsiz.y));
	
	b32 bgactive = isItemActive(item);
	
	
	{//box
		UIDrawCmd drawCmd;
		vec2  position = vec2{ 0,0 };
		vec2  dimensions = boxsiz;
		color col = style.colors[(bgactive ? (DeshInput->LMouseDown() ? UIStyleCol_CheckboxBgActive : UIStyleCol_CheckboxBgHovered) : UIStyleCol_CheckboxBg)];
		MakeFilledRect(drawCmd, position, dimensions, col);
		AddDrawCmd(item, drawCmd);
	}
	
	//fill if true
	if (*b) {
		UIDrawCmd drawCmd;
		vec2  position = fillpos;
		vec2  dimensions = fillsiz;
		color col = style.colors[UIStyleCol_CheckboxFilling];
		MakeFilledRect(drawCmd, position, dimensions, col);
		AddDrawCmd(item, drawCmd);
	}
	
	{//border
		UIDrawCmd drawCmd;
		vec2  position = vec2::ZERO;
		vec2  dimensions = item->size;
		color col = style.colors[UIStyleCol_CheckboxBorder];
		MakeRect(drawCmd, position, dimensions, 1, col);
		AddDrawCmd(item, drawCmd);
	}
	
	{//label
		UIDrawCmd drawCmd;
		vec2  position = vec2(boxsiz.x + style.itemSpacing.x, (boxsiz.y - style.fontHeight) * 0.5);
		color col = style.colors[UIStyleCol_Text];
		MakeText(drawCmd, cstring{ label.str, label.count }, position, col, GetTextScale());
		AddDrawCmd(item, drawCmd);
	}
	
	if (DeshInput->LMousePressed() && bgactive) {
		*b = !*b;
		PreventInputs;
	}
}

//@Combo

//a combo is built by selectables called within its Begin/End
b32 UI::BeginCombo(const char* label, const char* prev_val, vec2 pos) {
	UIItem* item = BeginItem(UIItemType_Combo, 1);
	item->position = pos;
	item->size = DecideItemSize(CalcTextSize(prev_val) * 1.5, item->position);
	
	AdvanceCursor(item);
	
	if (!combos.has(label)) {
		combos.add(label);
		combos[label] = false;
	}
	
	b32& open = combos[label];
	
	b32 active = isItemActive(item);
	if (active && DeshInput->LMousePressed()) { 
		open = !open; 
		PreventInputs;
	}
	
	{//background
		UIDrawCmd drawCmd;
		vec2  position = vec2::ZERO;
		vec2  dimensions = item->size;
		color col = style.colors[(active ? (DeshInput->LMouseDown() ? UIStyleCol_SelectableBgActive : UIStyleCol_SelectableBgHovered) : UIStyleCol_SelectableBg)];
		MakeFilledRect(drawCmd, position, dimensions, col);
		AddDrawCmd(item, drawCmd);
	}
	
	{//text
		UIDrawCmd drawCmd;
		drawCmd.scissorOffset = vec2::ZERO;
		drawCmd.scissorExtent = item->size;
		drawCmd.useWindowScissor = false;
	    vec2 position =
			vec2((item->size.x - CalcTextSize(prev_val).x) * style.buttonTextAlign.x,
				 (style.fontHeight * style.buttonHeightRelToFont - style.fontHeight) * style.buttonTextAlign.y);
		color col = style.colors[UIStyleCol_Text];
		MakeText(drawCmd, cstring{ (char*)prev_val, strlen(prev_val) }, position, col, GetTextScale());
		AddDrawCmd(item, drawCmd);
	}
	
	
	//we also check if the combo's button is visible, if not we dont draw the popout
	if (open && item->position.y < curwin->height) {
		BeginPopOut(toStr("comboPopOut", label).str, item->position.yAdd(item->size.y), vec2::ZERO, UIWindowFlags_FitAllElements | UIWindowFlags_NoBorder);
		StateAddFlag(UISComboBegan);
		return true;
	}
	return false;
}

b32 UI::BeginCombo(const char* label, const char* prev_val) {
	return BeginCombo(label, prev_val, PositionForNewItem());
}



void UI::EndCombo() {
	Assert(StateHasFlag(UISComboBegan), "Attempted to end a combo without calling BeginCombo first, or EndCombo was called for a combo that was not open!");
	StateRemoveFlag(UISComboBegan);
	EndPopOut();
}

//@Selectable

b32 SelectableCall(const char* label, vec2 pos, b32 selected, b32 move_cursor = 1) {
	
	UIItem* item = BeginItem(UIItemType_Selectable, 0);
	item->position = pos;
	item->size = DecideItemSize(UI::CalcTextSize(label) * 1.5, item->position);
	
	b32 clicked = 0;
	
	AdvanceCursor(item, move_cursor);
	
	b32 active = isItemActive(item);//ItemCanTakeInput && Math::PointInRectangle(DeshInput->mousePos, curwin->position + item->position.yAdd(item->size.y * selectables_added), item->size);
	if (active && DeshInput->LMousePressed()) {
		clicked = true;
		PreventInputs;
	}
	
	{//background
		UIDrawCmd drawCmd;
		vec2  position = vec2::ZERO;
		vec2  dimensions = item->size;
		color col;
		if (selected) col = style.colors[(active && DeshInput->LMouseDown() ? UIStyleCol_SelectableBgActive : UIStyleCol_SelectableBgHovered)];
		else          col = style.colors[(active ? (DeshInput->LMouseDown() ? UIStyleCol_SelectableBgActive : UIStyleCol_SelectableBgHovered) : UIStyleCol_SelectableBg)];
		MakeFilledRect(drawCmd, position, dimensions, col);
		AddDrawCmd(item, drawCmd);
	}
	
	{//text
		UIDrawCmd drawCmd;
		vec2 position =
			vec2((item->size.x - UI::CalcTextSize(label).x) * style.buttonTextAlign.x,
				 (style.fontHeight * style.buttonHeightRelToFont - style.fontHeight) * style.buttonTextAlign.y);
		
		color col = style.colors[UIStyleCol_Text];
		MakeText(drawCmd, cstring{ (char*)label, strlen(label) }, position, col, GetTextScale());
		AddDrawCmd(item, drawCmd);
	}
	
	
	return clicked;
}

b32 UI::Selectable(const char* label, vec2 pos, b32 selected) {
	return SelectableCall(label, pos, selected, 0);
}

b32 UI::Selectable(const char* label, b32 selected) {
	return SelectableCall(label, PositionForNewItem(), selected);
}

//@Header

b32 UI::BeginHeader(const char* label) {
	UIItem* item = BeginItem(UIItemType_Header);
	
	b32* open = 0;
	if (!headers.has(label)) {
		headers.add(label);
		headers[label] = false;
	}
	open = &headers[label];
	
	item->position = PositionForNewItem();
	item->size = DecideItemSize(vec2(MAX_F32, style.fontHeight * style.headerHeightRelToFont), item->position);
	
	AdvanceCursor(item);
	
	b32 active = isItemActive(item);
	
	if (active && DeshInput->LMousePressed()) { 
		*open = !*open; 
		PreventInputs;
	}
	
	f32 buttonrad = item->size.y / 4;
	
	vec2 bgpos = vec2{ buttonrad * 2 + 5, 0 };
	vec2 bgdim = vec2{ 
		item->size.x - bgpos.x, 
		item->size.y };
	
	if (*open) indentStack.add(style.indentAmount + globalIndent);
	
	{//background
		UIDrawCmd drawCmd;
		vec2 position = bgpos;
		vec2 dimensions = bgdim;
		color col = style.colors[(active ? (DeshInput->LMouseDown() ? UIStyleCol_HeaderBgActive : UIStyleCol_HeaderBgHovered) : UIStyleCol_HeaderBg)];
		MakeFilledRect(drawCmd, position, dimensions, col);
		AddDrawCmd(item, drawCmd);
	}
	
	{//button
		UIDrawCmd drawCmd;
		vec2  position = vec2{ item->size.y / 4, item->size.y / 2 };
		f32   radius = item->size.y / 4;
		color col = style.colors[(active ? (DeshInput->LMouseDown() ? UIStyleCol_HeaderBgActive : UIStyleCol_HeaderBgHovered) : UIStyleCol_HeaderBg)];
		if (*open) MakeFilledCircle(drawCmd, position, radius, 30, col);
		else       MakeCircle(drawCmd, position, radius, 30, 1, col);
		AddDrawCmd(item, drawCmd);
	}
	
	{//text
		UIDrawCmd drawCmd;
		drawCmd.scissorOffset = vec2::ZERO;
		drawCmd.scissorExtent = item->size;
		drawCmd.useWindowScissor = false;
		vec2 position = 
			vec2(bgpos.x + (item->size.x - bgpos.x - style.windowPadding.x - UI::CalcTextSize(label).x) * style.headerTextAlign.x + 3,
				 ((style.fontHeight * style.headerHeightRelToFont - style.fontHeight) * style.headerTextAlign.y));
		color col = style.colors[UIStyleCol_Text];
		MakeText(drawCmd, cstring{ (char*)label, strlen(label) }, position, col, GetTextScale());
		AddDrawCmd(item, drawCmd);
	}
	
	{//border
		UIDrawCmd drawCmd;
		vec2  position = bgpos;
		vec2  dimensions = bgdim;
		color col = style.colors[UIStyleCol_HeaderBorder];
		MakeRect(drawCmd, position, dimensions, 1, col);
		AddDrawCmd(item, drawCmd);
	}
	
	
	return *open;
}

void UI::EndHeader() {
	indentStack.pop();
}


//@Slider


void UI::Slider(const char* label, f32* val, f32 val_min, f32 val_max, UISliderFlags flags){
	UIItem* item = BeginItem(UIItemType_Slider);
	
	b32 being_moved = 0;
	if (!sliders.has(label)) {
		sliders.add(label);
		sliders[label] = false;
	}
	else {
		being_moved = sliders[label];
	}
	
	item->position = PositionForNewItem();
	item->size = DecideItemSize(vec2{ curwin->width * M_ONETHIRD, 10 }, item->position);
	
	AdvanceCursor(item);
	
	b32 active = isItemActive(item);
	
	if (active && DeshInput->LMousePressed()) {
		sliders[label] = 1;
		PreventInputs;
	}
	if (being_moved && DeshInput->LMouseDown()) {
		f32 ratio = (DeshInput->mousePos.x - item->position.x - curwin->position.x) / item->size.x;
		*val = ratio * (val_max - val_min) + val_min;
	}
	if (DeshInput->LMouseReleased()) {
		sliders[label] = 0;
		AllowInputs;
	}
	
	*val = Clamp(*val, val_min, val_max);
	
	vec2 draggersiz = vec2{ item->size.x / 8, item->size.y };
	vec2 draggerpos = vec2{ (*val - val_min) / (val_max - val_min) * (item->size.x - draggersiz.x), 0 };
	
	{//background
		UIDrawCmd drawCmd;
		vec2  position = vec2::ZERO;
		vec2  dimensions = item->size;
		color col = style.colors[UIStyleCol_SliderBg];
		MakeFilledRect(drawCmd, position, dimensions, col);
		AddDrawCmd(item, drawCmd);
	}
	
	{//dragger
		UIDrawCmd drawCmd;
		vec2  position = draggerpos;
		vec2  dimensions = draggersiz;
		color col = style.colors[((active || being_moved) ? (DeshInput->LMouseDown() ? UIStyleCol_SliderBarActive : UIStyleCol_SliderBarHovered) : UIStyleCol_SliderBar)];
		MakeFilledRect(drawCmd, position, dimensions, col);
		AddDrawCmd(item, drawCmd);
	}
	
	{//border
		UIDrawCmd drawCmd;
		vec2  position = vec2::ZERO;
		vec2  dimensions = item->size;
		color col = style.colors[UIStyleCol_SliderBorder];
		MakeRect(drawCmd, position, dimensions, 1, col);
		AddDrawCmd(item, drawCmd);
	}
}

//@Image

void UI::Image(Texture* image, vec2 pos, f32 alpha, UIImageFlags flags) {
	UIItem* item = BeginItem(UIItemType_Image);
	
	item->position = pos;
	item->size = (NextItemSize.x == -1 ? vec2((f32)image->width, (f32)image->height) : NextItemSize);
	NextItemSize = vec2(-1, 1);
	
	AdvanceCursor(item);
	
	//TODO(sushi) image borders
	{//image
		UIDrawCmd drawCmd;
		vec2 position = vec2::ZERO;
		vec2 dimensions = item->size;
		MakeTexture(drawCmd, image, position, dimensions, alpha);
		AddDrawCmd(item, drawCmd);
	}
	
}

void UI::Image(Texture* image, f32 alpha, UIImageFlags flags) {
	Image(image, PositionForNewItem(), alpha, flags);
}


void UI::Separator(f32 height) {
	UIItem* item = BeginItem(UIItemType_Separator);
	item->position = PositionForNewItem();
	item->size = vec2(MarginedRight() - item->position.x, height);
	
	AdvanceCursor(item);
	
	UIDrawCmd drawCmd;
	vec2 start = vec2(0, height / 2);
	vec2 end = vec2(item->size.x, height / 2);
	color col = style.colors[UIStyleCol_Separator];
	MakeLine(drawCmd, start, end, 1, col);
	AddDrawCmd(item, drawCmd);
	
}


//@InputText


//final input text
b32 InputTextCall(const char* label, char* buff, u32 buffSize, vec2 position, const char* preview, UIInputTextCallback callback, UIInputTextFlags flags, b32 moveCursor) {
	UIItem* item = BeginItem(UIItemType_InputText);
	
	UIInputTextState* state;
	
	upt charCount = strlen(buff);
	
	item->position = position;
	
	vec2 dim;
	if (flags & UIInputTextFlags_FitSizeToText) {
		dim = UI::CalcTextSize(string(buff));
	}
	else {
		dim = DecideItemSize(vec2(Clamp(100.f, 0.f, Clamp(curwin->width - 2.f * style.windowPadding.x, 1.f, FLT_MAX)), style.inputTextHeightRelToFont * style.fontHeight), item->position);
	}


	
	item->size = dim;
	
	b32 hovered = isItemActive(item);
	
	AdvanceCursor(item, moveCursor);
	
	if (!(state = inputTexts.at(label))) {
		state = inputTexts.atIdx(inputTexts.add(label));
		state->cursor = charCount;
		state->id = hash<string>{}(label);
		state->selectStart = 0;
		state->selectEnd = 0;
		state->cursorBlinkTime = 5;
	}
	else {
		state->callback = callback;
	}
	
	b32 active = CanTakeInput && (activeId == state->id);
	if (NextActive || DeshInput->KeyPressed(MouseButton::LEFT)) {
		if (NextActive || hovered) {
			activeId = state->id;
			StateRemoveFlag(UISNextItemActive);
		}
		else if (active) activeId = -1;
	}
	
	if (hovered) SetWindowCursor(CursorType_IBeam);
	
	if (charCount < state->cursor)
		state->cursor = charCount;
	
	//data for callback function
	UIInputTextCallbackData data;
	data.flags = flags;
	data.buffer = buff;
	data.selectionStart = state->selectStart;
	data.selectionEnd = state->selectEnd;
	
	b32 bufferChanged = 0;
	if (active) {
		if (DeshInput->KeyPressed(Key::RIGHT) && state->cursor < charCount) state->cursor++;
		if (DeshInput->KeyPressed(Key::LEFT) && state->cursor > 0) state->cursor--;
		
		data.cursorPos = state->cursor;
		
		//check if the user used up/down keys
		if (DeshInput->KeyPressed(Key::UP) && (flags & UIInputTextFlags_CallbackUpDown)) {
			data.eventFlag = UIInputTextFlags_CallbackUpDown;
			data.eventKey = Key::UP;
			callback(&data);
		}
		if (DeshInput->KeyPressed(Key::DOWN) && (flags & UIInputTextFlags_CallbackUpDown)) {
			data.eventFlag = UIInputTextFlags_CallbackUpDown;
			data.eventKey = Key::DOWN;
			callback(&data);
		}
		
		//gather text into buffer from inputs
		//make this only loop when a key has been pressed eventually
		
		persist TIMER_START(hold);
		persist TIMER_START(throttle);
		
		//TODO(sushi) make this not count modifier keys
		if (DeshInput->AnyKeyPressed()) {
			TIMER_RESET(hold);
		}
		
		auto insert = [&](char c, u32 idx) {
			memmove(buff + idx + 1, buff + idx, (buffSize - idx) * CHAR_SIZE);
			buff[idx] = c;
		};
		
		auto erase = [&](u32 idx) {
			if (charCount == 1) memset(buff, 0, buffSize);
			else                memmove(buff + idx, buff + idx + 1, (--charCount) * CHAR_SIZE);
			
		};
		
		char charPlaced;
		auto placeKey = [&](u32 i, u32 ins, char toPlace) {
			//TODO(sushi) handle Numerical flag better
			if (i >= Key::A && i <= Key::Z && !HasFlag(flags, UIInputTextFlags_Numerical)) {
				if (DeshInput->capsLock || DeshInput->ShiftDown())
					insert(toPlace, ins);
				else
					insert(toPlace + 32, ins);
			}
			else if (i >= Key::K0 && i <= Key::K9) {
				if (DeshInput->ShiftDown() && !HasFlag(flags, UIInputTextFlags_Numerical)) {
					switch (i) {
						case Key::K0: data.character = ')'; insert(')', ins); break;
						case Key::K1: data.character = '!'; insert('!', ins); break;
						case Key::K2: data.character = '@'; insert('@', ins); break;
						case Key::K3: data.character = '#'; insert('#', ins); break;
						case Key::K4: data.character = '$'; insert('$', ins); break;
						case Key::K5: data.character = '%'; insert('%', ins); break;
						case Key::K6: data.character = '^'; insert('^', ins); break;
						case Key::K7: data.character = '&'; insert('&', ins); break;
						case Key::K8: data.character = '*'; insert('*', ins); break;
						case Key::K9: data.character = '('; insert('(', ins); break;
					}
				}
				else {
					data.character = KeyStringsLiteral[i];
					insert(KeyStringsLiteral[i], ins);
				}
			}
			else {
				if (DeshInput->ShiftDown() && !HasFlag(flags, UIInputTextFlags_Numerical)) {
					switch (i) {
						case Key::SEMICOLON:  data.character = ':';  insert(':', ins);  break;
						case Key::APOSTROPHE: data.character = '"';  insert('"', ins);  break;
						case Key::LBRACKET:   data.character = '{';  insert('{', ins);  break;
						case Key::RBRACKET:   data.character = '}';  insert('}', ins);  break;
						case Key::BACKSLASH:  data.character = '\\'; insert('\\', ins); break;
						case Key::COMMA:      data.character = '<';  insert('<', ins);  break;
						case Key::PERIOD:     data.character = '>';  insert('>', ins);  break;
						case Key::SLASH:      data.character = '?';  insert('?', ins);  break;
						case Key::MINUS:      data.character = '_';  insert('_', ins);  break;
						case Key::EQUALS:     data.character = '+';  insert('+', ins);  break;
						case Key::TILDE:      data.character = '~';  insert('~', ins);  break;
					}
				}
				else {
					if (HasFlag(flags, UIInputTextFlags_Numerical) && KeyStringsLiteral[i] == '.') {
						data.character = '.';
						insert('.', ins);
					}
					else if(!HasFlag(flags, UIInputTextFlags_Numerical)) {
						data.character = KeyStringsLiteral[i];
						insert(KeyStringsLiteral[i], ins);
					}
				}
			}
			TIMER_RESET(state->timeSinceTyped);
			if (flags & UIInputTextFlags_CallbackAlways) {
				data.eventFlag = UIInputTextFlags_CallbackAlways;
				callback(&data);
			}
		};
		
		if (DeshInput->anyKeyDown) {
			if (TIMER_END(hold) < 1000) {
				if (DeshInput->KeyPressed(Key::BACKSPACE) && charCount > 0 && state->cursor != 0) {
					erase(--state->cursor);
					bufferChanged = 1;
				}
				else {
					for (int i = 0; i < Key::Key_COUNT; i++) {
						char toPlace = KeyStringsLiteral[i];
						if (DeshInput->KeyPressed(i) && charCount < buffSize && toPlace != '\0') {
							u32 ins = state->cursor++;
							placeKey(i, ins, toPlace);
							bufferChanged = 1;
							break;
						}
					}
				}
			}
			else {
				if (TIMER_END(throttle) > 50) {
					if (DeshInput->KeyDown(Key::BACKSPACE) && charCount > 0 && state->cursor != 0) {
						erase(--state->cursor);
						bufferChanged = 1;
					}
					else {
						for (int i = 0; i < Key::Key_COUNT; i++) {
							char toPlace = KeyStringsLiteral[i];
							if (DeshInput->KeyDown(i) && charCount < buffSize && toPlace != '\0') {
								u32 ins = state->cursor++;
								placeKey(i, ins, toPlace);
								bufferChanged = 1;
								break;
							}
						}
					}
					TIMER_RESET(throttle);
				}
			}
		}
	}
	
	if (!(flags & UIInputTextFlags_NoBackground)) {//text box
		UIDrawCmd drawCmd;
		vec2  position = vec2::ZERO;
		vec2  dimensions = dim;
		color col = style.colors[(!active ? (hovered ? UIStyleCol_InputTextBgHovered : UIStyleCol_InputTextBg) : UIStyleCol_InputTextBg)];
		MakeFilledRect(drawCmd, position, dimensions, col);
		AddDrawCmd(item, drawCmd);
	}
	
	vec2 textStart =
		vec2((dim.x - charCount * style.font->max_width) * style.inputTextTextAlign.x + 3, //TODO(sushi) make an input text offset style var
			 (style.fontHeight * style.inputTextHeightRelToFont - style.fontHeight) * style.inputTextTextAlign.y);
	
	{//text
		UIDrawCmd drawCmd;
		vec2 position = textStart;
		if (preview && !buff[0]) {
			color col = style.colors[UIStyleCol_Text];
			col.a = (u8)floor(0.5f * 255);
			MakeText(drawCmd, cstring{ (char*)preview, strlen(preview) }, position, col, GetTextScale());
			
		}
		else {
			color col = style.colors[UIStyleCol_Text];
			MakeText(drawCmd, cstring{ (char*)buff, strlen(buff) }, position, col, GetTextScale());
		}
		
		AddDrawCmd(item, drawCmd);
	}
	
	{//border
		UIDrawCmd drawCmd;
		vec2  position = vec2::ZERO;
		vec2  dimensions = item->size;
		color col = style.colors[UIStyleCol_InputTextBorder];
		MakeRect(drawCmd, position, dimensions, 1, col);
		AddDrawCmd(item, drawCmd); 
	}
	
	//TODO(sushi, Ui) impl different text cursors
	if (active) {//cursor
		UIDrawCmd drawCmd;
		vec2  start = textStart + vec2(state->cursor * style.font->max_width * style.fontHeight / style.font->aspect_ratio / style.font->max_width, 0);
		vec2  end = textStart + vec2(state->cursor * style.font->max_width * style.fontHeight / style.font->aspect_ratio / style.font->max_width, style.fontHeight - 1);
		color col = 
			color(255, 255, 255, 
				  u8(255 * (cos(M_2PI / (state->cursorBlinkTime / 2) * TIMER_END(state->timeSinceTyped) / 1000 
								- sin(M_2PI / (state->cursorBlinkTime / 2) * TIMER_END(state->timeSinceTyped) / 1000)) + 1) / 2));
		MakeLine(drawCmd, start, end, 1, col);
		AddDrawCmd(item, drawCmd);
	}
	
	if (flags & UIInputTextFlags_EnterReturnsTrue && DeshInput->KeyPressed(Key::ENTER) || DeshInput->KeyPressed(Key::NUMPADENTER)) {
		return true;
	}
	else if (flags & UIInputTextFlags_AnyChangeReturnsTrue && bufferChanged) {
		return true;
	}
	
	return false;
}

b32 UI::InputText(const char* label, char* buffer, u32 buffSize, const char* preview, UIInputTextFlags flags) {
	return InputTextCall(label, buffer, buffSize, PositionForNewItem(), preview, nullptr, flags, 1);
}

b32 UI::InputText(const char* label, char* buffer, u32 buffSize,  UIInputTextCallback callback, const char* preview, UIInputTextFlags flags) {
	return InputTextCall(label, buffer, buffSize, PositionForNewItem(), preview, callback, flags, 1);
}

b32 UI::InputText(const char* label, char* buffer, u32 buffSize,  UIInputTextState*& getInputTextState, const char* preview, UIInputTextFlags flags) {
	if (InputTextCall(label, buffer, buffSize, PositionForNewItem(), preview, nullptr, flags, 1)) {
		getInputTextState = inputTexts.at(label);
		return true;
	}
	getInputTextState = inputTexts.at(label);
	return false;
}

b32 UI::InputText(const char* label, char* buffer, u32 buffSize, vec2 pos, const char* preview, UIInputTextFlags flags) {
	return InputTextCall(label, buffer, buffSize, pos, preview, nullptr, flags, 0);
}

b32 UI::InputText(const char* label, char* buffer, u32 buffSize, vec2 pos, UIInputTextCallback callback, const char* preview, UIInputTextFlags flags) {
	return InputTextCall(label, buffer, buffSize, pos, preview, callback, flags, 0);
}

b32 UI::InputText(const char* label, char* buffer, u32 buffSize, vec2 pos, UIInputTextState*& getInputTextState, const char* preview, UIInputTextFlags flags) {
	if (InputTextCall(label, buffer, buffSize, pos, preview, nullptr, flags, 0)) {
		getInputTextState = inputTexts.at(label);
		return true;
	}
	getInputTextState = inputTexts.at(label);
	return false;
}

//this doesnt need to be an enum at this point,
//but im making it one incase this function becomes more complicated in the future
enum CustomItemStage_{
	CISNone = 0,
	CISItemBegan = 1 << 0,
	CISItemAdvancedCursor = 1 << 1,
}; typedef u32 CustomItemStage;
CustomItemStage cistage = CISNone;

//@BeginCustomItem
UIItem* UI::BeginCustomItem(u32 layeroffset) {
	Assert(!cistage, "attempt to start a custom item");
	AddFlag(cistage, CISItemBegan);
	return BeginItem(UIItemType_Custom, layeroffset);
}

void UI::CustomItemAdvanceCursor(UIItem* item, b32 move_cursor) {
	Assert(HasFlag(cistage, CISItemBegan), "attempt to advance a custom item who has not begun!");
	AddFlag(cistage, CISItemAdvancedCursor);
	AdvanceCursor(item, move_cursor);
	
}


void UI::EndCustomItem() {
	Assert(HasFlag(cistage, CISItemBegan), "attempt to end a custom item that hasnt been started");
	Assert(HasFlag(cistage, CISItemAdvancedCursor), "attempt to end a custom item who hasnt advanced the cursor yet");
	cistage = CISNone;
}


//@Utilities


//Push/Pop functions
void UI::PushColor(UIStyleCol idx, color color) {
	//save old color
	colorStack.add(ColorMod{ idx, style.colors[idx] });
	//change to new color
	style.colors[idx] = color;
}

void UI::PushVar(UIStyleVar idx, f32 nuStyle) {
	Assert(uiStyleVarTypes[idx].count == 1, "Attempt to use a f32 on a vec2 style variable!");
	f32* p = (f32*)((u8*)&style + uiStyleVarTypes[idx].offset);
	varStack.add(VarMod(idx, *p));
	*p = nuStyle;
}

void UI::PushVar(UIStyleVar idx, vec2 nuStyle) {
	Assert(uiStyleVarTypes[idx].count == 2, "Attempt to use a f32 on a vec2 style variable!");
	vec2* p = (vec2*)((u8*)&style + uiStyleVarTypes[idx].offset);
	varStack.add(VarMod(idx, *p));
	*p = nuStyle;
}

void UI::PushFont(Font* font) { 
	fontStack.add(style.font);
	style.font = font;
}

void UI::PushScale(vec2 scale) {
	scaleStack.add(style.globalScale);
	style.globalScale = scale;
}

void UI::PushLayer(u32 layer) {
	Assert(layer < UI_WINDOW_ITEM_LAYERS, "last layer is currently reserved by UI, increase the amount of layers in ui.h if you need more");
	layerStack.add(ui_state.layer);
	ui_state.layer = layer;
}

void UI::PushWindowLayer(u32 layer) {
	
}

//we always leave the current color on top of the stack and the previous gets popped
void UI::PopColor(u32 count) {
	//Assert(count < colorStack.size() - 1, "Attempt to pop too many colors!");
	while (count-- > 0) {
		style.colors[(colorStack.last)->element] = colorStack.last->oldCol;
		colorStack.pop();
	}
}

void UI::PopVar(u32 count) {
	while (count-- > 0) {
		//TODO(sushi, UiCl) do this better
		UIStyleVarType type = uiStyleVarTypes[varStack.last->var];
		if (type.count == 1) {
			f32* p = (f32*)((u8*)&style + type.offset);
			*p = varStack.last->oldFloat[0];
			varStack.pop();
		}
		else {
			vec2* p = (vec2*)((u8*)&style + type.offset);
			*p = vec2(varStack.last->oldFloat[0], varStack.last->oldFloat[1]);
			varStack.pop();
		}
	}
}

void UI::PopFont(u32 count) {
	while (count-- > 0) {
		style.font = *fontStack.last;
		fontStack.pop();
	}
}

void UI::PopScale(u32 count) {
	while (count-- > 0) {
		style.globalScale = *scaleStack.last;
		scaleStack.pop();
	}
}

void UI::PopLayer(u32 count) {
	while (count-- > 0) {
		ui_state.layer = *layerStack.last;
		layerStack.pop();
	}
}


//@Windows



//window input helper funcs 

void SetFocusedWindow(UIWindow* window) {
	//we must find what idx the window is at
	//i think
	for (int i = 0; i < windows.count; i++) {
		if (*windows.atIdx(i) == window) {
			for (int move = i; move < windows.count - 1; move++)
				windows.swap(move, move + 1);
			break;
		}
	}
}

//function to recursively check child windows
b32 CheckForHoveredChildWindow(UIWindow* parent, UIWindow* child) {
	b32 child_hovered = 0;
	if (WinBegan(child)) {
		for (UIWindow* c : child->children) {
			child_hovered = CheckForHoveredChildWindow(child, c);
		}
		if (!child_hovered) {
			switch (child->type) {
				case UIWindowType_Child: {
					if (MouseInArea(child->visibleRegionStart, child->visibleRegionSize * style.globalScale)) {
						WinSetChildHovered(parent);
						WinSetHovered(child);
						child_hovered = true;
						hovered = child;
						if (!HasFlag(child->flags, UIWindowFlags_DontSetGlobalHoverFlag))
							StateAddFlag(UISGlobalHovered);
					}
					else {
						WinUnSetHovered(child);
					}
				}break;
				case UIWindowType_PopOut: {
					//in popouts case, we treat it like any other window
					if (MouseInArea(child->position, child->dimensions * style.globalScale)) {
						WinSetChildHovered(parent);
						WinSetHovered(child);
						child_hovered = true;
						hovered = child;
						
						if (!HasFlag(child->flags, UIWindowFlags_DontSetGlobalHoverFlag))
							StateAddFlag(UISGlobalHovered);
					}
					else {
						WinUnSetHovered(child);
					}
				}break;
			}
		}
	}
	return child_hovered;
}

void CheckForHoveredWindow(UIWindow* window = 0) {
	b32 hovered_found = 0;
	for (int i = windows.count - 1; i > 0; i--) {
		UIWindow* w = *windows.atIdx(i);
		if (WinBegan(w)) {
			//check children first, because there could be popout children who are out of the parents bounds
			for (UIWindow* c : w->children) {
				if (CheckForHoveredChildWindow(w, c)) {
					//WinUnSetHovered(w);
					//WinSetChildHovered(w);
					hovered_found = 1;
					break;
				}
			}
			if (!hovered_found && MouseInArea(w->position, w->dimensions * w->style.globalScale)) {
				WinSetHovered(w);
				WinUnSetChildHovered(w);
				if (!HasFlag(w->flags, UIWindowFlags_DontSetGlobalHoverFlag))
					StateAddFlag(UISGlobalHovered);
				hovered_found = 1;
				hovered = w;
				
			}
			else {
				WinUnSetHovered(w);
			}
		}
	}
}

//this function is checked in UI::Update, while the other 3 are checked per window
void CheckWindowsForFocusInputs() {
	//special case where we always check for metrics first since it draws last
	
	for (int i = windows.count - 1; i > 0; i--) {
		UIWindow* w = *windows.atIdx(i);
		WinUnSetFocused(w);
		if (!(w->flags & UIWindowFlags_NoFocus)) {
			if (i == windows.count - 1 && WinHovered(w)) {
				WinSetFocused(w);
				break;
			}
			else if ((WinHovered(w) || WinChildHovered(w)) && ((w->flags & UIWindowFlags_FocusOnHover) ? 1 : DeshInput->LMousePressed())) {
				WinUnSetFocused((*windows.data.last));
				WinSetFocused(w);
				for (int move = i; move < windows.count - 1; move++)
					windows.swap(move, move + 1);
				break;
			}
		}
	}
	
}

void CheckWindowForResizingInputs(UIWindow* window) {
	//check for edge resizing
	if (!(window->flags & UIWindowFlags_NoResize) && (CanTakeInput || WinResizing)) {
		vec2 mp = DeshInput->mousePos;
		
		b32 latch = WinLatched(window);
		static vec2 mouse, wdims, wpos;
		
		
		b32 mpres = DeshInput->LMousePressed();
		b32 mdown = DeshInput->LMouseDown();
		b32 mrele = DeshInput->LMouseReleased();
		
		//get which side is active
		WinActiveSide& activeSide = window->win_state.active_side;
		constexpr f32 boundrysize = 2;
		
		if (!mdown) {
			if(MouseInArea(window->position.yAdd(-boundrysize), vec2(window->width,boundrysize + style.windowBorderSize)))
				activeSide = wTop;
			else if (MouseInArea(window->position.yAdd(window->height - style.windowBorderSize), vec2(window->width, boundrysize + style.windowBorderSize)))
				activeSide = wBottom;
			else if (MouseInArea(window->position, vec2(boundrysize + style.windowBorderSize, window->height)))
				activeSide = wLeft;
			else if (MouseInArea(window->position.xAdd(window->width - style.windowBorderSize), vec2(boundrysize + style.windowBorderSize, window->height)))
				activeSide = wRight;
			else activeSide = wNone;
		}
		
		if (mpres && !latch && activeSide != wNone) {
			WinSetLatched(window);
			mouse = mp;
			wdims = window->dimensions;
			wpos = window->position;
			SetFocusedWindow(window);
			SetResizingInput;
		}
		
		if (mrele) {
			WinUnSetLatched(window);
			AllowInputs;
		}
		
		switch (activeSide) {
			case wTop: {
				SetWindowCursor(CursorType_VResize);
				if (mdown) {
					window->position.y = wpos.y + (mp.y - mouse.y);
					window->dimensions = wdims.yAdd(mouse.y - mp.y);
					window->scy = Clamp(window->scy, 0.f, window->maxScroll.y);
				}
			}break;
			case wBottom: {
				SetWindowCursor(CursorType_VResize); 
				if (mdown) {
					window->dimensions = wdims.yAdd(mp.y - mouse.y);
					window->scy = Clamp(window->scy, 0.f, window->maxScroll.y);
				}
			}break;
			case wLeft: {
				SetWindowCursor(CursorType_HResize);
				if (mdown) {
					window->position.x = wpos.x + (mp.x - mouse.x);
					window->dimensions = wdims.xAdd(mouse.x - mp.x);
					window->scx = Clamp(window->scx, 0.f, window->maxScroll.x);
				}
			}break;
			case wRight: {
				SetWindowCursor(CursorType_HResize); 
				if (mdown) {
					window->dimensions = wdims.xAdd(mp.x - mouse.x);
					window->scx = Clamp(window->scx, 0.f, window->maxScroll.x);
				}
			}break;
			case wNone:break;
		}
	}
}

//TODO(sushi) PLEASE clean this shit up
void CheckWindowForScrollingInputs(UIWindow* window, b32 fromChild = 0) {
	//always clamp scroll to make sure that it doesnt get stuck pass max scroll when stuff changes inside the window
	window->scx = Clamp(window->scx, 0.f, window->maxScroll.x);
	window->scy = Clamp(window->scy, 0.f, window->maxScroll.y);
	
	//mouse wheel inputs
	//if this is a child window and it cant scroll, redirect the scrolling inputs to the parent
	if (window->parent && WinHovered(window) && window->maxScroll.x == 0 && window->maxScroll.y == 0) {
		CheckWindowForScrollingInputs(window->parent, 1);
		return;
	}
	if (((WinHovered(window) && !WinChildHovered(window)) || fromChild) && DeshInput->ScrollUp()) {
		window->scy -= style.scrollAmount.y;
		window->scy = Clamp(window->scy, 0.f, window->maxScroll.y); // clamp y again to prevent user from seeing it not be clamped for a frame
		return;
	}
	if (((WinHovered(window) && !WinChildHovered(window)) || fromChild) && DeshInput->ScrollDown()) {
		window->scy += style.scrollAmount.y;
		window->scy = Clamp(window->scy, 0.f, window->maxScroll.y); // clamp y again to prevent user from seeing it not be clamped for a frame
		return;
	}
	
	if (CanTakeInput || WinScrolling) {
		static b32 vscroll = 0, hscroll = 0;
		static vec2 offset;
		static b32 initial = true;
		u32 flags = window->flags;
		
		vec2 winpos = window->position;
		f32 wpx = window->x;
		f32 wpy = window->y;
		
		vec2 windim = window->dimensions;
		f32 winw = window->width;
		f32 winh = window->height;
		
		vec2 winmin = window->minSizeForFit;
		
		f32 scrollBarYw = style.scrollBarYWidth;
		f32 scrollBarXh = style.scrollBarXHeight;
		
		b32 mdown = DeshInput->LMouseDown();
		b32 mrele = DeshInput->LMouseReleased();
		
		if (!hscroll && !HasFlag(flags, UIWindowFlags_NoScrollY)) {
			f32 scrollbarheight = ScrollBaredBottom(window) - ScrollBaredTop(window);
			f32 draggerheight = scrollbarheight * scrollbarheight / winmin.y;
			vec2 draggerpos(ScrollBaredRight(), (scrollbarheight - draggerheight) * window->scy / window->maxScroll.y + BorderedTop(window));
			
			b32 scbgactive = MouseInWinArea(vec2(ScrollBaredRight(window), BorderedTop(window)), vec2(style.scrollBarYWidth, scrollbarheight));
			b32 scdractive = MouseInWinArea(draggerpos, vec2(style.scrollBarYWidth, draggerheight));
			
			if (scdractive && DeshInput->LMouseDown() || !initial) {
				if (initial) {
					offset = draggerpos - DeshInput->mousePos;
					initial = false;
					SetFocusedWindow(window);
					SetScrollingInput;
					vscroll = 1;
				}
				
				draggerpos.y = DeshInput->mousePos.y + offset.y;
				draggerpos.y = Clamp(draggerpos.y, 0, scrollbarheight - draggerheight);
				
				window->scy = draggerpos.y * window->maxScroll.y / (scrollbarheight - draggerheight);
				window->scy = Clamp(window->scy, 0.f, window->maxScroll.y);
			}
			if (DeshInput->LMouseReleased()) {
				initial = true;
				vscroll = 0;
				inputupon = 0;
				WinUnSetBeingScrolled(window);
				AllowInputs;
			}
			
		}
		if (!vscroll && !HasFlag(flags, UIWindowFlags_NoScrollX)) {
			f32 scrollbarwidth = ScrollBaredRight(window) - ScrollBaredLeft(window);
			f32 draggerwidth = scrollbarwidth * window->dimensions.x / winmin.x;
			vec2 draggerpos((scrollbarwidth - draggerwidth) * window->scx / window->maxScroll.x, ScrollBaredBottom(window));
			
			b32 scbgactive = MouseInWinArea(vec2(ScrollBaredBottom(window), BorderedLeft(window)), vec2(scrollbarwidth, style.scrollBarXHeight));
			b32 scdractive = MouseInWinArea(draggerpos, vec2(draggerwidth, style.scrollBarXHeight));
			
			if (scdractive && DeshInput->LMouseDown() || !initial) {
				if (initial) {
					offset = draggerpos - DeshInput->mousePos;
					initial = false;
					SetFocusedWindow(window);
					hscroll = 1;
					SetScrollingInput;
				}
				
				draggerpos.x = DeshInput->mousePos.x + offset.x;
				draggerpos.x = Clamp(draggerpos.x, 0, scrollbarwidth - draggerwidth);
				
				window->scx = draggerpos.x * window->maxScroll.x / (scrollbarwidth - draggerwidth);
				window->scx = Clamp(window->scx, 0.f, window->maxScroll.x);
			}
			if (DeshInput->LMouseReleased()) {
				initial = true;
				hscroll = 0;
				WinUnSetBeingScrolled(window);
				AllowInputs;
			}
		}
	}
}

void CheckWindowForDragInputs(UIWindow* window, b32 fromChild = 0) {
	if (CanTakeInput || WinDragging) { //drag
		//if this is a child window check the uppermost parent instead
		if (window->parent && WinHovered(window)) { 
			CheckWindowForDragInputs(window->parent, 1); 
			return;
		}
		
		static vec2 mouseOffset = vec2(0, 0);
		
		if (
			!(window->flags & UIWindowFlags_NoMove) &&
			(WinHovered(window) || fromChild) &&
			DeshInput->KeyPressed(MouseButton::LEFT)) {
			SetDraggingInput;
			WinSetBeingDragged(window);
			mouseOffset = window->position - DeshInput->mousePos;
			SetFocusedWindow(window);
		}
		if (WinBeingDragged(window)) {
			window->position = DeshInput->mousePos + mouseOffset;
		}
		if (DeshInput->KeyReleased(MouseButton::LEFT)) {
			WinUnSetBeingDragged(window);
			AllowInputs;
		}
	}
}

//@Begin

//begins a window with a name, position, and dimensions along with some optional flags
//if begin window is called with a name that was already called before it will work with
//the data that window previously had
TIMER_START(wincreate);
void BeginCall(const char* name, vec2 pos, vec2 dimensions, UIWindowFlags flags, UIWindowType type) {
	Assert(type != UIWindowType_Normal || !StateHasFlag(UISRowBegan), "Attempted to begin a window with a Row in progress! (Did you forget to call EndRow()?");
	TIMER_RESET(wincreate);
	//save previous window on stack
	windowStack.add(curwin);
	
	switch (type) {
		case UIWindowType_Normal: { //////////////////////////////////////////////////////////////////////
			//check if were making a new window or working with one we already know
			if (windows.has(name)) {
				curwin = windows[name];
				curwin->cursor = vec2(0, 0);
				if (NextWinPos.x != -1) curwin->position = NextWinPos;
				if (NextWinSize.x != -1) curwin->dimensions = NextWinSize;
				NextWinPos = vec2(-1, 0); NextWinSize = vec2(-1, 0);
				curwin->flags = flags;
			}
			else {
				curwin = new UIWindow();
				
				curwin->scroll = vec2(0, 0);
				curwin->name = name;
				curwin->position = pos;
				curwin->dimensions = dimensions;
				curwin->cursor = vec2(0, 0);
				curwin->flags = flags;
				
				windows.add(name, curwin);
			}
			curwin->style = style;
			curwin->type = UIWindowType_Normal;
		}break;
		case UIWindowType_Child: { ///////////////////////////////////////////////////////////////////////
			UIWindow* parent = curwin;
			
			UIItem* item = BeginItem(UIItemType_Window);
			
			//TODO(sushi) add custom positioning for child windows
			item->position = PositionForNewItem();
			
			//check if were making a new child or working with one we already know
			if (parent->children.has(name)) {
				item->size = parent->children[name]->dimensions;
				if (NextWinSize.x != -1 || NextWinSize.y != 0) {
					if (NextWinSize.x == MAX_F32)
						item->size.x = MarginedRight() - item->position.x;
					else if (NextWinSize.x == -1) {}
					else item->size.x = NextWinSize.x;
					
					if (NextWinSize.y == MAX_F32)
						item->size.y = MarginedBottom() - item->position.x;
					else if (NextWinSize.y == -1) {}
					else item->size.y = NextWinSize.y;
				}
				
				AdvanceCursor(item);
				
				curwin = parent->children[name];
				curwin->dimensions = item->size;
				curwin->cursor = vec2(0, 0);
				if (NextWinPos.x != -1) { curwin->position = NextWinPos; }
				NextWinPos = vec2(-1, 0); NextWinSize = vec2(-1, 0);
			}
			else {
				item->size = dimensions;
				AdvanceCursor(item);
				
				vec2 parentNewPos = PositionForNewItem();
				curwin = new UIWindow();
				
				curwin->scroll = vec2(0, 0);
				curwin->name = name;
				curwin->position = parentNewPos;
				curwin->dimensions = dimensions;
				curwin->cursor = vec2(0, 0);
				curwin->flags = flags;
				curwin->layer = parent->layer;
				
				parent->children.add(name, curwin);
			}
			
			indentStack.add(0);
			item->child = curwin;
			curwin->parent = parent;
			curwin->type = UIWindowType_Child;
		}break;
		case UIWindowType_PopOut: { //////////////////////////////////////////////////////////////////////
			UIWindow* parent = curwin;
			UIItem* item = BeginItem(UIItemType_PopOutWindow);
			item->position = pos;
			
			if (parent->children.has(name)) {
				item->size = parent->children[name]->dimensions;
				if (NextWinSize.x != -1 || NextWinSize.y != 0) {
					if (NextWinSize.x == MAX_F32)
						item->size.x = MarginedRight() - item->position.x;
					else if (NextWinSize.x == -1) {}
					else item->size.x = NextWinSize.x;
					
					if (NextWinSize.y == MAX_F32)
						item->size.y = MarginedBottom() - item->position.x;
					else if (NextWinSize.y == -1) {}
					else item->size.y = NextWinSize.y;
				}
				
				NextWinPos = vec2(-1, 0); NextWinSize = vec2(-1, 0);
				
				AdvanceCursor(item, 0);
				
				curwin = parent->children[name];
				curwin->dimensions = item->size;
				curwin->cursor = vec2(0, 0);
				if (NextWinPos.x != -1) { curwin->position = NextWinPos; }
			}
			else {
				item->size = dimensions;
				AdvanceCursor(item, 0);
				
				vec2 parentNewPos = PositionForNewItem();
				curwin = new UIWindow();
				
				curwin->scroll = vec2(0, 0);
				curwin->name = name;
				curwin->position = parentNewPos;
				curwin->dimensions = dimensions;
				curwin->cursor = vec2(0, 0);
				curwin->flags = flags;
				curwin->layer = parent->layer + 1;
				
				parent->children.add(name, curwin);
			}
			
			indentStack.add(0);
			item->child = curwin;
			curwin->parent = parent;
			curwin->type = UIWindowType_PopOut;
		}break;
	}
	
	WinSetBegan(curwin);
	
}


void UI::Begin(const char* name, UIWindowFlags flags, UIWindowType type) {
	BeginCall(name, vec2::ONE * 100, vec2(150, 300), flags, type);
}

void UI::Begin(const char* name, vec2 pos, vec2 dimensions, UIWindowFlags flags, UIWindowType type) {
	BeginCall(name, pos, dimensions, flags, type);
}

void UI::BeginChild(const char* name, vec2 dimensions, UIWindowFlags flags) {
	BeginCall(name, PositionForNewItem(), dimensions, flags, UIWindowType_Child);
}

void UI::BeginChild(const char* name, vec2 pos, vec2 dimensions, UIWindowFlags flags) {
	BeginCall(name, pos, dimensions, flags, UIWindowType_Child);
}

void UI::BeginPopOut(const char* name, vec2 pos, vec2 dimensions, UIWindowFlags flags) {
	//ui_state.layer = Min(++ui_state.layer, (u32)UI_LAYERS);
	BeginCall(name, pos, dimensions, flags, UIWindowType_PopOut);
	//ui_state.layer--; //NOTE this will break if we are already on last layer fix it
}

//@CalcWindowMinSize

//calculates the minimum size a window can be to contain all drawn elements
//this would probably be better to be handled as we add items to the window
//instead of doing it at the end, so maybe make an addItem() that calculates this
//everytime we add one
vec2 CalcWindowMinSize() {
	vec2 max;
	forI(UI_WINDOW_ITEM_LAYERS) {
		for (UIItem& item : curwin->items[i]) {
			//if (item.type == UIItemType_Window && item.child->type != UIWindowType_PopOut){
			if (item.trackedForMinSize) {
				max.x = Max(max.x, (item.position.x + curwin->scx) + item.size.x);
				max.y = Max(max.y, (item.position.y + curwin->scy) + item.size.y);
			}
			//}
		}
	}
	return max + style.windowPadding + vec2::ONE * style.windowBorderSize;
}

//Old titlebar code for when i reimplement it as its own call
#if 0 


//draw title bar
if (!(curwin->flags & UIWindowFlags_NoTitleBar)) {
	{
		UIDrawCmd drawCmd; //inst 40
		drawCmd.type = UIDrawType_FilledRectangle;
		drawCmd.position = curwin->position;
		drawCmd.dimensions = vec2{ curwin->width, style.titleBarHeight };
		drawCmd.color = style.colors[UIStyleCol_TitleBg];
		
		base.drawCmds.add(drawCmd); //inst 44
	}
	
	{//draw text if it exists
		if (curwin->name.size) {
			UIDrawCmd drawCmd; //inst 46
			drawCmd.type = UIDrawType_Text;
			drawCmd.text = curwin->name; //inst 48
			drawCmd.position = vec2(curwin->x + (curwin->width - curwin->name.size * style.font->max_width) * style.titleTextAlign.x,
									curwin->y + (style.titleBarHeight - style.fontHeight) * style.titleTextAlign.y);
			drawCmd.color = Color_White;
			drawCmd.scissorExtent = vec2{ curwin->width, style.titleBarHeight };
			drawCmd.scissorOffset = curwin->position;
			drawCmd.useWindowScissor = false;
			drawCmd.font = style.font;
			
			//TODO(sushi, Ui) add title text coloring
			
			base.drawCmds.add(drawCmd); //inst 54
		}
	}
	
	{//draw titlebar minimize button and check for it being clicked
		if (!((curwin->flags & UIWindowFlags_NoMinimizeButton) || (curwin->flags & UIWindowFlags_NoMinimizeButton))) {
			UIDrawCmd drawCmd;
			drawCmd.position = vec2(
									curwin->x + (curwin->width - curwin->name.size * style.font->max_width) * 0.01,
									curwin->y + (style.titleBarHeight * 0.5 - 2));
			drawCmd.dimensions = vec2(10, 4);
			
			if (Math::PointInRectangle(mp, drawCmd.position, drawCmd.dimensions)) {
				drawCmd.color = style.colors[UIStyleCol_TitleBg] * 0.7;
				if (DeshInput->KeyPressed(MouseButton::LEFT)) {
					curwin->minimized = !curwin->minimized;
				}
			}
			else {
				drawCmd.color = style.colors[UIStyleCol_TitleBg] * 0.3;
			}
			
			curwin->baseDrawCmds.add(drawCmd); //inst 54
		}
	}
}
#endif

//@End

void EndCall() {
	Assert(windowStack.count, "Attempted to end the base window");
	
	UIItem* preitem = BeginItem(UIItemType_PreItems);
	UIItem* postitem = BeginItem(UIItemType_PostItems);
	
	preitem->position = vec2::ZERO;
	postitem->position = vec2::ZERO;
	
	preitem->size  = curwin->dimensions;
	postitem->size = curwin->dimensions;
	
	
	vec2 mp = DeshInput->mousePos;
	
	curwin->minSizeForFit = CalcWindowMinSize();
	vec2 minSizeForFit = curwin->minSizeForFit;
	
	if (WinHasFlag(UIWindowFlags_FitAllElements))
		curwin->dimensions = minSizeForFit;
	
	b32 xCanScroll = CanScrollX();
	b32 yCanScroll = CanScrollY();
	
	if (!inputupon) {
		CheckWindowForScrollingInputs(curwin);
		CheckWindowForResizingInputs(curwin);
		CheckWindowForDragInputs(curwin);
	}
	
	//check to see if the elements we have drawn so far have gone beyond the window's size
	//and allow scrolling if it did, as well as define a max scrolling amount
	//also draw the scroll bar if allowed
	//TODO(sushi) clean up this code, it really needs it. here and in the update function
	if (!WinHasFlag(UIWindowFlags_NoScrollY) && yCanScroll) {
		curwin->maxScroll.y = minSizeForFit.y - curwin->dimensions.y + (xCanScroll ? style.scrollBarXHeight : 0);
		if (!WinHasFlag(UIWindowFlags_NoScrollBarY)) {
			f32 scrollbarheight = ScrollBaredBottom() - ScrollBaredTop();
			f32 draggerheight = scrollbarheight * scrollbarheight / minSizeForFit.y;
			vec2 draggerpos(ScrollBaredRight(), (scrollbarheight - draggerheight) * curwin->scy / curwin->maxScroll.y + BorderedTop());
			
			b32 scbgactive = MouseInWinArea(vec2(ScrollBaredRight(), BorderedTop()), vec2(style.scrollBarYWidth, scrollbarheight));
			b32 scdractive = MouseInWinArea(draggerpos, vec2(style.scrollBarYWidth, draggerheight));
			
			{//scroll bg
				UIDrawCmd drawCmd;
				vec2  position = vec2(ScrollBaredRight(), BorderedTop());
				vec2  dimensions = vec2(style.scrollBarYWidth, scrollbarheight);
				color col = style.colors[UIStyleCol_ScrollBarBg]; //TODO(sushi) add active/hovered scrollbarbg colors
				MakeFilledRect(drawCmd, position, dimensions, col);
				AddDrawCmd(postitem, drawCmd);
			}
			
			{//scroll dragger
				UIDrawCmd drawCmd;
				vec2  position = draggerpos;
				vec2  dimensions = vec2(style.scrollBarYWidth, draggerheight);
				color col = style.colors[(scdractive ? ((DeshInput->LMouseDown()) ? UIStyleCol_ScrollBarDraggerActive : UIStyleCol_ScrollBarDraggerHovered) : UIStyleCol_ScrollBarDragger)];
				MakeFilledRect(drawCmd, position, dimensions, col);
				AddDrawCmd(postitem, drawCmd);
			}
			
			//if both scroll bars are active draw a little square to obscure the empty space 
			if (CanScrollX()) {
				UIDrawCmd drawCmd;
				vec2  position = vec2(ScrollBaredRight(), scrollbarheight);
				vec2  dimensions = vec2(style.scrollBarYWidth, style.scrollBarXHeight);
				color col = style.colors[UIStyleCol_WindowBg];
				MakeFilledRect(drawCmd, position, dimensions, col);
				AddDrawCmd(postitem, drawCmd);
			}
		}
	}
	else curwin->maxScroll.y = 0;
	
	
	//do the same but for x
	if (!WinHasFlag(UIWindowFlags_NoScrollX) && CanScrollX()) {
		curwin->maxScroll.x = minSizeForFit.x - curwin->dimensions.x + (yCanScroll ? style.scrollBarYWidth : 0);
		if (!WinHasFlag(UIWindowFlags_NoScrollBarX)) {
			f32 scrollbarwidth = ScrollBaredRight() - ScrollBaredLeft();
			f32 draggerwidth = scrollbarwidth * curwin->dimensions.x / minSizeForFit.x;
			vec2 draggerpos((scrollbarwidth - draggerwidth) * curwin->scx / curwin->maxScroll.x, ScrollBaredBottom());
			
			b32 scbgactive = MouseInWinArea(vec2(ScrollBaredBottom(), BorderedLeft()), vec2(scrollbarwidth, style.scrollBarXHeight));
			b32 scdractive = MouseInWinArea(draggerpos, vec2(draggerwidth, style.scrollBarXHeight));
			
			{//scroll bg
				UIDrawCmd drawCmd;
				vec2  position = vec2(0, ScrollBaredBottom());
				vec2  dimensions = vec2(scrollbarwidth, style.scrollBarXHeight);
				color col = style.colors[UIStyleCol_ScrollBarBg]; //TODO(sushi) add active/hovered scrollbarbg colors
				MakeFilledRect(drawCmd, position, dimensions, col);
				AddDrawCmd(postitem, drawCmd);
			}
			
			{//scroll dragger
				UIDrawCmd drawCmd;
				vec2  position = draggerpos;
				vec2  dimensions = vec2(draggerwidth, style.scrollBarXHeight);
				color col = style.colors[(scdractive ? ((DeshInput->LMouseDown()) ? UIStyleCol_ScrollBarDraggerActive : UIStyleCol_ScrollBarDraggerHovered) : UIStyleCol_ScrollBarDragger)];
				MakeFilledRect(drawCmd, position, dimensions, col);
				AddDrawCmd(postitem, drawCmd);
			}
		}
	}
	else curwin->maxScroll.x = 0;
	
	
	//if the window isn't invisible draw things that havent been disabled
	if (!WinHasFlags(UIWindowFlags_Invisible)) {
		//draw background
		if (!WinHasFlag(UIWindowFlags_NoBackground)) {
			UIDrawCmd drawCmd;
			vec2  position = vec2::ZERO;
			vec2  dimensions = curwin->dimensions;
			color col = style.colors[UIStyleCol_WindowBg];
			MakeFilledRect(drawCmd, position, dimensions, col);
			AddDrawCmd(preitem, drawCmd);
		}
		
		//draw border
		if (!WinHasFlag(UIWindowFlags_NoBorder)) {
			UIDrawCmd drawCmd;
			vec2  position = vec2::ONE * ceil(style.windowBorderSize / 2);
			vec2  dimensions = curwin->dimensions - vec2::ONE * ceil(style.windowBorderSize);
			color col = style.colors[UIStyleCol_Border];
			f32   thickness = style.windowBorderSize;
			MakeRect(drawCmd, position, dimensions, thickness, col);
			AddDrawCmd(postitem, drawCmd);
		}
	}
	
	NextWinPos = vec2(-1, 0); NextWinSize = vec2(-1, 0);
	curwin->style = style;
	
	curwin->creation_time = TIMER_END(wincreate);
	
	//update stored window with new window state
	curwin = *windowStack.last;
	windowStack.pop();
}

void UI::End() {
	Assert(!StateHasFlag(UISRowBegan), "Attempted to end a window with a Row in progress!");
	Assert(!StateHasFlag(UISComboBegan), "Attempted to end a window with a Combo in progress!");
	
	curwin->visibleRegionStart = curwin->position;
	curwin->visibleRegionSize = curwin->dimensions;
	
	EndCall();
}

void UI::EndChild() {
	UIWindow* parent = curwin->parent;
	vec2 scrollBarAdjust = vec2((CanScrollY(parent) ? style.scrollBarYWidth : 0), (CanScrollX(parent) ? style.scrollBarXHeight : 0));
	curwin->visibleRegionStart = Max(parent->visibleRegionStart, curwin->position);
	curwin->visibleRegionSize = ClampMin(Min(parent->visibleRegionStart + parent->visibleRegionSize - scrollBarAdjust, curwin->position + curwin->dimensions) - curwin->visibleRegionStart, vec2::ZERO);
	
	EndCall();
	indentStack.pop();	
}

void UI::EndPopOut() {
	curwin->visibleRegionStart = curwin->position;
	curwin->visibleRegionSize = curwin->dimensions;
	
	EndCall();
	indentStack.pop();
}

void UI::SetNextWindowPos(vec2 pos) {
	NextWinPos = pos;
}

void UI::SetNextWindowPos(f32 x, f32 y) {
	NextWinPos = vec2(x,y);
}

void UI::SetNextWindowSize(vec2 size) {
	NextWinSize = size.yAdd(style.titleBarHeight);
}

void UI::SetNextWindowSize(f32 x, f32 y) {
	NextWinSize = vec2(x, y);
}

b32 UI::IsWinHovered() {
	return WinHovered(curwin);
}

b32 UI::AnyWinHovered() {
	return StateHasFlag(UISGlobalHovered) || !CanTakeInput;
}

inline UIWindow* MetricsDebugItemFindHoveredWindow(UIWindow* window = 0) {
	if (window) {
		if (WinChildHovered(window)) {
			for (UIWindow* w : window->children) {
				return MetricsDebugItemFindHoveredWindow(w);
			}
		}
		else if (WinHovered(window)) return window;
	}
	else {
		UIWindow* found = 0;
		for (UIWindow* w : windows) {
			found = MetricsDebugItemFindHoveredWindow(w);
			if (found) break;
		}
		return found;
	}
}

inline void MetricsDebugItem() {
	using namespace UI;
	
	enum DebugItemState {
		None,
		InspectingWindowPreItems,
		InspectingWindowItems,
		InspectingWindowPostItems,
		InspectingItem
	};
	
	persist DebugItemState distate = None;
	
	persist UIItem    iteml;
	persist UIWindow* debugee = 0;
	persist vec2      mplatch;
	
	if (distate != None && distate != InspectingItem) {
		AllowInputs;
		Text("Press ESC to cancel");
		Text("A: Select Pre Items");
		Text("S: Select Items");
		Text("D: Select Post Items");
		
		if (DeshInput->KeyPressed(Key::A)) distate = InspectingWindowPreItems;
		if (DeshInput->KeyPressed(Key::S)) distate = InspectingWindowItems;
		if (DeshInput->KeyPressed(Key::D)) distate = InspectingWindowPostItems;
		
		if (DeshInput->KeyPressed(Key::ESCAPE)) distate = None;
	}
	
	auto check_item = [&](UIItem& item) {
		vec2 ipos = hovered->position + item.position;
		if (MouseInArea(ipos, item.size)) {
			
			DebugRect(ipos, item.size);
			PushVar(UIStyleVar_WindowPadding, vec2(3, 3));
			BeginPopOut("MetricsDebugItemPopOut", ipos.xAdd(item.size.x) - curwin->position, vec2::ZERO, UIWindowFlags_FitAllElements | UIWindowFlags_NoBorder | UIWindowFlags_NoInteract);
			
			Text(UIItemTypeStrs[item.type], UITextFlags_NoWrap);
			Text(toStr("DrawCmds: ", item.drawCmds.count).str, UITextFlags_NoWrap);
			
			EndPopOut();
			PopVar();
			
			if (DeshInput->LMousePressed()) {
				mplatch = ipos.xAdd(item.size.x);
				iteml = item;
				distate = InspectingItem;
				debugee = hovered;
			}
			return true;
		}
		return false;
	};
	
	switch (distate) {
		case None: {
			if (Button("Debug Item with Cursor") ||
				DeshInput->KeyPressed(Key::D) && DeshInput->LShiftDown() && DeshInput->LCtrlDown()) {
				distate = InspectingWindowItems;
			}
		}break;
		case InspectingWindowPreItems: {
			if (hovered) {
				for (UIItem& item : hovered->preItems) {
					DebugRect(item.position + hovered->position, item.size);
					check_item(item);
				}
			}
		}break;
		case InspectingWindowItems: {
			if (hovered) {
				b32 item_found = 0;
				forI(UI_WINDOW_ITEM_LAYERS) {
					for (UIItem& item : hovered->items[i]) {
						item_found = check_item(item);
					}
				}
				//if we are mousing over empty space in a child window, highlight the child window
				if (!item_found && hovered->parent) {
					DebugRect(hovered->position, hovered->dimensions);
					PushVar(UIStyleVar_WindowPadding, vec2(3, 3));
					BeginPopOut("MetricsDebugItemPopOut", hovered->position.xAdd(hovered->width) - curwin->position, vec2::ZERO, UIWindowFlags_FitAllElements | UIWindowFlags_NoBorder | UIWindowFlags_NoInteract);
					
					Text(toStr("Child Window ", hovered->name).str, UITextFlags_NoWrap);
					
					EndPopOut();
					PopVar();
				}
			}
		}break;
		case InspectingWindowPostItems: {
			if (hovered) {
				for (UIItem& item : hovered->postItems) {
					check_item(item);
				}
			}
		}break;
		case InspectingItem: {
			vec2 ipos = iteml.position + debugee->position;
			
			
			PushVar(UIStyleVar_WindowPadding, vec2(3, 3));
			//PushColor(UIStyleCol_WindowBg, color(50, 50, 50));
			BeginPopOut("MetricsDebugItemPopOut", mplatch - curwin->position, vec2::ZERO, UIWindowFlags_FitAllElements | UIWindowFlags_NoInteract);
			
			Text(UIItemTypeStrs[iteml.type], UITextFlags_NoWrap);
			Text(toStr("DrawCmds: ", iteml.drawCmds.count).str, UITextFlags_NoWrap);
			Text("Select to break on drawCmd", UITextFlags_NoWrap);
			
			PushColor(UIStyleCol_WindowBg, color(30, 30, 30));
			BeginChild("MetricsDebugItemPopOutDrawCmdChild", vec2(0,0), UIWindowFlags_NoBorder | UIWindowFlags_FitAllElements);
			
			BeginRow(3, style.buttonHeightRelToFont* style.fontHeight);
			RowSetupRelativeColumnWidths({ 1.1f,1.1f,1.1f });
			for (UIDrawCmd& dc : iteml.drawCmds) {
				Text(toStr(UIDrawTypeStrs[dc.type]).str, UITextFlags_NoWrap);
				
				if (MouseInArea(GetLastItemScreenPos(), GetLastItemSize())) {
					switch (dc.type) {
						//case UIDrawType_Image:
						//case UIDrawType_Rectangle:
						//case UIDrawType_FilledRectangle: {
						//	DebugRect(dc.position, dc.dimensions);
						//}break;
						//case UIDrawType_Circle:
						//case UIDrawType_FilledCircle: {
						//	DebugCircle(dc.position, dc.thickness);
						//}break;
						//case UIDrawType_Line: {
						//	vec2 up = Min(dc.position, dc.position2);
						//	DebugRect(up, Max(dc.position, dc.position2) - up);
						//}break;
						//case UIDrawType_Text: {
						//	DebugRect(dc.position, CalcTextSize(dc.text));
						//}break;
					}
				}
				
				WinSetHovered(curwin);
				if (Button("Creation")) {
					break_drawCmd_create_hash = dc.hash;
				}
				if (Button("Draw")) {
					break_drawCmd_draw_hash = dc.hash;
				}
				WinUnSetHovered(curwin);
			}
			EndRow();
			
			EndChild();
			EndPopOut();
			PopColor();
			PopVar();
		}break;
		
	}
	
	if (distate) PreventInputs;
	else AllowInputs;
}

inline b32 MetricsCheckWindowBreaks(UIWindow* window, b32 winbegin) {
	if (WinChildHovered(window)) {
		for (UIWindow* c : window->children) {
			if (MetricsCheckWindowBreaks(c, winbegin))
				return true;
		}
		
	}
	else if (WinHovered(window)) {
		DebugRect(window->position, window->dimensions);
		if (DeshInput->LMousePressed()) {
			if (winbegin) break_window_begin = window;
			else break_window_end = window;
			return true;
		}
	}
	return false;
}

inline void MetricsBreaking() {
	using namespace UI;
	
	enum BreakState {
		BreakNone,
		BreakItem,
		BreakDrawCmd,
		BreakWindowBegin,
		BreakWindowEnd,
	};
	
	static BreakState breakstate = BreakNone;
	static b32 frame_skip = 0;
	
	auto check_win_items = [&](UIWindow* win) {
		forI(UI_WINDOW_ITEM_LAYERS) {
			for (UIItem& item : win->items[i]) {
				if (MouseInArea(win->position + item.position, item.size)) {
					DebugRect(win->position + item.position, item.size);
					if (DeshInput->LMousePressed()) {
						break_window_item = win;
						item_idx = item.item_idx;
						item_layer = item.item_layer;
						breakstate = BreakNone;
						frame_skip = 0;
						break;
					}
				}
			}
		}
	};
	
	auto check_win_drawcmds = [&](UIWindow* win) {
		static u32 selected = -1;
		forI(UI_WINDOW_ITEM_LAYERS) {
			for (UIItem& item : win->items[i]) {
				if (MouseInArea(win->position + item.position, item.size)) {
					DebugRect(win->position + item.position, item.size);
					vec2 ipos = win->position + item.position;
					selected = Clamp(selected, 0, item.drawCmds.count);
					int o = 0;
					for (UIDrawCmd& dc : item.drawCmds) {
						switch (dc.type) {
							case UIDrawType_FilledRectangle:
							case UIDrawType_Rectangle: {
								//if (MouseInArea(ipos + dc.position, dc.dimensions)) {
								//	DebugRect(ipos + dc.position, dc.dimensions, (o == selected ? Color_Green : Color_Red));
								//	
								//}
							}break;
						}
						o++;
					}
				}
			}
		}
	};
	
	
	
	static u32 break_on_cursor = 0;
	
	if (breakstate) {
		Text("Press ESC to cancel");
		if (DeshInput->KeyPressed(Key::ESCAPE))
			breakstate = BreakNone;
	}
	
	switch (breakstate) {
		case BreakNone: {
			if (Button("Break Item on Cursor") ||
				DeshInput->KeyPressed(Key::B) && DeshInput->LShiftDown() && DeshInput->LCtrlDown()) {
				breakstate = BreakItem;
			}
			if (Button("Break DrawCmd on Cursor") ||
				DeshInput->KeyPressed(Key::D) && DeshInput->LShiftDown() && DeshInput->LCtrlDown()) {
				breakstate = BreakDrawCmd;
			}
			if (Button("Break on Window Begin")) {
				breakstate = BreakWindowBegin;
				
			}
			if (Button("Break on Window Begin")) {
				breakstate = BreakWindowEnd;
			}
		}break;
		case BreakItem: {
			for (UIWindow* w : windows) {
				if (WinChildHovered(w)) {
					for (UIWindow* c : w->children) {
						check_win_items(c);
					}
				}
				else if (WinHovered(w)) {
					check_win_items(w);
				}
			}
		}break;
		case BreakDrawCmd: {
			
		}break;
		case BreakWindowBegin: {
			for (UIWindow* w : windows) {
				MetricsCheckWindowBreaks(w, 1);
			}
		}break;
		case BreakWindowEnd: {
			for (UIWindow* w : windows) {
				MetricsCheckWindowBreaks(w, 0);
			}
		}break;
	}
	
	if (frame_skip && break_on_cursor) {
		for (UIWindow* w : windows) {
			if (WinChildHovered(w)) {
				for (UIWindow* c : w->children) {
					if (break_on_cursor == 1)
						check_win_items(c);
					else {
						check_win_drawcmds(c);
					}
				}
			}
			else if (WinHovered(w)) {
				if (break_on_cursor == 1)
					check_win_items(w);
				else {
					check_win_drawcmds(w);
				}
			}
		}
		Text("Press ESC to cancel");
		if (DeshInput->KeyPressed(Key::ESCAPE)) break_on_cursor = 0;
		PreventInputs;
	}
	if (break_on_cursor) {
		frame_skip = 1;
	}
}

UIWindow* DisplayMetrics() {
	using namespace UI;
	
	persist UIWindow* debugee = nullptr;
	
	UIWindow* myself = 0; //pointer returned for drawing
	
	persist UIWindow* slomo = *windows.atIdx(0);
	persist UIWindow* quick = *windows.atIdx(0);
	persist UIWindow* mostitems = *windows.atIdx(0);
	persist UIWindow* longname = *windows.atIdx(0);
	
	array<UIWindow*> winsorted;
	for (UIWindow* win : windows) {
		//if (!(win->name == "METRICS")) {
		if (win->render_time > slomo->render_time)     slomo = win;
		if (win->render_time < quick->render_time)     quick = win;
		if (win->items_count > mostitems->items_count) mostitems = win;
		if (win->name.count > longname->name.count)   longname = win;
		winsorted.add(win);
		//}
	}
	
	bubble_sort(winsorted, [](UIWindow* win1, UIWindow* win2) {return win1->name[0] > win2->name[0]; });
	
	Begin("METRICS", DeshWindow->dimensions - vec2(300,500), vec2(300, 500));
	myself = curwin;
	
	Text(toStr("Active Windows: ", windowStack.count).str);
	
	Separator(20);
	
	string slomotext = toStr("Slowest Render:");
	string quicktext = toStr("Fastest Render:");
	string mostitext = toStr("Most Items: "); 
	
	{
		persist f32 sw = CalcTextSize(longname->name).x;
		persist f32 fw = CalcTextSize(slomotext).x + 5;
		
		PushVar(UIStyleVar_RowItemAlign, vec2{ 0, 0.5 });
		BeginRow(3, 11);
		RowSetupColumnWidths({ fw, sw, 55 });
		
		Text(slomotext.str);
		Text(slomo->name.str);
		if (Button("select")) debugee = slomo;
		
		Text(quicktext.str);
		Text(quick->name.str);
		if (Button("select")) debugee = quick;
		
		Text(mostitext.str);
		Text(mostitems->name.str);
		if (Button("select")) debugee = mostitems;
		
		PopVar();
		EndRow();
	}
	
	Separator(20);
	
	if (BeginHeader("UI Globals")) {
		const char* str1 = "global hovered";
		f32 fw = CalcTextSize(str1).x * 1.2;
		
		BeginRow(2, style.fontHeight * 1.2);
		RowSetupColumnWidths({ fw, 96 });
		
		
		
		Text(str1); Text(toStr(StateHasFlag(UISGlobalHovered)).str);
		Text("input state: ");
		switch (ui_state.input) {
			case ISNone:          Text("None");           break;
			case ISScrolling:     Text("Scrolling");      break;
			case ISResizing:      Text("Resizing");       break;
			case ISDragging:      Text("Dragging");       break;
			case ISPreventInputs: Text("Prevent Inputs"); break;
		}
		Text("input upon: "); Text((inputupon ? inputupon->name.str : "none"));
		
		EndRow();
		
		EndHeader();
	}
	
	if (BeginHeader("Windows")) {
		persist b32 showChildren = 0;
		
		Checkbox("show children", &showChildren);
		
		BeginChild("METRICSWindows", vec2(MAX_F32, 300));
		
		for (UIWindow* window : windows) {
			Text(toStr(window->name, "; hovered: ", WinHovered(window)).str, UITextFlags_NoWrap);
			UIItem* tex = GetLastItem();
			SetNextItemSize(tex->size);
			PushLayer(GetCenterLayer() - 1);
			if (Selectable("", tex->position, window == debugee)) {
				debugee = window;
			}
			PopLayer();
			if (showChildren) {
				addGlobalIndent(13);
				for (UIWindow* child : window->children) {
					Text(toStr(child->name, "; hovered: ", WinHovered(child)).str, UITextFlags_NoWrap);
					tex = GetLastItem();
					
					SetNextItemSize(tex->size);
					PushLayer(GetCenterLayer() - 1);
					if (Selectable("", tex->position, child == debugee)) {
						debugee = child;
						
					}
					PopLayer();
				}
				popGlobalIndent;
			}
		}
		
		EndChild();
		
		
		EndHeader();
	}
	
	if (BeginHeader("Breaking")) {
		MetricsBreaking();
		EndHeader();
	}
	
	if (BeginHeader("Cursor Debugging")) {
		MetricsDebugItem();
		EndHeader();
	}
	
	Separator(20);
	
	PushVar(UIStyleVar_RowItemAlign, vec2(0, 0.5f));
	BeginRow(2, style.fontHeight * 1.5f);
	RowSetupRelativeColumnWidths({ 1.2f, 1 });
	
	persist u32 selected = 0;
	if (BeginCombo("METRICSwindows", "windows")) {
		for (int i = 0; i < winsorted.count; i++) {
			if (UI::Selectable(winsorted[i]->name.str, selected == i)) {
				selected = i;
				debugee = winsorted[i];
			}
		}
		EndCombo();
	}
	
	Text(toStr("Selected Window: ", (debugee ? debugee->name : "none")).str);
	
	EndRow();
	
	if (debugee) {
		
		if (Button("Set Focused")) {
			SetFocusedWindow(debugee);
		}
		
		if (BeginHeader("Window Vars")) {
			BeginRow(2, style.fontHeight * 1.2);
			RowSetupColumnWidths({ CalcTextSize("Max Item Width: ").x , 10 });
			
			Text("Render Time: ");    Text(toStr(debugee->render_time, "ms").str);
			Text("Creation Time: ");  Text(toStr(debugee->creation_time, "ms").str);
			Text("Item Count: ");     Text(toStr(debugee->items_count).str);
			Text("Position: ");       Text(toStr(debugee->position).str);
			Text("Dimensions: ");     Text(toStr(debugee->dimensions).str);
			Text("Scroll: ");         Text(toStr(debugee->scroll).str);
			Text("Max Scroll: ");     Text(toStr(debugee->maxScroll).str);
			Text("Hovered: ");        Text(toStr(WinHovered(debugee)).str);
			Text("Focused: ");        Text(toStr(WinFocused(debugee)).str);
			Text("Max Item Width: "); Text(toStr(MaxItemWidth(debugee)).str);
			
			
			EndRow();
			
			if (BeginHeader("Items")) {
				SetNextWindowSize(vec2(MAX_F32, 300));
				BeginChild("METRICSItems", vec2(0,0)); {
					BeginRow(2, style.buttonHeightRelToFont * style.fontHeight);
					RowSetupRelativeColumnWidths({ 1,1 });
					forI(UI_WINDOW_ITEM_LAYERS) {
						for (UIItem& item : debugee->items[i]) {
							Text(UIItemTypeStrs[item.type]);
							if (MouseInArea(GetLastItemScreenPos(), GetLastItemSize())) {
								DebugRect(debugee->position + item.position, item.size);
							}
							
							if (Button("break")) {
								break_window_item = debugee;
								item_idx = item.item_idx;
								item_layer = item.item_layer;
							}
							
						}
					}
					EndRow();
				}EndChild();
				EndHeader();
			}
			
			
			if (debugee->children.count && BeginHeader("Children")) {
				for(UIWindow* c : debugee->children) {
					if (Button(c->name.str)) {
						debugee = c;
					}
				}
				EndHeader();
			}
			
			EndHeader();
		}
		
		persist b32 showItemBoxes = false;
		persist b32 showItemCursors = false;
		persist b32 showAllDrawCmdScissors = false;
		persist b32 showDrawCmdTriangles = false;
		persist b32 showBorderArea = false;
		persist b32 showMarginArea = false;
		persist b32 showScrollBarArea = false;
		
		
		if (BeginHeader("Window Debug Visuals")) {
			Checkbox("Show Item Boxes", &showItemBoxes);
			Checkbox("Show Item Cursors", &showItemCursors);
			Checkbox("Show All DrawCmd Scissors", &showAllDrawCmdScissors);
			Checkbox("Show All DrawCmd Triangles", &showDrawCmdTriangles);
			Checkbox("Show Bordered Area", &showBorderArea);
			Checkbox("Show Margined Area", &showMarginArea);
			Checkbox("Show ScrollBared Area", &showScrollBarArea);
			
			
			EndHeader();
		}
		
		if (showItemBoxes) {
			forI(UI_WINDOW_ITEM_LAYERS) {
				for (UIItem& item : debugee->items[i]) {
					DebugRect(debugee->position + item.position, item.size);
				}
			}
		}
		
		if (showItemCursors) { 
			forI(UI_WINDOW_ITEM_LAYERS) {
				for (UIItem& item : debugee->items[i]) {
					{
						//UIDrawCmd dc;
						//dc.color = Color_Green;
						//dc.position = debugee->position + item.initialCurPos + item.style.windowPadding - vec2::ONE * 3 / 2.f;
						//dc.dimensions = vec2::ONE * 3;
						//debugCmds.add(dc);
					}
				}
			}
		}
		
		if (showAllDrawCmdScissors) {
			for (UIItem& item : debugee->preItems) {
				for (UIDrawCmd& dc : item.drawCmds) {
					DebugRect(dc.scissorOffset, dc.scissorExtent, Color_Red);
				}
			}
			forI(UI_WINDOW_ITEM_LAYERS) {
				for (UIItem& item : debugee->items[i]) {
					for (UIDrawCmd& dc : item.drawCmds) {
						DebugRect(dc.scissorOffset, dc.scissorExtent, Color_Yellow);
					}
				}
			}
			for (UIItem& item : debugee->postItems) {
				for (UIDrawCmd& dc : item.drawCmds) {
					DebugRect(dc.scissorOffset, dc.scissorExtent, Color_Green);
				}
			}
			
			for (UIWindow* c : debugee->children) {
				for (UIItem& item : c->preItems) {
					for (UIDrawCmd& dc : item.drawCmds) {
						DebugRect(dc.scissorOffset, dc.scissorExtent, Color_Red);
					}
				}
				forI(UI_WINDOW_ITEM_LAYERS) {
					for (UIItem& item : c->items[i]) {
						for (UIDrawCmd& dc : item.drawCmds) {
							DebugRect(dc.scissorOffset, dc.scissorExtent, Color_Yellow);
						}
					}
				}
				for (UIItem& item : c->postItems) {
					for (UIDrawCmd& dc : item.drawCmds) {
						DebugRect(dc.scissorOffset, dc.scissorExtent, Color_Green);
					}
				}
			}
			
		}

		if (showDrawCmdTriangles) {
			for (UIItem& item : debugee->preItems) {
				for (UIDrawCmd& dc : item.drawCmds) {
					for (int i = 0; i < dc.counts.y; i += 3) {
						DebugTriangle(
							dc.vertices[dc.indices[i]].pos,
							dc.vertices[dc.indices[i + 1]].pos,
							dc.vertices[dc.indices[i + 2]].pos, Color_Green);
					}
				}
			}
			forI(UI_WINDOW_ITEM_LAYERS) {
				for (UIItem& item : debugee->items[i]) {
					for (UIDrawCmd& dc : item.drawCmds) {
						for (int i = 0; i < dc.counts.y; i += 3) {
							DebugTriangle(
								dc.vertices[dc.indices[i]].pos,
								dc.vertices[dc.indices[i + 1]].pos,
								dc.vertices[dc.indices[i + 2]].pos);
						}
					}
				}
			}
			for (UIItem& item : debugee->postItems) {
				for (UIDrawCmd& dc : item.drawCmds) {
					for (int i = 0; i < dc.counts.y; i += 3) {
						DebugTriangle(
							dc.vertices[dc.indices[i]].pos,
							dc.vertices[dc.indices[i + 1]].pos,
							dc.vertices[dc.indices[i + 2]].pos, Color_Blue);
					}
				}
			}
		}
		
		if (showBorderArea) {
			auto b = BorderedArea(debugee);
			DebugRect(b.first + debugee->position, b.second);
		}
		if (showMarginArea) {
			auto m = MarginedArea(debugee);
			DebugRect(m.first + debugee->position, m.second);
		}
		if (showScrollBarArea) {
			auto s = ScrollBaredArea(debugee);
			DebugRect(s.first + debugee->position, s.second);
		}
		
		
	}
	
	
	
	PopVar();
	End();
	
	//PopColor(5);
	
	return myself;
	
}

//this just sets a flag to show the window at the very end of the frame, so we can gather all data
//about windows incase the user tries to call this before making all their windows
b32 show_metrics = 0;
void UI::ShowMetricsWindow() {
	show_metrics = 1;
}

void UI::DemoWindow() {
	Begin("deshiUIDEMO", vec2::ONE * 300, vec2::ONE * 300);
	
	if (BeginHeader("Text")) {
		Text("heres some text");
		
		Separator(7);
		
		Text("heres some long text that should wrap if it reaches the end of the window");
		
		Separator(7);
		
		Text("heres some long text that shouldn't wrap when it reaches the end of the window", UITextFlags_NoWrap);
		
		EndHeader();
	}
	
	Separator(11); /////////////////////////////////////////////////////////////////////////////////////////
	
	if (BeginHeader("Button")) {
		persist string str = "heres some text";
		if (Button("change text")) {
			str = "heres some new text, because you pressed the button";
		}
		Text(str.str);
		
		Separator(7);
		
		persist color col = color(55, 45, 66);
		PushColor(UIStyleCol_ButtonBg, col);
		PushColor(UIStyleCol_ButtonBgActive, col);
		PushColor(UIStyleCol_ButtonBgHovered, col);
		if (Button("true while held", UIButtonFlags_ReturnTrueOnHold)) {
			col.r += 2;
			col.g += 2;
			col.b += 2;
		}
		PopColor(3);
		
		Separator(7);
		
		persist string str2 = "this will change on button release";
		if (Button("true on release", UIButtonFlags_ReturnTrueOnRelease)) {
			str2 = "this was changed on release";
		}
		Text(str2.str);
		
		
		EndHeader();
	}
	
	Separator(11); /////////////////////////////////////////////////////////////////////////////////////////
	
	if (BeginHeader("Slider")) {
		persist f32 sl1 = 0;
		
		Slider("slider1", &sl1, 0, 100); SameLine(); Text(toStr(sl1).str);
		
		EndHeader();
	}
	
	Separator(11); /////////////////////////////////////////////////////////////////////////////////////////
	
	if (BeginHeader("Headers")) {
		Text(
			 "Headers automatically indent according to UIStyleVar_IndentAmount\n"
			 "They also automatically adjust their width to the width of the window\n"
			 "TODO(sushi) make a way to turn these off!"
			 );
		
		Separator(7);
		
		
		if (BeginHeader("Header 1")) {
			Text("some text in header 1");
			if (BeginHeader("Header 2")) {
				Text("another nested header");
				EndHeader();
			}
			EndHeader();
		}
		EndHeader();
	}
	
	Separator(11); /////////////////////////////////////////////////////////////////////////////////////////
	
	if (BeginHeader("Row")) {
		Text(
			 "UI::Row() aligns any other item in a row"
			 );
		
		Separator(7);
		
		BeginRow(3, 15);
		RowSetupRelativeColumnWidths({ 1,1,1 });
		Text("some text");
		Button("a button");
		Button("another button");
		EndRow();
		
		Separator(7);
		
		Text(
			 "Rows aren't restricted to one Row, you can add as many items as you like as long as the amount of items is divisible by the amount of columns you made the Row with"
			 );
		
		persist f32 rowyalign = 0.5;
		persist f32 rowxalign = 0.5;
		
		PushVar(UIStyleVar_RowItemAlign, vec2(rowxalign, rowyalign));
		
		Separator(7);
		
		BeginRow(2, 30);
		RowSetupColumnWidths({ 100, 100 });
		Text("example of"); Text("aligning text");
		Text("evenly over"); Text("multiple rows");
		EndRow();
		
		Separator(7);
		
		Text("you can change how items are aligned within row cells as well");
		Slider("slider1", &rowxalign, 0, 1); SameLine(); Text(toStr("x align ", rowxalign).str);
		Slider("slider2", &rowyalign, 0, 1); SameLine(); Text(toStr("y align ", rowyalign).str);
		
		PopVar();
		
		Separator(7);
		
		
		
		Text("Rows also allow you to use either persist or relative column widths");
		
		Separator(7);
		
		persist f32 scw1 = 60;
		persist f32 scw2 = 60;
		persist f32 scw3 = 60;
		
		persist f32 dcw1 = 1;
		persist f32 dcw2 = 1;
		persist f32 dcw3 = 1;
		
		persist u32 selected = 0;
		if (Selectable("Static Column Widths", !selected)) selected = 0;
		if (Selectable("Relative column widths", selected)) selected = 1;
		
		switch (selected) {
			case 0: {
				BeginRow(3, 16);
				RowSetupColumnWidths({ scw1, scw2, scw3 });
				Text("text");
				Text("long text");
				Text("text");
				EndRow();
				
				Slider("demo_scw1", &scw1, 0, 90); SameLine(); Text(toStr(scw1).str);
				Slider("demo_scw2", &scw2, 0, 90); SameLine(); Text(toStr(scw2).str);
				Slider("demo_scw3", &scw3, 0, 90); SameLine(); Text(toStr(scw3).str);
			}break;
			case 1: {
				BeginRow(3, 16);
				RowSetupRelativeColumnWidths({ dcw1, dcw2, dcw3 });
				Text("text");
				Text("long text");
				Text("text");
				EndRow();
				
				Slider("demo_dcw1", &dcw1, 1, 5); SameLine(); Text(toStr(dcw1).str);
				Slider("demo_dcw2", &dcw2, 1, 5); SameLine(); Text(toStr(dcw2).str);
				Slider("demo_dcw3", &dcw3, 1, 5); SameLine(); Text(toStr(dcw3).str);
			}break;
		}
		
		
		EndHeader();
	}
	
	Separator(11); /////////////////////////////////////////////////////////////////////////////////////////
	
	if (BeginHeader("Child Windows")) {
		Text("You can nest windows inside another one by using BeginChild");
		
		BeginChild("demochild", vec2(curwin->width - style.indentAmount, 300));
		
		Text("Heres some text in the child window");
		
		Separator(7);
		
		Text("Child windows have all the same functionality of base windows, save for a few TODOS");
		
		Separator(7);
		
		persist Texture* tex = Storage::CreateTextureFromFile("lcdpix.png").second;
		
		Text("heres a image in the child window:");
		Image(tex);
		
		
		
		forI(15) {
			Text("heres a bunch of text in the child window");
		}
		
		Button("child window button");
		
		EndChild();
		
		EndHeader();
		
	}
	
	Separator(11); /////////////////////////////////////////////////////////////////////////////////////////
	
	if (BeginHeader("Combos")) {
		Text("Combos are a huge TODO right now, but heres how they currently look");
		
		Separator(7);
		
		persist u32 selected = 0;
		if (BeginCombo("uiDemoCombo", "preview text")) {
			forI(10) {
				if (Selectable(toStr("selection", i).str, selected == i)) {
					selected = i;
				}
			}
			EndCombo();
		}
		
		EndHeader();
	}
	
	Separator(11); /////////////////////////////////////////////////////////////////////////////////////////
	
	if (BeginHeader("Style Variables")) {
		Text("Adjusting these will adjust the base style variables of UI");
		Separator(7);
		
		Text("Window Padding (vec2)");
		Slider("demo_wpx", &style.windowPadding.x, 0, 100);
		SameLine();
		Slider("demo_wpy", &style.windowPadding.y, 0, 100);
		
		Text("Item Spacing (vec2)");
		Slider("demo_isx", &style.itemSpacing.x, 0, 100);
		SameLine();
		Slider("demo_isy", &style.itemSpacing.y, 0, 100);
		
		EndHeader();
	}
	
	End();
}

//@Init
//initializes core UI with an invisible working window covering the entire screen
//also initializes styles
void UI::Init() {
	AssertDS(DS_MEMORY,  "Attempt to load UI without loading Memory first");
	AssertDS(DS_WINDOW,  "Attempt to load UI without loading Window first");
	AssertDS(DS_STORAGE, "Attempt to load UI without loading Storage first");
	deshiStage |= DS_UI;
	
	TIMER_START(t_s);
	
	
	
	curwin = new UIWindow();
	curwin->name = "Base";
	curwin->position = vec2(0,0);
	curwin->dimensions = DeshWindow->dimensions;
	
	//load font
	style.font = Storage::CreateFontFromFileBDF("gohufont-11.bdf").second;
	Assert(style.font != Storage::NullFont());
	
	//push default color scheme
	//this is never meant to be popped
	PushColor(UIStyleCol_Border,         Color_DarkGrey);
	PushColor(UIStyleCol_WindowBg,       color(14, 14, 14));
	PushColor(UIStyleCol_Text,           Color_White);
	PushColor(UIStyleCol_Separator,      color(64, 64, 64));
	
	//backgrounds
	PushColor(UIStyleCol_ScrollBarBg,  Color_VeryDarkCyan);
	PushColor(UIStyleCol_ButtonBg,     Color_VeryDarkCyan);
	PushColor(UIStyleCol_CheckboxBg,   Color_VeryDarkCyan);
	PushColor(UIStyleCol_HeaderBg,     color(0, 100, 100, 255));
	PushColor(UIStyleCol_SliderBg,     Color_VeryDarkCyan);
	PushColor(UIStyleCol_InputTextBg,  Color_DarkCyan);
	PushColor(UIStyleCol_SelectableBg, Color_VeryDarkCyan);
	
	//active backgrounds
	PushColor(UIStyleCol_ScrollBarBgActive,  Color_VeryDarkCyan);
	PushColor(UIStyleCol_ButtonBgActive,     Color_Cyan);
	PushColor(UIStyleCol_CheckboxBgActive,   Color_Cyan);
	PushColor(UIStyleCol_HeaderBgActive,     color(0, 255, 255, 255));
	PushColor(UIStyleCol_SliderBgActive,     Color_Cyan);
	PushColor(UIStyleCol_InputTextBgActive,  Color_DarkCyan);
	PushColor(UIStyleCol_SelectableBgActive, Color_Cyan);
	
	//hovered backgrounds
	PushColor(UIStyleCol_ScrollBarBgHovered,  Color_VeryDarkCyan);
	PushColor(UIStyleCol_ButtonBgHovered,     Color_DarkCyan);
	PushColor(UIStyleCol_CheckboxBgHovered,   Color_DarkCyan);
	PushColor(UIStyleCol_HeaderBgHovered,     color(0, 128, 128, 255));
	PushColor(UIStyleCol_SliderBgHovered,     Color_DarkCyan);
	PushColor(UIStyleCol_InputTextBgHovered,  Color_DarkCyan);
	PushColor(UIStyleCol_SelectableBgHovered, Color_DarkCyan);
	
	//borders
	PushColor(UIStyleCol_ButtonBorder,   Color_Black);
	PushColor(UIStyleCol_CheckboxBorder, Color_Black);
	PushColor(UIStyleCol_HeaderBorder,   Color_Black);
	PushColor(UIStyleCol_SliderBorder,   Color_Black);
	PushColor(UIStyleCol_InputTextBorder,Color_Black);
	
	//misc
	PushColor(UIStyleCol_ScrollBarDragger,        Color_VeryDarkRed);
	PushColor(UIStyleCol_ScrollBarDraggerActive,  Color_Red);
	PushColor(UIStyleCol_ScrollBarDraggerHovered, Color_DarkRed);
	
	PushColor(UIStyleCol_CheckboxFilling,     Color_DarkMagenta);
	
	PushColor(UIStyleCol_HeaderButton,        Color_VeryDarkRed);
	PushColor(UIStyleCol_HeaderButtonActive,  Color_Red);
	PushColor(UIStyleCol_HeaderButtonHovered, Color_DarkRed);
	
	PushColor(UIStyleCol_SliderBar,		   Color_VeryDarkRed);
	PushColor(UIStyleCol_SliderBarActive,  Color_Red);
	PushColor(UIStyleCol_SliderBarHovered, Color_DarkRed);
	
	//push default style variables
	PushVar(UIStyleVar_WindowPadding,            vec2(10, 10));
	PushVar(UIStyleVar_WindowBorderSize,         2);
	PushVar(UIStyleVar_TitleBarHeight,           style.fontHeight * 1.2f);
	PushVar(UIStyleVar_TitleTextAlign,           vec2(1, 0.5f));
	PushVar(UIStyleVar_ItemSpacing,              vec2(1, 1));
	PushVar(UIStyleVar_ScrollAmount,             vec2(10, 10));
	PushVar(UIStyleVar_CheckboxSize,             vec2(10, 10));
	PushVar(UIStyleVar_CheckboxFillPadding,      2);
	PushVar(UIStyleVar_InputTextTextAlign,       vec2(0.f, 0.5f));
	PushVar(UIStyleVar_ButtonTextAlign,          vec2(0.5f, 0.5f));
	PushVar(UIStyleVar_HeaderTextAlign,          vec2(0.f, 0.5f));
	PushVar(UIStyleVar_ButtonHeightRelToFont,    1.3f);
	PushVar(UIStyleVar_HeaderHeightRelToFont,    1.3f);
	PushVar(UIStyleVar_InputTextHeightRelToFont, 1.3f);
	PushVar(UIStyleVar_CheckboxHeightRelToFont,  1.3f);
	PushVar(UIStyleVar_RowItemAlign,             vec2(0.5f, 0.5f));
	PushVar(UIStyleVar_ScrollBarYWidth,          5);
	PushVar(UIStyleVar_ScrollBarXHeight,         5);
	PushVar(UIStyleVar_IndentAmount,             12);
	PushVar(UIStyleVar_FontHeight,               (f32)style.font->max_height);
	
	PushScale(vec2(1, 1));
	
	initColorStackSize = colorStack.count;
	initStyleStackSize = varStack.count;
	
	windows.add("base", curwin);
	//windowStack.add(curwin);
	
	LogS("deshi","Finished UI initialization in ",TIMER_END(t_s),"ms");
}

//in our final draw system, this is the function that primarily does the work
//of figuring out how each draw call will be sent to the renderer
inline void DrawItem(UIItem& item, UIWindow* window) {
	
	vec2 winpos = vec2(window->x, window->y);
	vec2 winsiz = vec2(window->width, window->height) * window->style.globalScale;
	vec2 winScissorOffset = window->visibleRegionStart;
	vec2 winScissorExtent = window->visibleRegionSize;
	
	UIWindow* parent = window->parent;
	
	vec2 itempos = window->position + item.position;
	vec2 itemsiz = item.size;

	UIDrawCmd* lastdc = 0;
	for (UIDrawCmd& drawCmd : item.drawCmds) {
		BreakOnDrawCmdDraw;
		
		forI(drawCmd.counts.x) drawCmd.vertices[i].pos = floor(drawCmd.vertices[i].pos+itempos);
		
		vec2 dcse = (drawCmd.useWindowScissor ? winScissorExtent : drawCmd.scissorExtent * item.style.globalScale);
		vec2 dcso = (drawCmd.useWindowScissor ? winScissorOffset : itempos + drawCmd.scissorOffset);

		//modify the scissor offset and extent according to the kind of window we are drawing
		switch (window->type) {
			case UIWindowType_PopOut:
			case UIWindowType_Normal: {
				dcso = Min(winpos + winScissorExtent - dcse, Max(winpos, dcso));
				dcse += Min(dcso - winpos, vec2::ZERO);
				if (drawCmd.useWindowScissor)
					dcse += Min(winpos, vec2::ZERO);
			}break;
			case UIWindowType_Child: {
				dcso = Min(winScissorOffset + winScissorExtent - dcse, Max(dcso, winScissorOffset));
				dcse += Min(dcso - winScissorOffset, vec2::ZERO);
				if(drawCmd.useWindowScissor)
					dcse += Min(winScissorOffset, vec2::ZERO);
				
			}break;
		}

		dcse = ClampMin(dcse, vec2::ZERO);
		dcso = ClampMin(dcso, vec2::ZERO);
		
		//compare current stuff to last draw cmd to determine if we need to start a new twodCmd
		if(!lastdc) 
			Render::StartNewTwodCmd(window->layer, drawCmd.tex, dcso, dcse);
		else if (dcse != lastdc->scissorExtent || dcso != lastdc->scissorOffset || drawCmd.tex != lastdc->tex)
			Render::StartNewTwodCmd(window->layer, drawCmd.tex, dcso, dcse);
		
		Render::AddTwodVertices(window->layer, drawCmd.vertices, drawCmd.counts.x, drawCmd.indices, drawCmd.counts.y);
		
		drawCmd.scissorExtent = dcse;
		drawCmd.scissorOffset = dcso;
		lastdc = &drawCmd;
	}
}

inline void DrawWindow(UIWindow* p, UIWindow* parent = 0) {
	TIMER_START(winren);
	
	if (WinHovered(p) && !(p->flags & UIWindowFlags_DontSetGlobalHoverFlag))
		StateAddFlag(UISGlobalHovered);
	
	//draw pre cmds first
	for (UIItem& item : p->preItems) {
		DrawItem(item, p);
	}
	
	
	forI(UI_WINDOW_ITEM_LAYERS) {
		for (UIItem& item : p->items[i]) {
			if (item.type == UIItemType_Window) {
				item.child->position = p->position + item.position * item.style.globalScale;
				DrawWindow(item.child, p);
				WinUnSetBegan(item.child);
				continue;
			}
			DrawItem(item, p);
		}
	}
	
	
	//draw post items, such as scroll bars or context menus
	for (UIItem& item : p->postItems) {
		DrawItem(item, p);
	}
	
	//after we draw everything to do with the base window we then draw its popouts
	for (UIItem& item : p->popOuts) {
		item.child->position = p->position + item.position * item.style.globalScale;
		DrawWindow(item.child, p);
		WinUnSetBegan(item.child);
	}
	
	p->render_time = TIMER_END(winren);
	
	//when compiling for debug we defer this to after the metrics window
#ifndef DESHI_INTERNAL
	p->preItems.clear();
	p->postItems.clear();
	forI(UI_WINDOW_ITEM_LAYERS) {
		p->items[i].clear();
	}
#else
	p->render_time = TIMER_END(winren);
	p->items_count = 0;
	forI(UI_WINDOW_ITEM_LAYERS) {
		p->items_count += p->items[i].count;
	}
#endif
	
	
	
}

void CleanUpWindow(UIWindow* window) {
	window->preItems.clear();
	window->postItems.clear();
	window->popOuts.clear();
	forI(UI_WINDOW_ITEM_LAYERS) {
		window->items[i].clear();
	}
	for (UIWindow* c : window->children) {
		CleanUpWindow(c);
	}
}

//for checking that certain things were taken care of eg, popping colors/styles/windows
void UI::Update() {

	//there should only be default stuff in the stacks
	Assert(!windowStack.count, 
		   "Frame ended with hanging windows in the stack, make sure you call End() if you call Begin()!");
	
	
	//TODO(sushi) impl this for other stacks
	if (varStack.count != initStyleStackSize) {
		PRINTLN("Frame ended with hanging vars in the stack, make sure you pop vars if you push them!\nVars were:\n");
		
		for (u32 i = varStack.count - 1; i > initStyleStackSize - 1; i--)
			PRINTLN(styleVarStr[varStack[i].var] << "\n");
		
		Assert(0);
	}
	
	Assert(colorStack.size() == initColorStackSize, 
		   "Frame ended with hanging colors in the stack, make sure you pop colors if you push them!");
	
	Assert(indentStack.count == 1, "Forgot to call End for an indenting Begin!");
	
	MarginPositionOffset = vec2::ZERO;
	MarginSizeOffset = vec2::ZERO;
	

	
	//windows input checking functions
	CheckWindowsForFocusInputs();
	CheckForHoveredWindow();
	
	
	if (inputupon) CheckWindowForScrollingInputs(inputupon);
	if (inputupon) CheckWindowForResizingInputs(inputupon);
	if (inputupon) CheckWindowForDragInputs(inputupon);
	
	
	//reset cursor to default if no item decided to set it 
	if (!StateHasFlag(UISCursorSet)) DeshWindow->SetCursor(CursorType_Arrow);
	
	//draw windows in order 
	for (UIWindow* p : windows) {
		DrawWindow(p);
		WinUnSetBegan(p);
	}

#ifdef DESHI_INTERNAL
	//clear break vars in debug mode
	break_window_item = 0;
	item_idx = -1;
	item_layer = -1;
	break_window_begin = 0;
	break_window_end = 0;
	break_drawCmd_create_hash = -1;
	break_drawCmd_draw_hash = -1;
#endif
	
	if (show_metrics) {
		//DisplayMetrics();
		DrawWindow(DisplayMetrics());
		show_metrics = 0;
	}
	
	
	//it should be safe to do this any time the mouse is released
	if (DeshInput->LMouseReleased()) { AllowInputs; }
	
	
	//we defer window item clearing to after the metrics window is drawn
	//in debug builds
#ifdef DESHI_INTERNAL
	for (UIWindow* p : windows) {
		CleanUpWindow(p);
	}
	
	
#endif
	
	//draw all debug commands if there are any
	for (UIDrawCmd& drawCmd : debugCmds) {
	
		Render::StartNewTwodCmd(5, drawCmd.tex, vec2::ZERO, DeshWinSize);
		Render::AddTwodVertices(5, drawCmd.vertices, drawCmd.counts.x, drawCmd.indices, drawCmd.counts.y);


		//static s32 start = 0;
		//if (DeshInput->KeyPressed(Key::I)) start += 3;
		//if (DeshInput->KeyPressed(Key::K)) start = Max(start - 3, s32(0));
		//
		//Render::StartNewTwodCmd(6, drawCmd.tex, vec2::ZERO, DeshWinSize);
		//Vertex2 vp[3];
		//u32 ip[3];
		//
		//vp[0].pos = drawCmd.vertices[drawCmd.indices[start]].pos;	vp[0].uv={0,0}; vp[0].color=Color_Magenta.rgba;
		//vp[1].pos = drawCmd.vertices[drawCmd.indices[start+1]].pos;	vp[1].uv={0,0}; vp[1].color=Color_Magenta.rgba;
		//vp[2].pos = drawCmd.vertices[drawCmd.indices[start+2]].pos; vp[2].uv={0,0}; vp[2].color=Color_Magenta.rgba; 
		//ip[0] = 0; ip[1] = 1; ip[2] = 2;
		//
		//Render::AddTwodVertices(6, vp, 3, ip, 3);


	}
	debugCmds.clear();
	
	//if (CanTakeInput && DeshInput->LMouseDown()) PreventInputs;
	
	hovered = 0;
	StateRemoveFlag(UISGlobalHovered);
	StateRemoveFlag(UISCursorSet);
}

void UI::DrawDebugRect(vec2 pos, vec2 size, color col)          { DebugRect(pos, size, col); }
void UI::DrawDebugRectFilled(vec2 pos, vec2 size, color col)    { DebugRectFilled(pos, size, col); }
void UI::DrawDebugCircle(vec2 pos, f32 radius, color col)       { DebugCircle(pos, radius, col); }
void UI::DrawDebugCircleFilled(vec2 pos, f32 radius, color col) { DebugCircleFilled(pos, radius, col); }
void UI::DrawDebugLine(vec2 pos1, vec2 pos2, color col)         { DebugLine(pos1, pos2, col); }

