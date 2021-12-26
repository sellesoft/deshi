#include "ui.h"
#include "../utils/array_sorting.h"
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

static constexpr u32 CHAR_SIZE = sizeof(CHAR);

static const u32 UI_CENTER_LAYER = floor(UI_LAYERS / 2.f);


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
	u32 currlayer = UI_CENTER_LAYER;

}ui_state;

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

#define WinAddBegan(win)         AddFlag(win->win_state.flags, UIWSBegan)
#define WinAddEnded(win)         AddFlag(win->win_state.flags, UIWSEnded)
#define WinAddHovered(win)       AddFlag(win->win_state.flags, UIWSHovered)
#define WinAddChildHovered(win)  AddFlag(win->win_state.flags, UIWSChildHovered)
#define WinAddFocused(win)       AddFlag(win->win_state.flags, UIWSFocused)
#define WinAddLatched(win)       AddFlag(win->win_state.flags, UIWSLatch)
#define WinAddBeingScrolled(win) AddFlag(win->win_state.flags, UIWSBeingScrolled)
#define WinAddBeingResized(win)  AddFlag(win->win_state.flags, UIWSBeingResized)
#define WinAddBeingDragged(win)  AddFlag(win->win_state.flags, UIWSBeingDragged)

#define WinRemoveBegan(win)            RemoveFlag(win->win_state.flags, UIWSBegan)
#define WinRemoveEnded(win)            RemoveFlag(win->win_state.flags, UIWSEnded)
#define WinRemoveHovered(win)          RemoveFlag(win->win_state.flags, UIWSHovered)
#define WinRemoveChildHovered(win)     RemoveFlag(win->win_state.flags, UIWSChildHovered)
#define WinRemoveFocused(win)       RemoveFlag(win->win_state.flags, UIWSFocused)
#define WinRemoveLatched(win)       RemoveFlag(win->win_state.flags, UIWSLatch)
#define WinRemoveBeingScrolled(win) RemoveFlag(win->win_state.flags, UIWSBeingScrolled)
#define WinRemoveBeingResized(win)  RemoveFlag(win->win_state.flags, UIWSBeingResized)
#define WinRemoveBeingDragged(win)  RemoveFlag(win->win_state.flags, UIWSBeingDragged)

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
#define SetScrollingInput ui_state.input = ISScrolling; inputupon = window; WinAddBeingScrolled(window);
#define WinResizing       ui_state.input == ISResizing
#define WinDragging       ui_state.input == ISDragging
#define WinScrolling      ui_state.input == ISScrolling

#define EndEvalDimensions vec2(-1,-1); //used in place of dimensions for a window when the window should evaluate the dimensions of itself in EndCall()

//for breaking on an item from the metrics window
UIWindow* break_window = 0;
u32 item_idx = -1;
u32 item_layer = -1;

#ifdef DESHI_INTERNAL
#define BreakOnItem if(break_window && break_window == curwin && curwin->items[item_layer].count == item_idx){ DebugBreakpoint;}
#else
#define BreakOnItem
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

//calculates the items position and size based on its draw commands
//should really only be used when doing this manually is too annoying
inline void CalcItemSize(UIItem* item) {
	using namespace UI;
	
	vec2 max;
	for (UIDrawCmd& drawCmd : item->drawCmds) {
		switch (drawCmd.type) {
			case UIDrawType_Rectangle:
			case UIDrawType_FilledRectangle: {
				max.x = Max(max.x, drawCmd.position.x + drawCmd.dimensions.x);
				max.y = Max(max.y, drawCmd.position.y + drawCmd.dimensions.y);
			}break;
			case UIDrawType_Line: {
				vec2 ulm{ Min(drawCmd.position.x, drawCmd.position2.x), Min(drawCmd.position.y, drawCmd.position2.y) };
				vec2 lrm{ Max(drawCmd.position.x, drawCmd.position2.x), Max(drawCmd.position.y, drawCmd.position2.y) };
				lrm -= item->position;
				max.x = Max(max.x, lrm.x);
				max.y = Max(max.y, lrm.y);
				
			}break;
			case UIDrawType_Text: {
				vec2 textSize = CalcTextSize(drawCmd.text);
				max.x = Max(max.x, drawCmd.position.x + textSize.x);
				max.y = Max(max.y, drawCmd.position.y + textSize.y);
			}break;
		}
	}
	item->size = max;
}

inline b32 isItemHovered(UIItem* item) {
	return Math::PointInRectangle(DeshInput->mousePos, item->position + curwin->position, item->size * style.globalScale);
}

inline b32 isLocalAreaHovered(vec2 pos, vec2 size, UIItem* item) {
	return Math::PointInRectangle(DeshInput->mousePos, pos + item->position + curwin->position, size);
}

inline b32 isItemActive(UIItem* item) {
	return WinStateHasFlag(curwin, UIWSHovered) && CanTakeInput && isItemHovered(item);
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
	vec2 pos = 
		window->cursor + (style.windowPadding + MarginPositionOffset - window->scroll) + vec2(globalIndent, 0) + vec2::ONE * style.windowBorderSize ;
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
inline f32 BorderedRight(UIWindow* window = curwin)  { return window->dimensions.x - (window == curwin ? style.windowBorderSize : window->style.windowBorderSize); }
inline f32 BorderedLeft(UIWindow* window = curwin)   { return (window == curwin ? style.windowBorderSize : window->style.windowBorderSize); }
inline f32 BorderedTop(UIWindow* window = curwin)    { return (window == curwin ? style.windowBorderSize : window->style.windowBorderSize); }
inline f32 BorderedBottom(UIWindow* window = curwin) { return window->dimensions.y - (window == curwin ? style.windowBorderSize : window->style.windowBorderSize); }

inline f32 MarginedRight(UIWindow* window = curwin) { f32 ret = window->dimensions.x - (window == curwin ? style.windowBorderSize + style.windowPadding.x : window->style.windowBorderSize + window->style.windowPadding.x) - (CanScrollY(window) ? (window == curwin ? style.scrollBarYWidth : window->style.scrollBarYWidth) : 0) + MarginSizeOffset.x; MarginSizeOffset.x = 0; return ret; }
inline f32 MarginedLeft(UIWindow* window = curwin)   { return (window == curwin ? style.windowBorderSize + style.windowPadding.x : window->style.windowBorderSize + window->style.windowPadding.x) ; }
inline f32 MarginedTop(UIWindow* window = curwin)    { return (window == curwin ? style.windowBorderSize + style.windowPadding.y : window->style.windowBorderSize + window->style.windowPadding.y) ; }
inline f32 MarginedBottom(UIWindow* window = curwin) { f32 ret = window->dimensions.y - (window == curwin ? style.windowBorderSize + style.windowPadding.y : window->style.windowBorderSize + window->style.windowPadding.y) - (CanScrollX(window) ? (window == curwin ? style.scrollBarXHeight : window->style.scrollBarXHeight) : 0) + MarginSizeOffset.y; MarginSizeOffset.y = 0; return ret; }

inline f32 ScrollBaredRight(UIWindow* window = curwin)  { return BorderedRight(window) - (CanScrollY() ? (window == curwin ? style.scrollBarYWidth : window->style.scrollBarYWidth) : 0); }
inline f32 ScrollBaredLeft(UIWindow* window = curwin)   { return BorderedLeft(window); }
inline f32 ScrollBaredTop(UIWindow* window = curwin)    { return BorderedTop(window); }
inline f32 ScrollBaredBottom(UIWindow* window = curwin) { return BorderedBottom(window) - (CanScrollX() ? (window == curwin ? style.scrollBarXHeight : window->style.scrollBarXHeight) : 0); }

//return the maximum width an item can be in a non-scrolled state
inline f32 MaxItemWidth(UIWindow* window = curwin) {
	return MarginedRight(window) - MarginedLeft(window);
}

inline b32 MouseInArea(vec2 pos, vec2 size) {
	return Math::PointInRectangle(DeshInput->mousePos, pos, size);
}

inline b32 MouseInWinArea(vec2 pos, vec2 size) {
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

inline vec2 DecideWinSize(vec2 defaultSize, vec2 itemPos) {
	vec2 size;
	if (NextWinSize.x != -1) {
		if (NextWinSize.x == MAX_F32)
			size.x = MarginedRight() - itemPos.x;
		else if (NextWinSize.x == 0)
			if (defaultSize.x == MAX_F32)
				size.x = MarginedRight() - itemPos.x;
			else size.x = defaultSize.x;
		else size.x = NextWinSize.x;

		if (NextWinSize.y == MAX_F32)
			size.y = MarginedBottom() - itemPos.y;
		else if (NextWinSize.y == 0)
			if (defaultSize.y == MAX_F32)
				size.y = MarginedBottom() - itemPos.y;
			else size.y = defaultSize.y;
		else size.y = NextWinSize.y;
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

void WinAdddowCursor(CursorType curtype) {
	DeshWindow->SetCursor(curtype);
	StateAddFlag(UISCursorSet);
}


UIStyle& UI::GetStyle(){
	return style;
}

UIWindow* UI::GetWindow() {
    return curwin;
}

vec2 UI::GetLastItemPos() {
	//Assert(curwin->items.count, "Attempt to get last item position without creating any items!");
	return curwin->items[ui_state.currlayer].last->position;
}

vec2 UI::GetLastItemSize() {
	//Assert(curwin->items.count, "Attempt to get last item size without creating any items!");
	return curwin->items[ui_state.currlayer].last->size;
}

vec2 UI::GetLastItemScreenPos() {
	//Assert(curwin->items.count, "Attempt to get last item position without creating any items!");
	return curwin->position + curwin->items[ui_state.currlayer].last->position;
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
	if (curwin->items[ui_state.currlayer].last) {
		curwin->cursor.y = curwin->items[ui_state.currlayer].last->initialCurPos.y;
		curwin->cursor.x += curwin->items[ui_state.currlayer].last->initialCurPos.x + curwin->items[ui_state.currlayer].last->size.x + style.itemSpacing.x;
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
inline UIItem* UI::GetLastItem(u32 layeroffset) {
	return curwin->items[ui_state.currlayer + layeroffset].last;
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
	else {
		curwin->items[ui_state.currlayer + layeroffset].add(UIItem{ type, curwin->cursor, style });
#ifdef DESHI_INTERNAL
		UI::GetLastItem(layeroffset)->item_layer = ui_state.currlayer + layeroffset;
		UI::GetLastItem(layeroffset)->item_idx = curwin->items[ui_state.currlayer + layeroffset].count;
#endif
		BreakOnItem;
	}
	if (StateHasFlag(UISNextItemMinSizeIgnored)) {
		UI::GetLastItem(layeroffset)->trackedForMinSize = 0;
		StateRemoveFlag(UISNextItemMinSizeIgnored);
	}

	curwin->items_count++;
	return UI::GetLastItem(layeroffset);
}

inline void EndItem(UIItem* item) {
	//copy the last made item to lastitem, so we can look back at it independently of custom item nonsense
	//maybe only do this is we're making a custom item
	//lastitem = *item;
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

//internal debug drawing functions
void DebugRect(vec2 pos, vec2 size, color col = Color_Red) {
	UIDrawCmd dc{ UIDrawType_Rectangle };
	dc.color = col;
	dc.position = pos;
	dc.dimensions = size;
	dc.thickness = 1;
	debugCmds.add(dc);
}

void DebugRectFilled(vec2 pos, vec2 size, color col = Color_Red) {
	UIDrawCmd dc{ UIDrawType_FilledRectangle };
	dc.color = col;
	dc.position = pos;
	dc.dimensions = size;
	debugCmds.add(dc);
}

void DebugCircle(vec2 pos, f32 radius, color col = Color_Red) {
	UIDrawCmd dc{ UIDrawType_Circle };
	dc.color = col;
	dc.position = pos - vec2::ONE * radius / 2;
	dc.thickness = radius;
	dc.subdivisions = 30;
	debugCmds.add(dc);
}

void DebugCircleFilled(vec2 pos, f32 radius, color col = Color_Red) {
	UIDrawCmd dc{ UIDrawType_CircleFilled };
	dc.color = col;
	dc.position = pos;
	dc.thickness = radius;
	dc.subdivisions = 30;
	debugCmds.add(dc);
}

void DebugLine(vec2 pos, vec2 pos2, color col = Color_Red) {
	UIDrawCmd dc{ UIDrawType_Circle };
	dc.color = col;
	dc.position = pos;
	dc.position2 = pos2;
	dc.thickness = 1;
	debugCmds.add(dc);
}

//@Primitive Items



//rectangle


//TODO(sushi) decide if abstract objs should be placed in window space or screen space
void UI::Rect(vec2 pos, vec2 dimen, color color) {
	UIItem       item{ UIItemType_Abstract, curwin->cursor, style };
	UIDrawCmd drawCmd{ UIDrawType_Rectangle};
	drawCmd.position = vec2::ZERO;
	drawCmd.dimensions = dimen;
	drawCmd.color = color;
	
	item.position = pos;
	item.size = dimen;
	
	item.drawCmds.add(drawCmd);
	curwin->items[ui_state.currlayer].add(item);
}

void UI::RectFilled(vec2 pos, vec2 dimen, color color) {
	UIItem       item{ UIItemType_Abstract, curwin->cursor, style };
	UIDrawCmd drawCmd{ UIDrawType_FilledRectangle};
	drawCmd.position = vec2::ZERO;
	drawCmd.dimensions = dimen;
	drawCmd.color = color;
	
	item.position = pos;
	item.size = dimen;
	
	item.drawCmds.add(drawCmd);
	curwin->items[ui_state.currlayer].add(item);
}


//@Line


void UI::Line(vec2 start, vec2 end, f32 thickness, color color){
	UIItem       item{ UIItemType_Abstract, curwin->cursor, style };
	UIDrawCmd drawCmd{ UIDrawType_Line};
	drawCmd.position  = start;
	drawCmd.position2 = end;
	drawCmd.thickness = thickness;
	drawCmd.    color = color;
	
	item.position = vec2::ZERO;// { Min(drawCmd.position.x, drawCmd.position2.x), Min(drawCmd.position.y, drawCmd.position2.y) };
	item.    size = vec2{ Max(drawCmd.position.x, drawCmd.position2.x), Max(drawCmd.position.y, drawCmd.position2.y) } - item.position;
	
	item.drawCmds.add(drawCmd);
	curwin->items[ui_state.currlayer].add(item);
}


//@Circle


void UI::Circle(vec2 pos, f32 radius, u32 subdivisions, color color) {
	UIItem       item{ UIItemType_Abstract, curwin->cursor, style };
	UIDrawCmd drawCmd{ UIDrawType_Circle };
	drawCmd.position = vec2{ radius, radius };
	drawCmd.thickness = radius;
	drawCmd.subdivisions = subdivisions;
	drawCmd.color = color;
	
	item.position = pos - vec2::ONE * radius;
	item.size = vec2::ONE * radius * 2;
	
	item.drawCmds.add(drawCmd);
	curwin->items[ui_state.currlayer].add(item);
	
}

void UI::CircleFilled(vec2 pos, f32 radius, u32 subdivisions, color color) {
	UIItem       item{ UIItemType_Abstract, curwin->cursor, style };
	UIDrawCmd drawCmd{ UIDrawType_CircleFilled };
	drawCmd.position = vec2{ radius, radius };
	drawCmd.thickness = radius;
	drawCmd.subdivisions = subdivisions;
	drawCmd.color = color;
	
	item.position = pos - vec2::ONE * radius;
	item.size = vec2::ONE * radius * 2;
	
	item.drawCmds.add(drawCmd);
	curwin->items[ui_state.currlayer].add(item);
}


/////////////////////
////		   	 ////
////    Items    ////
////			 ////
/////////////////////


//@Text


//internal function for actually making and adding the drawCmd
local void TextCall(const char* text, vec2 pos, color color, UIItem* item) {
	UIDrawCmd drawCmd{ UIDrawType_Text};
	drawCmd.text = string(text); 
	drawCmd.position = pos;
	drawCmd.color = color;
	drawCmd.font = style.font;
	
	item->drawCmds.add(drawCmd);
}

//secondary, for unicode
local void TextCall(const wchar_t* text, vec2 pos, color color, UIItem* item) {
	UIDrawCmd drawCmd{ UIDrawType_WText};
	drawCmd.wtext = wstring(text);
	drawCmd.position = pos;
	drawCmd.color = color;
	drawCmd.font = style.font;
	
	item->drawCmds.add(drawCmd);
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
				u32 maxChars = floor(MarginedRight() - item->position.x) / style.font->max_width;
				
				//make sure max chars never equals 0
				if (!maxChars) maxChars++;
				
				//wrap each string in newline array
				for (string& t : newlined) {
					//we need to see if the string goes beyond the width of the window and wrap if it does
					if (maxChars < t.count) {
						//if this is true we know item's total width is just maxChars times font width
						item->size.x = maxChars * style.font->max_width;
						
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
					}
				}
				
				
				
			}break;
			default:Assert(!"unknown font type?");
		}
		
		if (NextItemSize.x != -1)
			item->size = NextItemSize;
		else CalcItemSize(item);
		
		item->size.y = workcur.y;
		
		NextItemSize = vec2{ -1, 0 };
	}
	else {
		//TODO(sushi) make NoWrap also check for newlines
		
		if (NextItemSize.x != -1) item->size = NextItemSize;
		else                      item->size = UI::CalcTextSize(in);
		
		NextItemSize = vec2{ -1, 0 };
		
		TextCall(in, vec2{ 0,0 }, style.colors[UIStyleCol_Text], item);
		CalcItemSize(item);
	}
	
	AdvanceCursor(item, move_cursor);
}

//second function for wrapping, using unicode
//these can probably be merged into one but i dont feel like doing that rn
local void TextW(const wchar_t* in, vec2 pos, color color, b32 nowrap, b32 move_cursor = true) {
	
	using namespace UI;
	UIItem* item = BeginItem(UIItemType_Text);
	item->position = pos;
	
	if (!nowrap) {
		wstring text = in;
		
		//we split wstring by newlines and put them into here 
		//maybe make this into its own function
		array<wstring> newlined;
		
		u32 newline = text.findFirstChar('\n');
		if (newline != wstring::npos && newline != text.count - 1) {
			wstring remainder = text.substr(newline + 1);
			newlined.add(text.substr(0, newline - 1));
			newline = remainder.findFirstChar('\n');
			while (newline != wstring::npos) {
				newlined.add(remainder.substr(0, newline - 1));
				remainder = remainder.substr(newline + 1);
				newline = remainder.findFirstChar('\n');
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
				f32 maxw = curwin->width - 2 * style.windowPadding.x;
				f32 currlinew = 0;
				
				for (wstring& t : newlined) {
					for (int i = 0; i < t.count; i++) {
						currlinew += font->GetPackedChar(t[i])->xadvance * wscale;
						
						if (currlinew >= maxw) {
							
							//find closest space to split by, if none we just split the word
							u32 lastspc = t.findLastChar(' ', i);
							wstring nustr = t.substr(0, (lastspc == wstring::npos) ? i - 1 : lastspc);
							TextCall(nustr.str, workcur, color, item);
							
							t = t.substr(nustr.count);
							workcur.y += style.fontHeight + style.itemSpacing.y;
							
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
				u32 maxChars = floor(((curwin->width - 2 * style.windowPadding.x) - workcur.x) / style.font->max_width);
				
				//make sure max chars never equals 0
				if (!maxChars) maxChars++;
				
				//wrap each wstring in newline array
				for (wstring& t : newlined) {
					//we need to see if the wstring goes beyond the width of the window and wrap if it does
					if (maxChars < t.count) {
						//if this is true we know item's total width is just maxChars times font width
						item->size.x = maxChars * style.font->max_width;
						
						//find closest space to split by
						u32 splitat = t.findLastChar(' ', maxChars);
						wstring nustr = t.substr(0, (splitat == wstring::npos) ? maxChars - 1 : splitat);
						TextCall(nustr.str, workcur, color, item);
						
						t = t.substr(nustr.count);
						workcur.y += style.fontHeight + style.itemSpacing.y;
						
						//continue to wrap if we need to
						while (t.count > maxChars) {
							splitat = t.findLastChar(' ', maxChars);
							nustr = t.substr(0, (splitat == wstring::npos) ? maxChars - 1 : splitat);
							TextCall(nustr.str, workcur, color, item);
							
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
					}
				}
				
				
				
			}break;
			default:Assert(!"unknown font type?");
		}
		
		if (NextItemSize.x != -1)
			item->size = NextItemSize;
		else CalcItemSize(item);
		
		item->size.y = workcur.y;
		
		NextItemSize = vec2{ -1, 0 };
	}
	else {
		//TODO(sushi) make NoWrap also check for newlines
		
		if (NextItemSize.x != -1) item->size = NextItemSize;
		else                      item->size = UI::CalcTextSize(in);
		
		NextItemSize = vec2{ -1, 0 };
		
		TextCall(in, vec2{ 0,0 }, color, item);
		CalcItemSize(item);
	}
	AdvanceCursor(item, move_cursor);
}

void UI::Text(const char* text, UITextFlags flags) {
	TextW(text, PositionForNewItem(), style.colors[UIStyleCol_Text], HasFlag(flags, UITextFlags_NoWrap));
}

void UI::Text(const char* text, vec2 pos, UITextFlags flags) {
	TextW(text, pos, style.colors[UIStyleCol_Text], HasFlag(flags, UITextFlags_NoWrap), 0);
}

void UI::Text(const wchar_t* text, UITextFlags flags){
	TextW(text, PositionForNewItem(), style.colors[UIStyleCol_Text], HasFlag(flags, UITextFlags_NoWrap));
}

void UI::Text(const wchar_t* text, vec2 pos, UITextFlags flags){
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
	item->size = DecideItemSize(vec2(Min(curwin->width, Max(50.f, CalcTextSize(text).x * 1.1f)), style.fontHeight * style.buttonHeightRelToFont), item->position);
	AdvanceCursor(item);
	
	b32 active = WinStateHasFlag(curwin, UIWSHovered) && isItemHovered(item);
	
	{//background
		UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
		drawCmd.position = vec2::ZERO;
		drawCmd.dimensions = item->size;
		drawCmd.color = style.colors[(active ? (DeshInput->LMouseDown() ? UIStyleCol_ButtonBgActive : UIStyleCol_ButtonBgHovered) : UIStyleCol_ButtonBg)];
		item->drawCmds.add(drawCmd);
	}
	
	{//border
		UIDrawCmd drawCmd{ UIDrawType_Rectangle }; //inst 58
		drawCmd.color = style.colors[UIStyleCol_ButtonBorder];
		drawCmd.position = vec2::ZERO;
		drawCmd.dimensions = item->size;
		item->drawCmds.add(drawCmd);
	}
	
	{//text
		UIDrawCmd drawCmd{ UIDrawType_Text };
		drawCmd.color = style.colors[UIStyleCol_Text];
		drawCmd.position =
			vec2((item->size.x - UI::CalcTextSize(text).x) * style.buttonTextAlign.x,
				 (style.fontHeight * style.buttonHeightRelToFont - style.fontHeight) * style.buttonTextAlign.y);
		drawCmd.scissorOffset = vec2::ZERO;
		drawCmd.scissorExtent = item->size;
		drawCmd.useWindowScissor = false;
		drawCmd.text = string(text);
		drawCmd.font = style.font;
		item->drawCmds.add(drawCmd);
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
	
	int fillPadding = style.checkboxFillPadding;
	vec2 fillpos = boxsiz * vec2(fillPadding / boxsiz.x, fillPadding / boxsiz.y);
	vec2 fillsiz = boxsiz * (vec2::ONE - 2 * vec2(fillPadding / boxsiz.x, fillPadding / boxsiz.y));
	
	b32 bgactive = isItemActive(item);
	
	
	{//box
		UIDrawCmd drawCmd{ UIDrawType_FilledRectangle};
		drawCmd.position = vec2{ 0,0 };
		drawCmd.dimensions = boxsiz;
		drawCmd.color = style.colors[
		(bgactive ? (DeshInput->LMouseDown() ? UIStyleCol_CheckboxBgActive : UIStyleCol_CheckboxBgHovered) : UIStyleCol_CheckboxBg)
		];
		
		item->drawCmds.add(drawCmd);
	}
	
	//fill if true
	if (*b) {
		UIDrawCmd drawCmd{ UIDrawType_FilledRectangle};
		drawCmd.position = fillpos;
		drawCmd.dimensions = fillsiz;
		drawCmd.color = style.colors[UIStyleCol_CheckboxFilling];
		
		item->drawCmds.add(drawCmd);
	}
	
	{//border
		UIDrawCmd drawCmd{ UIDrawType_Rectangle }; //inst 58
		drawCmd.color = style.colors[UIStyleCol_CheckboxBorder];
		drawCmd.position = vec2::ZERO;
		drawCmd.dimensions = item->size;
		item->drawCmds.add(drawCmd);
	}
	
	{//label
		UIDrawCmd drawCmd{ UIDrawType_Text};
		drawCmd.position = vec2(boxsiz.x + style.itemSpacing.x, (boxsiz.y - style.fontHeight) * 0.5);
		drawCmd.text = label;
		drawCmd.color = style.colors[UIStyleCol_Text];
		drawCmd.font = style.font;
		
		item->drawCmds.add(drawCmd);
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
		UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
		drawCmd.color = style.colors[(active ? (DeshInput->LMouseDown() ? UIStyleCol_SelectableBgActive : UIStyleCol_SelectableBgHovered) : UIStyleCol_SelectableBg)];
		drawCmd.position = vec2::ZERO;
		drawCmd.dimensions = item->size;
		item->drawCmds.add(drawCmd);
		
	}
	
	{//text
		UIDrawCmd drawCmd{ UIDrawType_Text };
		drawCmd.position =
			vec2((item->size.x - CalcTextSize(prev_val).x) * style.buttonTextAlign.x,
				 (style.fontHeight * style.buttonHeightRelToFont - style.fontHeight) * style.buttonTextAlign.y);
		drawCmd.text = string(prev_val);
		drawCmd.color = style.colors[UIStyleCol_Text];
		drawCmd.scissorOffset = vec2::ZERO;
		drawCmd.scissorExtent = item->size;
		drawCmd.useWindowScissor = false;
		drawCmd.font = style.font;
		item->drawCmds.add(drawCmd);
	}

	if (open) {
		BeginPopOut(toStr("comboPopOut", label).str, item->position.yAdd(item->size.y), vec2::ZERO, UIWindowFlags_FitAllElements);
		StateAddFlag(UISComboBegan);
	}
	
	return combos[label];
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
		UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
		drawCmd.position = vec2(0, 0);
		drawCmd.dimensions = item->size;
		if (selected)
			drawCmd.color = style.colors[(active && DeshInput->LMouseDown() ? UIStyleCol_SelectableBgActive : UIStyleCol_SelectableBgHovered)];
		else
			drawCmd.color = style.colors[(active ? (DeshInput->LMouseDown() ? UIStyleCol_SelectableBgActive : UIStyleCol_SelectableBgHovered) : UIStyleCol_SelectableBg)];
		item->drawCmds.add(drawCmd);
	}

	{//text
		UIDrawCmd drawCmd{ UIDrawType_Text };
		drawCmd.position =
			vec2((item->size.x - UI::CalcTextSize(label).x) * style.buttonTextAlign.x,
				(style.fontHeight * style.buttonHeightRelToFont - style.fontHeight) * style.buttonTextAlign.y);
		drawCmd.text = string(label);
		drawCmd.color = style.colors[UIStyleCol_Text];
		//drawCmd.scissorOffset = vec2(0, item->size.y * selectables_added);
		//drawCmd.scissorExtent = item->size;
		//drawCmd.useWindowScissor = false;
		drawCmd.font = style.font;
		item->drawCmds.add(drawCmd);
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
		UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
		drawCmd.position = bgpos;
		drawCmd.dimensions = bgdim;
		drawCmd.color = style.colors[(active ? (DeshInput->LMouseDown() ? UIStyleCol_HeaderBgActive : UIStyleCol_HeaderBgHovered) : UIStyleCol_HeaderBg)];
		item->drawCmds.add(drawCmd);
	}
	
	{//button
		UIDrawCmd drawCmd{ (*open ? UIDrawType_CircleFilled : UIDrawType_Circle) };
		drawCmd.position = vec2{ item->size.y / 4, item->size.y / 2 };
		drawCmd.thickness = item->size.y / 4;
		drawCmd.subdivisions = 30;
		drawCmd.color = style.colors[(active ? (DeshInput->LMouseDown() ? UIStyleCol_HeaderBgActive : UIStyleCol_HeaderBgHovered) : UIStyleCol_HeaderBg)];
		item->drawCmds.add(drawCmd);
	}
	
	{//text
		UIDrawCmd drawCmd{ UIDrawType_Text };
		drawCmd.color = style.colors[UIStyleCol_Text];
		drawCmd.position = 
			vec2(bgpos.x + (item->size.x - bgpos.x - style.windowPadding.x - UI::CalcTextSize(label).x) * style.headerTextAlign.x,
				 ((style.fontHeight * style.headerHeightRelToFont - style.fontHeight) * style.headerTextAlign.y));
		drawCmd.scissorOffset = vec2::ZERO;
		drawCmd.scissorExtent = item->size;
		drawCmd.useWindowScissor = false;
		drawCmd.text = string(label);
		drawCmd.font = style.font;
		item->drawCmds.add(drawCmd);
	}
	
	{//border
		UIDrawCmd drawCmd{ UIDrawType_Rectangle }; //inst 58
		drawCmd.color = style.colors[UIStyleCol_HeaderBorder];
		drawCmd.position = bgpos;
		drawCmd.dimensions = bgdim;
		item->drawCmds.add(drawCmd);
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
		UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
		drawCmd.position = vec2::ZERO;
		drawCmd.dimensions = item->size;
		drawCmd.color = style.colors[UIStyleCol_SliderBg];
		item->drawCmds.add(drawCmd);
	}
	
	{//dragger
		UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
		drawCmd.position = draggerpos;
		drawCmd.dimensions = draggersiz;
		drawCmd.color = style.colors[((active || being_moved) ? (DeshInput->LMouseDown() ? UIStyleCol_SliderBarActive : UIStyleCol_SliderBarHovered) : UIStyleCol_SliderBar)];
		item->drawCmds.add(drawCmd);
	}
	
	{//border
		UIDrawCmd drawCmd{ UIDrawType_Rectangle }; //inst 58
		drawCmd.color = style.colors[UIStyleCol_SliderBorder];
		drawCmd.position = vec2::ZERO;
		drawCmd.dimensions = item->size;
		item->drawCmds.add(drawCmd);
	}
	
	
}

//@Image

void UI::Image(Texture* image, vec2 pos, f32 alpha, UIImageFlags flags) {
	UIItem* item = BeginItem(UIItemType_Image);
	
	item->position = pos;
	item->size = (NextItemSize.x == -1 ? vec2(image->width, image->height) : NextItemSize);
	NextItemSize = vec2(-1, 1);
	
	AdvanceCursor(item);
	
	//TODO(sushi) image borders
	{//image
		UIDrawCmd drawCmd{ UIDrawType_Image };
		drawCmd.tex = image;
		drawCmd.position = vec2::ZERO;
		drawCmd.dimensions = item->size;
		drawCmd.thickness = alpha;
		item->drawCmds.add(drawCmd);
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
	
	UIDrawCmd drawCmd{ UIDrawType_Line };
	drawCmd.position  = vec2(0, height / 2) + item->position;
	drawCmd.position2 = vec2(item->size.x, height / 2) + item->position;
	drawCmd.thickness = 1;
	drawCmd.color = color(64,64,64);
	item->drawCmds.add(drawCmd);
	
}


//@InputText


//final input text
b32 InputTextCall(const char* label, char* buff, u32 buffSize, vec2 position, UIInputTextCallback callback, UIInputTextFlags flags, b32 moveCursor) {
	UIItem* item = BeginItem(UIItemType_InputText);
	
	UIInputTextState* state;
	
	size_t charCount = strlen(buff);
	
	item->position = position;
	
	vec2 dim;
	if (flags & UIInputTextFlags_FitSizeToText) {
		dim = UI::CalcTextSize(string(buff));
	}
	else {
		dim = DecideItemSize(vec2(Math::clamp(100.f, 0.f, Math::clamp(curwin->width - 2.f * style.windowPadding.x, 1.f, FLT_MAX)), style.inputTextHeightRelToFont * style.fontHeight), item->position);
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
	
	if (hovered) WinAdddowCursor(CursorType_IBeam);
	
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
		UIDrawCmd drawCmd{ UIDrawType_FilledRectangle};
		drawCmd.position = vec2::ZERO;
		drawCmd.dimensions = dim;
		drawCmd.color =
			style.colors[(!active ? (hovered ? UIStyleCol_InputTextBgHovered : UIStyleCol_InputTextBg) : UIStyleCol_InputTextBg)];
		
		item->drawCmds.add(drawCmd);
	}
	
	vec2 textStart =
		vec2((dim.x - charCount * style.font->max_width) * style.inputTextTextAlign.x + 3, //TODO(sushi) make an input text offset style var
			 (style.fontHeight * style.inputTextHeightRelToFont - style.fontHeight) * style.inputTextTextAlign.y);
	
	{//text
		UIDrawCmd drawCmd{ UIDrawType_Text};
		drawCmd.position = textStart;
		drawCmd.text = string(buff);
		drawCmd.color = style.colors[UIStyleCol_Text];
		drawCmd.font = style.font;
		
		item->drawCmds.add(drawCmd);
	}
	
	{//border
		UIDrawCmd drawCmd{ UIDrawType_Rectangle }; //inst 58
		drawCmd.color = style.colors[UIStyleCol_InputTextBorder];
		drawCmd.position = vec2::ZERO;
		drawCmd.dimensions = item->size;
		item->drawCmds.add(drawCmd); 
	}
	
	//TODO(sushi, Ui) impl different text cursors
	if (active) {//cursor
		UIDrawCmd drawCmd{ UIDrawType_Line};
		drawCmd.position = item->position + textStart + vec2(state->cursor * style.font->max_width * style.fontHeight / style.font->aspect_ratio / style.font->max_width, 0);
		drawCmd.position2 = item->position + textStart + vec2(state->cursor * style.font->max_width * style.fontHeight / style.font->aspect_ratio / style.font->max_width, style.fontHeight - 1);
		drawCmd.color =
			color(255, 255, 255,
				  255 * (
						 cos((2 * M_PI) / (state->cursorBlinkTime / 2) * TIMER_END(state->timeSinceTyped) / 1000 -
							 sin((2 * M_PI) / (state->cursorBlinkTime / 2) * TIMER_END(state->timeSinceTyped) / 1000)) + 1) / 2);
		drawCmd.thickness = 1;
		
		item->drawCmds.add(drawCmd);
	}
	
	if (flags & UIInputTextFlags_EnterReturnsTrue && DeshInput->KeyPressed(Key::ENTER) || DeshInput->KeyPressed(Key::NUMPADENTER)) {
		return true;
	}
	else if (flags & UIInputTextFlags_AnyChangeReturnsTrue && bufferChanged) {
		return true;
	}
	
	return false;
}

b32 UI::InputText(const char* label, char* buffer, u32 buffSize, UIInputTextFlags flags) {
	vec2 position = PositionForNewItem();
	
	return InputTextCall(label, buffer, buffSize, position, nullptr, flags, 1);
}

b32 UI::InputText(const char* label, char* buffer, u32 buffSize, UIInputTextCallback callback, UIInputTextFlags flags) {
	vec2 position = PositionForNewItem();
	
	return InputTextCall(label, buffer, buffSize, position, callback, flags, 1);
}

b32 UI::InputText(const char* label, char* buffer, u32 buffSize, UIInputTextState*& getInputTextState, UIInputTextFlags flags) {
	vec2 position = PositionForNewItem();
	
	if (InputTextCall(label, buffer, buffSize, position, nullptr, flags, 1)) {
		getInputTextState = inputTexts.at(label);
		return true;
	}
	getInputTextState = inputTexts.at(label);
	return false;
}

b32 UI::InputText(const char* label, char* buffer, u32 buffSize, vec2 pos, UIInputTextFlags flags) {
	return InputTextCall(label, buffer, buffSize, pos, nullptr, flags, 0);
}

b32 UI::InputText(const char* label, char* buffer, u32 buffSize, vec2 pos, UIInputTextCallback callback, UIInputTextFlags flags) {
	return InputTextCall(label, buffer, buffSize, pos, callback, flags, 0);
}

b32 UI::InputText(const char* label, char* buffer, u32 buffSize, vec2 pos, UIInputTextState*& getInputTextState, UIInputTextFlags flags) {
	if (InputTextCall(label, buffer, buffSize, pos, nullptr, flags, 0)) {
		getInputTextState = inputTexts.at(label);
		return true;
	}
	getInputTextState = inputTexts.at(label);
	return false;
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
	layerStack.add(ui_state.currlayer);
	ui_state.currlayer = layer;
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
		ui_state.currlayer = *layerStack.last;
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

void CheckForHoveredWindow() {
	b32 hovered_found = 0;
	for (int i = windows.count - 1; i > 0; i--) {
		UIWindow* w = *windows.atIdx(i);
		if (WinBegan(w)) {
			if (!hovered_found && MouseInArea(w->position, w->dimensions * w->style.globalScale)) {
				WinAddHovered(w);
				WinRemoveChildHovered(w);
				for (UIWindow* c : w->children) {
					if (WinBegan(c)) {
						vec2 scrollBarAdjust = vec2((CanScrollY() ? style.scrollBarYWidth : 0), (CanScrollX() ? style.scrollBarXHeight : 0));
						vec2 visRegionStart = vec2(Max(w->x, c->x), Max(w->y, c->y));
						vec2 visRegionEnd = vec2(Min(w->x + w->width - (CanScrollY() ? style.scrollBarYWidth : 0), c->x + c->width), Min(w->y + w->height - (CanScrollX() ? style.scrollBarXHeight : 0), c->y + c->height));
						vec2 childVisibleRegion = visRegionEnd - visRegionStart;
						c->visibleRegionStart = visRegionStart;
						c->visibleRegionSize = childVisibleRegion;
						if (MouseInArea(visRegionStart, childVisibleRegion * style.globalScale)) {
							WinAddChildHovered(w);
							WinAddHovered(c);
							break;
						}
						else {
							WinRemoveHovered(c);
						}
					}
				}


				hovered_found = 1;
			}
			else {
				WinRemoveHovered(w);
				//this kind of sucks
				for (UIWindow* c : w->children) WinRemoveHovered(c);
			}
		}
	}
}

//this function is checked in UI::Update, while the other 3 are checked per window
void CheckWindowsForFocusInputs() {
	//special case where we always check for metrics first since it draws last
	
	for (int i = windows.count - 1; i > 0; i--) {
		UIWindow* w = *windows.atIdx(i);
		WinRemoveFocused(w);
		if (!(w->flags & UIWindowFlags_NoFocus)) {
			if (i == windows.count - 1 && WinHovered(w)) {
				WinAddFocused(w);
				break;
			}
			else if ((WinHovered(w) || WinChildHovered(w)) && ((w->flags & UIWindowFlags_FocusOnHover) ? 1 : DeshInput->LMousePressed())) {
				WinRemoveFocused((*windows.data.last));
				WinAddFocused(w);
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
			WinAddLatched(window);
			mouse = mp;
			wdims = window->dimensions;
			wpos = window->position;
			SetFocusedWindow(window);
			SetResizingInput;
		}
		
		if (mrele) {
			WinRemoveLatched(window);
			AllowInputs;
		}
		
		switch (activeSide) {
			case wTop: {
				WinAdddowCursor(CursorType_VResize);
				if (mdown) {
					window->position.y = wpos.y + (mp.y - mouse.y);
					window->dimensions = wdims.yAdd(mouse.y - mp.y);
					window->scy = Clamp(window->scy, 0.f, window->maxScroll.y);
				}
			}break;
			case wBottom: {
				WinAdddowCursor(CursorType_VResize); 
				if (mdown) {
					window->dimensions = wdims.yAdd(mp.y - mouse.y);
					window->scy = Clamp(window->scy, 0.f, window->maxScroll.y);
				}
			}break;
			case wLeft: {
				WinAdddowCursor(CursorType_HResize);
				if (mdown) {
					window->position.x = wpos.x + (mp.x - mouse.x);
					window->dimensions = wdims.xAdd(mouse.x - mp.x);
					window->scx = Clamp(window->scx, 0.f, window->maxScroll.x);
				}
			}break;
			case wRight: {
				WinAdddowCursor(CursorType_HResize); 
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
	window->scx = Math::clamp(window->scx, 0.f, window->maxScroll.x);
	window->scy = Math::clamp(window->scy, 0.f, window->maxScroll.y);
	
	//mouse wheel inputs
	//if this is a child window and it cant scroll, redirect the scrolling inputs to the parent
	if (window->parent && WinHovered(window) && window->maxScroll.x == 0 && window->maxScroll.y == 0) {
		CheckWindowForScrollingInputs(window->parent, 1);
		return;
	}
	if (((WinHovered(window) && !WinChildHovered(window)) || fromChild) && DeshInput->ScrollUp()) {
		window->scy -= style.scrollAmount.y;
		window->scy = Math::clamp(window->scy, 0.f, window->maxScroll.y); // clamp y again to prevent user from seeing it not be clamped for a frame
		return;
	}
	if (((WinHovered(window) && !WinChildHovered(window)) || fromChild) && DeshInput->ScrollDown()) {
		window->scy += style.scrollAmount.y;
		window->scy = Math::clamp(window->scy, 0.f, window->maxScroll.y); // clamp y again to prevent user from seeing it not be clamped for a frame
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
				WinRemoveBeingScrolled(window);
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
				WinRemoveBeingScrolled(window);
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
			WinAddBeingDragged(window);
			mouseOffset = window->position - DeshInput->mousePos;
			SetFocusedWindow(window);
		}
		if (WinBeingDragged(window)) {
			window->position = DeshInput->mousePos + mouseOffset;
		}
		if (DeshInput->KeyReleased(MouseButton::LEFT)) {
			WinRemoveBeingDragged(window);
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

				NextWinPos = vec2(-1, 0); NextWinSize = vec2(-1, 0);

				AdvanceCursor(item);

				curwin = parent->children[name];
				curwin->dimensions = item->size;
				curwin->cursor = vec2(0, 0);
				if (NextWinPos.x != -1) { curwin->position = NextWinPos; }
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

				parent->children.add(name, curwin);
			}

			indentStack.add(0);
			item->child = curwin;
			curwin->parent = parent;
			curwin->type = UIWindowType_Child;
		}break;
		case UIWindowType_PopOut: { //////////////////////////////////////////////////////////////////////
			UIWindow* parent = curwin;
			UIItem* item = BeginItem(UIItemType_Window);
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

				parent->children.add(name, curwin);
			}

			indentStack.add(0);
			item->child = curwin;
			curwin->parent = parent;
			curwin->type = UIWindowType_PopOut;
		}break;
	}

	WinAddBegan(curwin);
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
	ui_state.currlayer = Min(++ui_state.currlayer, UI_LAYERS);
	BeginCall(name, pos, dimensions, flags, UIWindowType_PopOut);
	ui_state.currlayer--;
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
			if (item.trackedForMinSize) {
				max.x = Max(max.x, (item.position.x + curwin->scx) + item.size.x);
				max.y = Max(max.y, (item.position.y + curwin->scy) + item.size.y);
			}
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
			UIDrawCmd drawCmd{ UIDrawType_FilledRectangle};
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
				UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
				drawCmd.color = style.colors[UIStyleCol_ScrollBarBg]; //TODO(sushi) add active/hovered scrollbarbg colors
				drawCmd.position = vec2(ScrollBaredRight(), BorderedTop());
				drawCmd.dimensions = vec2(style.scrollBarYWidth, scrollbarheight);
				postitem->drawCmds.add(drawCmd);
			}

			{//scroll dragger
				UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
				drawCmd.color = style.colors[(scdractive ? ((DeshInput->LMouseDown()) ? UIStyleCol_ScrollBarDraggerActive : UIStyleCol_ScrollBarDraggerHovered) : UIStyleCol_ScrollBarDragger)];
				drawCmd.position = draggerpos;
				drawCmd.dimensions = vec2(style.scrollBarYWidth, draggerheight);
				postitem->drawCmds.add(drawCmd);
			}

			//if both scroll bars are active draw a little square to obscure the empty space 
			if (CanScrollX()) {
				UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
				drawCmd.color = style.colors[UIStyleCol_WindowBg];
				drawCmd.position = vec2(ScrollBaredRight(), scrollbarheight);
				drawCmd.dimensions = vec2(style.scrollBarYWidth, style.scrollBarXHeight);
				postitem->drawCmds.add(drawCmd);
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
				UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
				drawCmd.color = style.colors[UIStyleCol_ScrollBarBg]; //TODO(sushi) add active/hovered scrollbarbg colors
				drawCmd.position = vec2(0, ScrollBaredBottom());
				drawCmd.dimensions = vec2(scrollbarwidth, style.scrollBarXHeight);
				postitem->drawCmds.add(drawCmd);
			}

			{//scroll dragger
				UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
				drawCmd.color = style.colors[(scdractive ? ((DeshInput->LMouseDown()) ? UIStyleCol_ScrollBarDraggerActive : UIStyleCol_ScrollBarDraggerHovered) : UIStyleCol_ScrollBarDragger)];
				drawCmd.position = draggerpos;
				drawCmd.dimensions = vec2(draggerwidth, style.scrollBarXHeight);
				postitem->drawCmds.add(drawCmd);
			}
		}
	}
	else curwin->maxScroll.x = 0;


	//if the window isn't invisible draw things that havent been disabled
	if (!WinHasFlags(UIWindowFlags_Invisible)) {
		//draw background
		if (!WinHasFlag(UIWindowFlags_NoBackground)) {
			UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
			drawCmd.position = vec2::ZERO;
			drawCmd.dimensions = curwin->dimensions;
			drawCmd.color = style.colors[UIStyleCol_WindowBg];

			preitem->drawCmds.add(drawCmd);
		}

		//draw border
		if (!WinHasFlag(UIWindowFlags_NoBorder)) {
			UIDrawCmd drawCmd{ UIDrawType_Rectangle }; //inst 58
			drawCmd.color = style.colors[UIStyleCol_Border];
			drawCmd.position = vec2::ONE * ceil(style.windowBorderSize / 2);
			drawCmd.dimensions = curwin->dimensions - vec2::ONE * ceil(style.windowBorderSize);
			drawCmd.thickness = style.windowBorderSize;
			postitem->drawCmds.add(drawCmd);
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

	EndCall();
}

void UI::EndChild() {
	Assert(!StateHasFlag(UISRowBegan), "Attempted to end a window with a Row in progress!");
	Assert(!StateHasFlag(UISComboBegan), "Attempted to end a window with a Combo in progress!");

	EndCall();
	indentStack.pop();
}

void UI::EndPopOut() {
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

//void UI::WinSetName(const char* name) {
//	curwin->name = name;
//}

b32 UI::IsWinHovered() {
	return WinHovered(curwin);
}

b32 UI::AnyWinHovered() {
	return StateHasFlag(UISGlobalHovered) || !CanTakeInput;
}

UIWindow* DisplayMetrics() {
	using namespace UI;
	
	static UIWindow* debugee = nullptr;
	
	UIWindow* myself = 0; //pointer returned for drawing
	
	static UIWindow* slomo = *windows.atIdx(0);
	static UIWindow* quick = *windows.atIdx(0);
	static UIWindow* mostitems = *windows.atIdx(0);
	static UIWindow* longname = *windows.atIdx(0);
	
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
	
	Begin("METRICS", vec2::ZERO, vec2(300, 500));
	myself = curwin;
	
	Text(toStr("Active Windows: ", windowStack.count).str);
	
	Separator(20);
	
	string slomotext = toStr("Slowest Render:");
	string quicktext = toStr("Fastest Render:");
	string mostitext = toStr("Most Items: "); 
	
	{
		static f32 sw = CalcTextSize(longname->name).x;
		static f32 fw = CalcTextSize(slomotext).x + 5;
		
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

	static b32 break_on_cursor = 0;
	static b32 frame_skip = 0;
	if (!break_on_cursor && (Button("Break on Cursor") || 
		DeshInput->KeyPressed(Key::B) && DeshInput->LShiftDown() && DeshInput->LCtrlDown())) {
		break_on_cursor = 1;
	}

	if (break_on_cursor && frame_skip) {
		Text("Press ESC to cancel");
		if (DeshInput->KeyPressed(Key::ESCAPE)) break_on_cursor = 0;
		PreventInputs;
		for (UIWindow* w : windows) {
			if (WinChildHovered(w)) {
				for (UIWindow* c : w->children) {
					forI(UI_WINDOW_ITEM_LAYERS) {
						for (UIItem& item : c->items[i]) {
							if (MouseInArea(c->position + item.position, item.size)) {
								DebugRect(c->position + item.position, item.size);
								if (DeshInput->LMousePressed()) {
									break_window = c;
									item_idx = item.item_idx;
									item_layer = item.item_layer;
									break_on_cursor = 0;
									frame_skip = 0;
									break;
								}
							}
						}
					}
				}
			}
			else if (WinHovered(w)) {
				forI(UI_WINDOW_ITEM_LAYERS) {
					for (UIItem& item : w->items[i]) {
						if (MouseInArea(w->position + item.position, item.size)) {
							DebugRect(w->position + item.position, item.size);
							if (DeshInput->LMousePressed()) {
								break_window = w;
								item_idx = item.item_idx;
								item_layer = item.item_layer;
								break_on_cursor = 0;
								frame_skip = 0;
								break;
							}
						}
					}
				}
			} 
		}
	}
	if (break_on_cursor) {
		frame_skip = 1;
	}
	
	Separator(20);
	
	PushVar(UIStyleVar_RowItemAlign, vec2(0, 0.5));
	BeginRow(2, style.fontHeight * 1.5);
	RowSetupRelativeColumnWidths({ 1.2, 1 });
	
	static u32 selected = 0;
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
								break_window = debugee;
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
		persist b32 showBorderArea = false;
		persist b32 showMarginArea = false;
		persist b32 showScrollBarArea = false;
		
		
		if (BeginHeader("Window Debug Visuals")) {
			Checkbox("Show Item Boxes", &showItemBoxes);
			Checkbox("Show Item Cursors", &showItemCursors);
			Checkbox("Show All DrawCmd Scissors", &showAllDrawCmdScissors);
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
						UIDrawCmd dc{ UIDrawType_Rectangle };
						dc.color = Color_Green;
						dc.position = debugee->position + item.initialCurPos + item.style.windowPadding - vec2::ONE * 3 / 2.f;
						dc.dimensions = vec2::ONE * 3;
						debugCmds.add(dc);
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
	PushVar(UIStyleVar_TitleBarHeight,           style.fontHeight * 1.2);
	PushVar(UIStyleVar_TitleTextAlign,           vec2(1, 0.5));
	PushVar(UIStyleVar_ItemSpacing,              vec2(1, 1));
	PushVar(UIStyleVar_ScrollAmount,             vec2(10, 10));
	PushVar(UIStyleVar_CheckboxSize,             vec2(10, 10));
	PushVar(UIStyleVar_CheckboxFillPadding,      2);
	PushVar(UIStyleVar_InputTextTextAlign,       vec2(0, 0.5));
	PushVar(UIStyleVar_ButtonTextAlign,          vec2(0.5, 0.5));
	PushVar(UIStyleVar_HeaderTextAlign,          vec2(0.05, 0.5));
	PushVar(UIStyleVar_ButtonHeightRelToFont,    1.3);
	PushVar(UIStyleVar_HeaderHeightRelToFont,    1.3);
	PushVar(UIStyleVar_InputTextHeightRelToFont, 1.3);
	PushVar(UIStyleVar_CheckboxHeightRelToFont,  1.3);
	PushVar(UIStyleVar_RowItemAlign,             vec2(0.5, 0.5));
	PushVar(UIStyleVar_ScrollBarYWidth,          5);
	PushVar(UIStyleVar_ScrollBarXHeight,         5);
	PushVar(UIStyleVar_IndentAmount,             12);
	PushVar(UIStyleVar_FontHeight,               style.font->max_height);
	
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
	vec2 winScissorOffset;
	vec2 winScissorExtent;

	UIWindow* parent = window->parent;

	if (parent && window->type != UIWindowType_PopOut) {
		//if this is a child window we need to keep the child window's items within the parents
		//visible length of child window on each axis
		f32 xch = (parent->x + parent->width) - window->x;
		f32 ych = (parent->y + parent->height) - window->y;
		//how much to clip the extent of the child by if it's position goes negative relative to the parent
		f32 extentClipx = Clamp(winpos.x - parent->position.x, winpos.x - parent->position.x, 0.f);
		f32 extentClipy = Clamp(winpos.y - parent->position.y, winpos.y - parent->position.y, 0.f);
		winScissorOffset = { Max(parent->position.x,winpos.x), Max(parent->position.y, winpos.y) }; //NOTE scissor offset cant be negative
		winScissorExtent = { Min(winsiz.x, Clamp(xch, 0, xch) + extentClipx), Min(winsiz.y, Clamp(ych, 0, ych) + extentClipy) };
	}
	else {
		winScissorOffset = { Max(0.0f,winpos.x), Max(0.0f, winpos.y) }; //NOTE scissor offset cant be negative
		winScissorExtent = winsiz;
	}

	vec2 itempos = window->position + item.position;
	vec2 itemsiz = item.size;

	for (UIDrawCmd& drawCmd : item.drawCmds) {
		vec2   dcpos = itempos + drawCmd.position * item.style.globalScale;
		vec2  dcpos2 = itempos + drawCmd.position2 * item.style.globalScale;
		vec2   dcsiz = drawCmd.dimensions * item.style.globalScale;
		vec2    dcse = (drawCmd.useWindowScissor ? winScissorExtent : drawCmd.scissorExtent * item.style.globalScale);
		vec2    dcso = (drawCmd.useWindowScissor ? winScissorOffset : itempos + drawCmd.scissorOffset);
		f32      dct = drawCmd.thickness;
		u32      dcl = window->windowlayer;
		u32    dcsub = drawCmd.subdivisions;
		color  dccol = drawCmd.color;

		dcpos.x = floor(dcpos.x); dcpos.y = floor(dcpos.y);
		dcpos2.x = floor(dcpos2.x); dcpos2.y = floor(dcpos2.y);

		//modify the scissor offset and extent according to the kind of window we are drawing
		switch (window->type) {
			case UIWindowType_PopOut:
			case UIWindowType_Normal: {
				dcso.x = Max(winpos.x, dcso.x); dcso.y = Max(winpos.y, dcso.y); //force all items to stay within their windows
				dcso.x = Min(winpos.x + winScissorExtent.x - dcse.x, dcso.x); dcso.y = Min(winpos.y + winScissorExtent.y - dcse.y, dcso.y);
				if (drawCmd.useWindowScissor && winpos.x < 0) dcse.x += winpos.x; //if the window's pos goes negative, the scissor extent needs to adjust itself
				if (drawCmd.useWindowScissor && winpos.y < 0) dcse.y += winpos.y;
				dcse.x = Max(0.f, dcse.x); dcse.y = Max(0.f, dcse.y);
				dcso.x = Max(0.0f, dcso.x); dcso.y = Max(0.0f, dcso.y); //NOTE scissor offset cant be negative
			}break;
			case UIWindowType_Child: {
				dcso.x = Max(Max(parent->x, winpos.x), dcso.x);
				dcso.y = Max(Max(parent->y, winpos.y), dcso.y); //force all items to stay within their windows
				if ((winpos.x - parent->x) < 0)
					dcse.x += winpos.x - parent->x; //if the window's pos goes negative, the scissor extent needs to adjust itself
				if ((winpos.y - parent->y) < 0)
					dcse.y += winpos.y - parent->y;
				if (winpos.x < 0) dcse.x += winpos.x;
				if (winpos.y < 0) dcse.y += winpos.y;
				dcso.x = Min(winpos.x + winScissorExtent.x - dcse.x, dcso.x);
				dcso.y = Min(winpos.y + winScissorExtent.y - dcse.y, dcso.y);
				dcse.x = Max(0.f, dcse.x); dcse.y = Max(0.f, dcse.y);
				dcso.x = Max(0.0f, dcso.x); dcso.y = Max(0.0f, dcso.y); //NOTE scissor offset cant be negative
			}break;

		}


		Texture* dctex = drawCmd.tex;

		cstring dctext{ drawCmd.text.str,drawCmd.text.count };
		wcstring wdctext{ drawCmd.wtext.str, drawCmd.wtext.count };

		Font* font = drawCmd.font;

#if DESHI_INTERNAL
		//copy all drawCmd changes back to the actual drawCmd in debug mode so we
		//can visualize it in metrics
		drawCmd.scissorExtent = dcse;
		drawCmd.scissorOffset = dcso;
		drawCmd.position = dcpos;
		drawCmd.position2 = dcpos2;
		drawCmd.thickness = dct;
#endif

		switch (drawCmd.type) {
			case UIDrawType_FilledRectangle: {
				Render::FillRect2D(dcpos, dcsiz, dccol, dcl, dcso, dcse);
			}break;
			case UIDrawType_Rectangle: {
				Render::DrawRect2D(dcpos, dcsiz, dct, dccol, dcl, dcso, dcse);
			}break;
			case UIDrawType_Line: {
				Render::DrawLine2D(dcpos - item.position, dcpos2 - item.position, dct, dccol, dcl, dcso, dcse);
			}break;
			case UIDrawType_Circle: {
				Render::DrawCircle2D(dcpos, dct, dcsub, dccol, dcl, dcso, dcse);
			}break;
			case UIDrawType_CircleFilled: {
				Render::FillCircle2D(dcpos, dct, dcsub, dccol, dcl, dcso, dcse);
			}break;
			case UIDrawType_Text: {
				vec2 scale = vec2::ONE * item.style.fontHeight / item.style.font->max_height * item.style.globalScale;
				Render::DrawText2D(font, dctext, dcpos, dccol, scale, dcl, dcso, dcse);
			}break;
			case UIDrawType_WText: {
				vec2 scale = vec2::ONE * item.style.fontHeight / item.style.font->max_height * item.style.globalScale;
				Render::DrawText2D(font, wdctext, dcpos, dccol, scale, dcl, dcso, dcse);
			}break;

			case UIDrawType_Image: {
				Render::DrawTexture2D(dctex, dcpos, dcsiz, 0, dct, dcl, dcso, dcse);
			}break;
			default: {
				Assert(0, "unhandled UIDrawType!");
			}break;
		}
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
	
	//dont draw post-pre drawcmds if we're minimized
//	if (!p->minimized) {
		forI(UI_WINDOW_ITEM_LAYERS) {
			for (UIItem& item : p->items[i]) {
				if (item.type == UIItemType_Window) {
					item.child->position = p->position + item.position * item.style.globalScale;
					DrawWindow(item.child, p);
					WinRemoveBegan(item.child);
					continue;
				}
				DrawItem(item, p);
			}
		}
		
		//draw post items, such as scroll bars or context menus
		for (UIItem& item : p->postItems) {
			DrawItem(item, p);
		}
//	}

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

#ifdef DESHI_INTERNAL
	//clear break vars in debug mode
	break_window = 0;
	item_idx = -1;
	item_layer = -1;
#endif
	
	if (show_metrics) {
		DisplayMetrics();
		//DrawWindow(DisplayMetrics());
		show_metrics = 0;
	}

	//windows input checking functions
	CheckForHoveredWindow();
	CheckWindowsForFocusInputs();
	
	
	if (inputupon) CheckWindowForScrollingInputs(inputupon);
	if (inputupon) CheckWindowForResizingInputs(inputupon);
	if (inputupon) CheckWindowForDragInputs(inputupon);
	
	
	//reset cursor to default if no item decided to set it 
	if (!StateHasFlag(UISCursorSet)) DeshWindow->SetCursor(CursorType_Arrow);

	//draw windows in order 
	for (UIWindow* p : windows) {
		DrawWindow(p);
		WinRemoveBegan(p);
	}
	
	//it should be safe to do this any time the mouse is released
	if (DeshInput->LMouseReleased()) { AllowInputs; }
	
	
	//we defer window item clearing to after the metrics window is drawn
	//in debug builds
#ifdef DESHI_INTERNAL
	for (UIWindow* p : windows) {
		p->preItems.clear();
		p->postItems.clear();
		forI(UI_WINDOW_ITEM_LAYERS) {
			p->items[i].clear();
		}
		for (UIWindow* c : p->children) {
			c->preItems.clear();
			c->postItems.clear();
			forI(UI_WINDOW_ITEM_LAYERS) {
				c->items[i].clear();
			}
		}
	}


#endif
	
	//draw all debug commands if there are any
	
	for (UIDrawCmd& drawCmd : debugCmds) {
		vec2   dcpos = drawCmd.position;
		vec2  dcpos2 = drawCmd.position2;
		vec2   dcsiz = drawCmd.dimensions;
		vec2    dcse = DeshWindow->dimensions;
		vec2    dcso = vec2::ZERO;
		color  dccol = drawCmd.color;
		f32      dct = drawCmd.thickness;
		u32      dcl = UI_LAYERS - 1;
		u32    dcsub = drawCmd.subdivisions;
		
		Texture* dctex = drawCmd.tex;
		
		cstring dctext{ drawCmd.text.str,drawCmd.text.count };
		wcstring wdctext{ drawCmd.wtext.str, drawCmd.wtext.count };
		
		Font* font = drawCmd.font;
		
		switch (drawCmd.type) {
			case UIDrawType_FilledRectangle: {
				Render::FillRect2D(dcpos, dcsiz, dccol, dcl, dcso, dcse);
			}break;
			case UIDrawType_Rectangle: {
				Render::DrawRect2D(dcpos, dcsiz, dct, dccol, dcl, dcso, dcse);
			}break;
			case UIDrawType_Line: {
				Render::DrawLine2D(dcpos, dcpos2, dct, dccol, dcl, dcso, dcse);
			}break;
			case UIDrawType_Circle: {
				Render::DrawCircle2D(dcpos, dct, dcsub, dccol, dcl, dcso, dcse);
			}break;
			case UIDrawType_CircleFilled: {
				Render::FillCircle2D(dcpos, dct, dcsub, dccol, dcl, dcso, dcse);
			}break;
			case UIDrawType_Text: {
				Render::DrawText2D(font, dctext, dcpos, dccol, vec2::ONE, dcl, dcso, dcse);
			}break;
			case UIDrawType_WText: {
				Render::DrawText2D(font, wdctext, dcpos, dccol, vec2::ONE, dcl, dcso, dcse);
			}break;
			
			case UIDrawType_Image: {
				Render::DrawTexture2D(dctex, dcpos, dcsiz, 0, dct, dcl, dcso, dcse);
			}break;
			default: {
				Assert(0, "unhandled UIDrawType!");
			}break;
		}
	}
	debugCmds.clear();

	if (CanTakeInput && DeshInput->LMouseDown()) PreventInputs;

	StateRemoveFlag(UISGlobalHovered);
	StateRemoveFlag(UISCursorSet);
}

void UI::DrawDebugRect(vec2 pos, vec2 size, color col)          { DebugRect(pos, size, col); }
void UI::DrawDebugRectFilled(vec2 pos, vec2 size, color col)    { DebugRectFilled(pos, size, col); }
void UI::DrawDebugCircle(vec2 pos, f32 radius, color col)       { DebugCircle(pos, radius, col); }
void UI::DrawDebugCircleFilled(vec2 pos, f32 radius, color col) { DebugCircleFilled(pos, radius, col); }
void UI::DrawDebugLine(vec2 pos1, vec2 pos2, color col)         { DebugLine(pos1, pos2, col); }

