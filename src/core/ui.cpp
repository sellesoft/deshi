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
	{1, offsetof(UIStyle, buttonBorderSize)},
	{1, offsetof(UIStyle, titleBarHeight)},
	{2, offsetof(UIStyle, titleTextAlign)},
	{2, offsetof(UIStyle, scrollAmount)},
	{2, offsetof(UIStyle, checkboxSize)},
	{1, offsetof(UIStyle, checkboxFillPadding)},
	{2, offsetof(UIStyle, inputTextTextAlign)},
	{2, offsetof(UIStyle, buttonTextAlign)},
	{2, offsetof(UIStyle, headerTextAlign)},
	{2, offsetof(UIStyle, selectableTextAlign)},
	{2, offsetof(UIStyle, tabTextAlign)},
	{1, offsetof(UIStyle, buttonHeightRelToFont)},
	{1, offsetof(UIStyle, headerHeightRelToFont)},
	{1, offsetof(UIStyle, inputTextHeightRelToFont)},
	{1, offsetof(UIStyle, checkboxHeightRelToFont)},
	{1, offsetof(UIStyle, selectableHeightRelToFont)},
	{1, offsetof(UIStyle, tabHeightRelToFont)},
	{2, offsetof(UIStyle, rowItemAlign)},
	{2, offsetof(UIStyle, rowCellPadding)},
	{1, offsetof(UIStyle, scrollBarYWidth)},
	{1, offsetof(UIStyle, scrollBarXHeight)},
	{1, offsetof(UIStyle, indentAmount)},
	{1, offsetof(UIStyle, tabSpacing)},
	{1, offsetof(UIStyle, fontHeight)},
};


//window map which only stores known windows
//and their order in layers eg. when one gets clicked it gets moved to be first if its set to
local map<const char*, UIWindow*>        windows;   
local map<const char*, UIInputTextState> inputTexts;  //stores known input text labels and their state
local map<const char*, UITabBar>         tabBars;     //stores known tab bars
local map<const char*, UIRow>            rows;        //stores known Rows
local map<const char*, b32>              combos;      //stores known combos and if they are open or not
local map<const char*, b32>              sliders;     //stores whether a slider is being actively changed
local map<const char*, b32>              headers;     //stores whether a header is open or not
local array<UIWindow*>                   windowStack; //window stack which allow us to use windows like we do colors and styles
local array<ColorMod>                    colorStack; 
local array<VarMod>                      varStack; 
local array<vec2>                        scaleStack;  //global scales
local array<Font*>                       fontStack;
local array<u32>                         layerStack;
local array<f32>                         leftIndentStack{ 0 }; //stores global indentations
local array<f32>                         rightIndentStack{ 0 }; //stores global indentations
local array<u32>                         drawTargetStack{ 0 }; //stores draw target indexes for the renderer

local u32 itemFlags[UIItemType_COUNT]; //stores the default flags for every item that supports flagging, these can be set using SetItemFlags

local array<UIDrawCmd> debugCmds; //debug draw cmds that are always drawn last

local u32 initColorStackSize;
local u32 initStyleStackSize;

//global ui state flags
enum UIStateFlags_ {
	UISNone                   = 0, 
	UISGlobalHovered          = 1 << 0,
	UISRowBegan               = 1 << 1,
	UISComboBegan             = 1 << 2,
	UISTabBarBegan            = 1 << 3,
	UISTabBegan               = 1 << 4,
	UISCursorSet              = 1 << 5,
	UISNextItemSizeSet        = 1 << 6,
	UISNextItemActive         = 1 << 7,
	UISNextItemMinSizeIgnored = 1 << 8,
}; typedef u32 UIStateFlags;

//global ui input state
enum InputState_ {
	ISNone,
	ISScrolling,
	ISDragging,
	ISResizing,
	ISPreventInputs,
	ISExternalPreventInputs,
}; typedef u32 InputState;


//main UI state variables	
local UIWindow* curwin = 0;    //the window that is currently being worked with by the user
local UIWindow* hovered = 0;   //the window that the mouse is hovering over, this is set every Update
local UIWindow* inputupon = 0; //set to a window who has drag, scrolling, or resizing inputs being used on it 

local UIStateFlags stateFlags = UISNone;
local InputState   inputState = ISNone;

local u32 currlayer = UI_CENTER_LAYER;
local u32 winlayer = UI_CENTER_LAYER;
local u32 activeId = -1; //the id of an active widget eg. input text

local UIRow*    row;    //row being worked with
local UITabBar* tabBar; //tab bar being worked with

local vec2 NextWinSize = vec2(-1, 0);
local vec2 NextWinPos = vec2(-1, 0);
local vec2 NextItemPos = vec2(-1, 0);
local vec2 NextItemSize = vec2(-1, 0);
local vec2 NextCursorPos = vec2(-1,-1);

local vec2 MarginPositionOffset = vec2::ZERO;
local vec2 MarginSizeOffset = vec2::ZERO;

struct {
	u32 vertices = 0;
	u32 indices = 0;
	
	u32 draw_cmds = 0;
	u32 items = 0;
	u32 windows = 0;
}ui_stats;

//helper defines
#define StateHasFlag(flag) ((stateFlags) & (flag))
#define StateHasFlags(flags) (((stateFlags) & (flags)) == (flags))
#define StateAddFlag(flag) stateFlags |= flag
#define StateRemoveFlag(flag) ((stateFlags) &= (~(flag)))
#define StateResetFlags stateFlags = UISNone

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

#define leftIndent *leftIndentStack.last
#define rightIndent *rightIndentStack.last

#define CanTakeInput      inputState == ISNone
#define PreventInputs     inputState = ISPreventInputs
#define AllowInputs       inputState = ISNone;      inputupon = 0;
#define SetResizingInput  inputState = ISResizing;  inputupon = window;
#define SetDraggingInput  inputState = ISDragging;  inputupon = window;
#define SetScrollingInput inputState = ISScrolling; inputupon = window; WinSetBeingScrolled(window);
#define WinResizing       inputState == ISResizing
#define WinDragging       inputState == ISDragging
#define WinScrolling      inputState == ISScrolling

#define LeftMousePressed   DeshInput->LMousePressed()
#define LeftMouseDown      DeshInput->LMouseDown()
#define LeftMouseReleased  DeshInput->LMouseReleased()

#define RightMousePressed  DeshInput->RMousePressed()
#define RightMouseDown     DeshInput->RMouseDown()
#define RightMouseReleased DeshInput->RMouseReleased()

#define GetDefaultItemFlags(type, var) var |= itemFlags[type]

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

#ifdef BUILD_INTERNAL
#define BreakOnItem if(break_window_item && break_window_item == curwin && curwin->items[item_layer].count == item_idx){ DebugBreakpoint;}
#else
#define BreakOnItem
#endif

#ifdef BUILD_INTERNAL
#define BreakOnDrawCmdCreation if(break_drawCmd_create_hash == drawCmd.hash) {DebugBreakpoint;}
#define BreakOnDrawCmdDraw     if(break_drawCmd_draw_hash == drawCmd.hash) {DebugBreakpoint;}
#else
#define BreakOnDrawCmdCreation
#define BreakOnDrawCmdDraw 
#endif


//helper functions


//this calculates text taking into account newlines, BUT NOT WRAPPING
//useful for sizing a window to fit some text
vec2 UI::CalcTextSize(cstring text){DPZoneScoped;
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

vec2 UI::CalcTextSize(wcstring text) {DPZoneScoped;
	vec2 result = vec2{ 0, f32(style.fontHeight) };
	f32 line_width = 0;
	switch (style.font->type) {
		case FontType_BDF: case FontType_NONE: {
			while (text) {
				if (*text.str == '\n') {
					result.y += style.fontHeight;
					line_width = 0;
				}
				line_width += style.font->max_width * style.fontHeight / style.font->aspect_ratio / style.font->max_width;
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

inline b32 isItemHovered(UIItem* item) {DPZoneScoped;
	return Math::PointInRectangle(DeshInput->mousePos, item->position * style.globalScale + curwin->position, item->size * style.globalScale);
}

inline b32 isLocalAreaHovered(vec2 pos, vec2 size, UIItem* item) {DPZoneScoped;
	return Math::PointInRectangle(DeshInput->mousePos, pos + item->position + curwin->position, size);
}

inline b32 isItemActive(UIItem* item) {DPZoneScoped;
	return WinHovered(curwin) && CanTakeInput && isItemHovered(item);
}

//internal master cursor controller
//  this is an attempt to centralize what happens at the end of each item function
// 
//  i expect this to fall through at some point, as not all items are created equal and may need to
//  have different things happen after its creation, which could be handled as special cases within
//  the function itself.
inline void AdvanceCursor(UIItem* itemmade, b32 moveCursor = 1) {DPZoneScoped;
	
	//if a row is in progress, we must reposition the item to conform to row style variables
	//this means that you MUST ensure that this happens before any interactions with the item are calculated
	//for instance in the case of Button, this must happen before you check that the user has clicked it!
	if (StateHasFlag(UISRowBegan)) {
		//abstract item types (lines, rectangles, etc.) are not row'd, for now
		if (itemmade->type != UIItemType_Abstract) {
			UIColumn& col = row->columns[row->item_count % row->columns.count];
			row->item_count++;
			
			col.items.add(itemmade);
			
			f32 height = row->height;
			f32 width;
			//determine if the width is relative to the size of the item or not
			if (col.relative_width)
				width = itemmade->size.x * col.width;
			else
				width = col.width;
			
			vec2 align;
			if (col.alignment != vec2::ONE * -1)
				align = col.alignment;
			else
				align = itemmade->style.rowItemAlign;
			
			itemmade->position.y = row->position.y + (height - itemmade->size.y) * align.y + row->yoffset;
			itemmade->position.x = row->position.x + row->xoffset + (width - itemmade->size.x) * align.x;
			
			row->xoffset += width;
			if (col.max_width < itemmade->size.x) { col.reeval_width = true; col.max_width = itemmade->size.x; }
			if (row->max_height < itemmade->size.y) { row->reeval_height = true; row->max_height = itemmade->size.y; }
			row->max_height_frame = Max(row->max_height_frame, itemmade->size.y);
			
			//if we have finished a row, set xoffset to 0 and offset y for another row
			if (row->item_count % row->columns.count == 0) {
				row->xoffset = 0;
				row->yoffset += row->height;
			}
			
			//we dont need to handle moving the cursor here, because the final position of the cursor after a row is handled in EndRow()
		}
	}
	else if (moveCursor) curwin->cursor = vec2{ 0, itemmade->position.y + itemmade->size.y + style.itemSpacing.y - style.windowPadding.y + curwin->scy - style.windowBorderSize } ;
}

//returns if the window can scroll over x
inline b32 CanScrollX(UIWindow* window = curwin) {DPZoneScoped;
	return !HasFlag(window->flags, UIWindowFlags_NoScrollX) && window->width < window->minSizeForFit.x;
}

inline b32 CanScrollY(UIWindow* window = curwin) {DPZoneScoped;
	return !HasFlag(window->flags, UIWindowFlags_NoScrollY) && window->height < window->minSizeForFit.y;
}

//function for getting the position of a new item based on style, so the long string of additions
//is centralized for new additions
inline vec2 PositionForNewItem(UIWindow* window = curwin) {DPZoneScoped;
	vec2 pos = window->cursor + (style.windowPadding + MarginPositionOffset - window->scroll) + vec2(leftIndent, 0)
		+ vec2::ONE * ((HasFlag(window->flags, UIWindowFlags_NoBorder)) ? 0 : style.windowBorderSize);
	MarginPositionOffset = vec2::ZERO;
	if(NextCursorPos.x!=-1)pos.x=NextCursorPos.x;
	if(NextCursorPos.y!=-1)pos.y=NextCursorPos.y;
	NextCursorPos=vec2(-1,-1);
	return pos;
}

//returns a pair representing the area of the window that is bordered
//first is the position and second is the size
inline pair<vec2, vec2> BorderedArea(UIWindow* window = curwin) {DPZoneScoped;
	return make_pair(
					 vec2::ONE * style.windowBorderSize,
					 window->dimensions - vec2::ONE * 2 * style.windowBorderSize
					 );
}

//same as the bordered area, but also takes into account the margins
inline pair<vec2, vec2> MarginedArea(UIWindow* window = curwin) {DPZoneScoped;
	vec2 f = vec2::ONE * style.windowBorderSize + vec2::ONE * style.windowPadding;
	vec2 s = window->dimensions - 2 * f - MarginSizeOffset;
	s.x -= (CanScrollY() ? style.scrollBarYWidth : 0);
	//s.y -= (CanScrollX() ? style.scrollBarXHeight : 0);
	MarginSizeOffset = vec2::ZERO;
	return make_pair(f, s);
}

//the bordered area taking into account the scroll bars
inline pair<vec2, vec2> ScrollBaredArea(UIWindow* window = curwin) {DPZoneScoped;
	auto p = BorderedArea(window);
	p.second.x -= (CanScrollY(window) ? style.scrollBarYWidth : 0);
	p.second.y -= (CanScrollX(window) ? style.scrollBarXHeight : 0);
	return p;
}

//TODO(sushi) eventually change these to always use curwin instead of checking everytime
// probably just separate them into 2 overloaded functions each instead
FORCE_INLINE f32 BorderedRight(UIWindow* window = curwin)  {DPZoneScoped; return window->dimensions.x - (window == curwin ? style.windowBorderSize : window->style.windowBorderSize); }
FORCE_INLINE f32 BorderedLeft(UIWindow* window = curwin)   {DPZoneScoped; return (window == curwin ? style.windowBorderSize : window->style.windowBorderSize); }
FORCE_INLINE f32 BorderedTop(UIWindow* window = curwin)    {DPZoneScoped; return (window == curwin ? style.windowBorderSize : window->style.windowBorderSize); }
FORCE_INLINE f32 BorderedBottom(UIWindow* window = curwin) {DPZoneScoped; return window->dimensions.y - (window == curwin ? style.windowBorderSize : window->style.windowBorderSize); }

FORCE_INLINE f32 MarginedRight(UIWindow* window = curwin)  {DPZoneScoped; f32 ret = window->dimensions.x - (window == curwin ? style.windowBorderSize + style.windowPadding.x : window->style.windowBorderSize + window->style.windowPadding.x) - (CanScrollY(window) ? (window == curwin ? style.scrollBarYWidth : window->style.scrollBarYWidth) : 0) + MarginSizeOffset.x; MarginSizeOffset.x = 0; return ret; }
FORCE_INLINE f32 MarginedLeft(UIWindow* window = curwin)   {DPZoneScoped; return (window == curwin ? style.windowBorderSize + style.windowPadding.x : window->style.windowBorderSize + window->style.windowPadding.x) ; }
FORCE_INLINE f32 MarginedTop(UIWindow* window = curwin)    {DPZoneScoped; return (window == curwin ? style.windowBorderSize + style.windowPadding.y : window->style.windowBorderSize + window->style.windowPadding.y) ; }
FORCE_INLINE f32 MarginedBottom(UIWindow* window = curwin) {DPZoneScoped; f32 ret = window->dimensions.y - (window == curwin ? style.windowBorderSize + style.windowPadding.y : window->style.windowBorderSize + window->style.windowPadding.y) - (CanScrollX(window) ? (window == curwin ? style.scrollBarXHeight : window->style.scrollBarXHeight) : 0) + MarginSizeOffset.y; MarginSizeOffset.y = 0; return ret; }

FORCE_INLINE f32 ClientRight(UIWindow* window = curwin)  {DPZoneScoped; return BorderedRight(window) - (CanScrollY() ? (window == curwin ? style.scrollBarYWidth : window->style.scrollBarYWidth) : 0); }
FORCE_INLINE f32 ClientLeft(UIWindow* window = curwin)   {DPZoneScoped; return BorderedLeft(window); }
FORCE_INLINE f32 ClientTop(UIWindow* window = curwin)    {DPZoneScoped; return BorderedTop(window); }
FORCE_INLINE f32 ClientBottom(UIWindow* window = curwin) {DPZoneScoped; return BorderedBottom(window) - (CanScrollX() ? (window == curwin ? style.scrollBarXHeight : window->style.scrollBarXHeight) : 0); }

//return the maximum width an item can be in a non-scrolled state
FORCE_INLINE f32 MaxItemWidth(UIWindow* window = curwin) {DPZoneScoped;
	return MarginedRight(window) - MarginedLeft(window);
}

FORCE_INLINE b32 MouseInArea(vec2 pos, vec2 size) {DPZoneScoped;
	return Math::PointInRectangle(DeshInput->mousePos, pos, size);
}

FORCE_INLINE b32 MouseInWinArea(vec2 pos, vec2 size) {DPZoneScoped;
	return Math::PointInRectangle(DeshInput->mousePos - curwin->position, pos, size);
}

inline vec2 DecideItemSize(vec2 defaultSize, vec2 itemPos) {DPZoneScoped;
	vec2 size;
	if (NextItemSize.x != -1) {
		if (NextItemSize.x == MAX_F32)
			size.x = MarginedRight() - itemPos.x - rightIndent;
		else if (NextItemSize.x == 0)
			if (defaultSize.x == MAX_F32)
			size.x = MarginedRight() - itemPos.x - rightIndent;
		else size.x = defaultSize.x;
		else size.x = NextItemSize.x;
		
		if (NextItemSize.y == MAX_F32)
			size.y = MarginedBottom() - itemPos.y;
		else if (NextItemSize.y == 0)
			if(defaultSize.y == MAX_F32)
			size.y = MarginedBottom() - itemPos.y;
		else size.y = defaultSize.y;
		else size.y = NextItemSize.y;
		
		if (NextItemSize.x == -2) size.x = size.y;
		if (NextItemSize.y == -2) size.y = size.x;
		
	}
	else {
		if (defaultSize.x == MAX_F32)
			size.x = MarginedRight() - itemPos.x - rightIndent;
		else size.x = defaultSize.x;
		
		if (defaultSize.y == MAX_F32)
			size.y = MarginedBottom() - itemPos.y;
		else size.y = defaultSize.y;
	}
	
	NextItemSize.x = -1;
	NextItemSize.y = -1;
	return size;
}

FORCE_INLINE void SetWindowCursor(CursorType curtype) {DPZoneScoped;
	DeshWindow->SetCursor(curtype);
	StateAddFlag(UISCursorSet);
}

FORCE_INLINE vec2 GetTextScale() {DPZoneScoped;
	return vec2::ONE * style.fontHeight / (f32)style.font->max_height;
}


UIStyle& UI::GetStyle(){DPZoneScoped;
	return style;
}

UIWindow* UI::GetWindow() {DPZoneScoped;
	return curwin;
}

vec2 UI::GetLastItemPos() {DPZoneScoped;
	//Assert(curwin->items.count, "Attempt to get last item position without creating any items!");
	return curwin->items[currlayer].last->position;
}

vec2 UI::GetLastItemSize() {DPZoneScoped;
	//Assert(curwin->items.count, "Attempt to get last item size without creating any items!");
	return curwin->items[currlayer].last->size;
}

vec2 UI::GetLastItemScreenPos() {DPZoneScoped;
	//Assert(curwin->items.count, "Attempt to get last item position without creating any items!");
	return curwin->position + curwin->items[currlayer].last->position;
}

vec2 UI::GetWindowRemainingSpace() {DPZoneScoped;
	return vec2(MarginedRight() - curwin->curx, MarginedBottom() - curwin->cury);
}

vec2 UI::GetWinCursor() {DPZoneScoped;
	return PositionForNewItem();
}

u32 UI::GetCenterLayer() {DPZoneScoped;
	return UI_CENTER_LAYER;
}

f32 UI::GetBorderedRight()                {DPZoneScoped; return BorderedRight(); }
f32 UI::GetBorderedLeft()                 {DPZoneScoped; return BorderedLeft(); }
f32 UI::GetBorderedTop()                  {DPZoneScoped; return BorderedTop(); }
f32 UI::GetBorderedBottom()               {DPZoneScoped; return BorderedBottom(); }
f32 UI::GetMarginedRight()                {DPZoneScoped; return MarginedRight(); }
f32 UI::GetMarginedLeft()                 {DPZoneScoped; return MarginedLeft(); }
f32 UI::GetMarginedTop()                  {DPZoneScoped; return MarginedTop(); }
f32 UI::GetMarginedBottom()               {DPZoneScoped; return MarginedBottom(); }
f32 UI::GetClientRight()             	  {DPZoneScoped; return ClientRight(); }
f32 UI::GetClientLeft()              	  {DPZoneScoped; return ClientLeft(); }
f32 UI::GetClientTop()               	  {DPZoneScoped; return ClientTop(); }
f32 UI::GetClientBottom()            	  {DPZoneScoped; return ClientBottom(); }
pair<vec2, vec2> UI::GetBorderedArea()    {DPZoneScoped; return BorderedArea(); }
pair<vec2, vec2> UI::GetMarginedArea()    {DPZoneScoped; return MarginedArea(); }
pair<vec2, vec2> UI::GetClientArea() {DPZoneScoped; return ScrollBaredArea(); }

//returns the cursor to the same line as the previous and moves it to the right by the 
//width of the object
void UI::SameLine() {DPZoneScoped;
	//Assert(curwin->items.count, "Attempt to sameline an item creating any items!");
	if (curwin->items[currlayer].last) {
		curwin->cursor.y = curwin->items[currlayer].last->initialCurPos.y;
		curwin->cursor.x += curwin->items[currlayer].last->initialCurPos.x + curwin->items[currlayer].last->size.x + style.itemSpacing.x;
	}
}

void UI::SetWinCursor(vec2 pos){
	NextCursorPos=pos;
}

void UI::SetWinCursorX(f32 x){
	NextCursorPos.x=x;
}

void UI::SetWinCursorY(f32 y){
	NextCursorPos.y=y;
}	


void UI::SetScroll(vec2 scroll) {DPZoneScoped;
	if (scroll.x == MAX_F32)
		curwin->scx = curwin->maxScroll.x;
	else
		curwin->scx = scroll.x;
	
	if (scroll.y == MAX_F32)
		curwin->scy = curwin->maxScroll.y;
	else
		curwin->scy = scroll.y;
}

void UI::SetNextItemActive() {DPZoneScoped;
	StateAddFlag(UISNextItemActive);
}

void UI::SetNextItemSize(vec2 size) {DPZoneScoped;
	NextItemSize = size;
}

void UI::SetMarginPositionOffset(vec2 offset) {DPZoneScoped;
	MarginPositionOffset = offset;
}

void UI::SetMarginSizeOffset(vec2 offset){DPZoneScoped;
	MarginSizeOffset = offset;
}

void UI::SetNextItemMinSizeIgnored() {DPZoneScoped;
	StateAddFlag(UISNextItemMinSizeIgnored);
}

void UI::SetPreventInputs() {DPZoneScoped;
	inputState = ISExternalPreventInputs;
}

void UI::SetAllowInputs() {DPZoneScoped;
	if(inputState == ISExternalPreventInputs)
		AllowInputs;
}


//internal last item getter, returns nullptr if there are none
FORCE_INLINE UIItem* UI::GetLastItem(u32 layeroffset) {DPZoneScoped;
	return curwin->items[currlayer + layeroffset].last;
}

//helper for making any new UIItem, since now we must work with item pointers internally
//this function also decides if we are working with a new item or continuing to work on a previous
inline UIItem* BeginItem(UIItemType type, u32 userflags = 0, u32 layeroffset = 0) {DPZoneScoped;
	
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
		curwin->items[currlayer + layeroffset].add(UIItem{ type, curwin->cursor, style });
#ifdef BUILD_INTERNAL
		UI::GetLastItem(layeroffset)->item_layer = currlayer + layeroffset;
		UI::GetLastItem(layeroffset)->item_idx = curwin->items[currlayer + layeroffset].count;
		BreakOnItem;
#endif
	}
	if (StateHasFlag(UISNextItemMinSizeIgnored)) {
		UI::GetLastItem(layeroffset)->trackedForMinSize = 0;
		StateRemoveFlag(UISNextItemMinSizeIgnored);
	}
	
	GetDefaultItemFlags(type, UI::GetLastItem(layeroffset)->flags);
	AddFlag(UI::GetLastItem()->flags, userflags);
	
	ui_stats.items++;
	curwin->items_count++;
	return UI::GetLastItem(layeroffset);
}

inline void EndItem(UIItem* item) {DPZoneScoped;}

//this is for debugging debug cmds, all it does extra is hash the drawCmd
//so we can break on it later
inline void AddDrawCmd(UIItem* item, UIDrawCmd& drawCmd) {DPZoneScoped;
	if (!drawCmd.tex) drawCmd.tex = style.font->tex;
	drawCmd.hash = hash<UIDrawCmd>{}(drawCmd);
	drawCmd.render_surface_target_idx = *drawTargetStack.last;
	item->drawCmds.add(drawCmd);
	ui_stats.draw_cmds++;
	ui_stats.vertices += drawCmd.counts.x;
	ui_stats.indices += drawCmd.counts.y;
	Assert(drawCmd.counts.x < UIDRAWCMD_MAX_VERTICES);
	Assert(drawCmd.counts.y < UIDRAWCMD_MAX_INDICES);
	BreakOnDrawCmdCreation;
}

//4 verts, 6 indices
FORCE_INLINE vec2
MakeLine(Vertex2* putverts, u32* putindices, vec2 offsets, vec2 start, vec2 end, f32 thickness, color color) {DPZoneScoped;
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
MakeLine(UIDrawCmd& drawCmd, vec2 start, vec2 end, f32 thickness, color color) {DPZoneScoped;
	drawCmd.counts += MakeLine(drawCmd.vertices, drawCmd.indices, drawCmd.counts, start, end, thickness, color);
	drawCmd.type = UIDrawType_Line;
}

//3 verts, 3 indices
FORCE_INLINE vec2 
MakeFilledTriangle(Vertex2* putverts, u32* putindices, vec2 offsets, vec2 p1, vec2 p2, vec2 p3, color color) {DPZoneScoped;
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
MakeFilledTriangle(UIDrawCmd& drawCmd, vec2 p1, vec2 p2, vec2 p3, color color) {DPZoneScoped;
	drawCmd.counts += MakeFilledTriangle(drawCmd.vertices, drawCmd.indices, drawCmd.counts, p1, p2, p3, color);
	drawCmd.type = UIDrawType_FilledTriangle;
}

FORCE_INLINE vec2
MakeTriangle(Vertex2* putverts, u32* putindices, vec2 offsets, vec2 p0, vec2 p1, vec2 p2, f32 thickness, color color) {DPZoneScoped;
	Assert(putverts && putindices);
	if (color.a == 0) return vec2::ZERO;
	
	u32     col = color.rgba;
	Vertex2* vp = putverts + (u32)offsets.x;
	u32*     ip = putindices + (u32)offsets.y;
	
	MakeLine(vp, ip, vec2::ZERO,  p0, p1, 1, color);
	MakeLine(vp, ip, vec2(4, 6),  p1, p2, 1, color);
	MakeLine(vp, ip, vec2(8, 12), p2, p0, 1, color);
	
	return vec2(12, 18);
	
	//TODO(sushi) this should be fixed to replace reliance on MakeLine
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
MakeTriangle(UIDrawCmd& drawCmd, vec2 p1, vec2 p2, vec2 p3, f32 thickness, color color) {DPZoneScoped;
	drawCmd.counts += MakeTriangle(drawCmd.vertices, drawCmd.indices, drawCmd.counts, p1, p2, p3, thickness, color);
	drawCmd.type = UIDrawType_Triangle;
}

//4 verts, 6 indices
FORCE_INLINE vec2
MakeFilledRect(Vertex2* putverts, u32* putindices, vec2 offsets, vec2 pos, vec2 size, color color) {DPZoneScoped;
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
MakeFilledRect(UIDrawCmd& drawCmd, vec2 pos, vec2 size, color color) {DPZoneScoped;
	drawCmd.counts += MakeFilledRect(drawCmd.vertices, drawCmd.indices, drawCmd.counts, pos, size, color);
	drawCmd.type = UIDrawType_FilledRectangle;
}
//8 verts, 24 indices
FORCE_INLINE vec2
MakeRect(Vertex2* putverts, u32* putindices, vec2 offsets, vec2 pos, vec2 size, f32 thickness, color color) {DPZoneScoped;
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
MakeRect(UIDrawCmd& drawCmd, vec2 pos, vec2 size, f32 thickness, color color) {DPZoneScoped;
	drawCmd.counts += MakeRect(drawCmd.vertices, drawCmd.indices, drawCmd.counts, pos, size, thickness, color);
	drawCmd.type = UIDrawType_Rectangle;
}

FORCE_INLINE vec2
MakeCircle(Vertex2* putverts, u32* putindices, vec2 offsets, vec2 pos, f32 radius, u32 subdivisions_int, f32 thickness, color color) {DPZoneScoped;
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
MakeCircle(UIDrawCmd& drawCmd, vec2 pos, f32 radius, u32 subdivisions, f32 thickness, color color) {DPZoneScoped;
	drawCmd.counts += MakeCircle(drawCmd.vertices, drawCmd.indices, drawCmd.counts, pos, radius, subdivisions, thickness, color);
	drawCmd.type = UIDrawType_Circle;
}

FORCE_INLINE vec2 
MakeFilledCircle(Vertex2* putverts, u32* putindices, vec2 offsets, vec2 pos, f32 radius, u32 subdivisions_int, color color) {DPZoneScoped;
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
MakeFilledCircle(UIDrawCmd& drawCmd, vec2 pos, f32 radius, u32 subdivisions_int, color color) {DPZoneScoped;
	drawCmd.counts += MakeFilledCircle(drawCmd.vertices, drawCmd.indices, drawCmd.counts, pos, radius, subdivisions_int, color);
	drawCmd.type = UIDrawType_FilledCircle;
}

FORCE_INLINE vec2
MakeText(Vertex2* putverts, u32* putindices, vec2 offsets, cstring text, vec2 pos, color color, vec2 scale) {DPZoneScoped;
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
MakeText(UIDrawCmd& drawCmd, cstring text, vec2 pos, color color, vec2 scale) {DPZoneScoped;
	drawCmd.counts += MakeText(drawCmd.vertices, drawCmd.indices, drawCmd.counts, text, pos, color, scale);
	drawCmd.tex = style.font->tex;
	drawCmd.type = UIDrawType_Text;
}

FORCE_INLINE vec2
MakeText(Vertex2* putverts, u32* putindices, vec2 offsets, wcstring text, vec2 pos, color color, vec2 scale) {DPZoneScoped;
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
MakeText(UIDrawCmd& drawCmd, wcstring text, vec2 pos, color color, vec2 scale) {DPZoneScoped;
	drawCmd.counts += MakeText(drawCmd.vertices, drawCmd.indices, drawCmd.counts, text, pos, color, scale);
	drawCmd.type = UIDrawType_Text;
}

FORCE_INLINE vec2 
MakeTexture(Vertex2* putverts, u32* putindices, vec2 offsets, Texture* texture, vec2 p0, vec2 p1, vec2 p2, vec2 p3, f32 alpha, b32 flipx = 0, b32 flipy = 0) {DPZoneScoped;
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
	
	if (flipx) {
		vec2 u0 = vp[0].uv, u1 = vp[1].uv, u2 = vp[2].uv, u3 = vp[3].uv;
		vp[0].uv = u1; vp[1].uv = u0; vp[2].uv = u3; vp[3].uv = u2;
	}
	if (flipy) {
		vec2 u0 = vp[0].uv, u1 = vp[1].uv, u2 = vp[2].uv, u3 = vp[3].uv;
		vp[0].uv = u3; vp[1].uv = u2; vp[2].uv = u1; vp[3].uv = u0;
	}
	
	return vec2(4, 6);
}

FORCE_INLINE void
MakeTexture(UIDrawCmd& drawCmd, Texture* texture, vec2 pos, vec2 size, f32 alpha, b32 flipx = 0, b32 flipy = 0) {DPZoneScoped;
	drawCmd.counts += MakeTexture(drawCmd.vertices, drawCmd.indices, drawCmd.counts, texture, pos, pos + size.ySet(0), pos + size, pos + size.xSet(0), alpha, flipx, flipy);
	drawCmd.type = UIDrawType_Image;
	drawCmd.tex = texture;
}

//internal debug drawing functions
void DebugTriangle(vec2 p0, vec2 p1, vec2 p2, color col = Color_Red) {DPZoneScoped;
	UIDrawCmd dc;
	MakeTriangle(dc, p0, p1, p2, 1, col);
	debugCmds.add(dc);
}

void DebugTriangleFilled(vec2 p0, vec2 p1, vec2 p2, color col = Color_Red) {DPZoneScoped;
	UIDrawCmd dc;
	MakeFilledTriangle(dc, p0, p1, p2, col);
	debugCmds.add(dc);
}

void DebugRect(vec2 pos, vec2 size, color col = Color_Red) {DPZoneScoped;
	UIDrawCmd dc;
	MakeRect(dc, pos, size, 1, col);
	debugCmds.add(dc);
}

void DebugRectFilled(vec2 pos, vec2 size, color col = Color_Red) {DPZoneScoped;
	UIDrawCmd dc;
	MakeFilledRect(dc, pos, size, col);
	debugCmds.add(dc);
}

void DebugCircle(vec2 pos, f32 radius, color col = Color_Red) {DPZoneScoped;
	UIDrawCmd dc;
	MakeCircle(dc, pos, radius, 20, 1, col);
	debugCmds.add(dc);
}

void DebugCircleFilled(vec2 pos, f32 radius, color col = Color_Red) {DPZoneScoped;
	UIDrawCmd dc;
	MakeFilledCircle(dc, pos, radius, 20, col);
	debugCmds.add(dc);
}

void DebugLine(vec2 pos, vec2 pos2, color col = Color_Red) {DPZoneScoped;
	UIDrawCmd dc;
	MakeLine(dc, pos, pos2, 1, col);
	debugCmds.add(dc);
}

void DebugText(vec2 pos, char* text, color col = Color_White) {DPZoneScoped;
	UIDrawCmd dc;
	MakeText(dc, cstring{ text, strlen(text) }, pos, col, vec2::ONE);
	debugCmds.add(dc);
}

void UI::BeginRow(const char* label, u32 columns, f32 rowHeight, UIRowFlags flags) {DPZoneScoped;
	Assert(!StateHasFlag(UISRowBegan), "Attempted to start a new Row without finishing one already in progress!");
	if (!rows.has(label)) { 
		rows.add(label); 
		row = rows.at(label);
		row->columns.resize(columns);
		row->flags = flags;
		row->height = rowHeight;
		row->label = label;
		forI(columns) row->columns[i] = { 0.f,false };
	}
	
	
	row = rows.at(label);
	row->position = PositionForNewItem();
	row->yoffset = 0;
	row->xoffset = 0;
	StateAddFlag(UISRowBegan);
}

void UI::EndRow() {DPZoneScoped;
	Assert(StateHasFlag(UISRowBegan), "Attempted to a end a row without calling BeginRow() first!");
	Assert(row->item_count % row->columns.count == 0, "Attempted to end a Row without giving the correct amount of items!");
	
	if (HasFlag(row->flags, UIRowFlags_AutoSize)) {
		if (row->reeval_height) row->height = row->max_height;
		for (UIColumn& col : row->columns)
			if (col.reeval_width) col.width = col.max_width;
		
		if (row->max_height_frame < row->max_height) row->max_height = row->max_height_frame;
		
	}
	
	if (HasFlag(row->flags, UIRowFlags_FitWidthOfArea)) {
		//TODO set up Row fitting relative to given edges
	}
	
	row->max_height_frame = 0;
	curwin->cursor = vec2{ 0, row->position.y + row->yoffset + style.itemSpacing.y - style.windowPadding.y + curwin->scroll.y };
	StateRemoveFlag(UISRowBegan);
}

//this function sets up a static column width for a specified column that does not respect the size of the object
void UI::RowSetupColumnWidth(u32 column, f32 width) {DPZoneScoped;
	Assert(StateHasFlag(UISRowBegan), "Attempted to set a column's width with no Row in progress!");
	Assert(column <= row->columns.count, "Attempted to set a column who doesn't exists width!");
	if(!HasFlag(row->flags, UIRowFlags_AutoSize))
		row->columns[column] = { width, false };
}

//this function sets up static column widths that do not respect the size of the item at all
void UI::RowSetupColumnWidths(array<f32> widths) {DPZoneScoped;
	Assert(StateHasFlag(UISRowBegan), "Attempted to pass column widths without first calling BeginRow()!");
	Assert(widths.count == row->columns.count, "Passed in the wrong amount of column widths for in progress Row");
	if(!HasFlag(row->flags, UIRowFlags_AutoSize))
		forI(row->columns.count)
		row->columns[i] = { widths[i], false };
}

//see the function below for what this does
void UI::RowSetupRelativeColumnWidth(u32 column, f32 width) {DPZoneScoped;
	Assert(StateHasFlag(UISRowBegan), "Attempted to set a column's width with no Row in progress!");
	Assert(column <= row->columns.count, "Attempted to set a column who doesn't exists width!");
	if(!HasFlag(row->flags, UIRowFlags_AutoSize))
		row->columns[column] = { width, true };
}

//this function sets it so that column widths are relative to the size of the item the cell holds
//meaning it should be passed something like 1.2 or 1.3, indicating that the column should have a width of 
//1.2 * width of the item
void UI::RowSetupRelativeColumnWidths(array<f32> widths) {DPZoneScoped;
	Assert(StateHasFlag(UISRowBegan), "Attempted to pass column widths without first calling BeginRow()!");
	Assert(widths.count == row->columns.count, "Passed in the wrong amount of column widths for in progress Row");
	if(!HasFlag(row->flags, UIRowFlags_AutoSize))
		forI(row->columns.count)
		row->columns[i] = { widths[i], true };
}

void UI::RowFitBetweenEdges(array<f32> ratios, f32 left_edge, f32 right_edge) {DPZoneScoped;
	Assert(StateHasFlag(UISRowBegan), "Attempted to pass column widths without first calling BeginRow()!");
	Assert(ratios.count == row->columns.count);
	AddFlag(row->flags, UIRowFlags_FitWidthOfArea);
	row->left_edge = left_edge;
	row->right_edge = right_edge;
	f32 ratio_sum = 0;
	forI(row->columns.count) {
		ratio_sum += ratios[i];
		row->columns[i].width = ratios[i];
	}
	
	Assert(1 - ratio_sum < 0.999998888f, "ratios given do not add up to one!");
}

void UI::RowSetupColumnAlignments(array<vec2> alignments) {DPZoneScoped;
	Assert(StateHasFlag(UISRowBegan), "Attempted to pass column widths without first calling BeginRow()!");
	Assert(alignments.count == row->columns.count);
	forI(row->columns.count)
		row->columns[i].alignment = alignments[i];
}

//@Behavoir functions
//these functions generalize behavoir that are used by several things

enum ButtonType_ {
	ButtonType_TrueOnHold,
	ButtonType_TrueOnRelease,
	ButtonType_TrueOnPressed,
}; typedef u32 ButtonType;

b32 ButtonBehavoir(ButtonType type) {DPZoneScoped;
	switch (type) {
		case ButtonType_TrueOnHold: {
			if (DeshInput->LMouseDown()) { PreventInputs; return true; }
			else return false;
		}break;
		case ButtonType_TrueOnRelease: {
			PreventInputs;
			if (DeshInput->LMouseReleased()) return true;
			else return false;
		}break;
		case ButtonType_TrueOnPressed: {
			if (DeshInput->LMousePressed()) {
				PreventInputs;
				return true;
			}
		}break;
	}
	return false;
}

//returns true if buffer was changed
b32 TextInputBehavoir(void* buff, u32 buffSize, b32 unicode, upt& charCount, u32& cursor) {DPZoneScoped;
	persist TIMER_START(throttle);
	
	auto insert = [&](char c, u32 idx) {
		memmove((char*)buff + idx + 1, (char*)buff + idx, (buffSize - idx) * u8size);
		((char*)buff)[idx] = c;
		charCount++;
	};
	
	auto erase = [&](u32 idx) {
		if (charCount == 1) memset((char*)buff, 0, buffSize);
		else memmove((char*)buff + idx, (char*)buff + idx + 1, (charCount-- - idx) * u8size);
	};
	
	auto winsert = [&](wchar c, u32 idx) {
		memmove((wchar*)buff + idx + 1, (wchar*)buff + idx, (buffSize - idx) * wcharsize);
		((wchar*)buff)[idx] = c;
		charCount++;
	};
	
	auto werase = [&](u32 idx) {
		if (charCount == 1) memset((wchar*)buff, 0, buffSize);
		else memmove((wchar*)buff + idx, (wchar*)buff + idx + 1, (charCount-- - idx) * wcharsize);
	};
	
	
	forI(DeshInput->charCount) {
		if (charCount < buffSize) {
			if (unicode) winsert((wchar)DeshInput->charIn[i], cursor++);
			else insert((char)DeshInput->charIn[i], cursor++);
		}
	}
	
	if (charCount) {
		if (DeshInput->time_key_held < 500) {
			if (DeshInput->KeyPressed(Key::BACKSPACE) && cursor != 0) {
				if (unicode) werase(--cursor);
				else erase(--cursor);
				return true;
			}
			else if (DeshInput->KeyPressed(Key::DELETE) && cursor != charCount) {
				if (unicode) werase(cursor);
				else erase(cursor);
				return true;
			}
		}
		else if (TIMER_END(throttle) > 25) {
			if (DeshInput->KeyDown(Key::BACKSPACE) && cursor != 0) {
				if (unicode) werase(--cursor);
				else erase(--cursor);
				return true;
			}
			else if (DeshInput->KeyDown(Key::DELETE) && cursor != charCount) {
				if (unicode) werase(cursor);
				else erase(cursor);
				return true;
			}
			TIMER_RESET(throttle);
		}
	}
	return false;
}


//@Primitive Items
//NOTE item pos conflicts with vertex positions, so set only one (see DrawItem())


void UI::Rect(vec2 pos, vec2 dimen, color color) {DPZoneScoped;
	UIItem       item{ UIItemType_Abstract, curwin->cursor, style };
	UIDrawCmd drawCmd;
	MakeRect(drawCmd, vec2::ZERO, dimen, 1, color);
	AddDrawCmd(&item, drawCmd);
	
	item.position = pos;
	item.size = dimen;
	curwin->items[currlayer].add(item);
}

void UI::RectFilled(vec2 pos, vec2 dimen, color color) {DPZoneScoped;
	UIItem       item{ UIItemType_Abstract, curwin->cursor, style };
	UIDrawCmd drawCmd;
	MakeFilledRect(drawCmd, vec2::ZERO, dimen, color);
	AddDrawCmd(&item, drawCmd);
	
	item.position = pos;
	item.size = dimen;
	curwin->items[currlayer].add(item);
}


//@Line


void UI::Line(vec2 start, vec2 end, f32 thickness, color color){DPZoneScoped;
	UIItem       item{ UIItemType_Abstract, curwin->cursor, style };
	UIDrawCmd drawCmd;
	MakeLine(drawCmd, start, end, thickness, color);
	AddDrawCmd(&item, drawCmd);
	
	item.position = vec2::ZERO;// { Min(drawCmd.position.x, drawCmd.position2.x), Min(drawCmd.position.y, drawCmd.position2.y) };
	item.    size = Max(start, end) - item.position;
	curwin->items[currlayer].add(item);
}


//@Circle


void UI::Circle(vec2 pos, f32 radius, f32 thickness, u32 subdivisions, color color) {DPZoneScoped;
	UIItem       item{ UIItemType_Abstract, curwin->cursor, style };
	UIDrawCmd drawCmd;
	MakeCircle(drawCmd, pos, radius, subdivisions, thickness, color);
	AddDrawCmd(&item, drawCmd);
	
	item.position = vec2::ZERO;//pos - vec2::ONE * radius;
	item.size = vec2::ONE * radius * 2;
	
	//item.drawCmds.add(drawCmd);
	curwin->items[currlayer].add(item);
	
}

void UI::CircleFilled(vec2 pos, f32 radius, u32 subdivisions, color color) {DPZoneScoped;
	UIItem       item{ UIItemType_Abstract, curwin->cursor, style };
	UIDrawCmd drawCmd;
	MakeFilledCircle(drawCmd, pos, radius, subdivisions, color);
	AddDrawCmd(&item, drawCmd);
	
	item.position = vec2::ZERO;//pos - vec2::ONE * radius;
	item.size = vec2::ONE * radius * 2;
	
	//item.drawCmds.add(drawCmd);
	curwin->items[currlayer].add(item);
}


/////////////////////
////		   	 ////
////    Items    ////
////			 ////
/////////////////////


//@Text


//internal function for actually making and adding the drawCmd
local void TextCall(char* text, vec2 pos, color color, UIItem* item) {DPZoneScoped;
	UIDrawCmd drawCmd;
	MakeText(drawCmd, cstring{ text, strlen(text) }, pos, color, GetTextScale());
	AddDrawCmd(item, drawCmd);
}

//secondary, for unicode
local void TextCall(wchar* text, vec2 pos, color color, UIItem* item) {DPZoneScoped;
	UIDrawCmd drawCmd;
	MakeText(drawCmd, wcstring{ text, wcslen(text) }, pos, color, GetTextScale());
	AddDrawCmd(item, drawCmd);
}

//main function for wrapping, where position is starting position of text relative to the top left of the window
//this function also decides if text is to be wrapped or not, and if not simply calls TextEx (to clean up the Text() functions)
local void TextW(const char* in, vec2 pos, color color, b32 nowrap, b32 move_cursor = true) {DPZoneScoped;
	
	using namespace UI;
	UIItem* item = BeginItem(UIItemType_Text);
	item->position = pos;
	
	if (!nowrap) {
		string text = in;
		
		//we split string by newlines and put them into here 
		//maybe make this into its own function
		array<string> newlined;
		
		u32 newline = text.findFirstChar('\n');
		if (newline != npos && newline != text.count - 1) {
			string remainder = text.substr(newline + 1);
			newlined.add(text.substr(0, newline - 1));
			newline = remainder.findFirstChar('\n');
			while (newline != npos) {
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
							string nustr = t.substr(0, (lastspc == npos) ? i - 1 : lastspc);
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
						string nustr = t.substr(0, (splitat == npos) ? maxChars - 1 : splitat);
						TextCall(nustr.str, workcur, color, item);
						
						if (nustr.count == t.count) continue;
						
						t = t.substr(nustr.count);
						workcur.y += style.fontHeight + style.itemSpacing.y;
						
						//continue to wrap if we need to
						while (t.count > maxChars) {
							splitat = t.findLastChar(' ', maxChars);
							nustr = t.substr(0, (splitat == npos) ? maxChars - 1 : splitat);
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
local void TextW(const wchar* in, vec2 pos, color color, b32 nowrap, b32 move_cursor = true) {DPZoneScoped;
	
	using namespace UI;
	UIItem* item = BeginItem(UIItemType_Text);
	item->position = pos;
	
	if (!nowrap) {
		wstring text = in;
		
		//we split wstring by newlines and put them into here 
		//maybe make this into its own function
		array<wstring> newlined;
		
		u32 newline = text.findFirstChar('\n');
		if (newline != npos && newline != text.count - 1) {
			wstring remainder = text.substr(newline + 1);
			newlined.add(text.substr(0, newline - 1));
			newline = remainder.findFirstChar('\n');
			while (newline != npos) {
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
				f32 maxw = MarginedRight() - item->position.x - rightIndent;
				f32 currlinew = 0;
				
				for (wstring& t : newlined) {
					for (int i = 0; i < t.count; i++) {
						currlinew += font->GetPackedChar(t[i])->xadvance * wscale;
						
						if (currlinew >= maxw) {
							
							//find closest space to split by, if none we just split the word
							u32 lastspc = t.findLastChar(' ', i);
							wstring nustr = t.substr(0, (lastspc == npos) ? i - 1 : lastspc);
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
				u32 maxChars = u32(floor(MarginedRight() - item->position.x - rightIndent) / style.font->max_width);
				
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
						wstring nustr = t.substr(0, (splitat == npos) ? maxChars - 1 : splitat);
						TextCall(nustr.str, workcur, color, item);
						
						if (nustr.count == t.count) continue;
						
						t = t.substr(nustr.count);
						workcur.y += style.fontHeight + style.itemSpacing.y;
						
						//continue to wrap if we need to
						while (t.count > maxChars) {
							splitat = t.findLastChar(' ', maxChars);
							nustr = t.substr(0, (splitat == npos) ? maxChars - 1 : splitat);
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

void UI::Text(const char* text, UITextFlags flags) {DPZoneScoped;
	GetDefaultItemFlags(UIItemType_Text, flags);
	TextW(text, PositionForNewItem(), style.colors[UIStyleCol_Text], HasFlag(flags, UITextFlags_NoWrap));
}

void UI::Text(const char* text, vec2 pos, UITextFlags flags) {DPZoneScoped;
	GetDefaultItemFlags(UIItemType_Text, flags);
	TextW(text, pos, style.colors[UIStyleCol_Text], HasFlag(flags, UITextFlags_NoWrap), 0);
}

void UI::Text(const wchar* text, UITextFlags flags){DPZoneScoped;
	GetDefaultItemFlags(UIItemType_Text, flags);
	TextW(text, PositionForNewItem(), style.colors[UIStyleCol_Text], HasFlag(flags, UITextFlags_NoWrap));
}

void UI::Text(const wchar* text, vec2 pos, UITextFlags flags){DPZoneScoped;
	GetDefaultItemFlags(UIItemType_Text, flags);
	TextW(text, pos, style.colors[UIStyleCol_Text], HasFlag(flags, UITextFlags_NoWrap), 0);
}

void UI::TextF(const char* fmt, ...) {DPZoneScoped;
	string s;
	va_list argptr;
	va_start(argptr, fmt);
	s.count  = vsnprintf(nullptr, 0, fmt, argptr);
	s.str   = (char*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
	s.allocator->commit(s.str, s.count+1);
	s.space = s.count+1;
	vsnprintf(s.str, s.count+1, fmt, argptr);
	va_end(argptr);
	TextW(s.str, PositionForNewItem(), style.colors[UIStyleCol_Text], false);
}

//@Button

b32 UI::Button(const char* text, vec2 pos, UIButtonFlags flags) {DPZoneScoped;
	UIItem* item = BeginItem(UIItemType_Button, flags);
	item->position = pos;
	//item->size = DecideItemSize(vec2(Min(MarginedRight() - item->position.x, Max(50.f, CalcTextSize(text).x * 1.1f)), style.fontHeight * style.buttonHeightRelToFont), item->position);
	item->size = DecideItemSize(vec2(Max(50.f, CalcTextSize(text).x * 1.1f), style.fontHeight * style.buttonHeightRelToFont), item->position);
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
	
	//border
	f32 border_size = (HasFlag(item->flags, UIButtonFlags_NoBorder)) ? 0 : style.buttonBorderSize;
	if(border_size > M_EPSILON){
		UIDrawCmd drawCmd;
		vec2  borpos = vec2::ZERO;
		vec2  bordim = item->size;
		color borcol = style.colors[UIStyleCol_ButtonBorder];
		MakeRect(drawCmd, borpos, bordim, border_size, borcol);
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
		if (HasFlag(item->flags, UIButtonFlags_ReturnTrueOnHold))
			return ButtonBehavoir(ButtonType_TrueOnHold);
		if (HasFlag(item->flags, UIButtonFlags_ReturnTrueOnRelease))
			return ButtonBehavoir(ButtonType_TrueOnRelease);
		return ButtonBehavoir(ButtonType_TrueOnPressed);
	}
	return false;
}

b32 UI::Button(const char* text, UIButtonFlags flags) {DPZoneScoped;
	return Button(text, PositionForNewItem(), flags);
}

//@Checkbox

void UI::Checkbox(string label, b32* b) {DPZoneScoped;
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
		vec2  dimensions = boxsiz;
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
b32 UI::BeginCombo(const char* label, const char* prev_val, vec2 pos) {DPZoneScoped;
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
		PushVar(UIStyleVar_WindowMargins, vec2(0, 0));
		PushVar(UIStyleVar_ItemSpacing, vec2(0, 0));
		//TODO BUG the combo selection popout doesnt actually follow the button
		SetNextWindowSize(vec2(item->size.x, style.fontHeight * style.selectableHeightRelToFont * 8));
		BeginPopOut(toStr("comboPopOut", label).str, item->position.yAdd(item->size.y), vec2::ZERO, UIWindowFlags_NoBorder);
		StateAddFlag(UISComboBegan);
		return true;
	}
	return false;
}

b32 UI::BeginCombo(const char* label, const char* prev_val) {DPZoneScoped;
	return BeginCombo(label, prev_val, PositionForNewItem());
}



void UI::EndCombo() {DPZoneScoped;
	Assert(StateHasFlag(UISComboBegan), "Attempted to end a combo without calling BeginCombo first, or EndCombo was called for a combo that was not open!");
	StateRemoveFlag(UISComboBegan);
	EndPopOut();
	PopVar(2);
}

//@Selectable

b32 SelectableCall(const char* label, vec2 pos, b32 selected, b32 move_cursor = 1) {DPZoneScoped;
	UIItem* item = BeginItem(UIItemType_Selectable, 0, 0);
	item->position = pos;
	
	vec2 defsize;
	if (StateHasFlag(UISComboBegan)) {
		defsize = vec2(MAX_F32, style.fontHeight * style.selectableHeightRelToFont);
	}
	else defsize = vec2(UI::CalcTextSize(label).x, style.fontHeight * style.selectableHeightRelToFont);
	
	item->size = DecideItemSize(defsize, item->position);
	
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
			vec2((item->size.x - UI::CalcTextSize(label).x) * style.selectableTextAlign.x,
				 (style.fontHeight * style.selectableHeightRelToFont - style.fontHeight) * style.selectableTextAlign.y);
		
		color col = style.colors[UIStyleCol_Text];
		MakeText(drawCmd, cstring{ (char*)label, strlen(label) }, position, col, GetTextScale());
		AddDrawCmd(item, drawCmd);
	}
	
	
	return clicked;
}

b32 UI::Selectable(const char* label, vec2 pos, b32 selected) {DPZoneScoped;
	return SelectableCall(label, pos, selected, 0);
}

b32 UI::Selectable(const char* label, b32 selected) {DPZoneScoped;
	return SelectableCall(label, PositionForNewItem(), selected);
}

//@Header

b32 UI::BeginHeader(const char* label, UIHeaderFlags flags) {DPZoneScoped;
	UIItem* item = BeginItem(UIItemType_Header, flags);
	
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
	
	f32 arrowSpaceWidth = style.indentAmount;
	f32 arrowwidth = ceil(arrowSpaceWidth / 2);
	f32 arrowheight = ceil(item->size.y / 1.5);
	
	vec2 bgpos = vec2{ arrowSpaceWidth, 0 };
	vec2 bgdim = vec2{ item->size.x - bgpos.x, item->size.y };
	
	if (*open) { 
		if(!HasFlag(item->flags, UIHeaderFlags_NoIndentLeft))  PushLeftIndent(style.indentAmount + leftIndent); 
		if(!HasFlag(item->flags, UIHeaderFlags_NoIndentRight)) PushRightIndent(style.indentAmount + rightIndent);
	}
	
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
		
		
		//TODO(sushi) this is ugly please fix it 
		if (*open) { 
			vec2 arrowright((arrowSpaceWidth - arrowwidth) / 2, (item->size.y - arrowheight) / 2);
			vec2 arrowleft((arrowSpaceWidth + arrowwidth) / 2, (item->size.y - arrowheight) / 2);
			vec2 arrowpoint(arrowSpaceWidth / 2, (item->size.y + arrowheight) / 2);
			MakeFilledTriangle(drawCmd, arrowleft, arrowright, arrowpoint, col);
		}
		else {
			vec2 arrowtop((arrowSpaceWidth - arrowwidth) / 2, (item->size.y - arrowheight) / 2);
			vec2 arrowpoint((arrowSpaceWidth + arrowwidth) / 2, item->size.y / 2);
			vec2 arrowbot((arrowSpaceWidth - arrowwidth) / 2, (item->size.y + arrowheight) / 2);
			MakeFilledTriangle(drawCmd, arrowtop, arrowpoint, arrowbot, col);
		}
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
	
	if(!HasFlag(item->flags, UIHeaderFlags_NoBorder)){//border
		UIDrawCmd drawCmd;
		vec2  position = bgpos;
		vec2  dimensions = bgdim;
		color col = style.colors[UIStyleCol_HeaderBorder];
		MakeRect(drawCmd, position, dimensions, 1, col);
		AddDrawCmd(item, drawCmd);
	}
	
	
	return *open;
}

void UI::EndHeader() {DPZoneScoped;
	PopLeftIndent();
	PopRightIndent();
}

//@BeginTabBar

void UI::BeginTabBar(const char* label, UITabBarFlags flags){DPZoneScoped;
	Assert(!StateHasFlag(UISTabBarBegan), "attempt to start a new tab bar without finishing one");
	StateAddFlag(UISTabBarBegan);
	if (!tabBars.has(label)) tabBars.add(label);
	tabBar = tabBars.at(label);
	GetDefaultItemFlags(UIItemType_TabBar, flags);
	tabBar->flags = flags;
	
	UIItem* item = BeginItem(UIItemType_TabBar);
	item->position = PositionForNewItem();
	
	f32 tabBarLineHeight = 3;
	
	item->size = DecideItemSize(vec2(MAX_F32, style.tabHeightRelToFont * style.fontHeight + tabBarLineHeight), item->position);
	
	tabBar->tabHeight = style.tabHeightRelToFont * style.fontHeight;
	tabBar->item = item;
	
	AdvanceCursor(item);
	
	{//bar
		UIDrawCmd drawCmd;
		vec2  position = vec2(0, tabBar->tabHeight);
		vec2  dimensions = vec2(item->size.x, tabBarLineHeight);
		color col = style.colors[UIStyleCol_TabBar];
		MakeFilledRect(drawCmd, position, dimensions, col);
		AddDrawCmd(item, drawCmd);
	}
}

b32 UI::BeginTab(const char* label){DPZoneScoped;
	Assert(StateHasFlag(UISTabBarBegan), "attempt to begin a tab without beginning a tab bar first");
	
	UITab* tab = 0;
	if (!tabBar->tabs.has(label)) { 
		tabBar->tabs.add(label);
		tab = tabBar->tabs.at(label);
		tab->height = style.tabHeightRelToFont * style.fontHeight;
		tab->width = CalcTextSize(label).x * 1.2;
	}
	tab = tabBar->tabs.at(label);
	//UITab& tab = tabBar->tabs[label];
	
	UIItem* item = BeginItem(UIItemType_Tab);
	
	item->position = vec2(tabBar->item->position.x + tabBar->xoffset, tabBar->item->position.y);
	item->size = vec2(tab->width, tab->height);
	
	tab->item = item;
	
	b32 selected = tab == tabBar->tabs.atIdx(tabBar->selected);
	
	b32 active = isItemActive(item) || selected;
	
	tabBar->xoffset += tab->width + style.tabSpacing;
	
	{//background
		UIDrawCmd drawCmd;
		vec2  position = vec2::ZERO;
		vec2  dimensions = item->size;
		color col = style.colors[(active ? (LeftMousePressed ? UIStyleCol_TabBgActive : UIStyleCol_TabBgHovered) : UIStyleCol_TabBg) ];
		MakeFilledRect(drawCmd, position, dimensions, col);
		AddDrawCmd(item, drawCmd);
	}
	
	{//text
		UIDrawCmd drawCmd;
		drawCmd.scissorOffset = vec2::ZERO;
		drawCmd.scissorExtent = item->size;
		drawCmd.useWindowScissor = false;
		vec2 position =
			vec2((item->size.x - UI::CalcTextSize(label).x) * style.tabTextAlign.x,
				 ((style.fontHeight * style.tabHeightRelToFont - style.fontHeight) * style.tabTextAlign.y));
		color col = style.colors[UIStyleCol_Text];
		MakeText(drawCmd, cstring{ (char*)label, strlen(label) }, position, col, GetTextScale());
		AddDrawCmd(item, drawCmd);
	}
	
	{//border
		UIDrawCmd drawCmd;
		vec2  position = vec2::ZERO;
		vec2  dimensions = item->size;
		color col = style.colors[UIStyleCol_TabBorder];
		MakeRect(drawCmd, position, dimensions, 1, col);
		AddDrawCmd(item, drawCmd);
	}
	
	if (selected) {
		StateAddFlag(UISTabBegan);
		if (!HasFlag(tabBar->flags, UITabBarFlags_NoLeftIndent)) PushLeftIndent(style.indentAmount + leftIndent);
		if (!HasFlag(tabBar->flags, UITabBarFlags_NoRightIndent))PushRightIndent(style.indentAmount + rightIndent);
	}
	
	return selected;
}

void UI::EndTab() {DPZoneScoped;
	Assert(StateHasFlag(UISTabBegan), "attempt to end a tab without beginning one first");
	StateRemoveFlag(UISTabBegan);
	if (!HasFlag(tabBar->flags, UITabBarFlags_NoLeftIndent)) PopLeftIndent();
	if (!HasFlag(tabBar->flags, UITabBarFlags_NoRightIndent))PopRightIndent();
}

void UI::EndTabBar(){DPZoneScoped;
	Assert(!StateHasFlag(UISTabBegan), "attempt to end a tab bar without ending a tab");
	Assert(StateHasFlag(UISTabBarBegan), "attempt to end a tab bar without beginning one first");
	StateRemoveFlag(UISTabBarBegan);
	
	tabBar->xoffset = 0;
	forI(tabBar->tabs.count) {
		UIItem* item = tabBar->tabs.atIdx(i)->item;
		if (LeftMousePressed && MouseInWinArea(item->position, item->size)) {
			tabBar->selected = i;
		}
	}
	
}


//@Slider


void UI::Slider(const char* label, f32* val, f32 val_min, f32 val_max, UISliderFlags flags){DPZoneScoped;
	UIItem* item = BeginItem(UIItemType_Slider, flags);
	
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

void UI::Image(Texture* image, vec2 pos, f32 alpha, UIImageFlags flags) {DPZoneScoped;
	UIItem* item = BeginItem(UIItemType_Image, flags);
	
	item->position = pos;
	item->size = DecideItemSize(vec2((f32)image->width, (f32)image->height), item->position);
	
	AdvanceCursor(item);
	
	b32 flipx = HasFlag(item->flags, UIImageFlags_FlipX);
	b32 flipy = HasFlag(item->flags, UIImageFlags_FlipY);
	
	
	//TODO(sushi) image borders
	{//image
		UIDrawCmd drawCmd;
		vec2 position = vec2::ZERO;
		vec2 dimensions = item->size;
		MakeTexture(drawCmd, image, position, dimensions, alpha, flipx, flipy);
		AddDrawCmd(item, drawCmd);
	}
	
}

void UI::Image(Texture* image, f32 alpha, UIImageFlags flags) {DPZoneScoped;
	Image(image, PositionForNewItem(), alpha, flags);
}


void UI::Separator(f32 height) {DPZoneScoped;
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
b32 InputTextCall(const char* label, void* buff, u32 buffSize, b32 unicode, vec2 position, const char* preview, UIInputTextCallback callback, UIInputTextFlags flags, b32 moveCursor) {DPZoneScoped;
	GetDefaultItemFlags(UIItemType_InputText, flags);
	
	UIItem* item = BeginItem(UIItemType_InputText);
	
	UIInputTextState* state;
	
	upt charCount = (unicode ? wcslen((wchar*)buff) : strlen((char*)buff));
	
	item->position = position;
	
	vec2 dim;
	if (flags & UIInputTextFlags_FitSizeToText) {
		if(unicode) dim = UI::CalcTextSize((wchar*)buff);
		else dim = UI::CalcTextSize((char*)buff);
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
	data.buffer = (char*)buff;
	data.wbuffer = (wchar*)buff;
	data.selectionStart = state->selectStart;
	data.selectionEnd = state->selectEnd;
	b32 bufferChanged = 0;
	if (active) {
		if (DeshInput->KeyPressed(Key::RIGHT) && state->cursor < charCount) state->cursor++;
		if (DeshInput->KeyPressed(Key::LEFT) && state->cursor > 0) state->cursor--;
		if (DeshInput->KeyPressed(Key::HOME)) state->cursor = 0;
		if (DeshInput->KeyPressed(Key::END)) state->cursor = charCount;
		
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
		
		
		if (TextInputBehavoir(buff, buffSize, unicode, charCount, state->cursor)) {
			bufferChanged = 1;
			TIMER_RESET(state->timeSinceTyped);
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
		if (unicode) {
			if (preview && !((wchar*)buff)[0]) {
				color col = style.colors[UIStyleCol_Text];
				col.a = (u8)floor(0.5f * 255);
				MakeText(drawCmd, cstring{ (char*)preview, strlen(preview) }, position, col, GetTextScale());
				
			}
			else {
				color col = style.colors[UIStyleCol_Text];
				MakeText(drawCmd, wcstring{ (wchar*)buff, wcslen((wchar*)buff) }, position, col, GetTextScale());
			}
		}
		else {
			if (preview && !((char*)buff)[0]) {
				color col = style.colors[UIStyleCol_Text];
				col.a = (u8)floor(0.5f * 255);
				MakeText(drawCmd, cstring{ (char*)preview, strlen(preview) }, position, col, GetTextScale());
				
			}
			else {
				color col = style.colors[UIStyleCol_Text];
				MakeText(drawCmd, cstring{ (char*)buff, strlen((char*)buff) }, position, col, GetTextScale());
			}
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
	
	f32 lineoffset;
	if (unicode) lineoffset = UI::CalcTextSize(wcstring{ (wchar*)buff, state->cursor }).x;
	else lineoffset = UI::CalcTextSize(cstring{ (char*)buff, state->cursor }).x;
	
	
	
	if (active) {//cursor
		UIDrawCmd drawCmd;
		vec2  start = textStart +  vec2(lineoffset, 0);//vec2(state->cursor * style.font->max_width * style.fontHeight / style.font->aspect_ratio / style.font->max_width, 0);
		vec2  end = textStart + vec2(lineoffset, style.fontHeight - 1);
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

b32 UI::InputText(const char* label, char* buffer, u32 buffSize, const char* preview, UIInputTextFlags flags) {DPZoneScoped;
	return InputTextCall(label, buffer, buffSize, 0, PositionForNewItem(), preview, nullptr, flags, 1);
}

b32 UI::InputText(const char* label, char* buffer, u32 buffSize,  UIInputTextCallback callback, const char* preview, UIInputTextFlags flags) {DPZoneScoped;
	return InputTextCall(label, buffer, buffSize, 0, PositionForNewItem(), preview, callback, flags, 1);
}

b32 UI::InputText(const char* label, char* buffer, u32 buffSize, vec2 pos, const char* preview, UIInputTextFlags flags) {DPZoneScoped;
	return InputTextCall(label, buffer, buffSize, 0, pos, preview, nullptr, flags, 0);
}

b32 UI::InputText(const char* label, char* buffer, u32 buffSize, vec2 pos, UIInputTextCallback callback, const char* preview, UIInputTextFlags flags) {DPZoneScoped;
	return InputTextCall(label, buffer, buffSize, 0, pos, preview, callback, flags, 0);
}

b32 UI::InputText(const char* label, wchar* buffer, u32 buffSize, const char* preview, UIInputTextFlags flags) {DPZoneScoped;
	return InputTextCall(label, buffer, buffSize, 1, PositionForNewItem(), preview, nullptr, flags, 1);
}

b32 UI::InputText(const char* label, wchar* buffer, u32 buffSize, UIInputTextCallback callback, const char* preview, UIInputTextFlags flags) {DPZoneScoped;
	return InputTextCall(label, buffer, buffSize, 1, PositionForNewItem(), preview, callback, flags, 1);
}

b32 UI::InputText(const char* label, wchar* buffer, u32 buffSize, vec2 pos, const char* preview, UIInputTextFlags flags) {DPZoneScoped;
	return InputTextCall(label, buffer, buffSize, 1, pos, preview, nullptr, flags, 0);
}

b32 UI::InputText(const char* label, wchar* buffer, u32 buffSize, vec2 pos, UIInputTextCallback callback, const char* preview, UIInputTextFlags flags) {DPZoneScoped;
	return InputTextCall(label, buffer, buffSize, 1, pos, preview, callback, flags, 0);
}



//this doesnt need to be an enum at this point,
//but im making it one incase this function becomes more complicated in the future
enum CustomItemStage_{
	CISNone = 0,
	CISItemBegan = 1 << 0,
	CISItemAdvancedCursor = 1 << 1,
}; typedef u32 CustomItemStage;
CustomItemStage cistage = CISNone;

b32 UI::IsLastItemHovered(){DPZoneScoped; //TODO handle layers
	return WinHovered(curwin) && MouseInArea(GetLastItemScreenPos(), GetLastItemSize());
}

void UI::AddItemFlags(UIItemType type, Flags flags){DPZoneScoped;
	AddFlag(itemFlags[type], flags);
}

void UI::RemoveItemFlags(UIItemType type, Flags flags){DPZoneScoped;
	RemoveFlag(itemFlags[type], flags);
}

void UI::ResetItemFlags(UIItemType type){DPZoneScoped;
	itemFlags[type] = 0;
}



//@Utilities


//Push/Pop functions
void UI::PushColor(UIStyleCol idx, color color) {DPZoneScoped;
	//save old color
	colorStack.add(ColorMod{ idx, style.colors[idx] });
	//change to new color
	style.colors[idx] = color;
}

void UI::PushVar(UIStyleVar idx, f32 nuStyle) {DPZoneScoped;
	Assert(uiStyleVarTypes[idx].count == 1, "Attempt to use a f32 on a vec2 style variable!");
	f32* p = (f32*)((u8*)&style + uiStyleVarTypes[idx].offset);
	varStack.add(VarMod(idx, *p));
	*p = nuStyle;
}

void UI::PushVar(UIStyleVar idx, vec2 nuStyle) {DPZoneScoped;
	Assert(uiStyleVarTypes[idx].count == 2, "Attempt to use a f32 on a vec2 style variable!");
	vec2* p = (vec2*)((u8*)&style + uiStyleVarTypes[idx].offset);
	varStack.add(VarMod(idx, *p));
	*p = nuStyle;
}

void UI::PushFont(Font* font) {DPZoneScoped; 
	fontStack.add(style.font);
	style.font = font;
}

void UI::PushScale(vec2 scale) {DPZoneScoped;
	scaleStack.add(style.globalScale);
	style.globalScale = scale;
}

void UI::PushLayer(u32 layer) {DPZoneScoped;
	Assert(layer < UI_WINDOW_ITEM_LAYERS, "last layer is currently reserved by UI, increase the amount of layers in ui.h if you need more");
	layerStack.add(currlayer);
	currlayer = layer;
}

void UI::PushWindowLayer(u32 layer) {DPZoneScoped;
	WarnFuncNotImplemented("");
}

void UI::PushLeftIndent(f32 indent)	{DPZoneScoped;
	leftIndentStack.add(indent);
}

void UI::PushRightIndent(f32 indent){DPZoneScoped;
	rightIndentStack.add(indent);
}

void UI::PushDrawTarget(u32 idx){DPZoneScoped;
	Assert(idx < Render::GetMaxSurfaces());
	drawTargetStack.add(idx);
}

void UI::PushDrawTarget(Window* window){DPZoneScoped;
	Assert(window->renderer_surface_index != -1, "Attempt to push a draw target that has not been registered with the renderer");
	drawTargetStack.add(window->renderer_surface_index);
}

//we always leave the current color on top of the stack and the previous gets popped
void UI::PopColor(u32 count) {DPZoneScoped;
	Assert(count <= colorStack.count - initColorStackSize);
	//Assert(count < colorStack.size() - 1, "Attempt to pop too many colors!");
	while (count-- > 0) {
		style.colors[(colorStack.last)->element] = colorStack.last->oldCol;
		colorStack.pop();
	}
}

void UI::PopVar(u32 count) {DPZoneScoped;
	Assert(count <= varStack.count - initStyleStackSize);
	while (count-- > 0) {
		UIStyleVarType type = uiStyleVarTypes[varStack.last->var];
		if (type.count == 1) {
			f32* p = (f32*)((u8*)&style + type.offset);
			*p = varStack.last->oldFloat[0];
		}
		else {
			vec2* p = (vec2*)((u8*)&style + type.offset);
			*p = vec2(varStack.last->oldFloat[0], varStack.last->oldFloat[1]);
		}
		varStack.pop();
	}
}

void UI::PopFont(u32 count) {DPZoneScoped;
	Assert(count <= fontStack.count);
	while (count-- > 0) {
		style.font = *fontStack.last;
		fontStack.pop();
	}
}

void UI::PopScale(u32 count) {DPZoneScoped;
	Assert(count <= scaleStack.count);
	while (count-- > 0) {
		style.globalScale = *scaleStack.last;
		scaleStack.pop();
	}
}

void UI::PopLayer(u32 count) {DPZoneScoped;
	Assert(count <= layerStack.count);
	while (count-- > 0) {
		currlayer = *layerStack.last;
		layerStack.pop();
	}
}

void UI::PopLeftIndent(u32 count) {DPZoneScoped;
	Assert(count < leftIndentStack.count);
	while (count-- > 0) {
		leftIndentStack.pop();
	}
}

void UI::PopRightIndent(u32 count){DPZoneScoped;
	Assert(count < rightIndentStack.count);
	while (count-- > 0) {
		rightIndentStack.pop();
	}
}

void UI::PopDrawTarget(u32 count) {DPZoneScoped;
	Assert(count < drawTargetStack.count
		   );
	while (count-- > 0) {
		drawTargetStack.pop();
	}
}



//@Windows



//window input helper funcs 

void SetFocusedWindow(UIWindow* window) {DPZoneScoped;
	//we must find what idx the window is at
	//i think
	for (int i = 0; i < windows.count; i++) {
		if (windows[i] == window) {
			for (int move = i; move < windows.count - 1; move++)
				windows.swap(move, move + 1);
			break;
		}
	}
}

//function to recursively check child windows
b32 CheckForHoveredChildWindow(UIWindow* parent, UIWindow* child) {DPZoneScoped;
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

void CheckForHoveredWindow(UIWindow* window = 0) {DPZoneScoped;
	b32 hovered_found = 0;
	for (int i = windows.count - 1; i > 0; i--) {
		UIWindow* w = windows[i];
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
		if (hovered_found) break;
	}
}

//this function is checked in UI::Update, while the other 3 are checked per window
void CheckWindowsForFocusInputs() {DPZoneScoped;
	//special case where we always check for metrics first since it draws last
	
	for (int i = windows.count - 1; i > 0; i--) {
		UIWindow* w = windows[i];
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

void CheckWindowForResizingInputs(UIWindow* window) {DPZoneScoped;
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
			f32 borderscaledx = style.windowBorderSize * style.globalScale.x;
			f32 borderscaledy = style.windowBorderSize * style.globalScale.y;
			f32 widthscaled = window->width * style.globalScale.x;
			f32 heightscaled = window->height * style.globalScale.y;
			
			if(MouseInArea(window->position.yAdd(-boundrysize), vec2(widthscaled,boundrysize + borderscaledy)))
				activeSide = wTop;
			else if (MouseInArea(window->position.yAdd(heightscaled - borderscaledy), vec2(widthscaled, boundrysize + borderscaledy)))
				activeSide = wBottom;
			else if (MouseInArea(window->position, vec2(boundrysize + borderscaledx, heightscaled)))
				activeSide = wLeft;
			else if (MouseInArea(window->position.xAdd(widthscaled - borderscaledx), vec2(boundrysize + borderscaledx, heightscaled)))
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
void CheckWindowForScrollingInputs(UIWindow* window, b32 fromChild = 0) {DPZoneScoped;
	//always clamp scroll to make sure that it doesnt get stuck pass max scroll when stuff changes inside the window
	window->scx = Clamp(window->scx, 0.f, window->maxScroll.x);
	window->scy = Clamp(window->scy, 0.f, window->maxScroll.y);
	if(DeshInput->scrollY){
		int i = 0;
	}
	//mouse wheel inputs
	//if this is a child window and it cant scroll, redirect the scrolling inputs to the parent
	if (window->parent && WinHovered(window) && window->maxScroll.x == 0 && window->maxScroll.y == 0) {
		CheckWindowForScrollingInputs(window->parent, 1);
		return;
	}
	if (((WinHovered(window) && !WinChildHovered(window)) || fromChild)) {
		window->scy -= style.scrollAmount.y * DeshInput->scrollY;
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
			f32 scrollbarheight = ClientBottom(window) - ClientTop(window);
			f32 draggerheight = scrollbarheight * scrollbarheight / winmin.y;
			vec2 draggerpos(ClientRight(), (scrollbarheight - draggerheight) * window->scy / window->maxScroll.y + BorderedTop(window));
			
			b32 scbgactive = MouseInWinArea(vec2(ClientRight(window), BorderedTop(window)), vec2(style.scrollBarYWidth, scrollbarheight));
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
			f32 scrollbarwidth = ClientRight(window) - ClientLeft(window);
			f32 draggerwidth = scrollbarwidth * window->dimensions.x / winmin.x;
			vec2 draggerpos((scrollbarwidth - draggerwidth) * window->scx / window->maxScroll.x, ClientBottom(window));
			
			b32 scbgactive = MouseInWinArea(vec2(ClientBottom(window), BorderedLeft(window)), vec2(scrollbarwidth, style.scrollBarXHeight));
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

void CheckWindowForDragInputs(UIWindow* window, b32 fromChild = 0) {DPZoneScoped;
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
void BeginCall(const char* name, vec2 pos, vec2 dimensions, UIWindowFlags flags, UIWindowType type) {DPZoneScoped;
	Assert(type != UIWindowType_Normal || !StateHasFlag(UISRowBegan), "Attempted to begin a window with a Row in progress! (Did you forget to call EndRow()?");
	TIMER_RESET(wincreate);
	//save previous window on stack
	windowStack.add(curwin);
	ui_stats.windows++;
	
	switch (type) {
		case UIWindowType_Normal: { //////////////////////////////////////////////////////////////////////
			//check if were making a new window or working with one we already know
			if (windows.has(name)) {
				curwin = windows[name];
				curwin->cursor = vec2(0, 0);
				if (NextWinPos.x != -1) curwin->position = NextWinPos + DeshWindow->GetClientAreaPosition();
				if (NextWinSize.x != -1) curwin->dimensions = NextWinSize;
				NextWinPos = vec2(-1, 0); NextWinSize = vec2(-1, 0);
				curwin->flags = flags;
			}
			else {
				curwin = new UIWindow();
				
				curwin->scroll = vec2(0, 0);
				curwin->name = name;
				curwin->position = pos + DeshWindow->GetClientAreaPosition();
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
						item->size.x = MarginedRight() - item->position.x - rightIndent;
					else if (NextWinSize.x == -1) {}
					else item->size.x = NextWinSize.x;
					
					if (NextWinSize.y == MAX_F32)
						item->size.y = MarginedBottom() - item->position.y;
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
			
			UI::PushLeftIndent(0);
			UI::PushRightIndent(0);
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
				if (NextWinPos.x != -1) { curwin->position = NextWinPos + DeshWindow->GetClientAreaPosition(); }
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
			
			UI::PushLeftIndent(0);
			UI::PushRightIndent(0);
			item->child = curwin;
			curwin->parent = parent;
			curwin->type = UIWindowType_PopOut;
		}break;
	}
	
	WinSetBegan(curwin);
	
}


void UI::Begin(const char* name, UIWindowFlags flags) {DPZoneScoped;
	BeginCall(name, vec2::ONE * 100, vec2(150, 300), flags, UIWindowType_Normal);
}

void UI::Begin(const char* name, vec2 pos, vec2 dimensions, UIWindowFlags flags) {DPZoneScoped;
	BeginCall(name, pos, dimensions, flags, UIWindowType_Normal);
}

void UI::BeginChild(const char* name, vec2 dimensions, UIWindowFlags flags) {DPZoneScoped;
	BeginCall(name, PositionForNewItem(), dimensions, flags, UIWindowType_Child);
}

void UI::BeginChild(const char* name, vec2 pos, vec2 dimensions, UIWindowFlags flags) {DPZoneScoped;
	BeginCall(name, pos, dimensions, flags, UIWindowType_Child);
}

void UI::BeginPopOut(const char* name, vec2 pos, vec2 dimensions, UIWindowFlags flags) {DPZoneScoped;
	//currlayer = Min(++currlayer, (u32)UI_LAYERS);
	BeginCall(name, pos, dimensions, flags, UIWindowType_PopOut);
	//currlayer--; //NOTE this will break if we are already on last layer fix it
}

//@CalcWindowMinSize

//calculates the minimum size a window can be to contain all drawn elements
//this would probably be better to be handled as we add items to the window
//instead of doing it at the end, so maybe make an addItem() that calculates this
//everytime we add one
vec2 CalcWindowMinSize() {DPZoneScoped;
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
if (!(curwin->flags & UIWindowFlags_NoTitleBar)) {DPZoneScoped;
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

void EndCall() {DPZoneScoped;
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
			f32 scrollbarheight = ClientBottom() - ClientTop();
			f32 draggerheight = scrollbarheight * scrollbarheight / minSizeForFit.y;
			vec2 draggerpos(ClientRight(), (scrollbarheight - draggerheight) * curwin->scy / curwin->maxScroll.y + BorderedTop());
			
			b32 scbgactive = MouseInWinArea(vec2(ClientRight(), BorderedTop()), vec2(style.scrollBarYWidth, scrollbarheight));
			b32 scdractive = MouseInWinArea(draggerpos, vec2(style.scrollBarYWidth, draggerheight));
			
			{//scroll bg
				UIDrawCmd drawCmd;
				vec2  position = vec2(ClientRight(), BorderedTop());
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
				vec2  position = vec2(ClientRight(), scrollbarheight);
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
			f32 scrollbarwidth = ClientRight() - ClientLeft();
			f32 draggerwidth = scrollbarwidth * curwin->dimensions.x / minSizeForFit.x;
			vec2 draggerpos((scrollbarwidth - draggerwidth) * curwin->scx / curwin->maxScroll.x, ClientBottom());
			
			b32 scbgactive = MouseInWinArea(vec2(ClientBottom(), BorderedLeft()), vec2(scrollbarwidth, style.scrollBarXHeight));
			b32 scdractive = MouseInWinArea(draggerpos, vec2(draggerwidth, style.scrollBarXHeight));
			
			{//scroll bg
				UIDrawCmd drawCmd;
				vec2  position = vec2(0, ClientBottom());
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

void UI::End() {DPZoneScoped;
	Assert(!StateHasFlag(UISRowBegan), "Attempted to end a window with a Row in progress!");
	Assert(!StateHasFlag(UISComboBegan), "Attempted to end a window with a Combo in progress!");
	
	curwin->visibleRegionStart = curwin->position;
	curwin->visibleRegionSize = curwin->dimensions;
	
	EndCall();
}

void UI::EndChild() {DPZoneScoped;
	UIWindow* parent = curwin->parent;
	vec2 scrollBarAdjust = vec2((CanScrollY(parent) ? style.scrollBarYWidth : 0), (CanScrollX(parent) ? style.scrollBarXHeight : 0));
	curwin->visibleRegionStart = Max(parent->visibleRegionStart, curwin->position);
	curwin->visibleRegionSize = ClampMin(Min(parent->visibleRegionStart + parent->visibleRegionSize - scrollBarAdjust, curwin->position + curwin->dimensions) - curwin->visibleRegionStart, vec2::ZERO);
	
	EndCall();
	PopLeftIndent();
	PopRightIndent();
}

void UI::EndPopOut() {DPZoneScoped;
	curwin->visibleRegionStart = curwin->position;
	curwin->visibleRegionSize = curwin->dimensions;
	
	EndCall();
	PopLeftIndent();
	PopRightIndent();
}

void UI::SetNextWindowPos(vec2 pos) {DPZoneScoped;
	NextWinPos = pos;
}

void UI::SetNextWindowPos(f32 x, f32 y) {DPZoneScoped;
	NextWinPos = vec2(x,y);
}

void UI::SetNextWindowSize(vec2 size) {DPZoneScoped;
	NextWinSize = size.yAdd(style.titleBarHeight);
}

void UI::SetNextWindowSize(f32 x, f32 y) {DPZoneScoped;
	NextWinSize = vec2(x, y);
}

b32 UI::IsWinHovered() {DPZoneScoped;
	return WinHovered(curwin);
}

b32 UI::AnyWinHovered() {DPZoneScoped;
	return StateHasFlag(UISGlobalHovered) || !CanTakeInput;
}

inline UIWindow* MetricsDebugItemFindHoveredWindow(UIWindow* window = 0) {DPZoneScoped;
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

inline void MetricsDebugItem() {DPZoneScoped;
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
			PushVar(UIStyleVar_WindowMargins, vec2(3, 3));
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
					PushVar(UIStyleVar_WindowMargins, vec2(3, 3));
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
			
			
			PushVar(UIStyleVar_WindowMargins, vec2(3, 3));
			//PushColor(UIStyleCol_WindowBg, color(50, 50, 50));
			BeginPopOut("MetricsDebugItemPopOut", mplatch - curwin->position, vec2::ZERO, UIWindowFlags_FitAllElements | UIWindowFlags_NoInteract);
			
			Text(UIItemTypeStrs[iteml.type], UITextFlags_NoWrap);
			Text(toStr("DrawCmds: ", iteml.drawCmds.count).str, UITextFlags_NoWrap);
			Text("Select to break on drawCmd", UITextFlags_NoWrap);
			
			PushColor(UIStyleCol_WindowBg, color(30, 30, 30));
			BeginChild("MetricsDebugItemPopOutDrawCmdChild", vec2(0,0), UIWindowFlags_NoBorder | UIWindowFlags_FitAllElements);
			
			BeginRow("MetricsItemAlignment", 3, style.buttonHeightRelToFont* style.fontHeight);
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

inline b32 MetricsCheckWindowBreaks(UIWindow* window, b32 winbegin) {DPZoneScoped;
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

inline void MetricsBreaking() {DPZoneScoped;
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
					//DebugRect(win->position + item.position, item.size);
					vec2 ipos = win->position + item.position;
					selected = Clamp(selected, 0, item.drawCmds.count);
					int o = 0;
					for (UIDrawCmd& dc : item.drawCmds) {
						for (u32 tri = 0; tri < dc.counts.y; tri += 3) {
							vec2 
								p0 = ipos + dc.vertices[dc.indices[tri]].pos,
							p1 = ipos + dc.vertices[dc.indices[tri+1]].pos,
							p2 = ipos + dc.vertices[dc.indices[tri+2]].pos;
							DebugTriangle(p0, p1, p2, color(255, 0,0, 70));
							if (Math::PointInTriangle(DeshInput->mousePos, p0, p1, p2)) {
								DebugTriangle(p0, p1, p2);
								if (DeshInput->LMousePressed()) {
									
								}
							}
						}
						
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
			for (UIWindow* w : windows) {
				if (WinChildHovered(w)) {
					for (UIWindow* c : w->children) {
						check_win_drawcmds(c);
					}
				}
				else if (WinHovered(w)) {
					check_win_drawcmds(w);
				}
			}
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

UIWindow* DisplayMetrics() {DPZoneScoped;
	using namespace UI;
	
	persist UIWindow* debugee = nullptr;
	
	UIWindow* myself = 0; //pointer returned for drawing
	
	persist UIWindow* slomo = windows.data[0];
	persist UIWindow* quick = windows.data[0];
	persist UIWindow* mostitems = windows.data[0];
	persist UIWindow* longname = windows.data[0];
	
	array<UIWindow*> winsorted;
	for (UIWindow* win : windows) {
		//TODO optional metrics filter or allow text filtering 
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
	//WinSetHovered(curwin);
	
	BeginRow("Metrics_General_Stats", 2, 0, UIRowFlags_AutoSize);
	RowSetupColumnAlignments({ {1, 0.5}, {0, 0.5} });
	Text("FPS: "); Text(toStr(1/DeshTime->deltaTime).str);
	EndRow();
	
	if (BeginHeader("UI Stats")) {
		
		BeginRow("Metrics_UI_Stats", 2, 0, UIRowFlags_AutoSize);
		RowSetupColumnAlignments({ {1, 0.5}, {0, 0.5} });
		
		Text("Windows: ");      Text(toStr(ui_stats.windows).str);
		Text("Items: ");        Text(toStr(ui_stats.items).str);
		Text("DrawCmds: ");     Text(toStr(ui_stats.draw_cmds).str);
		Text("Vertices: ");     Text(toStr(ui_stats.vertices).str);
		Text("Indices: ");      Text(toStr(ui_stats.indices).str);
		Text("Global Hover: "); Text(toStr(StateHasFlag(UISGlobalHovered)).str);
		Text("input state: ");
		switch (inputState) {
			case ISNone:                  Text("None");                    break;
			case ISScrolling:             Text("Scrolling");               break;
			case ISResizing:              Text("Resizing");                break;
			case ISDragging:              Text("Dragging");                break;
			case ISPreventInputs:         Text("Prevent Inputs");          break;
			case ISExternalPreventInputs: Text("External Prevent Inputs"); break;
		}
		Text("input upon: "); Text((inputupon ? inputupon->name.str : "none"));
		
		EndRow();		
		EndHeader();
	}
	
	if (BeginHeader("Windows")) {
		
		{//window stats (maybe put this in a header?)
			string slomotext = toStr("Slowest Render:");
			string quicktext = toStr("Fastest Render:");
			string mostitext = toStr("Most Items: "); 
			
			persist f32 sw = CalcTextSize(longname->name).x;
			persist f32 fw = CalcTextSize(slomotext).x + 5;
			
			PushVar(UIStyleVar_RowItemAlign, vec2{ 0, 0.5 });
			BeginRow("MetricsWindowStatsAlignment", 3, 11, UIRowFlags_AutoSize);
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
		
		persist b32 showChildren = 0;
		
		Checkbox("show children", &showChildren);
		
		SetNextWindowSize(vec2(MAX_F32, 300));
		BeginChild("METRICSWindows", vec2::ZERO, UIWindowFlags_NoScrollX);
		WinSetHovered(curwin);
		
		PushVar(UIStyleVar_SelectableTextAlign, vec2(0, 0.5));
		for (UIWindow* window : windows) {
			SetNextItemSize(vec2(MAX_F32, 0));
			if (Selectable(toStr(window->name, "; hovered: ", WinHovered(window)).str, window == debugee)) {
				debugee = window;
			}
			if (MouseInArea(GetLastItemScreenPos(), GetLastItemSize())) {
				DebugRect(window->position, window->dimensions * window->style.globalScale);
			}
			if (showChildren) {
				PushLeftIndent(13);
				for (UIWindow* child : window->children) {
					SetNextItemSize(vec2(MAX_F32, 0));
					if (Selectable(toStr(child->name, "; hovered: ", WinHovered(child)).str, child == debugee)) {
						debugee = child;
					}
					if (MouseInArea(GetLastItemScreenPos(), GetLastItemSize())) {
						DebugRect(child->position, child->dimensions * child->style.globalScale);
					}
				}
				PopLeftIndent();
			}
		}
		PopVar();
		WinUnSetHovered(curwin);
		EndChild();
		
		
		EndHeader();
	}
	
	if (BeginHeader("Breaking")) {
		MetricsBreaking();
		EndHeader();
	}
	
	if (BeginHeader("Cursor Debugging")) {
		MetricsDebugItem();
		TextF("Cursor Pos: (%4d,%4d)", s32(DeshInput->mouseX), s32(DeshInput->mouseY));
		EndHeader();
	}
	
	Separator(20);
	
	
	Text(toStr("Selected Window: ", (debugee ? debugee->name : "none")).str);
	
	
	if (debugee) {
		
		if (Button("Set Focused")) {
			SetFocusedWindow(debugee);
		}
		
		if (BeginHeader("Vars")) {
			BeginRow("MetricsWindowVarAlignment", 2, style.fontHeight * 1.2, UIRowFlags_AutoSize);
			RowSetupColumnWidths({ CalcTextSize("Max Item Width: ").x , 10 });
			RowSetupColumnAlignments({{0,0.5},{0,0.5}});
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
			
			
			
			EndHeader();
		}
		
		if (BeginHeader("Items")) {
			SetNextWindowSize(vec2(MAX_F32, 300));
			BeginChild("METRICSItems", vec2(0,0)); {
				forI(UI_WINDOW_ITEM_LAYERS) {
					u32 count = 0;
					for (UIItem& item : debugee->items[i]) {
						if (BeginHeader(toStr(UIItemTypeStrs[item.type], " ", count).str)) {
							persist f32 frs = CalcTextSize("FilledRectangle").x;
							BeginRow("121255552525", 3, style.buttonHeightRelToFont * style.fontHeight);
							RowSetupColumnWidths({ frs, 50, 40 });
							
							for (UIDrawCmd& dc : item.drawCmds) {
								Text(UIDrawTypeStrs[dc.type]);
								if (MouseInArea(GetLastItemScreenPos(), GetLastItemSize())) {
									for (int tri = 0; tri < dc.counts.y; tri += 3) {
										vec2
											p0 = item.position * item.style.globalScale + debugee->position + item.style.globalScale * dc.vertices[dc.indices[tri]].pos,
										p1 = item.position * item.style.globalScale + debugee->position + item.style.globalScale * dc.vertices[dc.indices[tri + 1]].pos,
										p2 = item.position * item.style.globalScale + debugee->position + item.style.globalScale * dc.vertices[dc.indices[tri + 2]].pos;
										DebugTriangle(p0, p1, p2);
									}
								}
								if (Button("Create")) {
									break_drawCmd_create_hash = dc.hash;
								}
								if (Button("Draw")) {
									break_drawCmd_draw_hash = dc.hash;
								}
								
							}
							EndRow();
							EndHeader();
						}
						count++;
					}
				}
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
		
		persist b32 showItemBoxes = false;
		persist b32 showItemCursors = false;
		persist b32 showItemNames = false;
		persist b32 showItemCoords = false;
		persist b32 showAllDrawCmdScissors = false;
		persist b32 showDrawCmdTriangles = false;
		persist b32 showBorderArea = false;
		persist b32 showMarginArea = false;
		persist b32 showScrollBarArea = false;
		
		
		if (BeginHeader("Debug Visuals")) {
			Checkbox("Show Item Boxes", &showItemBoxes);
			Checkbox("Show Item Cursors", &showItemCursors);
			Checkbox("Show Item Names", &showItemNames);
			Checkbox("Show Item Coords", &showItemCoords);
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
		
		if(showItemNames){
			forI(UI_WINDOW_ITEM_LAYERS){
				for(UIItem& item : debugee->items[i]){
					DebugText(debugee->position + item.position, (char*)UIItemTypeStrs[item.type]);
				}
			}
		}
		
		if(showItemCoords){
			forI(UI_WINDOW_ITEM_LAYERS){
				for(UIItem& item : debugee->items[i]){
					DebugText(debugee->position + item.position, toStr("(",item.position.x,",",item.position.y,")").str);
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
				vec2 ipos = debugee->position + item.position * item.style.globalScale;
				for (UIDrawCmd& dc : item.drawCmds) {
					for (int i = 0; i < dc.counts.y; i += 3) {
						DebugTriangle(
									  ipos + dc.vertices[dc.indices[i]].pos * item.style.globalScale,
									  ipos + dc.vertices[dc.indices[i + 1]].pos * item.style.globalScale,
									  ipos + dc.vertices[dc.indices[i + 2]].pos * item.style.globalScale, Color_Green);
					}
				}
			}
			forI(UI_WINDOW_ITEM_LAYERS) {
				for (UIItem& item : debugee->items[i]) {
					vec2 ipos = debugee->position + item.position * item.style.globalScale;
					for (UIDrawCmd& dc : item.drawCmds) {
						for (int i = 0; i < dc.counts.y; i += 3) {
							DebugTriangle(
										  ipos + dc.vertices[dc.indices[i]].pos * item.style.globalScale,
										  ipos + dc.vertices[dc.indices[i + 1]].pos * item.style.globalScale,
										  ipos + dc.vertices[dc.indices[i + 2]].pos * item.style.globalScale);
						}
					}
				}
			}
			for (UIItem& item : debugee->postItems) {
				vec2 ipos = debugee->position + item.position * item.style.globalScale;
				for (UIDrawCmd& dc : item.drawCmds) {
					for (int i = 0; i < dc.counts.y; i += 3) {
						DebugTriangle(
									  ipos + dc.vertices[dc.indices[i]].pos * item.style.globalScale,
									  ipos + dc.vertices[dc.indices[i + 1]].pos * item.style.globalScale,
									  ipos + dc.vertices[dc.indices[i + 2]].pos * item.style.globalScale, Color_Blue);
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
	
	
	//WinUnSetHovered(curwin);
	
	End();
	
	//PopColor(5);
	
	return myself;
	
}

//this just sets a flag to show the window at the very end of the frame, so we can gather all data
//about windows incase the user tries to call this before making all their windows
b32 show_metrics = 0;
void UI::ShowMetricsWindow() {DPZoneScoped;
	show_metrics = 1;
}

void UI::DemoWindow() {DPZoneScoped;
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
		
		Separator(7);
		
		Text("Style Vars: ");
		
		persist char button_border_size_buff[7] = {};
		Text("Button Border Size (f32): "); SameLine();
		if(InputText("demo_button_border_size", button_border_size_buff, 7, "1.0", UIInputTextFlags_Numerical | UIInputTextFlags_AnyChangeReturnsTrue  | UIInputTextFlags_EnterReturnsTrue)){
			style.buttonBorderSize = stod(button_border_size_buff);
		}
		
		persist color button_border_color = style.colors[UIStyleCol_ButtonBorder];
		Text("Style Colors: ");
		Text("TODO button colors");
		
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
		
		BeginRow("Demo_35135", 3, 15);
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
		
		BeginRow("Demo_3541351", 2, 30);
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
				BeginRow("Demo_1351351", 3, 16);
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
				BeginRow("Demo_66462", 3, 16);
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
void UI::Init() {DPZoneScoped;
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
	PushColor(UIStyleCol_Border,    0x000000ff);
	PushColor(UIStyleCol_WindowBg,  0x111111ff);
	PushColor(UIStyleCol_Text,      Color_White);
	PushColor(UIStyleCol_Separator, color(64, 64, 64));
	
	//backgrounds
	PushColor(UIStyleCol_ScrollBarBg,  Color_VeryDarkCyan);
	PushColor(UIStyleCol_ButtonBg,     Color_VeryDarkCyan);
	PushColor(UIStyleCol_CheckboxBg,   Color_VeryDarkCyan);
	PushColor(UIStyleCol_HeaderBg,     color(0, 100, 100, 255));
	PushColor(UIStyleCol_SliderBg,     Color_VeryDarkCyan);
	PushColor(UIStyleCol_InputTextBg,  Color_DarkCyan);
	PushColor(UIStyleCol_SelectableBg, Color_VeryDarkCyan);
	PushColor(UIStyleCol_TabBg,        Color_VeryDarkCyan);
	
	//active backgrounds
	PushColor(UIStyleCol_ScrollBarBgActive,  Color_VeryDarkCyan);
	PushColor(UIStyleCol_ButtonBgActive,     Color_Cyan);
	PushColor(UIStyleCol_CheckboxBgActive,   Color_Cyan);
	PushColor(UIStyleCol_HeaderBgActive,     color(0, 255, 255, 255));
	PushColor(UIStyleCol_SliderBgActive,     Color_Cyan);
	PushColor(UIStyleCol_InputTextBgActive,  Color_DarkCyan);
	PushColor(UIStyleCol_SelectableBgActive, Color_Cyan);
	PushColor(UIStyleCol_TabBgActive,        Color_Cyan);
	
	//hovered backgrounds
	PushColor(UIStyleCol_ScrollBarBgHovered,  Color_VeryDarkCyan);
	PushColor(UIStyleCol_ButtonBgHovered,     Color_DarkCyan);
	PushColor(UIStyleCol_CheckboxBgHovered,   Color_DarkCyan);
	PushColor(UIStyleCol_HeaderBgHovered,     color(0, 128, 128, 255));
	PushColor(UIStyleCol_SliderBgHovered,     Color_DarkCyan);
	PushColor(UIStyleCol_InputTextBgHovered,  Color_DarkCyan);
	PushColor(UIStyleCol_SelectableBgHovered, Color_DarkCyan);
	PushColor(UIStyleCol_TabBgHovered,        Color_DarkCyan);
	
	//borders
	PushColor(UIStyleCol_ButtonBorder,    Color_Black);
	PushColor(UIStyleCol_CheckboxBorder,  Color_Black);
	PushColor(UIStyleCol_HeaderBorder,    Color_Black);
	PushColor(UIStyleCol_SliderBorder,    Color_Black);
	PushColor(UIStyleCol_InputTextBorder, Color_Black);
	PushColor(UIStyleCol_TabBorder,       Color_Black);
	
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
	
	PushColor(UIStyleCol_TabBar, Color_DarkCyan);
	
	//push default style variables
	PushVar(UIStyleVar_WindowMargins,             vec2(10, 10));
	PushVar(UIStyleVar_WindowBorderSize,          1);
	PushVar(UIStyleVar_ButtonBorderSize,          1);
	PushVar(UIStyleVar_TitleBarHeight,            style.fontHeight * 1.2f);
	PushVar(UIStyleVar_TitleTextAlign,            vec2(1, 0.5f));
	PushVar(UIStyleVar_ItemSpacing,               vec2(1, 1));
	PushVar(UIStyleVar_ScrollAmount,              vec2(10, 10));
	PushVar(UIStyleVar_CheckboxSize,              vec2(10, 10));
	PushVar(UIStyleVar_CheckboxFillPadding,       2);
	PushVar(UIStyleVar_InputTextTextAlign,        vec2(0.f,  0.5f));
	PushVar(UIStyleVar_ButtonTextAlign,           vec2(0.5f, 0.5f));
	PushVar(UIStyleVar_HeaderTextAlign,           vec2(0.f,  0.5f));
	PushVar(UIStyleVar_SelectableTextAlign,       vec2(0.5f, 0.5f));
	PushVar(UIStyleVar_TabTextAlign,              vec2(0.5f, 0.5f));
	PushVar(UIStyleVar_ButtonHeightRelToFont,     1.3f);
	PushVar(UIStyleVar_HeaderHeightRelToFont,     1.3f);
	PushVar(UIStyleVar_InputTextHeightRelToFont,  1.3f);
	PushVar(UIStyleVar_CheckboxHeightRelToFont,   1.3f);
	PushVar(UIStyleVar_SelectableHeightRelToFont, 1.3f);
	PushVar(UIStyleVar_TabHeightRelToFont,        1.3f);
	PushVar(UIStyleVar_RowItemAlign,              vec2(0.5f, 0.5f));
	PushVar(UIStyleVar_ScrollBarYWidth,           5);
	PushVar(UIStyleVar_ScrollBarXHeight,          5);
	PushVar(UIStyleVar_IndentAmount,              12);
	PushVar(UIStyleVar_TabSpacing,                5);
	PushVar(UIStyleVar_FontHeight,                (f32)style.font->max_height);
	
	PushScale(vec2(1, 1));
	
	initColorStackSize = colorStack.count;
	initStyleStackSize = varStack.count;
	
	windows.add("base", curwin);
	//windowStack.add(curwin);
	
	LogS("deshi","Finished UI initialization in ",TIMER_END(t_s),"ms");
}

//in our final draw system, this is the function that primarily does the work
//of figuring out how each draw call will be sent to the renderer
inline void DrawItem(UIItem& item, UIWindow* window) {DPZoneScoped;
	
	vec2 winpos = vec2(window->x, window->y);
	vec2 winsiz = vec2(window->width, window->height) * window->style.globalScale;
	vec2 winScissorOffset = window->visibleRegionStart;
	vec2 winScissorExtent = window->visibleRegionSize * window->style.globalScale;
	
	UIWindow* parent = window->parent;
	
	vec2 itempos = window->position + item.position * item.style.globalScale;
	vec2 itemsiz = item.size;
	
	UIDrawCmd* lastdc = 0;
	for (UIDrawCmd& drawCmd : item.drawCmds) {
		BreakOnDrawCmdDraw;
		
		//NOTE this expects vertex positions to be in item space
		forI(drawCmd.counts.x) drawCmd.vertices[i].pos = floor((drawCmd.vertices[i].pos * item.style.globalScale +itempos));
		
		vec2 dcse = (drawCmd.useWindowScissor ? winScissorExtent : drawCmd.scissorExtent * item.style.globalScale);
		vec2 dcso = (drawCmd.useWindowScissor ? winScissorOffset : itempos + drawCmd.scissorOffset);
		
		//modify the scissor offset and extent according to the kind of window we are drawing
		switch (window->type) {
			case UIWindowType_PopOut:
			case UIWindowType_Normal: {
				dcso = Min(winpos + winScissorExtent - dcse, Max(winpos, dcso));
				dcse += Min(dcso - winpos, vec2::ZERO + DeshWindow->GetClientAreaPosition());
				if (drawCmd.useWindowScissor)
					dcse += Min(winpos, vec2::ZERO + DeshWindow->GetClientAreaPosition());
			}break;
			case UIWindowType_Child: {
				dcso = Min(winScissorOffset + winScissorExtent - dcse, Max(dcso, winScissorOffset));
				dcse += Min(dcso - winScissorOffset, vec2::ZERO + DeshWindow->GetClientAreaPosition());
				if(drawCmd.useWindowScissor)
					dcse += Min(winScissorOffset, vec2::ZERO + DeshWindow->GetClientAreaPosition());
				
			}break;
		}
		
		dcse = ClampMin(dcse, vec2::ZERO);
		dcso = ClampMin(dcso, vec2::ZERO);
		
		Render::SetSurfaceDrawTargetByIdx(drawCmd.render_surface_target_idx);
		
		//compare current stuff to last draw cmd to determine if we need to start a new twodCmd
		if(!lastdc || dcse != lastdc->scissorExtent || dcso != lastdc->scissorOffset || drawCmd.tex != lastdc->tex) 
			Render::StartNewTwodCmd(window->layer, drawCmd.tex, dcso, dcse);
		Render::AddTwodVertices(window->layer, drawCmd.vertices, drawCmd.counts.x, drawCmd.indices, drawCmd.counts.y);
		
		drawCmd.scissorExtent = dcse;
		drawCmd.scissorOffset = dcso;
		lastdc = &drawCmd;
	}
	
	Render::SetSurfaceDrawTargetByIdx(0);
}

inline void DrawWindow(UIWindow* p, UIWindow* parent = 0) {DPZoneScoped;
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
#ifndef BUILD_INTERNAL
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

void CleanUpWindow(UIWindow* window) {DPZoneScoped;
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
void UI::Update() {DPZoneScoped;
	
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
	
	Assert(colorStack.count == initColorStackSize, 
		   "Frame ended with hanging colors in the stack, make sure you pop colors if you push them!");
	
	Assert(leftIndentStack.count == 1, "Forgot to call End for an indenting Begin!");
	Assert(rightIndentStack.count == 1, "Forgot to call End for an indenting Begin!");
	Assert(drawTargetStack.count == 1, "Forgot to pop a draw target!");
	
	forI(UIItemType_COUNT)
		Assert(itemFlags[i] == 0, "Forgot to clear an item's default flags!");
	
	hovered = 0;
	StateRemoveFlag(UISGlobalHovered);
	MarginPositionOffset = vec2::ZERO;
	MarginSizeOffset = vec2::ZERO;
	
#ifdef BUILD_INTERNAL
	//clear break vars in debug mode
	break_window_item = 0;
	item_idx = -1;
	item_layer = -1;
	break_window_begin = 0;
	break_window_end = 0;
	break_drawCmd_create_hash = -1;
	break_drawCmd_draw_hash = -1;
#endif
	
	
	
	ui_stats = { 0 };
	
	if (show_metrics) {
		DisplayMetrics();
		show_metrics = 0;
	}
	
	//windows input checking functions
	CheckWindowsForFocusInputs();
	CheckForHoveredWindow();
	
	if (inputupon) CheckWindowForScrollingInputs(inputupon);
	if (inputupon) CheckWindowForResizingInputs(inputupon);
	if (inputupon) CheckWindowForDragInputs(inputupon);
	
	
	//reset cursor to default if no item decided to set it 
	if (!StateHasFlag(UISCursorSet)){
		if(StateHasFlag(UISGlobalHovered)){
			DeshWindow->SetCursor(CursorType_Arrow);
		}
	}else{
		StateRemoveFlag(UISCursorSet);
	}
	
	
	//draw windows in order 
	for (UIWindow* p : windows) {
		DrawWindow(p);
		WinUnSetBegan(p);
		p->items_count = 0;
	}
	
	
	//it should be safe to do this any time the mouse is released
	if (DeshInput->LMouseReleased()) { AllowInputs; }
	
	
	//we defer window item clearing to after the metrics window is drawn
	//in debug builds
#ifdef BUILD_INTERNAL
	for (UIWindow* p : windows) {
		CleanUpWindow(p);
	}
#endif
	
	//draw all debug commands if there are any
	for (UIDrawCmd& drawCmd : debugCmds) {
		Render::StartNewTwodCmd(5, drawCmd.tex, vec2::ZERO, DeshWinSize);
		Render::AddTwodVertices(5, drawCmd.vertices, drawCmd.counts.x, drawCmd.indices, drawCmd.counts.y);
	}
	debugCmds.clear();
}

void UI::DrawDebugRect(vec2 pos, vec2 size, color col)             {DPZoneScoped; DebugRect(pos, size, col); }
void UI::DrawDebugRectFilled(vec2 pos, vec2 size, color col)       {DPZoneScoped; DebugRectFilled(pos, size, col); }
void UI::DrawDebugCircle(vec2 pos, f32 radius, color col)          {DPZoneScoped; DebugCircle(pos, radius, col); }
void UI::DrawDebugCircleFilled(vec2 pos, f32 radius, color col)    {DPZoneScoped; DebugCircleFilled(pos, radius, col); }
void UI::DrawDebugLine(vec2 pos1, vec2 pos2, color col)            {DPZoneScoped; DebugLine(pos1, pos2, col); }
void UI::DrawDebugTriangle(vec2 p0, vec2 p1, vec2 p2, color color) {DPZoneScoped; DebugTriangle(p0, p1, p2, color); }
