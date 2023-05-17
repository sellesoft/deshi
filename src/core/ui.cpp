//color pallete 
//current palette:
//https://lospec.com/palette-list/slso8
//TODO(sushi, Ui) implement menu style file loading sort of stuff yeah
//TODO(sushi, Ui) standardize what UI element each color belongs to
local struct {
	//TODO make a nice default palette maybe
	
} colors;

//global styling
UIStyle_old style;

//for color stack, saves what element was changed and what it's old color was 
struct ColorMod {
	UIStyleCol element;
	color       oldCol;
};

#define UI_LAYERS 11
static const u32 UI_CENTER_LAYER = (u32)floor((f32)UI_LAYERS / 2.f);

//for style variable stack
struct UIVarMod {
	UIStyleVar  var;
	f32 oldFloat[2];
	UIVarMod(){}
	UIVarMod(UIStyleVar var, f32 old)  { this->var = var; oldFloat[0] = old; }
	UIVarMod(UIStyleVar var, vec2 old){ this->var = var; oldFloat[0] = old.x; oldFloat[1] = old.y; }
};

//number of vars that are req for variable then offset of that var in UIStyle_old
struct UIStyleVarType {
	u32 count;
	u32 offset;
};

local const UIStyleVarType uiStyleVarTypes[] = {
	{2, offsetof(UIStyle_old,             windowMargins)},
	{2, offsetof(UIStyle_old,               itemSpacing)},
	{1, offsetof(UIStyle_old,          windowBorderSize)},
	{1, offsetof(UIStyle_old,          buttonBorderSize)},
	{1, offsetof(UIStyle_old,            titleBarHeight)},
	{2, offsetof(UIStyle_old,            titleTextAlign)},
	{2, offsetof(UIStyle_old,              scrollAmount)},
	{2, offsetof(UIStyle_old,              checkboxSize)},
	{1, offsetof(UIStyle_old,       checkboxFillPadding)},
	{2, offsetof(UIStyle_old,        inputTextTextAlign)},
	{2, offsetof(UIStyle_old,           buttonTextAlign)},
	{2, offsetof(UIStyle_old,           headerTextAlign)},
	{2, offsetof(UIStyle_old,       selectableTextAlign)},
	{2, offsetof(UIStyle_old,              tabTextAlign)},
	{1, offsetof(UIStyle_old,     buttonHeightRelToFont)},
	{1, offsetof(UIStyle_old,     headerHeightRelToFont)},
	{1, offsetof(UIStyle_old,  inputTextHeightRelToFont)},
	{1, offsetof(UIStyle_old,   checkboxHeightRelToFont)},
	{1, offsetof(UIStyle_old, selectableHeightRelToFont)},
	{1, offsetof(UIStyle_old,        tabHeightRelToFont)},
	{2, offsetof(UIStyle_old,              rowItemAlign)},
	{2, offsetof(UIStyle_old,            rowCellPadding)},
	{1, offsetof(UIStyle_old,           scrollBarYWidth)},
	{1, offsetof(UIStyle_old,          scrollBarXHeight)},
	{1, offsetof(UIStyle_old,              indentAmount)},
	{1, offsetof(UIStyle_old,                tabSpacing)},
	{1, offsetof(UIStyle_old,                fontHeight)},
	{2, offsetof(UIStyle_old,   windowSnappingTolerance)},
};


//window map which only stores known windows
//and their order in layers eg. when one gets clicked it gets moved to be first if its set to
local map<str8, UIWindow*>        windows;    //TODO convert this to actaully store the windows
local map<str8, UIInputTextState> inputTexts;  //stores known input text labels and their state
local map<str8, UIHeader>         headers;     //stores known headers
local map<str8, UITabBar>         tabBars;     //stores known tab bars
local map<str8, UIMenu>           menus;       //stores known menus
local map<str8, UIRow>            rows;        //stores known Rows
local map<str8, b32>              combos;      //stores known combos and if they are open or not
local map<str8, b32>              sliders;     //stores whether a slider is being actively changed
local arrayT<UIWindow*>            windowStack; 
local arrayT<UIHeader*>            headerStack;
local arrayT<ColorMod>             colorStack; 
local arrayT<UIVarMod>             varStack; 
local arrayT<vec2>                 scaleStack;  //global scales
local arrayT<Font*>                fontStack;
local arrayT<u32>                  layerStack;
local arrayT<f32>                  leftIndentStack{ 0 }; //stores global indentations
local arrayT<f32>                  rightIndentStack{ 0 }; //stores global indentations
local arrayT<u32>                  drawTargetStack{ 0 }; //stores draw target indexes for the renderer

local u32 itemFlags[UIItemType_COUNT]; //stores the default flags for every item that supports flagging, these can be set using SetItemFlags

local arrayT<UIDrawCmd> debugCmds; //debug draw cmds that are always drawn last

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
	UISCustomItemBegan        = 1 << 5,
	UISCursorSet              = 1 << 6,
	UISNextItemSizeSet        = 1 << 7,
	UISNextItemActive         = 1 << 8,
	UISNextItemMinSizeIgnored = 1 << 9,
	UISContinuingWindow       = 1 << 10,
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
local UIWindow* curwin    = 0;    //the window that is currently being worked with by the user
local UIWindow* hovered   = 0;   //the window that the mouse is hovering over, this is set every Update
local UIWindow* inputupon = 0; //set to a window who has drag, scrolling, or resizing inputs being used on it 

local UIStateFlags stateFlags = UISNone;
local InputState   inputState = ISNone;

local u32 currlayer = UI_CENTER_LAYER;
local u32 winlayer  = UI_CENTER_LAYER;
local u32 activeId  = -1; //the id of an active item eg. input text

local UIRow*    curRow;    //row being worked with
local UITabBar* curTabBar; //tab bar being worked with

local vec2 NextWinSize   = Vec2(-1,-1);
local vec2 NextWinPos    = Vec2(-1, 0);
local vec2 NextItemPos   = Vec2(-1, 0);
local vec2 NextItemSize  = Vec2(-1, 0);
local vec2 NextCursorPos = Vec2(-1,-1);

local vec2 MarginPositionOffset = vec2::ZERO;
local vec2 MarginSizeOffset     = vec2::ZERO;

UIStats ui_stats;

//helper defines
#define StateHasFlag(flag)    ((stateFlags) & (flag))
#define StateHasFlags(flags)  (((stateFlags) & (flags)) == (flags))
#define StateAddFlag(flag)    stateFlags |= flag
#define StateRemoveFlag(flag) ((stateFlags) &= (~(flag)))
#define StateResetFlags       stateFlags = UISNone

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

#define CanTakeInput      (inputState == ISNone)
#define PreventInputs     inputState = ISPreventInputs
#define AllowInputs       inputState = ISNone;      inputupon = 0;
#define SetResizingInput  inputState = ISResizing;  inputupon = window;
#define SetDraggingInput  inputState = ISDragging;  inputupon = window;
#define SetScrollingInput inputState = ISScrolling; inputupon = window; WinSetBeingScrolled(window);
#define WinResizing       (inputState == ISResizing)
#define WinDragging       (inputState == ISDragging)
#define WinScrolling      (inputState == ISScrolling)

#define LeftMousePressed   input_lmouse_pressed()
#define LeftMouseDown      input_lmouse_down()
#define LeftMouseReleased  input_lmouse_released()

#define RightMousePressed  DeshInput->RMousePressed()
#define RightMouseDown     DeshInput->RMouseDown()
#define RightMouseReleased DeshInput->RMouseReleased()

#define GetDefaultItemFlags(type, var) var |= itemFlags[type]

#define DefaultVec2 Vec2(-MAX_F32, -MAX_F32)

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
#define BreakOnDrawCmdCreation if(break_drawCmd_create_hash == drawCmd.hash){DebugBreakpoint;}
#define BreakOnDrawCmdDraw     if(break_drawCmd_draw_hash == drawCmd.hash){DebugBreakpoint;}
#else
#define BreakOnDrawCmdCreation
#define BreakOnDrawCmdDraw 
#endif


//helper functions


//this calculates text taking into account newlines, BUT NOT WRAPPING
//useful for sizing a window to fit some text
vec2 UI::CalcTextSize(str8 text){DPZoneScoped;
	vec2 result = vec2{0, f32(style.fontHeight)};
	f32 line_width = 0;
	switch(style.font->type){
		case FontType_BDF: case FontType_NONE:{
			u32 codepoint;
			while(text && (codepoint = str8_advance(&text).codepoint)){
				if(codepoint == '\n'){
					result.y += style.fontHeight;
					line_width = 0;
				}
				line_width += style.font->max_width * style.fontHeight / style.font->aspect_ratio / style.font->max_width;
				if(line_width > result.x) result.x = line_width;
			}
		}break;
		case FontType_TTF:{
			u32 codepoint;
			while(text && (codepoint = str8_advance(&text).codepoint)){
				if(codepoint == '\n'){
					result.y += style.fontHeight;
					line_width = 0;
				}
				line_width += font_packed_char(style.font, codepoint)->xadvance * style.fontHeight / style.font->aspect_ratio / style.font->max_width;
				if(line_width > result.x) result.x = line_width;
			}
		}break;
		default: Assert(!"unhandled font type"); break;
	}
	return result;
}

inline b32 isItemHovered(UIItem_old* item){DPZoneScoped;
	return Math::PointInRectangle(input_mouse_position(), item->position * style.globalScale + curwin->position, item->size * style.globalScale);
}

inline b32 isLocalAreaHovered(vec2 pos, vec2 size, UIItem_old* item){DPZoneScoped;
	return Math::PointInRectangle(input_mouse_position(), pos + item->position + curwin->position, size);
}

inline b32 isItemActive(UIItem_old* item){DPZoneScoped;
	return WinHovered(curwin) && CanTakeInput && isItemHovered(item);
}

//we use ## in labels to cutoff what UI actually shows the user when using an item that displays it
//this function just returns the left side of the ##
inline str8 delimitLabel(str8 label){
	return str8_eat_until_str(label, str8_lit("##"));
}

//internal master cursor controller
inline void AdvanceCursor(UIItem_old* itemmade, b32 moveCursor = 1){DPZoneScoped;
	
	//if a row is in progress, we must reposition the item to conform to row style variables
	//this means that you MUST ensure that this happens before any interactions with the item are calculated
	//for instance in the case of Button, this must happen before you check that the user has clicked it!
	if(StateHasFlag(UISRowBegan)){
		//abstract item types (lines, rectangles, etc.) are not row'd, for now
		if(itemmade->type != UIItemType_Abstract){
			UIColumn& col = curRow->columns[curRow->item_count % curRow->columns.count];
			curRow->item_count++;
			
			col.items.add(itemmade);
			
			f32 height = curRow->height;
			f32 width;
			//determine if the width is relative to the size of the item or not
			if(col.relative_width)
				width = itemmade->size.x * col.width;
			else
				width = col.width;
			
			vec2 align;
			if(col.alignment != vec2::ONE * -1)
				align = col.alignment;
			else
				align = itemmade->style.rowItemAlign;
			
			itemmade->position.y = curRow->position.y + (height - itemmade->size.y) * align.y + curRow->yoffset;
			itemmade->position.x = curRow->position.x + curRow->xoffset + (width - itemmade->size.x) * align.x;
			
			curRow->xoffset += width;
			if(col.max_width < itemmade->size.x){ col.reeval_width = true; col.max_width = itemmade->size.x; }
			if(curRow->max_height < itemmade->size.y){ curRow->reeval_height = true; curRow->max_height = itemmade->size.y; }
			curRow->max_height_frame = Max(curRow->max_height_frame, itemmade->size.y);
			
			//if we have finished a row, set xoffset to 0 and offset y for another row
			if(curRow->item_count % curRow->columns.count == 0){
				curRow->xoffset = 0;
				curRow->yoffset += curRow->height;
			}
			
			//we dont need to handle moving the cursor here, because the final position of the cursor after a row is handled in EndRow()
		}
	}
	else if(moveCursor) curwin->cursor = vec2{ 0, itemmade->position.y + itemmade->size.y + style.itemSpacing.y - style.windowMargins.y + curwin->scy - style.windowBorderSize } ;
}

//returns if the window can scroll over x
inline b32 CanScrollX(UIWindow* window = curwin){DPZoneScoped;
	return !HasFlag(window->flags, UIWindowFlags_NoScrollX) && window->width < window->minSizeForFit.x;
}

inline b32 CanScrollY(UIWindow* window = curwin){DPZoneScoped;
	return !HasFlag(window->flags, UIWindowFlags_NoScrollY) && window->height < window->minSizeForFit.y;
}

//function for getting the position of a new item based on style, so the long string of additions
//is centralized for new additions
inline vec2 DecideItemPos(UIWindow* window = curwin){DPZoneScoped;
	vec2 pos = window->cursor + (style.windowMargins + MarginPositionOffset - window->scroll) + Vec2(leftIndent, 0)
		+ vec2::ONE * ((HasFlag(window->flags, UIWindowFlags_NoBorder)) ? 0 : style.windowBorderSize);
	MarginPositionOffset = vec2::ZERO;
	if(NextCursorPos.x!=-1)pos.x=NextCursorPos.x;
	if(NextCursorPos.y!=-1)pos.y=NextCursorPos.y;
	NextCursorPos=Vec2(-1,-1);
	return pos;
}

//returns a pair representing the area of the window that is bordered
//first is the position and second is the size
inline pair<vec2, vec2> BorderedArea_old(UIWindow* window = curwin){DPZoneScoped;
	return make_pair(
					 vec2::ONE * style.windowBorderSize,
					 window->dimensions - vec2::ONE * 2 * style.windowBorderSize
					 );
}

//same as the bordered area, but also takes into account the margins
inline pair<vec2, vec2> MarginedArea_old(UIWindow* window = curwin){DPZoneScoped;
	vec2 f = vec2::ONE * style.windowBorderSize + vec2::ONE * style.windowMargins;
	vec2 s = window->dimensions - 2 * f - MarginSizeOffset;
	s.x -= (CanScrollY() ? style.scrollBarYWidth : 0);
	//s.y -= (CanScrollX() ? style.scrollBarXHeight : 0);
	MarginSizeOffset = vec2::ZERO;
	return make_pair(f, s);
}

//the bordered area taking into account the scroll bars
inline pair<vec2, vec2> ScrollBaredArea_old(UIWindow* window = curwin){DPZoneScoped;
	auto p = BorderedArea_old(window);
	p.second.x -= (CanScrollY(window) ? style.scrollBarYWidth : 0);
	p.second.y -= (CanScrollX(window) ? style.scrollBarXHeight : 0);
	return p;
}

//TODO(sushi) eventually change these to always use curwin instead of checking everytime
// probably just separate them into 2 overloaded functions each instead
FORCE_INLINE f32 BorderedRight()                  {DPZoneScoped; return curwin->dimensions.x - style.windowBorderSize; }
FORCE_INLINE f32 BorderedLeft()                   {DPZoneScoped; return style.windowBorderSize; }
FORCE_INLINE f32 BorderedTop()                    {DPZoneScoped; return style.windowBorderSize; }
FORCE_INLINE f32 BorderedBottom()                 {DPZoneScoped; return curwin->dimensions.y - style.windowBorderSize; }
FORCE_INLINE f32 BorderedRight(UIWindow* window)  {DPZoneScoped; return window->dimensions.x - window->style.windowBorderSize; }
FORCE_INLINE f32 BorderedLeft(UIWindow* window)   {DPZoneScoped; return window->style.windowBorderSize; }
FORCE_INLINE f32 BorderedTop(UIWindow* window)    {DPZoneScoped; return window->style.windowBorderSize; }
FORCE_INLINE f32 BorderedBottom(UIWindow* window){DPZoneScoped; return window->dimensions.y - window->style.windowBorderSize; }

FORCE_INLINE f32 MarginedRight()                  {DPZoneScoped; f32 ret = curwin->dimensions.x - (style.windowBorderSize + style.windowMargins.x) - (CanScrollY(curwin) ? style.scrollBarYWidth : 0) + MarginSizeOffset.x; MarginSizeOffset.x = 0; return ret; }
FORCE_INLINE f32 MarginedLeft()                   {DPZoneScoped; return style.windowBorderSize + style.windowMargins.x; }
FORCE_INLINE f32 MarginedTop()                    {DPZoneScoped; return style.windowBorderSize + style.windowMargins.y; }
FORCE_INLINE f32 MarginedBottom()                 {DPZoneScoped; f32 ret = curwin->dimensions.y -  (style.windowBorderSize + style.windowMargins.y) - (CanScrollX(curwin) ? style.scrollBarXHeight : 0) + MarginSizeOffset.y; MarginSizeOffset.y = 0; return ret; }
FORCE_INLINE f32 MarginedRight(UIWindow* window)  {DPZoneScoped; f32 ret = window->dimensions.x - (window->style.windowBorderSize + window->style.windowMargins.x) - (CanScrollY(window) ? window->style.scrollBarYWidth : 0) + MarginSizeOffset.x; MarginSizeOffset.x = 0; return ret; }
FORCE_INLINE f32 MarginedLeft(UIWindow* window)   {DPZoneScoped; return window->style.windowBorderSize + window->style.windowMargins.x; }
FORCE_INLINE f32 MarginedTop(UIWindow* window)    {DPZoneScoped; return window->style.windowBorderSize + window->style.windowMargins.y; }
FORCE_INLINE f32 MarginedBottom(UIWindow* window){DPZoneScoped; f32 ret = window->dimensions.y - (window->style.windowBorderSize + window->style.windowMargins.y) - (CanScrollX(window) ? window->style.scrollBarXHeight : 0) + MarginSizeOffset.y; MarginSizeOffset.y = 0; return ret; }

FORCE_INLINE f32 ClientRight()                    {DPZoneScoped; return BorderedRight() - (CanScrollY() ? style.scrollBarYWidth : 0); }
FORCE_INLINE f32 ClientLeft()                     {DPZoneScoped; return BorderedLeft(); }
FORCE_INLINE f32 ClientTop()                      {DPZoneScoped; return BorderedTop(); }
FORCE_INLINE f32 ClientBottom()                   {DPZoneScoped; return BorderedBottom() - (CanScrollX() ? style.scrollBarXHeight : 0); }
FORCE_INLINE f32 ClientRight(UIWindow* window)    {DPZoneScoped; return BorderedRight(window) - (CanScrollY() ? window->style.scrollBarYWidth : 0); }
FORCE_INLINE f32 ClientLeft(UIWindow* window)     {DPZoneScoped; return BorderedLeft(window); }
FORCE_INLINE f32 ClientTop(UIWindow* window)      {DPZoneScoped; return BorderedTop(window); }
FORCE_INLINE f32 ClientBottom(UIWindow* window)   {DPZoneScoped; return BorderedBottom(window) - (CanScrollX() ? window->style.scrollBarXHeight : 0); }

//return the maximum width an item can be in a non-scrolled state
FORCE_INLINE f32 MaxItemWidth(UIWindow* window = curwin){DPZoneScoped;
	return MarginedRight(window) - MarginedLeft(window);
}

FORCE_INLINE b32 MouseInArea(vec2 pos, vec2 size){DPZoneScoped;
	return Math::PointInRectangle(input_mouse_position(), pos, size);
}

FORCE_INLINE b32 MouseInWinArea(vec2 pos, vec2 size){DPZoneScoped;
	return Math::PointInRectangle(input_mouse_position() - curwin->position, pos, size);
}

inline vec2 DecideItemSize(vec2 defaultSize, vec2 itemPos){DPZoneScoped;
	vec2 size;
	if(NextItemSize.x != -1){
		if(NextItemSize.x == MAX_F32){
			size.x = MarginedRight() - itemPos.x - rightIndent;
		}else if(NextItemSize.x == 0){
			size.x = (defaultSize.x == MAX_F32) ? MarginedRight()  - itemPos.x - rightIndent : defaultSize.x;
		}else{
			size.x = NextItemSize.x;
		}
		
		if(NextItemSize.y == MAX_F32){
			size.y = MarginedBottom() - itemPos.y;
		}else if(NextItemSize.y == 0){
			size.y = (defaultSize.y == MAX_F32) ? MarginedBottom() - itemPos.y : defaultSize.y;
		}else{
			size.y = NextItemSize.y;
		}
		
		if(NextItemSize.x == -2) size.x = size.y;
		if(NextItemSize.y == -2) size.y = size.x;
	}else{
		size.x = (defaultSize.x == MAX_F32) ? MarginedRight()  - itemPos.x - rightIndent : defaultSize.x;
		size.y = (defaultSize.y == MAX_F32) ? MarginedBottom() - itemPos.y               : defaultSize.y;
	}
	
	NextItemSize.x = -1;
	NextItemSize.y = -1;
	return size;
}

FORCE_INLINE void SetWindowCursor(CursorType curtype){DPZoneScoped;
	window_set_cursor_type(DeshWindow, curtype);
	StateAddFlag(UISCursorSet);
}

FORCE_INLINE vec2 GetTextScale(){DPZoneScoped;
	return vec2::ONE * style.fontHeight / (f32)style.font->max_height;
}


UIStyle_old& UI::GetStyle(){DPZoneScoped;
	return style;
}

UIWindow* UI::GetWindow(){DPZoneScoped;
	return curwin;
}

vec2 UI::GetLastItemPos(){DPZoneScoped;
	//Assert(curwin->items.count, "Attempt to get last item position without creating any items!");
	return curwin->items[currlayer].last->position;
}

vec2 UI::GetLastItemSize(){DPZoneScoped;
	//Assert(curwin->items.count, "Attempt to get last item size without creating any items!");
	return curwin->items[currlayer].last->size;
}

vec2 UI::GetLastItemScreenPos(){DPZoneScoped;
	//Assert(curwin->items.count, "Attempt to get last item position without creating any items!");
	return curwin->position + curwin->items[currlayer].last->position;
}

vec2 UI::GetWindowRemainingSpace(){DPZoneScoped;
	return Vec2(MarginedRight(), MarginedBottom()) - DecideItemPos();
}

//NOTE this should match DecideItemPos(), just without resetting NextCursorPos
vec2 UI::GetWinCursor(){DPZoneScoped;
	vec2 pos = curwin->cursor + (style.windowMargins + MarginPositionOffset - curwin->scroll) + Vec2(leftIndent, 0)
		+ vec2::ONE * ((HasFlag(curwin->flags, UIWindowFlags_NoBorder)) ? 0 : style.windowBorderSize);
	MarginPositionOffset = vec2::ZERO;
	if(NextCursorPos.x!=-1)pos.x=NextCursorPos.x;
	if(NextCursorPos.y!=-1)pos.y=NextCursorPos.y;
	return pos;
}

u32 UI::GetCenterLayer(){DPZoneScoped;
	return UI_CENTER_LAYER;
}

u32 UI::GetCurrentLayer(){DPZoneScoped;
	return currlayer;
}

u32 UI::GetTopMostLayer(){DPZoneScoped;
	return UI_LAYERS;
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
pair<vec2, vec2> UI::GetBorderedArea()    {DPZoneScoped; return BorderedArea_old(); }
pair<vec2, vec2> UI::GetMarginedArea()    {DPZoneScoped; return MarginedArea_old(); }
pair<vec2, vec2> UI::GetClientArea()      {DPZoneScoped; return ScrollBaredArea_old(); }
f32 UI::GetRightIndent()                  {DPZoneScoped; return rightIndent; }
f32 UI::GetLeftIndent()                   {DPZoneScoped; return leftIndent; }

UIStats UI::GetStats(){return ui_stats;}

UIInputTextState* UI::GetInputTextState(str8 label){
	return &inputTexts[label];
}

//returns the cursor to the same line as the previous and moves it to the right by the 
//width of the object
void UI::SameLine(){DPZoneScoped;
	//Assert(curwin->items.count, "Attempt to sameline an item creating any items!");
	if(curwin->items[currlayer].last){
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

//TODO investigate why this doesn't actually scroll to the very bottom
void UI::SetScroll(vec2 scroll){DPZoneScoped;
    if(WinScrolling) return; //TODO(sushi) add an option for this
	if(scroll.x == MAX_F32)
		curwin->scx = curwin->maxScroll.x;
	else
		curwin->scx = scroll.x;
	
	if(scroll.y == MAX_F32)
		curwin->scy = curwin->maxScroll.y;
	else
		curwin->scy = scroll.y;
}

void SetScrollX(f32 scroll){
	if(WinScrolling) return; //TODO(sushi) add an option for this
	if(scroll == MAX_F32)
		curwin->scx = curwin->maxScroll.x;
	else
		curwin->scx = scroll;
} 

void SetScrollY(f32 scroll){
	if(WinScrolling) return; //TODO(sushi) add an option for this
	if(scroll == MAX_F32)
		curwin->scy = curwin->maxScroll.y;
	else
		curwin->scy = scroll;
} 


void UI::SetNextItemActive(){DPZoneScoped;
	StateAddFlag(UISNextItemActive);
}

void UI::SetNextItemSize(vec2 size){DPZoneScoped;
	NextItemSize = size;
}

void UI::SetNextItemWidth(f32 width){DPZoneScoped;
	NextItemSize.x = width;
}

void UI::SetNextItemHeight(f32 height){DPZoneScoped;
	NextItemSize.y = height;
}

void UI::SetMarginPositionOffset(vec2 offset){DPZoneScoped;
	MarginPositionOffset = offset;
}

void UI::SetMarginSizeOffset(vec2 offset){DPZoneScoped;
	MarginSizeOffset = offset;
}

void UI::SetNextItemMinSizeIgnored(){DPZoneScoped;
	StateAddFlag(UISNextItemMinSizeIgnored);
}

void UI::SetPreventInputs(){DPZoneScoped;
	inputState = ISExternalPreventInputs;
}

void UI::SetAllowInputs(){DPZoneScoped;
	if(inputState == ISExternalPreventInputs)
		AllowInputs;
}


//internal last item getter, returns nullptr if there are none
UIItem_old* UI::GetLastItem(u32 layeroffset){DPZoneScoped;
	return curwin->items[currlayer + layeroffset].last;
}

//helper for making any new UIItem_old
inline UIItem_old* BeginItem(UIItemType type, u32 userflags = 0, u32 layeroffset = 0){DPZoneScoped;
	Assert(!StateHasFlag(UISCustomItemBegan), "attempt to start an item while a custom item is in progress");
	if(type == UIItemType_PreItems){
		curwin->preItems.add(UIItem_old{ type, curwin->cursor, style });
		return curwin->preItems.last;
	}
	else if(type == UIItemType_PostItems){
		curwin->postItems.add(UIItem_old{ type, curwin->cursor, style });
		return curwin->postItems.last;
	}
	else if(type == UIItemType_PopOutWindow){
		curwin->popOuts.add(UIItem_old{ type, curwin->cursor, style });
		return curwin->popOuts.last;
	}
	else{
		curwin->items[currlayer + layeroffset].add(UIItem_old{ type, curwin->cursor, style });
#ifdef BUILD_INTERNAL
		UI::GetLastItem(layeroffset)->item_layer = currlayer + layeroffset;
		UI::GetLastItem(layeroffset)->item_idx = curwin->items[currlayer + layeroffset].count;
		BreakOnItem;
#endif
	}
	if(StateHasFlag(UISNextItemMinSizeIgnored)){
		UI::GetLastItem(layeroffset)->trackedForMinSize = 0;
		StateRemoveFlag(UISNextItemMinSizeIgnored);
	}
	
	GetDefaultItemFlags(type, UI::GetLastItem(layeroffset)->flags);
	AddFlag(UI::GetLastItem()->flags, userflags);
	
	ui_stats.items++;
	curwin->items_count++;
	return UI::GetLastItem(layeroffset);
}

inline void EndItem(UIItem_old* item){DPZoneScoped;}


inline void AddDrawCmd(UIItem_old* item, UIDrawCmd& drawCmd){DPZoneScoped;
	if(!drawCmd.tex) drawCmd.tex = style.font->tex;
	drawCmd.hash = hash<UIDrawCmd>{}(drawCmd);
	drawCmd.render_surface_target_idx = *drawTargetStack.last;
	item->drawCmds.add(drawCmd);
	ui_stats.draw_cmds++;
	ui_stats.vertices += drawCmd.counts.x;
	ui_stats.indices += drawCmd.counts.y;
	BreakOnDrawCmdCreation;
}

FORCE_INLINE void
MakeLine(UIDrawCmd& drawCmd, vec2 start, vec2 end, f32 thickness, color color){DPZoneScoped;
	render_make_line(drawCmd.vertices, drawCmd.indices, drawCmd.counts, start, end, thickness, color);
	drawCmd.counts += render_make_line_counts(); 
	drawCmd.type = UIDrawType_Line;
}

FORCE_INLINE void
MakeFilledTriangle(UIDrawCmd& drawCmd, vec2 p1, vec2 p2, vec2 p3, color color){DPZoneScoped;
	render_make_filledtriangle(drawCmd.vertices, drawCmd.indices, drawCmd.counts, p1, p2, p3, color);
	drawCmd.counts += render_make_filledtriangle_counts(); 
	drawCmd.type = UIDrawType_FilledTriangle;
}

FORCE_INLINE void
MakeTriangle(UIDrawCmd& drawCmd, vec2 p1, vec2 p2, vec2 p3, f32 thickness, color color){DPZoneScoped;
	render_make_triangle(drawCmd.vertices, drawCmd.indices, drawCmd.counts, p1, p2, p3, thickness, color);
	drawCmd.counts += render_make_triangle_counts();
	drawCmd.type = UIDrawType_Triangle;
}

FORCE_INLINE void
MakeFilledRect(UIDrawCmd& drawCmd, vec2 pos, vec2 size, color color){DPZoneScoped;
	render_make_filledrect(drawCmd.vertices, drawCmd.indices, drawCmd.counts, pos, size, color);
	drawCmd.counts += render_make_filledrect_counts();
	drawCmd.type = UIDrawType_FilledRectangle;
}

FORCE_INLINE void
MakeRect(UIDrawCmd& drawCmd, vec2 pos, vec2 size, f32 thickness, color color){DPZoneScoped;
	render_make_rect(drawCmd.vertices, drawCmd.indices, drawCmd.counts, pos, size, thickness, color);
	drawCmd.counts += render_make_rect_counts();
	drawCmd.type = UIDrawType_Rectangle;
}

FORCE_INLINE void
MakeCircle(UIDrawCmd& drawCmd, vec2 pos, f32 radius, u32 subdivisions, f32 thickness, color color){DPZoneScoped;
	render_make_circle(drawCmd.vertices, drawCmd.indices, drawCmd.counts, pos, radius, subdivisions, thickness, color);
	drawCmd.counts += render_make_circle_counts(subdivisions);
	drawCmd.type = UIDrawType_Circle;
}

FORCE_INLINE void
MakeFilledCircle(UIDrawCmd& drawCmd, vec2 pos, f32 radius, u32 subdivisions_int, color color){DPZoneScoped;
	render_make_filledcircle(drawCmd.vertices, drawCmd.indices, drawCmd.counts, pos, radius, subdivisions_int, color);
	drawCmd.counts += render_make_filledcircle_counts(subdivisions_int);
	drawCmd.type = UIDrawType_FilledCircle;
}

FORCE_INLINE void
MakeText(UIDrawCmd& drawCmd, str8 text, vec2 pos, color color, vec2 scale){DPZoneScoped;
	render_make_text(drawCmd.vertices, drawCmd.indices, drawCmd.counts, text, style.font, pos, color, scale);
	drawCmd.counts += render_make_text_counts(str8_length(text));
	drawCmd.tex = style.font->tex;
	drawCmd.type = UIDrawType_Text;
}

FORCE_INLINE void
MakeTexture(UIDrawCmd& drawCmd, Texture* texture, vec2 pos, vec2 size, f32 alpha, b32 flipx = 0, b32 flipy = 0){DPZoneScoped;
	render_make_texture(drawCmd.vertices, drawCmd.indices, drawCmd.counts, texture, pos, pos + size.ySet(0), pos + size, pos + size.xSet(0), alpha, flipx, flipy);
	drawCmd.counts += render_make_texture_counts();
	drawCmd.type = UIDrawType_Image;
	drawCmd.tex = texture;
}

//internal debug drawing functions
void DebugTriangle(vec2 p0, vec2 p1, vec2 p2, color col = Color_Red){DPZoneScoped;
	UIDrawCmd dc;
	MakeTriangle(dc, p0, p1, p2, 1, col);
	debugCmds.add(dc);
}

void DebugTriangleFilled(vec2 p0, vec2 p1, vec2 p2, color col = Color_Red){DPZoneScoped;
	UIDrawCmd dc;
	MakeFilledTriangle(dc, p0, p1, p2, col);
	debugCmds.add(dc);
}

void DebugRect(vec2 pos, vec2 size, color col = Color_Red){DPZoneScoped;
	UIDrawCmd dc;
	MakeRect(dc, pos, size, 1, col);
	debugCmds.add(dc);
}

void DebugRectFilled(vec2 pos, vec2 size, color col = Color_Red){DPZoneScoped;
	UIDrawCmd dc;
	MakeFilledRect(dc, pos, size, col);
	debugCmds.add(dc);
}

void DebugCircle(vec2 pos, f32 radius, color col = Color_Red){DPZoneScoped;
	UIDrawCmd dc;
	MakeCircle(dc, pos, radius, 20, 1, col);
	debugCmds.add(dc);
}

void DebugCircleFilled(vec2 pos, f32 radius, color col = Color_Red){DPZoneScoped;
	UIDrawCmd dc;
	MakeFilledCircle(dc, pos, radius, 20, col);
	debugCmds.add(dc);
}

void DebugLine(vec2 pos, vec2 pos2, color col = Color_Red){DPZoneScoped;
	UIDrawCmd dc;
	MakeLine(dc, pos, pos2, 1, col);
	debugCmds.add(dc);
}

void DebugText(vec2 pos, str8 text, color col = Color_White){DPZoneScoped;
	UIDrawCmd dc;
	MakeText(dc, text, pos, col, vec2::ONE);
	debugCmds.add(dc);
}

void UI::BeginRow(str8 label, u32 columns, f32 rowHeight, UIRowFlags flags){DPZoneScoped;
	Assert(!StateHasFlag(UISRowBegan), "Attempted to start a new Row without finishing one already in progress!");
	if(!rows.has(label)){ 
		rows.add(label); 
		curRow = rows.at(label);
		curRow->columns.resize(columns);
		curRow->flags = flags;
		curRow->height = rowHeight;
		curRow->label = label;
		forI(columns) curRow->columns[i] = { 0.f,false };
	}
	
	
	curRow = rows.at(label);
	curRow->position = DecideItemPos();
	curRow->yoffset = 0;
	curRow->xoffset = 0;
	StateAddFlag(UISRowBegan);
}

void UI::EndRow(){DPZoneScoped;
	Assert(StateHasFlag(UISRowBegan), "Attempted to a end a row without calling BeginRow() first!");
	Assert(curRow->item_count % curRow->columns.count == 0, "Attempted to end a Row without giving the correct amount of items!");
	
	if(HasFlag(curRow->flags, UIRowFlags_AutoSize)){
		if(curRow->reeval_height) curRow->height = curRow->max_height;
		for(UIColumn& col : curRow->columns)
			if(col.reeval_width) col.width = col.max_width;
		
		if(curRow->max_height_frame < curRow->max_height) curRow->max_height = curRow->max_height_frame;
		
	}
	
	//if(HasFlag(curRow->flags, UIRowFlags_FitWidthOfArea)){
	//TODO set up Row fitting relative to given edges
	//}
	
	curRow->max_height_frame = 0;
	curwin->cursor = vec2{ 0, curRow->position.y + curRow->yoffset + style.itemSpacing.y - style.windowMargins.y + curwin->scroll.y };
	StateRemoveFlag(UISRowBegan);
}

//this function sets up a static column width for a specified column that does not respect the size of the object
void UI::RowSetupColumnWidth(u32 column, f32 width){DPZoneScoped;
	Assert(StateHasFlag(UISRowBegan), "Attempted to set a column's width with no Row in progress!");
	Assert(column <= curRow->columns.count, "Attempted to set a column who doesn't exists width!");
	if(!HasFlag(curRow->flags, UIRowFlags_AutoSize))
		curRow->columns[column] = { width, false };
}

//this function sets up static column widths that do not respect the size of the item at all
void UI::RowSetupColumnWidths(arrayT<f32> widths){DPZoneScoped;
	Assert(StateHasFlag(UISRowBegan), "Attempted to pass column widths without first calling BeginRow()!");
	Assert(widths.count == curRow->columns.count, "Passed in the wrong amount of column widths for in progress Row");
	if(!HasFlag(curRow->flags, UIRowFlags_AutoSize))
		forI(curRow->columns.count)
		curRow->columns[i] = { widths[i], false };
}

//see the function below for what this does
void UI::RowSetupRelativeColumnWidth(u32 column, f32 width){DPZoneScoped;
	Assert(StateHasFlag(UISRowBegan), "Attempted to set a column's width with no Row in progress!");
	Assert(column <= curRow->columns.count, "Attempted to set a column who doesn't exists width!");
	if(!HasFlag(curRow->flags, UIRowFlags_AutoSize))
		curRow->columns[column] = { width, true };
}

//this function sets it so that column widths are relative to the size of the item the cell holds
//meaning it should be passed something like 1.2 or 1.3, indicating that the column should have a width of 
//1.2 * width of the item
void UI::RowSetupRelativeColumnWidths(arrayT<f32> widths){DPZoneScoped;
	Assert(StateHasFlag(UISRowBegan), "Attempted to pass column widths without first calling BeginRow()!");
	Assert(widths.count == curRow->columns.count, "Passed in the wrong amount of column widths for in progress Row");
	if(!HasFlag(curRow->flags, UIRowFlags_AutoSize))
		forI(curRow->columns.count)
		curRow->columns[i] = { widths[i], true };
}

void UI::RowFitBetweenEdges(arrayT<f32> ratios, f32 left_edge, f32 right_edge){DPZoneScoped;
	Assert(StateHasFlag(UISRowBegan), "Attempted to pass column widths without first calling BeginRow()!");
	Assert(ratios.count == curRow->columns.count);
	//AddFlag(curRow->flags, UIRowFlags_FitWidthOfArea);
	curRow->left_edge = left_edge;
	curRow->right_edge = right_edge;
	f32 ratio_sum = 0;
	forI(curRow->columns.count){
		ratio_sum += ratios[i];
		curRow->columns[i].width = ratios[i];
	}
	
	Assert(1 - ratio_sum < 0.999998888f, "ratios given do not add up to one!");
}

void UI::RowSetupColumnAlignments(arrayT<vec2> alignments){DPZoneScoped;
	Assert(StateHasFlag(UISRowBegan), "Attempted to pass column widths without first calling BeginRow()!");
	Assert(alignments.count == curRow->columns.count);
	forI(curRow->columns.count)
		curRow->columns[i].alignment = alignments[i];
}

//@Behavoir functions
//these functions generalize behavoir that are used by several things

enum ButtonType_ {
	ButtonType_TrueOnHold,
	ButtonType_TrueOnRelease,
	ButtonType_TrueOnReleaseIfHovered,
	ButtonType_TrueOnPressed,
}; typedef u32 ButtonType;

//pos and size are expected to be screen space
b32 ButtonBehavoir(ButtonType type, vec2 pos = DefaultVec2, vec2 size = DefaultVec2){DPZoneScoped;
	switch (type){
		case ButtonType_TrueOnHold:{
			if(input_lmouse_down()){ PreventInputs; return true; }
			else return false;
		}break;
		case ButtonType_TrueOnRelease:{
			PreventInputs;
			if(input_lmouse_released()) return true;
			else return false;
		}break;
		case ButtonType_TrueOnPressed:{
			if(input_lmouse_pressed()){
				PreventInputs;
				return true;
			}
		}break;
		case ButtonType_TrueOnReleaseIfHovered:{
			Assert(pos != DefaultVec2 && size != DefaultVec2, "attempt to use buttontype TrueOnReleaseIfHovered without passing information about where to check for hover");
			if(input_lmouse_pressed() && Math::PointInRectangle(input_mouse_position(), pos, size)){
				PreventInputs;
			}
			if(input_lmouse_released() && Math::PointInRectangle(input_mouse_position(), pos, size)){
				return true;
			}
		}break;
	}
	return false;
}

//returns true if buffer was changed
b32 TextInputBehavoir(str8* buffer, u64 buffer_size, UIInputTextState* state){DPZoneScoped;
	//write input to buffer (if it fits)
	if(buffer->count + DeshInput->charCount < buffer_size){
		s32 space_left = buffer_size - state->cursor;
		CopyMemory(buffer->str + state->cursor + DeshInput->charCount,
				   buffer->str + state->cursor,
				   DeshInput->charCount*sizeof(u8));
		CopyMemory(buffer->str + state->cursor,
				   DeshInput->charIn,
				   DeshInput->charCount*sizeof(u8));
		buffer->count += DeshInput->charCount;
		state->cursor += DeshInput->charCount;
		buffer->str[buffer->count] = '\0';
	}
	
	//handle backspace or delete
	persist Stopwatch throttle = start_stopwatch();
	if(buffer->count){
		if(DeshInput->time_key_held < 500){
			if(key_pressed(Key_BACKSPACE) && state->cursor > 0){
				u32 advance = 0;
				while((state->cursor > 0) && utf8_continuation_byte(*(buffer->str+state->cursor-1))){
					state->cursor -= 1;
					advance += 1;
				}
				state->cursor -= 1;
				advance += 1;
				CopyMemory(buffer->str + state->cursor,
						   buffer->str + state->cursor + advance,
						   (buffer->str + buffer->count) - (buffer->str + state->cursor + advance) + 1);
				return true;
			}else if(key_pressed(Key_DELETE) && state->cursor < buffer->count){
				DecodedCodepoint decoded = decoded_codepoint_from_utf8(buffer->str+state->cursor,4);
				CopyMemory(buffer->str + state->cursor,
						   buffer->str + state->cursor + decoded.advance,
						   (buffer->str + buffer->count) - (buffer->str + state->cursor + decoded.advance) + 1);
				return true;
			}
		}else if(peek_stopwatch(throttle) > 25){
			reset_stopwatch(&throttle);
			if(key_down(Key_BACKSPACE) && state->cursor > 0){
				u32 advance = 0;
				while((state->cursor > 0) && utf8_continuation_byte(*(buffer->str+state->cursor-1))){
					state->cursor -= 1;
					advance += 1;
				}
				state->cursor -= 1;
				advance += 1;
				CopyMemory(buffer->str + state->cursor,
						   buffer->str + state->cursor + advance,
						   (buffer->str + buffer->count) - (buffer->str + state->cursor + advance) + 1);
				return true;
			}else if(key_down(Key_DELETE) && state->cursor < buffer->count){
				DecodedCodepoint decoded = decoded_codepoint_from_utf8(buffer->str+state->cursor,4);
				CopyMemory(buffer->str + state->cursor,
						   buffer->str + state->cursor + decoded.advance,
						   (buffer->str + buffer->count) - (buffer->str + state->cursor + decoded.advance) + 1);
				return true;
			}
		}
	}
	return DeshInput->charCount;
}


//@Primitive Items
//NOTE item pos conflicts with vertex positions, so set only one (see DrawItem())


void UI::Rect(vec2 pos, vec2 dimen, color color){DPZoneScoped;
	UIItem_old       item{ UIItemType_Abstract, curwin->cursor, style };
	UIDrawCmd drawCmd;
	MakeRect(drawCmd, vec2::ZERO, dimen, 1, color);
	AddDrawCmd(&item, drawCmd);
	
	item.position = pos;
	item.size = dimen;
	curwin->items[currlayer].add(item);
}

void UI::RectFilled(vec2 pos, vec2 dimen, color color){DPZoneScoped;
	UIItem_old       item{ UIItemType_Abstract, curwin->cursor, style };
	UIDrawCmd drawCmd;
	MakeFilledRect(drawCmd, vec2::ZERO, dimen, color);
	AddDrawCmd(&item, drawCmd);
	
	item.position = pos;
	item.size = dimen;
	curwin->items[currlayer].add(item);
}


//@Line


void UI::Line(vec2 start, vec2 end, f32 thickness, color color){DPZoneScoped;
	UIItem_old       item{ UIItemType_Abstract, curwin->cursor, style };
	UIDrawCmd drawCmd;
	MakeLine(drawCmd, start, end, thickness, color);
	AddDrawCmd(&item, drawCmd);
	
	item.position = vec2::ZERO;// { Min(drawCmd.position.x, drawCmd.position2.x), Min(drawCmd.position.y, drawCmd.position2.y) };
	item.    size = Max(start, end) - item.position;
	curwin->items[currlayer].add(item);
}


//@Circle


void UI::Circle(vec2 pos, f32 radius, f32 thickness, u32 subdivisions, color color){DPZoneScoped;
	UIItem_old       item{ UIItemType_Abstract, curwin->cursor, style };
	UIDrawCmd drawCmd;
	MakeCircle(drawCmd, pos, radius, subdivisions, thickness, color);
	AddDrawCmd(&item, drawCmd);
	
	item.position = vec2::ZERO;//pos - vec2::ONE * radius;
	item.size = vec2::ONE * radius * 2;
	
	//item.drawCmds.add(drawCmd);
	curwin->items[currlayer].add(item);
	
}

void UI::CircleFilled(vec2 pos, f32 radius, u32 subdivisions, color color){DPZoneScoped;
	UIItem_old       item{ UIItemType_Abstract, curwin->cursor, style };
	UIDrawCmd drawCmd;
	MakeFilledCircle(drawCmd, pos, radius, subdivisions, color);
	AddDrawCmd(&item, drawCmd);
	
	item.position = vec2::ZERO;//pos - vec2::ONE * radius;
	item.size = vec2::ONE * radius * 2;
	
	//item.drawCmds.add(drawCmd);
	curwin->items[currlayer].add(item);
}


//~//////////////////
////             ////
////    Items    ////
////             ////
/////////////////////
//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @TextOld
//main function for wrapping, where position is starting position of text relative to the top left of the window
//this function also decides if text is to be wrapped or not
local void TextW(str8 in, vec2 pos, color color, b32 nowrap, b32 move_cursor = true){DPZoneScoped;
	using namespace UI;
	if(!in){
		LogW("ui","TextOld() was passed an empty string.");
		return;
	}
	
	UIItem_old* item = BeginItem(UIItemType_Text);
	item->position = pos;
	vec2 workcur = vec2{ 0,0 };
	
	//TODO make this differenciate between monospace/non-monospace when i eventually add that to Font
	//split input text into lines based on wrapping and newlines
	f32 wscale = style.fontHeight / style.font->aspect_ratio / style.font->max_width;
	f32 maxw = MarginedRight() - item->position.x;
	f32 line_width = 0;
	u8* last_space = 0;
	f32 last_space_width = 0;
	u8* line_start = in.str;
	
	u32 codepoint;
	str8 remaining = in;
	while(remaining && (codepoint = str8_advance(&remaining).codepoint)){
		if      (codepoint == '\n'){
			str8 line{line_start, remaining.str-1-line_start};
			
			UIDrawCmd drawCmd;
			MakeText(drawCmd, line, workcur, color, GetTextScale());
			AddDrawCmd(item, drawCmd);
			
			workcur.y += style.fontHeight + style.itemSpacing.y;
			item->size.x = Max(item->size.x, line_width);
			line_width = 0;
			last_space = 0;
			last_space_width = 0;
			line_start = remaining.str;
		}else if(codepoint == ' '){
			last_space = remaining.str-1;
			last_space_width = line_width;
		}
		
		f32 char_width = 0;
		if      (style.font->type == FontType_TTF){
			char_width = font_packed_char(style.font, codepoint)->xadvance * wscale;
		}else if(style.font->type == FontType_BDF || style.font->type == FontType_NONE){
			char_width = style.font->max_width;
		}else{
			Assert(!"unknown font type?");
		}
		
		line_width += char_width;
		if(!nowrap && (line_width >= maxw && !HasFlag(curwin->flags, UIWindowFlags_FitAllElements))){
			str8 line{line_start, 0};
			if(last_space){
				line.count = last_space-line_start;
				line_start = last_space+1;
				item->size.x = Max(item->size.x, last_space_width);
			}else{
				line.count = remaining.str-1-line_start;
				line_start = remaining.str-1;
				item->size.x = Max(item->size.x, line_width-char_width);
			}
			if(line.count > 0){
				UIDrawCmd drawCmd;
				MakeText(drawCmd, line, workcur, color, GetTextScale());
				AddDrawCmd(item, drawCmd);
				
				workcur.y += style.fontHeight + style.itemSpacing.y;
				line_width = 0;
				last_space = 0;
				last_space_width = 0;
				str8_decrement(&remaining, remaining.str-line_start);
			}
		}
	}
	if(line_start < in.str+in.count){
		str8 line{line_start, in.str+in.count-line_start};
		
		UIDrawCmd drawCmd;
		MakeText(drawCmd, line, workcur, color, GetTextScale());
		AddDrawCmd(item, drawCmd);
		
		workcur.y += style.fontHeight + style.itemSpacing.y;
		item->size.x = Max(item->size.x, line_width);
	}
	
	if(NextItemSize.x != -1) item->size = NextItemSize;
	//else CalcItemSize(item);
	NextItemSize = vec2{ -1, 0 };
	item->size.y = workcur.y;
	
	AdvanceCursor(item, move_cursor);
}

void UI::TextOld(str8 text, UITextFlags flags){
	GetDefaultItemFlags(UIItemType_Text, flags);
	TextW(text, DecideItemPos(), style.colors[UIStyleCol_Text], HasFlag(flags, UITextFlags_NoWrap));
}

void UI::TextOld(str8 text, vec2 pos, UITextFlags flags){
	GetDefaultItemFlags(UIItemType_Text, flags);
	TextW(text, pos, style.colors[UIStyleCol_Text], HasFlag(flags, UITextFlags_NoWrap));
}

void UI::TextF(str8 fmt, ...){DPZoneScoped;
	string s(deshi_temp_allocator);
	va_list argptr;
	va_start(argptr, fmt);
	s.count = stbsp_vsnprintf(nullptr, 0, (const char*)fmt.str, argptr);
	s.str   = (char*)s.allocator->reserve(s.count+1); Assert(s.str, "Failed to allocate memory");
	s.space = s.count+1;
	stbsp_vsnprintf(s.str, s.count+1, (const char*)fmt.str, argptr);
	va_end(argptr);
	TextW(str8{(u8*)s.str, (s64)s.count}, DecideItemPos(), style.colors[UIStyleCol_Text], false);
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @Button
b32 UI::Button(str8 text, vec2 pos, UIButtonFlags flags){DPZoneScoped;
	vec2 text_size = CalcTextSize(text);
	
	UIItem_old* item = BeginItem(UIItemType_Button, flags);
	item->position = pos;
	//item->size = DecideItemSize(Vec2(Min(MarginedRight() - item->position.x, Max(50.f, CalcTextSize(text).x * 1.1f)), style.fontHeight * style.buttonHeightRelToFont), item->position);
	item->size = DecideItemSize(Vec2(ClampMin(1.1f*text_size.x, 50.f), style.fontHeight * style.buttonHeightRelToFont), item->position);
	AdvanceCursor(item);
	
	b32 active = WinHovered(curwin) && isItemHovered(item);
	
	{//background
		UIDrawCmd drawCmd;
		vec2  bgpos = vec2::ZERO;
		vec2  bgdim = item->size;
		color bgcol = style.colors[(active ? (input_lmouse_down() ? UIStyleCol_ButtonBgActive : UIStyleCol_ButtonBgHovered) : UIStyleCol_ButtonBg)];
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
		vec2 texpos = Vec2((item->size.x - text_size.x) * style.buttonTextAlign.x,
						   (style.fontHeight * style.buttonHeightRelToFont - style.fontHeight) * style.buttonTextAlign.y);
		color texcol = style.colors[UIStyleCol_Text];
		MakeText(drawCmd, text, texpos, texcol, GetTextScale());
		AddDrawCmd(item, drawCmd);
	}
	
	if(active){
		//TODO just pass the flags to the behavoir function and just let it figure it out 
		if(HasFlag(item->flags, UIButtonFlags_ReturnTrueOnHold))
			return ButtonBehavoir(ButtonType_TrueOnHold);
		if(HasFlag(item->flags, ButtonType_TrueOnPressed))
			return ButtonBehavoir(ButtonType_TrueOnRelease);
		if(HasFlag(item->flags, UIButtonFlags_ReturnTrueOnRelease))
			return ButtonBehavoir(ButtonType_TrueOnRelease);
		return ButtonBehavoir(ButtonType_TrueOnReleaseIfHovered, curwin->position + item->position, item->size);
	}
	return false;
}

b32 UI::Button(str8 text, UIButtonFlags flags){DPZoneScoped;
	return Button(text, DecideItemPos(), flags);
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @Checkbox
void UI::Checkbox(str8 label, b32* b){DPZoneScoped;
	UIItem_old* item = BeginItem(UIItemType_Checkbox);
	
	vec2 boxpos = DecideItemPos();
	vec2 boxsiz = vec2::ONE * style.checkboxHeightRelToFont * style.fontHeight;
	
	item->position = boxpos;
	item->size = boxsiz;
	item->size.x += style.itemSpacing.x + CalcTextSize(label).x;
	AdvanceCursor(item);
	
	u32 fillPadding = (u32)style.checkboxFillPadding;
	vec2 fillpos = boxsiz * Vec2(fillPadding / boxsiz.x, fillPadding / boxsiz.y);
	vec2 fillsiz = boxsiz * (vec2::ONE - 2 * Vec2(fillPadding / boxsiz.x, fillPadding / boxsiz.y));
	b32 bgactive = isItemActive(item);
	
	{//box
		UIDrawCmd drawCmd;
		vec2  position = vec2{ 0,0 };
		vec2  dimensions = boxsiz;
		color col = style.colors[(bgactive ? (input_lmouse_down() ? UIStyleCol_CheckboxBgActive : UIStyleCol_CheckboxBgHovered) : UIStyleCol_CheckboxBg)];
		MakeFilledRect(drawCmd, position, dimensions, col);
		AddDrawCmd(item, drawCmd);
	}
	
	//fill if true
	if(*b){
		UIDrawCmd drawCmd;
		vec2  position = fillpos;
		vec2  dimensions = fillsiz;
		MakeFilledRect(drawCmd, position, dimensions, style.colors[UIStyleCol_CheckboxFilling]);
		AddDrawCmd(item, drawCmd);
	}
	
	{//border
		UIDrawCmd drawCmd;
		vec2  position = vec2::ZERO;
		vec2  dimensions = boxsiz;
		MakeRect(drawCmd, position, dimensions, 1, style.colors[UIStyleCol_CheckboxBorder]);
		AddDrawCmd(item, drawCmd);
	}
	
	{//label
		UIDrawCmd drawCmd;
		vec2  position = Vec2(boxsiz.x + style.itemSpacing.x, (boxsiz.y - style.fontHeight) * 0.5);
		MakeText(drawCmd, label, position, style.colors[UIStyleCol_Text], GetTextScale());
		AddDrawCmd(item, drawCmd);
	}
	
	if(input_lmouse_pressed() && bgactive){
		*b = !(*b);
		PreventInputs;
	}
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @Combo
//a combo is built by selectables called within its Begin/End
b32 UI::BeginCombo(str8 label, str8 preview, vec2 pos){DPZoneScoped;
	vec2 preview_size = CalcTextSize(preview);
	
	UIItem_old* item = BeginItem(UIItemType_Combo, 1);
	item->position = pos;
	item->size = DecideItemSize(preview_size * 1.5, item->position);
	
	AdvanceCursor(item);
	
	if(!combos.has(label)){
		combos.add(label);
		combos[label] = false;
	}
	
	b32& open = combos[label];
	b32 active = isItemActive(item);
	if(active && input_lmouse_pressed()){
		open = !open; 
		PreventInputs;
	}
	
	{//background
		UIDrawCmd drawCmd;
		vec2  position = vec2::ZERO;
		vec2  dimensions = item->size;
		color col = style.colors[(active ? (input_lmouse_down() ? UIStyleCol_SelectableBgActive : UIStyleCol_SelectableBgHovered) : UIStyleCol_SelectableBg)];
		MakeFilledRect(drawCmd, position, dimensions, col);
		AddDrawCmd(item, drawCmd);
	}
	
	{//text
		UIDrawCmd drawCmd;
		drawCmd.scissorOffset = vec2::ZERO;
		drawCmd.scissorExtent = item->size;
		drawCmd.useWindowScissor = false;
		vec2 position = Vec2((item->size.x - preview_size.x) * style.buttonTextAlign.x,
							 (style.fontHeight * style.buttonHeightRelToFont - style.fontHeight) * style.buttonTextAlign.y);
		MakeText(drawCmd, preview, position, style.colors[UIStyleCol_Text], GetTextScale());
		AddDrawCmd(item, drawCmd);
	}
	
	//we also check if the combo's button is visible, if not we dont draw the popout
	if(open && item->position.y < curwin->height){
		PushVar(UIStyleVar_WindowMargins, Vec2(0, 0));
		PushVar(UIStyleVar_ItemSpacing,   Vec2(0, 0));
		//TODO(sushi) !FixMe the combo selection popout doesnt actually follow the button
		SetNextWindowSize(Vec2(item->size.x, style.fontHeight * style.selectableHeightRelToFont * 8));
		BeginPopOut(str8_concat(str8_lit("comboPopOut"),label,deshi_temp_allocator), item->position.yAdd(item->size.y), vec2::ZERO, UIWindowFlags_NoBorder);
		StateAddFlag(UISComboBegan);
		return true;
	}
	return false;
}

b32 UI::BeginCombo(str8 label, str8 preview){DPZoneScoped;
	return BeginCombo(label, preview, DecideItemPos());
}

void UI::EndCombo(){DPZoneScoped;
	Assert(StateHasFlag(UISComboBegan), "Attempted to end a combo without calling BeginCombo first, or EndCombo was called for a combo that was not open!");
	StateRemoveFlag(UISComboBegan);
	EndPopOut();
	PopVar(2);
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @Selectable
b32 SelectableCall(str8 label, vec2 pos, b32 selected, b32 move_cursor = 1){DPZoneScoped;
	UIItem_old* item = BeginItem(UIItemType_Selectable, 0, 0);
	item->position = pos;
	
	vec2 label_size = UI::CalcTextSize(label);
	vec2 defsize;
	if(StateHasFlag(UISComboBegan)) defsize = Vec2(MAX_F32, style.fontHeight * style.selectableHeightRelToFont);
	else                            defsize = Vec2(label_size.x, style.fontHeight * style.selectableHeightRelToFont);
	item->size = DecideItemSize(defsize, item->position);
	AdvanceCursor(item, move_cursor);
	
	b32 clicked = 0;
	b32 active = isItemActive(item);//ItemCanTakeInput && Math::PointInRectangle(input_mouse_position(), curwin->position + item->position.yAdd(item->size.y * selectables_added), item->size);
	if(active && input_lmouse_pressed()){
		clicked = true;
		PreventInputs;
	}
	
	{//background
		UIDrawCmd drawCmd;
		vec2  position = vec2::ZERO;
		vec2  dimensions = item->size;
		color col;
		if(selected) col = style.colors[(active && input_lmouse_down() ? UIStyleCol_SelectableBgActive : UIStyleCol_SelectableBgHovered)];
		else         col = style.colors[(active ? (input_lmouse_down() ? UIStyleCol_SelectableBgActive : UIStyleCol_SelectableBgHovered) : UIStyleCol_SelectableBg)];
		MakeFilledRect(drawCmd, position, dimensions, col);
		AddDrawCmd(item, drawCmd);
	}
	
	{//text
		UIDrawCmd drawCmd;
		vec2 position = Vec2((item->size.x - label_size.x) * style.selectableTextAlign.x,
							 (style.fontHeight * style.selectableHeightRelToFont - style.fontHeight) * style.selectableTextAlign.y);
		MakeText(drawCmd, label, position, style.colors[UIStyleCol_Text], GetTextScale());
		AddDrawCmd(item, drawCmd);
	}
	
	return clicked;
}

b32 UI::Selectable(str8 label, vec2 pos, b32 selected){DPZoneScoped;
	return SelectableCall(label, pos, selected, 0);
}

b32 UI::Selectable(str8 label, b32 selected){DPZoneScoped;
	return SelectableCall(label, DecideItemPos(), selected);
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @Header
b32 UI::BeginHeader(str8 label, UIHeaderFlags flags){DPZoneScoped;
	UIItem_old* item = BeginItem(UIItemType_Header, flags);
	
	b32* open = 0;
	if(!headers.has(label)){
		headers.add(label);
		headers[label].open = false;
	}
	headerStack.add(&headers[label]);
	(*headerStack.last)->flags = flags;
	open = &headers[label].open;
	
	item->position = DecideItemPos();
	item->size = DecideItemSize(Vec2(MAX_F32, style.fontHeight * style.headerHeightRelToFont), item->position);
	AdvanceCursor(item);
	
	b32 active = isItemActive(item);
	if(active && input_lmouse_pressed()){ 
		*open = !*open; 
		PreventInputs;
	}
	
	if(*open){
		if(!HasFlag(item->flags, UIHeaderFlags_NoIndentLeft))  PushLeftIndent(style.indentAmount + leftIndent); 
		if(!HasFlag(item->flags, UIHeaderFlags_NoIndentRight)) PushRightIndent(style.indentAmount + rightIndent);
	}
	
	f32 arrowSpaceWidth = style.indentAmount;
	f32 arrowwidth = ceil(arrowSpaceWidth / 2);
	f32 arrowheight = ceil(item->size.y / 1.5);
	vec2 bgpos = vec2{ arrowSpaceWidth, 0 };
	vec2 bgdim = vec2{ item->size.x - bgpos.x, item->size.y };
	
	{//background
		UIDrawCmd drawCmd;
		vec2 position = bgpos;
		vec2 dimensions = bgdim;
		color col = style.colors[(active ? (input_lmouse_down() ? UIStyleCol_HeaderBgActive : UIStyleCol_HeaderBgHovered) : UIStyleCol_HeaderBg)];
		MakeFilledRect(drawCmd, position, dimensions, col);
		AddDrawCmd(item, drawCmd);
	}
	
	{//button
		UIDrawCmd drawCmd;
		vec2  position = vec2{ item->size.y / 4, item->size.y / 2 };
		f32   radius = item->size.y / 4;
		color col = style.colors[(active ? (input_lmouse_down() ? UIStyleCol_HeaderBgActive : UIStyleCol_HeaderBgHovered) : UIStyleCol_HeaderBg)];
		
		//TODO(sushi) this is ugly please fix it 
		if(*open){ 
			vec2 arrowright = Vec2((arrowSpaceWidth - arrowwidth) / 2, (item->size.y - arrowheight) / 2);
			vec2 arrowleft = Vec2((arrowSpaceWidth + arrowwidth) / 2, (item->size.y - arrowheight) / 2);
			vec2 arrowpoint = Vec2(arrowSpaceWidth / 2, (item->size.y + arrowheight) / 2);
			MakeFilledTriangle(drawCmd, arrowleft, arrowright, arrowpoint, col);
		}else{
			vec2 arrowtop = Vec2((arrowSpaceWidth - arrowwidth) / 2, (item->size.y - arrowheight) / 2);
			vec2 arrowpoint = Vec2((arrowSpaceWidth + arrowwidth) / 2, item->size.y / 2);
			vec2 arrowbot = Vec2((arrowSpaceWidth - arrowwidth) / 2, (item->size.y + arrowheight) / 2);
			MakeFilledTriangle(drawCmd, arrowtop, arrowpoint, arrowbot, col);
		}
		AddDrawCmd(item, drawCmd);
	}
	
	{//text
		UIDrawCmd drawCmd;
		drawCmd.scissorOffset = vec2::ZERO;
		drawCmd.scissorExtent = item->size;
		drawCmd.useWindowScissor = false;
		vec2 position = Vec2(bgpos.x + (item->size.x - bgpos.x - style.windowMargins.x - UI::CalcTextSize(label).x) * style.headerTextAlign.x + 3,
							 (style.fontHeight * style.headerHeightRelToFont - style.fontHeight) * style.headerTextAlign.y);
		MakeText(drawCmd, delimitLabel(label), position, style.colors[UIStyleCol_Text], GetTextScale());
		AddDrawCmd(item, drawCmd);
	}
	
	if(!HasFlag(item->flags, UIHeaderFlags_NoBorder)){//border
		UIDrawCmd drawCmd;
		vec2  position = bgpos;
		vec2  dimensions = bgdim;
		MakeRect(drawCmd, position, dimensions, 1, style.colors[UIStyleCol_HeaderBorder]);
		AddDrawCmd(item, drawCmd);
	}
	
	return *open;
}

void UI::EndHeader(){DPZoneScoped;
	Assert(headerStack.count, "attempt to end a header that doesnt exist");
	if(!HasFlag((*headerStack.last)->flags, UIHeaderFlags_NoIndentLeft))  PopLeftIndent();
	if(!HasFlag((*headerStack.last)->flags, UIHeaderFlags_NoIndentRight)) PopRightIndent();
	headerStack.pop();
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @TabBar @Tab
void UI::BeginTabBar(str8 label, UITabBarFlags flags){DPZoneScoped;
	Assert(!StateHasFlag(UISTabBarBegan), "attempt to start a new tab bar without finishing one");
	StateAddFlag(UISTabBarBegan);
	
	if(!tabBars.has(label)) tabBars.add(label);
	curTabBar = tabBars.at(label);
	
	f32 tabBarLineHeight = 3;
	
	UIItem_old* item = BeginItem(UIItemType_TabBar);
	item->position = DecideItemPos();
	item->size     = DecideItemSize(Vec2(MAX_F32, style.tabHeightRelToFont * style.fontHeight + tabBarLineHeight), item->position);
	AdvanceCursor(item);
	
	GetDefaultItemFlags(UIItemType_TabBar, flags);
	curTabBar->flags     = flags;
	curTabBar->tabHeight = style.tabHeightRelToFont * style.fontHeight;
	curTabBar->item      = item;
	
	{//bar
		UIDrawCmd drawCmd;
		vec2  position = Vec2(0, curTabBar->tabHeight);
		vec2  dimensions = Vec2(item->size.x, tabBarLineHeight);
		MakeFilledRect(drawCmd, position, dimensions, style.colors[UIStyleCol_TabBar]);
		AddDrawCmd(item, drawCmd);
	}
}

b32 UI::BeginTab(str8 label){DPZoneScoped;
	Assert(StateHasFlag(UISTabBarBegan), "attempt to begin a tab without beginning a tab bar first");
	
	vec2 label_size = CalcTextSize(label);
	
	UITab* tab = 0;
	if(!curTabBar->tabs.has(label)){ 
		curTabBar->tabs.add(label);
		tab = curTabBar->tabs.at(label);
		tab->height = style.tabHeightRelToFont * style.fontHeight;
		tab->width  = label_size.x * 1.2;
	}
	tab = curTabBar->tabs.at(label);
	//UITab& tab = curtabBar->tabs[label];
	
	UIItem_old* item = BeginItem(UIItemType_Tab);
	item->position = Vec2(curTabBar->item->position.x + curTabBar->xoffset, curTabBar->item->position.y);
	item->size = Vec2(tab->width, tab->height);
	tab->item = item;
	
	curTabBar->xoffset += tab->width + style.tabSpacing;
	
	b32 selected = (tab == curTabBar->tabs.atIdx(curTabBar->selected));
	b32 active = isItemActive(item) || selected;
	{//background
		UIDrawCmd drawCmd;
		vec2  position   = vec2::ZERO;
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
		vec2 position = Vec2((item->size.x - label_size.x) * style.tabTextAlign.x,
							 (style.fontHeight * style.tabHeightRelToFont - style.fontHeight) * style.tabTextAlign.y);
		MakeText(drawCmd, delimitLabel(label), position, style.colors[UIStyleCol_Text], GetTextScale());
		AddDrawCmd(item, drawCmd);
	}
	
	{//border
		UIDrawCmd drawCmd;
		vec2  position   = vec2::ZERO;
		vec2  dimensions = item->size;
		MakeRect(drawCmd, position, dimensions, 1, style.colors[UIStyleCol_TabBorder]);
		AddDrawCmd(item, drawCmd);
	}
	
	if(selected){
		StateAddFlag(UISTabBegan);
		if(!HasFlag(curTabBar->flags, UITabBarFlags_NoLeftIndent))  PushLeftIndent (style.indentAmount + leftIndent);
		if(!HasFlag(curTabBar->flags, UITabBarFlags_NoRightIndent)) PushRightIndent(style.indentAmount + rightIndent);
	}
	return selected;
}

void UI::EndTab(){DPZoneScoped;
	Assert(StateHasFlag(UISTabBegan), "attempt to end a tab without beginning one first");
	StateRemoveFlag(UISTabBegan);
	if(!HasFlag(curTabBar->flags, UITabBarFlags_NoLeftIndent))  PopLeftIndent();
	if(!HasFlag(curTabBar->flags, UITabBarFlags_NoRightIndent)) PopRightIndent();
}

void UI::EndTabBar(){DPZoneScoped;
	Assert(!StateHasFlag(UISTabBegan), "attempt to end a tab bar without ending a tab");
	Assert(StateHasFlag(UISTabBarBegan), "attempt to end a tab bar without beginning one first");
	StateRemoveFlag(UISTabBarBegan);
	
	curTabBar->xoffset = 0;
	forI(curTabBar->tabs.count){
		UIItem_old* item = curTabBar->tabs.atIdx(i)->item;
		if(WinHovered(curwin) && LeftMousePressed && MouseInWinArea(item->position, item->size)){
			curTabBar->selected = i;
		}
	}
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @Slider
void UI::Slider(str8 label, f32* val, f32 val_min, f32 val_max, UISliderFlags flags){DPZoneScoped;
	b32 being_moved = 0;
	if(!sliders.has(label)){
		sliders.add(label);
		sliders[label] = false;
	}else{
		being_moved = sliders[label];
	}
	
	UIItem_old* item = BeginItem(UIItemType_Slider, flags);
	item->position = DecideItemPos();
	item->size = DecideItemSize(vec2{ curwin->width * M_ONETHIRD, 10 }, item->position);
	AdvanceCursor(item);
	
	b32 active = isItemActive(item);
	if(active && input_lmouse_pressed()){
		sliders[label] = 1;
		PreventInputs;
	}
	if(input_lmouse_released()){
		sliders[label] = 0;
		AllowInputs;
	}
	if(being_moved && input_lmouse_down()){
		f32 ratio = (input_mouse_position().x - item->position.x - curwin->position.x) / item->size.x;
		*val = ratio * (val_max - val_min) + val_min;
	}
	*val = Clamp(*val, val_min, val_max);
	
	vec2 draggersiz = vec2{ item->size.x / 8, item->size.y };
	vec2 draggerpos = vec2{ (*val - val_min) / (val_max - val_min) * (item->size.x - draggersiz.x), 0 };
	
	{//background
		UIDrawCmd drawCmd;
		vec2  position = vec2::ZERO;
		vec2  dimensions = item->size;
		MakeFilledRect(drawCmd, position, dimensions, style.colors[UIStyleCol_SliderBg]);
		AddDrawCmd(item, drawCmd);
	}
	
	{//dragger
		UIDrawCmd drawCmd;
		vec2  position = draggerpos;
		vec2  dimensions = draggersiz;
		color col = style.colors[((active || being_moved) ? (input_lmouse_down() ? UIStyleCol_SliderBarActive : UIStyleCol_SliderBarHovered) : UIStyleCol_SliderBar)];
		MakeFilledRect(drawCmd, position, dimensions, col);
		AddDrawCmd(item, drawCmd);
	}
	
	{//border
		UIDrawCmd drawCmd;
		vec2  position = vec2::ZERO;
		vec2  dimensions = item->size;
		MakeRect(drawCmd, position, dimensions, 1, style.colors[UIStyleCol_SliderBorder]);
		AddDrawCmd(item, drawCmd);
	}
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @Image
void UI::Image(Texture* image, vec2 pos, f32 alpha, UIImageFlags flags){DPZoneScoped;
	UIItem_old* item = BeginItem(UIItemType_Image, flags);
	item->position = pos;
	item->size = DecideItemSize(Vec2((f32)image->width, (f32)image->height), item->position);
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

void UI::Image(Texture* image, f32 alpha, UIImageFlags flags){DPZoneScoped;
	Image(image, DecideItemPos(), alpha, flags);
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @Separator
void UI::Separator(f32 height){DPZoneScoped;
	UIItem_old* item = BeginItem(UIItemType_Separator);
	item->position = DecideItemPos();
	item->size = Vec2(MarginedRight() - item->position.x, height);
	AdvanceCursor(item);
	
	UIDrawCmd drawCmd;
	vec2 start = Vec2(0, height / 2);
	vec2 end = Vec2(item->size.x, height / 2);
	MakeLine(drawCmd, start, end, 1, style.colors[UIStyleCol_Separator]);
	AddDrawCmd(item, drawCmd);
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @InputText
//final input text
b32 InputTextCall(str8 label, u8* buff, u32 buffSize, vec2 position, str8 preview, UIInputTextCallback callback, UIInputTextFlags flags, b32 moveCursor){DPZoneScoped;
	UIItem_old* item = BeginItem(UIItemType_InputText);
	item->position = position;
	
	str8 buffer{buff,0};
	for(u8* p = buff; *p != 0; ++p, ++buffer.count);
	vec2 dim;
	GetDefaultItemFlags(UIItemType_InputText, flags);
	if(HasFlag(flags, UIInputTextFlags_FitSizeToText)){
		dim = UI::CalcTextSize(buffer);
	}else{
		dim = DecideItemSize(Vec2(Clamp(100.f, 0.f, Clamp(curwin->width - 2.f * style.windowMargins.x, 1.f, FLT_MAX)),
								  style.inputTextHeightRelToFont * style.fontHeight), item->position);
	}
	item->size = dim;
	AdvanceCursor(item, moveCursor);
	
	UIInputTextState* state;
	if(!(state = inputTexts.at(label))){
		state = inputTexts.atIdx(inputTexts.add(label));
		state->id              = hash<str8>{}(label);
		state->cursor          = buffer.count;
		state->cursorBlinkTime = 5;
		state->selectStart     = 0;
		state->selectEnd       = 0;
	}else{
		state->callback        = callback;
	}
	
	b32 hovered = isItemActive(item);
	b32 active = CanTakeInput && (activeId == state->id);
	if(NextActive || key_pressed(Mouse_LEFT)){
		if(NextActive || hovered){
			activeId = state->id;
			StateRemoveFlag(UISNextItemActive);
		}else if(active){
			activeId = -1;
		}
	}
	
	if(hovered) SetWindowCursor(CursorType_IBeam);
	if(state->cursor > buffer.count) state->cursor = buffer.count;
	
	//data for callback function
	UIInputTextCallbackData data;
	data.flags          = flags;
	data.buffer         = buff;
	data.bufferSize     = buffSize;
	data.selectionStart = state->selectStart;
	data.selectionEnd   = state->selectEnd;
	b32 bufferChanged = false;
	if(active){
		if(key_pressed(Key_RIGHT) && state->cursor < buffer.count){
			DecodedCodepoint decoded = decoded_codepoint_from_utf8(buffer.str+state->cursor, 4);
			state->cursor += decoded.advance;
		}
		if(key_pressed(Key_LEFT) && state->cursor > 0){
			while((state->cursor > 0) && utf8_continuation_byte(*(buffer.str+state->cursor-1))){
				state->cursor -= 1;
			}
			state->cursor -= 1;
		}
		if(key_pressed(Key_HOME)) state->cursor = 0;
		if(key_pressed(Key_END)) state->cursor = buffer.count;
		data.cursorPos = state->cursor;
		
		//check if the user used up/down keys
		if(key_pressed(Key_UP)   && HasFlag(flags, UIInputTextFlags_CallbackUpDown)){
			data.eventFlag = UIInputTextFlags_CallbackUpDown;
			data.eventKey  = Key_UP;
			callback(&data);
		}
		if(key_pressed(Key_DOWN) && HasFlag(flags, UIInputTextFlags_CallbackUpDown)){
			data.eventFlag = UIInputTextFlags_CallbackUpDown;
			data.eventKey  = Key_DOWN;
			callback(&data);
		}
		
		//actual text input behavoir occurs here ---------------------------------------------------------
		if(!HasFlag(flags, UIInputTextFlags_NoEdit) && TextInputBehavoir(&buffer, buffSize, state)){
			bufferChanged = true;
			state->timeSinceTyped = start_stopwatch();
		}
	}
	
	if(!HasFlag(flags, UIInputTextFlags_NoBackground)){//text box
		UIDrawCmd drawCmd;
		vec2  position = vec2::ZERO;
		vec2  dimensions = dim;
		color col = style.colors[(!active ? (hovered ? UIStyleCol_InputTextBgHovered : UIStyleCol_InputTextBg) : UIStyleCol_InputTextBg)];
		MakeFilledRect(drawCmd, position, dimensions, col);
		AddDrawCmd(item, drawCmd);
	}
	
	vec2 buffer_size = UI::CalcTextSize(buffer);
	vec2 textStart = Vec2((dim.x - buffer_size.x) * style.inputTextTextAlign.x + 3, //TODO(sushi) make an input text offset style var
						  (style.fontHeight * style.inputTextHeightRelToFont - style.fontHeight) * style.inputTextTextAlign.y);
	{//text
		UIDrawCmd drawCmd;
		if(preview && (*buff == '\0')){
			color col = style.colors[UIStyleCol_Text];
			col.a = (u8)floor(0.5f * 255);
			MakeText(drawCmd, preview, textStart, col, GetTextScale());
		}else{
			color col = style.colors[UIStyleCol_Text];
			MakeText(drawCmd, buffer,  textStart, col, GetTextScale());
		}
		AddDrawCmd(item, drawCmd);
	}
	
	{//border
		UIDrawCmd drawCmd;
		vec2  position   = vec2::ZERO;
		vec2  dimensions = item->size;
		MakeRect(drawCmd, position, dimensions, 1, style.colors[UIStyleCol_InputTextBorder]);
		AddDrawCmd(item, drawCmd); 
	}
	
	//TODO(sushi, Ui) impl different text cursors
	f32 lineoffset = UI::CalcTextSize(str8{buff, state->cursor}).x;
	if(active){//cursor
		UIDrawCmd drawCmd;
		vec2  start = textStart + Vec2(lineoffset, 0);//Vec2(state->cursor * style.font->max_width * style.fontHeight / style.font->aspect_ratio / style.font->max_width, 0);
		vec2  end   = textStart + Vec2(lineoffset, style.fontHeight - 1);
		f64 stopwatch = peek_stopwatch(state->timeSinceTyped);
		color col(255, 255, 255, 
				  u8(255 * (cos(M_2PI / (state->cursorBlinkTime / 2) * stopwatch / 1000 
								- sin(M_2PI / (state->cursorBlinkTime / 2) * stopwatch / 1000)) + 1) / 2));
		MakeLine(drawCmd, start, end, 1, col);
		AddDrawCmd(item, drawCmd);
	}
	
	if      (HasFlag(flags, UIInputTextFlags_EnterReturnsTrue)     && key_pressed(Key_ENTER)){
		return true;
	}else if(HasFlag(flags, UIInputTextFlags_AnyChangeReturnsTrue) && bufferChanged){
		return true;
	}
	return false;
}
b32 UI::InputText(str8 label, u8* buffer, u32 buffSize, str8 preview, UIInputTextFlags flags){DPZoneScoped;
	return InputTextCall(label, buffer, buffSize, DecideItemPos(), preview, nullptr, flags, 1);
}
b32 UI::InputText(str8 label, u8* buffer, u32 buffSize,  UIInputTextCallback callback, str8 preview, UIInputTextFlags flags){DPZoneScoped;
	return InputTextCall(label, buffer, buffSize, DecideItemPos(), preview, callback, flags, 1);
}
b32 UI::InputText(str8 label, u8* buffer, u32 buffSize, vec2 pos, str8 preview, UIInputTextFlags flags){DPZoneScoped;
	return InputTextCall(label, buffer, buffSize, pos, preview, nullptr, flags, 0);
}
b32 UI::InputText(str8 label, u8* buffer, u32 buffSize, vec2 pos, UIInputTextCallback callback, str8 preview, UIInputTextFlags flags){DPZoneScoped;
	return InputTextCall(label, buffer, buffSize, pos, preview, callback, flags, 0);
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @Menu
//void BeginMenuCall(vec2 pos, vec2 size, UIMenuFlags flags){
//	UIItem_old* item = BeginItem(UIItemType_Menu, flags);
//}
//
//void UI::BeginMenu(vec2 pos, vec2 size, UIMenuFlags flags){
//	BeginMenuCall(pos, size, flags);
//}
//
//void UI::BeginMenu(vec2 pos, UIMenuFlags flags){
//	BeginMenuCall(pos, Vec2(MAX_F32,MAX_F32), flags);
//}
//
//void UI::EndMenu(){
//	
//}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @Custom
UIItem_old* UI::BeginCustomItem(){
	UIItem_old* item = BeginItem(UIItemType_Custom);
	StateAddFlag(UISCustomItemBegan);
	return item;
}

void UI::CustomItem_AdvanceCursor(UIItem_old* item, b32 move_cursor){
	Assert(StateHasFlag(UISCustomItemBegan), "attempt to use a custom item command without beginning a custom item first");
	AdvanceCursor(item, move_cursor);
}

void UI::EndCustomItem(){
	Assert(StateHasFlag(UISCustomItemBegan), "attempt to end a custom item without begining one first");
	StateRemoveFlag(UISCustomItemBegan);
}

//TODO decide if we should just expose the internal drawing commands 
void UI::CustomItem_DCMakeLine(UIDrawCmd& drawCmd, vec2 start, vec2 end, f32 thickness, color color){
	Assert(StateHasFlag(UISCustomItemBegan), "attempt to use a custom item command without beginning a custom item first");
	MakeLine( drawCmd, start, end, thickness, color);
}
void UI::CustomItem_DCMakeFilledTriangle(UIDrawCmd& drawCmd, vec2 p1, vec2 p2, vec2 p3, color color){
	Assert(StateHasFlag(UISCustomItemBegan), "attempt to use a custom item command without beginning a custom item first");
	MakeFilledTriangle( drawCmd, p1, p2, p3, color);
}
void UI::CustomItem_DCMakeTriangle(UIDrawCmd& drawCmd, vec2 p1, vec2 p2, vec2 p3, f32 thickness, color color){
	Assert(StateHasFlag(UISCustomItemBegan), "attempt to use a custom item command without beginning a custom item first");
	MakeTriangle( drawCmd, p1, p2, p3, thickness, color);
}
void UI::CustomItem_DCMakeFilledRect(UIDrawCmd& drawCmd, vec2 pos, vec2 size, color color){
	Assert(StateHasFlag(UISCustomItemBegan), "attempt to use a custom item command without beginning a custom item first");
	MakeFilledRect( drawCmd, pos, size, color);
}
void UI::CustomItem_DCMakeRect(UIDrawCmd& drawCmd, vec2 pos, vec2 size, f32 thickness, color color){
	Assert(StateHasFlag(UISCustomItemBegan), "attempt to use a custom item command without beginning a custom item first");
	MakeRect( drawCmd, pos, size, thickness, color);
}
void UI::CustomItem_DCMakeCircle(UIDrawCmd& drawCmd, vec2 pos, f32 radius, u32 subdivisions, f32 thickness, color color){
	Assert(StateHasFlag(UISCustomItemBegan), "attempt to use a custom item command without beginning a custom item first");
	MakeCircle( drawCmd, pos, radius, subdivisions, thickness, color);
}
void UI::CustomItem_DCMakeFilledCircle(UIDrawCmd& drawCmd, vec2 pos, f32 radius, u32 subdivisions_int, color color){
	Assert(StateHasFlag(UISCustomItemBegan), "attempt to use a custom item command without beginning a custom item first");
	MakeFilledCircle( drawCmd, pos, radius, subdivisions_int, color);
}
void UI::CustomItem_DCMakeText(UIDrawCmd& drawCmd, str8 text, vec2 pos, color color, vec2 scale){
	Assert(StateHasFlag(UISCustomItemBegan), "attempt to use a custom item command without beginning a custom item first");
	MakeText( drawCmd, text, pos, color, scale);
}
void UI::CustomItem_DCMakeTexture(UIDrawCmd& drawCmd, Texture* texture, vec2 p0, vec2 p1, vec2 p2, vec2 p3, f32 alpha, b32 flipx, b32 flipy){
	Assert(StateHasFlag(UISCustomItemBegan), "attempt to use a custom item command without beginning a custom item first");
	render_make_texture(drawCmd.vertices, drawCmd.indices, drawCmd.counts, texture, p0, p1, p2, p3, alpha, flipx, flipy);
}
void UI::CustomItem_AddDrawCmd(UIItem_old* item, UIDrawCmd& drawCmd){
	Assert(StateHasFlag(UISCustomItemBegan), "attempt to use a custom item command without beginning a custom item first");
	AddDrawCmd(item, drawCmd);
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @State
b32 UI::IsLastItemHovered(){DPZoneScoped; 
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


//~/////////////////////
////                ////
////    Push/Pop    ////
////                ////
////////////////////////
void UI::PushColor(UIStyleCol idx, color color){DPZoneScoped;
	//save old color
	colorStack.add(ColorMod{ idx, style.colors[idx] });
	//change to new color
	style.colors[idx] = color;
}

void UI::PushVar(UIStyleVar idx, f32 nuStyle){DPZoneScoped;
	Assert(uiStyleVarTypes[idx].count == 1, "Attempt to use a f32 on a vec2 style variable!");
	f32* p = (f32*)((u8*)&style + uiStyleVarTypes[idx].offset);
	varStack.add(UIVarMod(idx, *p));
	*p = nuStyle;
}

void UI::PushVar(UIStyleVar idx, vec2 nuStyle){DPZoneScoped;
	Assert(uiStyleVarTypes[idx].count == 2, "Attempt to use a f32 on a vec2 style variable!");
	vec2* p = (vec2*)((u8*)&style + uiStyleVarTypes[idx].offset);
	varStack.add(UIVarMod(idx, *p));
	*p = nuStyle;
}

void UI::PushFont(Font* font){DPZoneScoped; 
	fontStack.add(style.font);
	style.font = font;
}

void UI::PushScale(vec2 scale){DPZoneScoped;
	scaleStack.add(style.globalScale);
	style.globalScale = scale;
}

void UI::PushLayer(u32 layer){DPZoneScoped;
	Assert(layer < UI_WINDOW_ITEM_LAYERS, "last layer is currently reserved by UI, increase the amount of layers in ui.h if you need more");
	layerStack.add(currlayer);
	currlayer = layer;
}

void UI::PushWindowLayer(u32 layer){DPZoneScoped;
	WarnFuncNotImplemented("");
}

void UI::PushLeftIndent(f32 indent){DPZoneScoped;
	leftIndentStack.add(indent);
}

void UI::PushRightIndent(f32 indent){DPZoneScoped;
	rightIndentStack.add(indent);
}

void UI::PushDrawTarget(u32 idx){DPZoneScoped;
	Assert(idx < render_max_surface_count());
	drawTargetStack.add(idx);
}

void UI::PushDrawTarget(Window* window){DPZoneScoped;
	Assert(window->index != -1, "Attempt to push a draw target that has not been registered with the renderer");
	drawTargetStack.add(window->index);
}

//we always leave the current color on top of the stack and the previous gets popped
void UI::PopColor(u32 count){DPZoneScoped;
	Assert(count <= colorStack.count - initColorStackSize);
	//Assert(count < colorStack.size() - 1, "Attempt to pop too many colors!");
	while(count-- > 0){
		style.colors[(colorStack.last)->element] = colorStack.last->oldCol;
		colorStack.pop();
	}
}

void UI::PopVar(u32 count){DPZoneScoped;
	Assert(count <= varStack.count - initStyleStackSize);
	while (count-- > 0){
		UIStyleVarType type = uiStyleVarTypes[varStack.last->var];
		if(type.count == 1){
			f32* p = (f32*)((u8*)&style + type.offset);
			*p = varStack.last->oldFloat[0];
		}else{
			vec2* p = (vec2*)((u8*)&style + type.offset);
			*p = Vec2(varStack.last->oldFloat[0], varStack.last->oldFloat[1]);
		}
		varStack.pop();
	}
}

void UI::PopFont(u32 count){DPZoneScoped;
	Assert(count <= fontStack.count);
	while(count-- > 0){
		style.font = *fontStack.last;
		fontStack.pop();
	}
}

void UI::PopScale(u32 count){DPZoneScoped;
	Assert(count <= scaleStack.count);
	while(count-- > 0){
		style.globalScale = *scaleStack.last;
		scaleStack.pop();
	}
}

void UI::PopLayer(u32 count){DPZoneScoped;
	Assert(count <= layerStack.count);
	while(count-- > 0){
		currlayer = *layerStack.last;
		layerStack.pop();
	}
}

void UI::PopLeftIndent(u32 count){DPZoneScoped;
	Assert(count < leftIndentStack.count);
	while(count-- > 0){
		leftIndentStack.pop();
	}
}

void UI::PopRightIndent(u32 count){DPZoneScoped;
	Assert(count < rightIndentStack.count);
	while(count-- > 0){
		rightIndentStack.pop();
	}
}

void UI::PopDrawTarget(u32 count){DPZoneScoped;
	Assert(count < drawTargetStack.count);
	while(count-- > 0){
		drawTargetStack.pop();
	}
}


//~////////////////////
////               ////
////    Windows    ////
////               ////
///////////////////////
//window input helper funcs 
void SetFocusedWindow(UIWindow* window){DPZoneScoped;
	//we must find what idx the window is at
	//i think
	for(int i = 0; i < windows.count; i++){
		if(windows[i] == window){
			for(int move = i; move < windows.count - 1; move++)
				windows.swap(move, move + 1);
			break;
		}
	}
}

//function to recursively check child windows
b32 CheckForHoveredChildWindow(UIWindow* parent, UIWindow* child){DPZoneScoped;
	b32 child_hovered = 0;
	if(WinBegan(child)){
		for(UIWindow* c : child->children){
			child_hovered = CheckForHoveredChildWindow(child, c);
		}
		if(!child_hovered){
			switch (child->type){
				case UIWindowType_Child:{
					if(MouseInArea(child->visibleRegionStart, child->visibleRegionSize * style.globalScale)){
						WinSetChildHovered(parent);
						WinSetHovered(child);
						child_hovered = true;
						hovered = child;
						if(!HasFlag(child->flags, UIWindowFlags_DontSetGlobalHoverFlag))
							StateAddFlag(UISGlobalHovered);
					}
					else{
						WinUnSetHovered(child);
					}
				}break;
				case UIWindowType_PopOut:{
					//in popouts case, we treat it like any other window
					if(MouseInArea(child->position, child->dimensions * style.globalScale)){
						WinSetChildHovered(parent);
						WinSetHovered(child);
						child_hovered = true;
						hovered = child;
						
						if(!HasFlag(child->flags, UIWindowFlags_DontSetGlobalHoverFlag))
							StateAddFlag(UISGlobalHovered);
					}
					else{
						WinUnSetHovered(child);
					}
				}break;
			}
		}
	}
	return child_hovered;
}

void CheckForHoveredWindow(UIWindow* window = 0){DPZoneScoped;
	b32 hovered_found = 0;
	for(int i = windows.count - 1; i > 0; i--){
		UIWindow* w = windows[i];
		if(WinBegan(w)){
			//check children first, because there could be popout children who are out of the parents bounds
			for(UIWindow* c : w->children){
				if(CheckForHoveredChildWindow(w, c)){
					//WinUnSetHovered(w);
					//WinSetChildHovered(w);
					hovered_found = 1;
					break;
				}
			}
			if(!hovered_found && MouseInArea(w->position, w->dimensions * w->style.globalScale)){
				WinSetHovered(w);
				WinUnSetChildHovered(w);
				if(!HasFlag(w->flags, UIWindowFlags_DontSetGlobalHoverFlag))
					StateAddFlag(UISGlobalHovered);
				hovered_found = 1;
				hovered = w;
				
			}
			else{
				WinUnSetHovered(w);
			}
		}
		if(hovered_found) break;
	}
}

//this function is checked in UI::Update, while the other 3 are checked per window
void CheckWindowsForFocusInputs(){DPZoneScoped;
	//special case where we always check for metrics first since it draws last
	
	for(int i = windows.count - 1; i > 0; i--){
		UIWindow* w = windows[i];
		WinUnSetFocused(w);
		if(!(w->flags & UIWindowFlags_NoFocus)){
			if(i == windows.count - 1 && WinHovered(w)){
				WinSetFocused(w);
				break;
			}
			else if((WinHovered(w) || WinChildHovered(w)) && ((w->flags & UIWindowFlags_FocusOnHover) ? 1 : input_lmouse_pressed())){
				WinUnSetFocused((*windows.data.last));
				WinSetFocused(w);
				for(int move = i; move < windows.count - 1; move++)
					windows.swap(move, move + 1);
				break;
			}
		}
	}
	
}

void CheckWindowForResizingInputs(UIWindow* window){DPZoneScoped;
	//check for edge resizing
	if(!(window->flags & UIWindowFlags_NoResize) && (CanTakeInput || WinResizing)){
		vec2 mp = input_mouse_position();
		
		b32 latch = WinLatched(window);
		static vec2 mouse, wdims, wpos;
		
		
		b32 mpres = input_lmouse_pressed();
		b32 mdown = input_lmouse_down();
		b32 mrele = input_lmouse_released();
		
		//get which side is active
		WinActiveSide& activeSide = window->win_state.active_side;
		constexpr f32 boundrysize = 2;
		
		if(!mdown){
			f32 borderscaledx = style.windowBorderSize * style.globalScale.x;
			f32 borderscaledy = style.windowBorderSize * style.globalScale.y;
			f32 widthscaled = window->width * style.globalScale.x;
			f32 heightscaled = window->height * style.globalScale.y;
			
			if(MouseInArea(window->position.yAdd(-boundrysize), Vec2(widthscaled,boundrysize + borderscaledy)))
				activeSide = wTop;
			else if(MouseInArea(window->position.yAdd(heightscaled - borderscaledy), Vec2(widthscaled, boundrysize + borderscaledy)))
				activeSide = wBottom;
			else if(MouseInArea(window->position, Vec2(boundrysize + borderscaledx, heightscaled)))
				activeSide = wLeft;
			else if(MouseInArea(window->position.xAdd(widthscaled - borderscaledx), Vec2(boundrysize + borderscaledx, heightscaled)))
				activeSide = wRight;
			else activeSide = wNone;
		}
		
		if(mpres && !latch && activeSide != wNone){
			WinSetLatched(window);
			mouse = mp;
			wdims = window->dimensions;
			wpos = window->position;
			SetFocusedWindow(window);
			SetResizingInput;
		}
		
		if(mrele){
			WinUnSetLatched(window);
			AllowInputs;
		}
		
		switch (activeSide){
			case wTop:{
				SetWindowCursor(CursorType_VResize);
				if(mdown){
					window->position.y = wpos.y + (mp.y - mouse.y);
					window->dimensions = wdims.yAdd(mouse.y - mp.y);
					window->scy = Clamp(window->scy, 0.f, window->maxScroll.y);
				}
			}break;
			case wBottom:{
				SetWindowCursor(CursorType_VResize); 
				if(mdown){
					window->dimensions = wdims.yAdd(mp.y - mouse.y);
					window->scy = Clamp(window->scy, 0.f, window->maxScroll.y);
				}
			}break;
			case wLeft:{
				SetWindowCursor(CursorType_HResize);
				if(mdown){
					window->position.x = wpos.x + (mp.x - mouse.x);
					window->dimensions = wdims.xAdd(mouse.x - mp.x);
					window->scx = Clamp(window->scx, 0.f, window->maxScroll.x);
				}
			}break;
			case wRight:{
				SetWindowCursor(CursorType_HResize); 
				if(mdown){
					window->dimensions = wdims.xAdd(mp.x - mouse.x);
					window->scx = Clamp(window->scx, 0.f, window->maxScroll.x);
				}
			}break;
			case wNone:break;
		}
	}
}

//TODO(sushi) PLEASE clean this shit up
void CheckWindowForScrollingInputs(UIWindow* window, b32 fromChild = 0){DPZoneScoped;
	//always clamp scroll to make sure that it doesnt get stuck pass max scroll when stuff changes inside the window
	window->scx = Clamp(window->scx, 0.f, window->maxScroll.x);
	window->scy = Clamp(window->scy, 0.f, window->maxScroll.y);
	if(DeshInput->scrollY){
		int i = 0;
	}
	//mouse wheel inputs
	//if this is a child window and it cant scroll, redirect the scrolling inputs to the parent
	if(window->parent && WinHovered(window) && window->maxScroll.x == 0 && window->maxScroll.y == 0){
		CheckWindowForScrollingInputs(window->parent, 1);
		return;
	}
	if(((WinHovered(window) && !WinChildHovered(window)) || fromChild)){
		window->scy -= style.scrollAmount.y * DeshInput->scrollY;
		window->scy = Clamp(window->scy, 0.f, window->maxScroll.y); // clamp y again to prevent user from seeing it not be clamped for a frame
		return;
	}
	
	if(CanTakeInput || WinScrolling){
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
		
		b32 mdown = input_lmouse_down();
		b32 mrele = input_lmouse_released();
		
		if(!hscroll && !HasFlag(flags, UIWindowFlags_NoScrollY)){
			f32 scrollbarheight = ClientBottom(window) - ClientTop(window);
			f32 draggerheight = scrollbarheight * scrollbarheight / winmin.y;
			vec2 draggerpos = Vec2(ClientRight(), (scrollbarheight - draggerheight) * window->scy / window->maxScroll.y + BorderedTop(window));
			
			b32 scbgactive = MouseInWinArea(Vec2(ClientRight(window), BorderedTop(window)), Vec2(style.scrollBarYWidth, scrollbarheight));
			b32 scdractive = MouseInWinArea(draggerpos, Vec2(style.scrollBarYWidth, draggerheight));
			
			if(scdractive && input_lmouse_down() || !initial){
				if(initial){
					offset = draggerpos - input_mouse_position();
					initial = false;
					SetFocusedWindow(window);
					SetScrollingInput;
					vscroll = 1;
				}
				
				draggerpos.y = input_mouse_position().y + offset.y;
				draggerpos.y = Clamp(draggerpos.y, 0, scrollbarheight - draggerheight);
				
				window->scy = draggerpos.y * window->maxScroll.y / (scrollbarheight - draggerheight);
				window->scy = Clamp(window->scy, 0.f, window->maxScroll.y);
			}
			if(input_lmouse_released()){
				initial = true;
				vscroll = 0;
				inputupon = 0;
				WinUnSetBeingScrolled(window);
				AllowInputs;
			}
			
		}
		if(!vscroll && !HasFlag(flags, UIWindowFlags_NoScrollX)){
			f32 scrollbarwidth = ClientRight(window) - ClientLeft(window);
			f32 draggerwidth = scrollbarwidth * window->dimensions.x / winmin.x;
			vec2 draggerpos = Vec2((scrollbarwidth - draggerwidth) * window->scx / window->maxScroll.x, ClientBottom(window));
			
			b32 scbgactive = MouseInWinArea(Vec2(ClientBottom(window), BorderedLeft(window)), Vec2(scrollbarwidth, style.scrollBarXHeight));
			b32 scdractive = MouseInWinArea(draggerpos, Vec2(draggerwidth, style.scrollBarXHeight));
			
			if(scdractive && input_lmouse_down() || !initial){
				if(initial){
					offset = draggerpos - input_mouse_position();
					initial = false;
					SetFocusedWindow(window);
					hscroll = 1;
					SetScrollingInput;
				}
				
				draggerpos.x = input_mouse_position().x + offset.x;
				draggerpos.x = Clamp(draggerpos.x, 0, scrollbarwidth - draggerwidth);
				
				window->scx = draggerpos.x * window->maxScroll.x / (scrollbarwidth - draggerwidth);
				window->scx = Clamp(window->scx, 0.f, window->maxScroll.x);
			}
			if(input_lmouse_released()){
				initial = true;
				hscroll = 0;
				WinUnSetBeingScrolled(window);
				AllowInputs;
			}
		}
	}
}

void CheckWindowForDragInputs(UIWindow* window, b32 fromChild = 0){DPZoneScoped;
	if(CanTakeInput || WinDragging){ //drag
		//if this is a child window check the uppermost parent instead
		if(window->parent && WinHovered(window)){ 
			CheckWindowForDragInputs(window->parent, 1); 
			return;
		}
		
		static vec2 mouseOffset = Vec2(0, 0);
		
		if(
		   !(window->flags & UIWindowFlags_NoMove) &&
		   (WinHovered(window) || fromChild) &&
		   key_pressed(Mouse_LEFT)){
			SetDraggingInput;
			WinSetBeingDragged(window);
			mouseOffset = window->position - input_mouse_position();
			SetFocusedWindow(window);
		}
		if(WinBeingDragged(window)){
			window->position = input_mouse_position() + mouseOffset;
			//TODO clean up this code
			if(HasFlag(window->flags, UIWindowFlags_SnapToOtherWindows)){
				for(UIWindow* win : windows){
					if(win != window && !str8_equal_lazy(win->name, str8_lit("base"))){
						f32 stx = style.windowSnappingTolerance.x;
						f32 sty = style.windowSnappingTolerance.y;
						if(window->y >= win->y && window->y <= (win->y + win->height) || 
						   win->y >= window->y && win->y <= (window->y + window->height)){
							if(fabs(window->x - (win->x + win->width)) < stx) //left vs right
								window->x = win->x + win->width;
							else if(fabs((window->x + window->width) - win->x) < stx) //right vs left
								window->x = (win->x - window->width);
							else if(fabs(window->x - win->x) < stx) //left vs left
								window->x = win->x;
							else if(fabs((window->x+window->width) - (win->x+win->width)) < stx) //right vs right
								window->x = win->x + win->width - window->width;
						}
						if(window->x >= win->x && window->x <= (win->x + win->width) || 
						   win->x >= window->x && win->x <= (window->x + window->width)){
							if(fabs(window->y - (win->y + win->height)) < sty) //top vs bottom
								window->y = win->y + win->height;
							else if(fabs((window->y + window->height) - win->y) < sty) //bottom vs top
								window->y = (win->y - window->height);
							else if(fabs(window->y - win->y) < sty) //top vs top
								window->y = win->y;
							else if(fabs((window->y+window->height) - (win->y+win->height)) < stx) //bottom vs bottom
								window->y = win->y + win->height - window->height;
						}
					}
				}
			}
		}
		if(key_released(Mouse_LEFT)){
			WinUnSetBeingDragged(window);
			AllowInputs;
		}
	}
}

//@Begin

//begins a window with a name, position, and dimensions along with some optional flags
//if begin window is called with a name that was already called before it will work with
//the data that window previously had
Stopwatch wincreate; //TODO move this onto the window and add to it when the user uses Continue
void BeginCall(str8 name, vec2 pos, vec2 dimensions, UIWindowFlags flags, UIWindowType type){DPZoneScoped;
	Assert(type != UIWindowType_Normal || !StateHasFlag(UISRowBegan), "Attempted to begin a window with a Row in progress! (Did you forget to call EndRow()?");
	wincreate = start_stopwatch();
	//save previous window on stack
	windowStack.add(curwin);
	ui_stats.windows++;
	
	switch (type){
		case UIWindowType_Normal:{ //////////////////////////////////////////////////////////////////////
			//check if were making a new window or working with one we already know
			if(windows.has(name)){
				curwin = windows[name];
			}else{
				curwin = new UIWindow();
				curwin->name = name;
				curwin->position   = (NextWinPos.x != -1) ? NextWinPos : pos;
				curwin->dimensions.x = dimensions.x;
				curwin->dimensions.y = dimensions.y;
				windows.add(name, curwin);
			}
			curwin->type   = UIWindowType_Normal;
			curwin->style  = style;
			curwin->scroll = Vec2(0, 0);
			curwin->cursor = Vec2(0, 0);
			curwin->flags  = flags;
			if(NextWinPos.x != -1) curwin->position = NextWinPos;
			if(NextWinSize.x == MAX_F32) curwin->dimensions.x = DeshWindow->width;
			else if(NextWinSize.x != -1) curwin->dimensions.x = NextWinSize.x;
			if(NextWinSize.y == MAX_F32) curwin->dimensions.y = DeshWindow->height;
			else if(NextWinSize.y != -1) curwin->dimensions.y = NextWinSize.y;
		}break;
		case UIWindowType_Child:{ ///////////////////////////////////////////////////////////////////////
			UIWindow* parent = curwin;
			UIItem_old* item = BeginItem(UIItemType_Window);
			
			//TODO(sushi) add custom positioning for child windows
			item->position = pos;
			
			//check if were making a new child or working with one we already know
			if(parent->children.has(name)){
				if(NextWinSize.x == MAX_F32) item->size.x = MarginedRight() - item->position.x - rightIndent;
				else if(NextWinSize.x != -1) item->size.x = NextWinSize.x;
				else                         item->size.x = parent->children[name]->dimensions.x;
				if(NextWinSize.y == MAX_F32) item->size.y = MarginedBottom() - item->position.y;
				else if(NextWinSize.y != -1) item->size.y = NextWinSize.y;
				else                         item->size.y = parent->children[name]->dimensions.y;
				AdvanceCursor(item);
				
				curwin = parent->children[name];
				curwin->dimensions = item->size;
				curwin->cursor = Vec2(0, 0);
				if(NextWinPos.x != -1) curwin->position = NextWinPos;
			}else{
				if(NextWinSize.x == MAX_F32) item->size.x = MarginedRight() - item->position.x - rightIndent;
				else if(NextWinSize.x != -1) item->size.x = NextWinSize.x;
				else                         item->size.x = dimensions.x;
				if(NextWinSize.y == MAX_F32) item->size.y = MarginedBottom() - item->position.y;
				else if(NextWinSize.y != -1) item->size.y = NextWinSize.y;
				else                         item->size.y = dimensions.y;
				AdvanceCursor(item);
				
				curwin = new UIWindow();
				curwin->scroll = Vec2(0, 0);
				curwin->name = name;
				curwin->position = (NextWinPos.x != -1) ? NextWinPos : DecideItemPos(parent);
				curwin->dimensions = item->size;
				curwin->cursor = Vec2(0, 0);
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
		case UIWindowType_PopOut:{ //////////////////////////////////////////////////////////////////////
			UIWindow* parent = curwin;
			UIItem_old* item = BeginItem(UIItemType_PopOutWindow);
			item->position = pos;
			
			if(parent->children.has(name)){
				if(NextWinSize.x == MAX_F32) item->size.x = MarginedRight() - item->position.x;
				else if(NextWinSize.x != -1) item->size.x = NextWinSize.x;
				else                         item->size.x = parent->children[name]->dimensions.x;
				if(NextWinSize.y == MAX_F32) item->size.y = MarginedBottom() - item->position.y;
				else if(NextWinSize.y != -1) item->size.y = NextWinSize.y;
				else                         item->size.y = parent->children[name]->dimensions.y;
				AdvanceCursor(item, 0);
				
				curwin = parent->children[name];
				curwin->dimensions = item->size;
				curwin->cursor = Vec2(0, 0);
				if(NextWinPos.x != -1) curwin->position = NextWinPos;
			}else{
				if(NextWinSize.x == MAX_F32) item->size.x = MarginedRight() - item->position.x;
				else if(NextWinSize.x != -1) item->size.x = NextWinSize.x;
				else                         item->size.x = dimensions.x;
				if(NextWinSize.y == MAX_F32) item->size.y = MarginedBottom() - item->position.y;
				else if(NextWinSize.y != -1) item->size.y = NextWinSize.y;
				else                         item->size.y = dimensions.y;
				AdvanceCursor(item, 0);
				
				curwin = new UIWindow();
				curwin->scroll = Vec2(0, 0);
				curwin->name = name;
				curwin->position   = (NextWinPos.x != -1) ? NextWinPos : DecideItemPos(parent);
				curwin->dimensions = item->size;
				curwin->cursor = Vec2(0, 0);
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
	NextWinPos = Vec2(-1,-1); NextWinSize = Vec2(-1,-1);
}


void UI::Begin(str8 name, UIWindowFlags flags){DPZoneScoped;
	BeginCall(name, vec2::ONE * 100, Vec2(150, 300), flags, UIWindowType_Normal);
}

void UI::Begin(str8 name, vec2 pos, vec2 dimensions, UIWindowFlags flags){DPZoneScoped;
	BeginCall(name, pos, dimensions, flags, UIWindowType_Normal);
}

void UI::BeginChild(str8 name, vec2 dimensions, UIWindowFlags flags){DPZoneScoped;
	BeginCall(name, DecideItemPos(), dimensions, flags, UIWindowType_Child);
}

void UI::BeginChild(str8 name, vec2 pos, vec2 dimensions, UIWindowFlags flags){DPZoneScoped;
	BeginCall(name, pos, dimensions, flags, UIWindowType_Child);
}

void UI::BeginPopOut(str8 name, vec2 pos, vec2 dimensions, UIWindowFlags flags){DPZoneScoped;
	//currlayer = Min(++currlayer, (u32)UI_LAYERS);
	BeginCall(name, pos, dimensions, flags, UIWindowType_PopOut);
	//currlayer--; //NOTE this will break if we are already on last layer fix  i
}

//@CalcWindowMinSize

//calculates the minimum size a window can be to contain all drawn elements
//this would probably be better to be handled as we add items to the window
//instead of doing it at the end, so maybe make an addItem() that calculates this
//everytime we add one
vec2 CalcWindowMinSize(){DPZoneScoped;
	vec2 max;
	forI(UI_WINDOW_ITEM_LAYERS){
		for(UIItem_old& item : curwin->items[i]){
			//if(item.type == UIItemType_Window && item.child->type != UIWindowType_PopOut){
			if(item.trackedForMinSize){
				max.x = Max(max.x, item.position.x + curwin->scx + item.size.x);
				max.y = Max(max.y, item.position.y + curwin->scy + item.size.y);
			}
			//}
		}
	}
	return max + style.windowMargins + vec2::ONE * style.windowBorderSize;
}

//Old titlebar code for when i reimplement it as its own call
#if 0 


//draw title bar
if(!(curwin->flags & UIWindowFlags_NoTitleBar)){DPZoneScoped;
	{
		UIDrawCmd drawCmd; //inst 40
		drawCmd.type = UIDrawType_FilledRectangle;
		drawCmd.position = curwin->position;
		drawCmd.dimensions = vec2{ curwin->width, style.titleBarHeight };
		drawCmd.color = style.colors[UIStyleCol_TitleBg];
		
		base.drawCmds.add(drawCmd); //inst 44
	}
	
	{//draw text if it exists
		if(curwin->name.size){
			UIDrawCmd drawCmd; //inst 46
			drawCmd.type = UIDrawType_Text;
			drawCmd.text = curwin->name; //inst 48
			drawCmd.position = Vec2(curwin->x + (curwin->width - curwin->name.size * style.font->max_width) * style.titleTextAlign.x,
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
		if(!((curwin->flags & UIWindowFlags_NoMinimizeButton) || (curwin->flags & UIWindowFlags_NoMinimizeButton))){
			UIDrawCmd drawCmd;
			drawCmd.position = Vec2(
									curwin->x + (curwin->width - curwin->name.size * style.font->max_width) * 0.01,
									curwin->y + (style.titleBarHeight * 0.5 - 2));
			drawCmd.dimensions = Vec2(10, 4);
			
			if(Math::PointInRectangle(mp, drawCmd.position, drawCmd.dimensions)){
				drawCmd.color = style.colors[UIStyleCol_TitleBg] * 0.7;
				if(key_pressed(Mouse_LEFT)){
					curwin->minimized = !curwin->minimized;
				}
			}
			else{
				drawCmd.color = style.colors[UIStyleCol_TitleBg] * 0.3;
			}
			
			curwin->baseDrawCmds.add(drawCmd); //inst 54
		}
	}
}
#endif

//@End

void EndCall(){DPZoneScoped;
	Assert(windowStack.count, "Attempted to end the base window");
	
	UIItem_old* preitem = BeginItem(UIItemType_PreItems);
	UIItem_old* postitem = BeginItem(UIItemType_PostItems);
	
	preitem->position = vec2::ZERO;
	postitem->position = vec2::ZERO;
	
	preitem->size  = curwin->dimensions;
	postitem->size = curwin->dimensions;
	
	vec2 mp = input_mouse_position();
	
	curwin->minSizeForFit = CalcWindowMinSize();
	vec2 minSizeForFit = curwin->minSizeForFit;
	
	if(WinHasFlag(UIWindowFlags_FitAllElements))
		curwin->dimensions = minSizeForFit;
	
	if(curwin->parent){
		vec2 scrollBarAdjust = Vec2((CanScrollY(curwin->parent) ? style.scrollBarYWidth  : 0),
									(CanScrollX(curwin->parent) ? style.scrollBarXHeight : 0));
		curwin->visibleRegionStart = Max(curwin->parent->visibleRegionStart, curwin->position);
		curwin->visibleRegionSize  = ClampMin(Min(curwin->parent->visibleRegionStart + curwin->parent->visibleRegionSize - scrollBarAdjust, curwin->position + curwin->dimensions) - curwin->visibleRegionStart, vec2::ZERO);
	}else{
		curwin->visibleRegionStart = curwin->position;
		curwin->visibleRegionSize  = curwin->dimensions;
	}
	
	b32 xCanScroll = CanScrollX();
	b32 yCanScroll = CanScrollY();
	
	if(!inputupon){
		CheckWindowForScrollingInputs(curwin);
		CheckWindowForResizingInputs(curwin);
		CheckWindowForDragInputs(curwin);
	}
	
	//check to see if the elements we have drawn so far have gone beyond the window's size
	//and allow scrolling if it did, as well as define a max scrolling amount
	//also draw the scroll bar if allowed
	//TODO(sushi) clean up this code, it really needs it. here and in the update function
	if(!WinHasFlag(UIWindowFlags_NoScrollY) && yCanScroll){
		curwin->maxScroll.y = minSizeForFit.y - curwin->dimensions.y + (xCanScroll ? style.scrollBarXHeight : 0);
		if(!WinHasFlag(UIWindowFlags_NoScrollBarY)){
			f32 scrollbarheight = ClientBottom() - ClientTop();
			f32 draggerheight = scrollbarheight * scrollbarheight / minSizeForFit.y;
			vec2 draggerpos = Vec2(ClientRight(), (scrollbarheight - draggerheight) * curwin->scy / curwin->maxScroll.y + BorderedTop());
			
			b32 scbgactive = MouseInWinArea(Vec2(ClientRight(), BorderedTop()), Vec2(style.scrollBarYWidth, scrollbarheight));
			b32 scdractive = MouseInWinArea(draggerpos, Vec2(style.scrollBarYWidth, draggerheight));
			
			{//scroll bg
				UIDrawCmd drawCmd;
				vec2  position = Vec2(ClientRight(), BorderedTop());
				vec2  dimensions = Vec2(style.scrollBarYWidth, scrollbarheight);
				color col = style.colors[UIStyleCol_ScrollBarBg]; //TODO(sushi) add active/hovered scrollbarbg colors
				MakeFilledRect(drawCmd, position, dimensions, col);
				AddDrawCmd(postitem, drawCmd);
			}
			
			{//scroll dragger
				UIDrawCmd drawCmd;
				vec2  position = draggerpos;
				vec2  dimensions = Vec2(style.scrollBarYWidth, draggerheight);
				color col = style.colors[(scdractive ? ((input_lmouse_down()) ? UIStyleCol_ScrollBarDraggerActive : UIStyleCol_ScrollBarDraggerHovered) : UIStyleCol_ScrollBarDragger)];
				MakeFilledRect(drawCmd, position, dimensions, col);
				AddDrawCmd(postitem, drawCmd);
			}
			
			//if both scroll bars are active draw a little square to obscure the empty space 
			if(CanScrollX()){
				UIDrawCmd drawCmd;
				vec2  position = Vec2(ClientRight(), scrollbarheight);
				vec2  dimensions = Vec2(style.scrollBarYWidth, style.scrollBarXHeight);
				color col = style.colors[UIStyleCol_WindowBg];
				MakeFilledRect(drawCmd, position, dimensions, col);
				AddDrawCmd(postitem, drawCmd);
			}
		}
	}
	else curwin->maxScroll.y = 0;
	
	
	//do the same but for x
	if(!WinHasFlag(UIWindowFlags_NoScrollX) && CanScrollX()){
		curwin->maxScroll.x = minSizeForFit.x - curwin->dimensions.x + (yCanScroll ? style.scrollBarYWidth : 0);
		if(!WinHasFlag(UIWindowFlags_NoScrollBarX)){
			f32 scrollbarwidth = ClientRight() - ClientLeft();
			f32 draggerwidth = scrollbarwidth * curwin->dimensions.x / minSizeForFit.x;
			vec2 draggerpos = Vec2((scrollbarwidth - draggerwidth) * curwin->scx / curwin->maxScroll.x, ClientBottom());
			
			b32 scbgactive = MouseInWinArea(Vec2(ClientBottom(), BorderedLeft()), Vec2(scrollbarwidth, style.scrollBarXHeight));
			b32 scdractive = MouseInWinArea(draggerpos, Vec2(draggerwidth, style.scrollBarXHeight));
			
			{//scroll bg
				UIDrawCmd drawCmd;
				vec2  position = Vec2(0, ClientBottom());
				vec2  dimensions = Vec2(scrollbarwidth, style.scrollBarXHeight);
				color col = style.colors[UIStyleCol_ScrollBarBg]; //TODO(sushi) add active/hovered scrollbarbg colors
				MakeFilledRect(drawCmd, position, dimensions, col);
				AddDrawCmd(postitem, drawCmd);
			}
			
			{//scroll dragger
				UIDrawCmd drawCmd;
				vec2  position = draggerpos;
				vec2  dimensions = Vec2(draggerwidth, style.scrollBarXHeight);
				color col = style.colors[(scdractive ? ((input_lmouse_down()) ? UIStyleCol_ScrollBarDraggerActive : UIStyleCol_ScrollBarDraggerHovered) : UIStyleCol_ScrollBarDragger)];
				MakeFilledRect(drawCmd, position, dimensions, col);
				AddDrawCmd(postitem, drawCmd);
			}
		}
	}
	else curwin->maxScroll.x = 0;
	
	
	//if the window isn't invisible draw things that havent been disabled
	if(!WinHasFlags(UIWindowFlags_Invisible)){
		//draw background
		if(!WinHasFlag(UIWindowFlags_NoBackground)){
			UIDrawCmd drawCmd;
			vec2  position = vec2::ZERO;
			vec2  dimensions = curwin->dimensions;
			color col = style.colors[UIStyleCol_WindowBg];
			MakeFilledRect(drawCmd, position, dimensions, col);
			AddDrawCmd(preitem, drawCmd);
		}
		
		//draw border
		if(!WinHasFlag(UIWindowFlags_NoBorder)){
			UIDrawCmd drawCmd;
			vec2  position = vec2::ONE * ceil(style.windowBorderSize / 2);
			vec2  dimensions = curwin->dimensions - vec2::ONE * ceil(style.windowBorderSize);
			color col = style.colors[UIStyleCol_Border];
			f32   thickness = style.windowBorderSize;
			MakeRect(drawCmd, position, dimensions, thickness, col);
			AddDrawCmd(postitem, drawCmd);
		}
	}
	
	NextWinPos = Vec2(-1, 0); NextWinSize = Vec2(-1,-1);
	curwin->style = style;
	
	curwin->creation_time = peek_stopwatch(wincreate);
	
	//update stored window with new window state
	curwin = *windowStack.last;
	windowStack.pop();
}

void UI::End(){DPZoneScoped;
	Assert(!StateHasFlag(UISRowBegan), "Attempted to end a window with a Row in progress!");
	Assert(!StateHasFlag(UISComboBegan), "Attempted to end a window with a Combo in progress!");
	EndCall();
}

void UI::EndChild(){DPZoneScoped;
	EndCall();
	PopLeftIndent();
	PopRightIndent();
}

void UI::EndPopOut(){DPZoneScoped;
	EndCall();
	PopLeftIndent();
	PopRightIndent();
}

void UI::Continue(str8 name){
	StateAddFlag(UISContinuingWindow);
	str8 part = str8_eat_until(name, '/');
	str8_nadvance(&name, part.count+1);
	Assert(windows.has(part), "Tried to continue a non existant window, or a window that is a child without specifying it's parent first");
	UIWindow* next = windows[part];
	while(name){
		part = str8_eat_until(name, '/');
		str8_nadvance(&name, part.count+1);
		Assert(next->children.has(part), "Tried to continue a nonexistant window");
		next = next->children[part];
	}
	windowStack.add(curwin);
	curwin = next;
}

void UI::EndContinue(){
	Assert(StateHasFlag(UISContinuingWindow), "attempted to end Continue without Continuing a window first");
	StateRemoveFlag(UISContinuingWindow);
	curwin = *windowStack.last;
	windowStack.pop();
}

void UI::SetNextWindowPos(vec2 pos){DPZoneScoped;
	NextWinPos = pos;
}

void UI::SetNextWindowPos(f32 x, f32 y){DPZoneScoped;
	NextWinPos = Vec2(x,y);
}

void UI::SetNextWindowSize(vec2 size){DPZoneScoped;
	NextWinSize = size.yAdd(style.titleBarHeight);
}

void UI::SetNextWindowSize(f32 x, f32 y){DPZoneScoped;
	NextWinSize = Vec2(x, y);
}

b32 UI::IsWinHovered(){DPZoneScoped;
	return WinHovered(curwin);
}

b32 UI::AnyWinHovered(){DPZoneScoped;
	return StateHasFlag(UISGlobalHovered) || !CanTakeInput;
}

inline UIWindow* MetricsDebugItemFindHoveredWindow(UIWindow* window = 0){DPZoneScoped;
	if(window){
		if(WinChildHovered(window)){
			for(UIWindow* w : window->children){
				return MetricsDebugItemFindHoveredWindow(w);
			}
		}
		else if(WinHovered(window)) return window;
	}
	else{
		UIWindow* found = 0;
		for(UIWindow* w : windows){
			found = MetricsDebugItemFindHoveredWindow(w);
			if(found) break;
		}
		return found;
	}
	return 0;
}

inline void MetricsDebugItem(){DPZoneScoped;
	using namespace UI;
	
	enum DebugItemState {
		None,
		InspectingWindowPreItems,
		InspectingWindowItems,
		InspectingWindowPostItems,
		InspectingItem
	};
	
	persist DebugItemState distate = None;
	
	persist UIItem_old    iteml;
	persist UIWindow* debugee = 0;
	persist vec2      mplatch;
	
	if(distate != None && distate != InspectingItem){
		AllowInputs;
		TextOld(str8_lit("Press ESC to cancel"));
		TextOld(str8_lit("A: Select Pre Items"));
		TextOld(str8_lit("S: Select Items"));
		TextOld(str8_lit("D: Select Post Items"));
		
		if(key_pressed(Key_A)) distate = InspectingWindowPreItems;
		if(key_pressed(Key_S)) distate = InspectingWindowItems;
		if(key_pressed(Key_D)) distate = InspectingWindowPostItems;
		
		if(key_pressed(Key_ESCAPE)) distate = None;
	}
	
	auto check_item = [&](UIItem_old& item){
		vec2 ipos = hovered->position + item.position;
		if(MouseInArea(ipos, item.size)){
			
			DebugRect(ipos, item.size);
			PushVar(UIStyleVar_WindowMargins, Vec2(3, 3));
			BeginPopOut(str8_lit("MetricsDebugItemPopOut"), ipos.xAdd(item.size.x) - curwin->position, vec2::ZERO, UIWindowFlags_FitAllElements | UIWindowFlags_NoBorder | UIWindowFlags_NoInteract);
			
			TextOld(UIItemTypeStrs[item.type], UITextFlags_NoWrap);
			TextF(str8_lit("DrawCmds: %d"), item.drawCmds.count);
			
			EndPopOut();
			PopVar();
			
			if(input_lmouse_pressed()){
				mplatch = ipos.xAdd(item.size.x);
				iteml = item;
				distate = InspectingItem;
				debugee = hovered;
			}
			return true;
		}
		return false;
	};
	
	switch (distate){
		case None:{
			if(Button(str8_lit("Debug Item with Cursor")) ||
			   key_pressed(Key_D | InputMod_LctrlLshift)){
				distate = InspectingWindowItems;
			}
		}break;
		case InspectingWindowPreItems:{
			if(hovered){
				for(UIItem_old& item : hovered->preItems){
					DebugRect(item.position + hovered->position, item.size);
					check_item(item);
				}
			}
		}break;
		case InspectingWindowItems:{
			if(hovered){
				b32 item_found = 0;
				forI(UI_WINDOW_ITEM_LAYERS){
					for(UIItem_old& item : hovered->items[i]){
						item_found = check_item(item);
					}
				}
				//if we are mousing over empty space in a child window, highlight the child window
				if(!item_found && hovered->parent){
					DebugRect(hovered->position, hovered->dimensions);
					PushVar(UIStyleVar_WindowMargins, Vec2(3, 3));
					BeginPopOut(str8_lit("MetricsDebugItemPopOut"), hovered->position.xAdd(hovered->width) - curwin->position, vec2::ZERO, UIWindowFlags_FitAllElements | UIWindowFlags_NoBorder | UIWindowFlags_NoInteract);
					string s = toStr("Child Window ", hovered->name);
					TextOld(str8{(u8*)s.str, (s64)s.count}, UITextFlags_NoWrap);
					
					EndPopOut();
					PopVar();
				}
			}
		}break;
		case InspectingWindowPostItems:{
			if(hovered){
				for(UIItem_old& item : hovered->postItems){
					check_item(item);
				}
			}
		}break;
		case InspectingItem:{
			vec2 ipos = iteml.position + debugee->position;
			
			PushVar(UIStyleVar_WindowMargins, Vec2(3, 3));
			//PushColor(UIStyleCol_WindowBg, color(50, 50, 50));
			BeginPopOut(str8_lit("MetricsDebugItemPopOut"), mplatch - curwin->position, vec2::ZERO, UIWindowFlags_FitAllElements);
			
			TextOld(UIItemTypeStrs[iteml.type], UITextFlags_NoWrap);
			TextF(str8_lit("DrawCmds: %d"), iteml.drawCmds.count);
			TextOld(str8_lit("Select to break on drawCmd"), UITextFlags_NoWrap);
			
			PushColor(UIStyleCol_WindowBg, color(30, 30, 30));
			BeginChild(str8_lit("MetricsDebugItemPopOutDrawCmdChild"), Vec2(0,0), UIWindowFlags_NoBorder | UIWindowFlags_FitAllElements);
			
			BeginRow(str8_lit("MetricsItemAlignment"), 3, style.buttonHeightRelToFont* style.fontHeight);
			RowSetupRelativeColumnWidths({ 1.1f,1.1f,1.1f });
			for(UIDrawCmd& dc : iteml.drawCmds){
				TextOld(UIDrawTypeStrs[dc.type], UITextFlags_NoWrap);
				
				if(MouseInArea(GetLastItemScreenPos(), GetLastItemSize())){
					for(u32 tri = 0; tri < dc.counts.y; tri += 3){
						vec2 
							p0 = ipos + dc.vertices[dc.indices[tri]].pos,
						p1 = ipos + dc.vertices[dc.indices[tri+1]].pos,
						p2 = ipos + dc.vertices[dc.indices[tri+2]].pos;
						DebugTriangle(p0, p1, p2, color(255, 0,0, 70));
					}
				}
				
				WinSetHovered(curwin);
				if(Button(str8_lit("Creation"))){
					break_drawCmd_create_hash = dc.hash;
				}
				if(Button(str8_lit("Draw"))){
					break_drawCmd_draw_hash = dc.hash;
				}
				WinUnSetHovered(curwin);
			}
			EndRow();
			EndChild();
			
			if(hovered!=curwin && input_lmouse_pressed()){
				distate=InspectingWindowItems;
			}
			
			EndPopOut();
			PopColor();
			PopVar();
		}break;
		
	}
	
	if(distate) PreventInputs;
	else AllowInputs;
}

inline b32 MetricsCheckWindowBreaks(UIWindow* window, b32 winbegin){DPZoneScoped;
	if(WinChildHovered(window)){
		for(UIWindow* c : window->children){
			if(MetricsCheckWindowBreaks(c, winbegin))
				return true;
		}
		
	}
	else if(WinHovered(window)){
		DebugRect(window->position, window->dimensions);
		if(input_lmouse_pressed()){
			if(winbegin) break_window_begin = window;
			else break_window_end = window;
			return true;
		}
	}
	return false;
}

inline void MetricsBreaking(){DPZoneScoped;
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
	
	auto check_win_items = [&](UIWindow* win){
		forI(UI_WINDOW_ITEM_LAYERS){
			for(UIItem_old& item : win->items[i]){
				if(MouseInArea(win->position + item.position, item.size)){
					DebugRect(win->position + item.position, item.size);
					if(input_lmouse_pressed()){
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
	
	auto check_win_drawcmds = [&](UIWindow* win){
		static u32 selected = -1;
		forI(UI_WINDOW_ITEM_LAYERS){
			for(UIItem_old& item : win->items[i]){
				if(MouseInArea(win->position + item.position, item.size)){
					//DebugRect(win->position + item.position, item.size);
					vec2 ipos = win->position + item.position;
					selected = Clamp(selected, 0, item.drawCmds.count);
					int o = 0;
					for(UIDrawCmd& dc : item.drawCmds){
						for(u32 tri = 0; tri < dc.counts.y; tri += 3){
							vec2 
								p0 = ipos + dc.vertices[dc.indices[tri]].pos,
							p1 = ipos + dc.vertices[dc.indices[tri+1]].pos,
							p2 = ipos + dc.vertices[dc.indices[tri+2]].pos;
							DebugTriangle(p0, p1, p2, color(255, 0,0, 70));
							if(Math::PointInTriangle(input_mouse_position(), p0, p1, p2)){
								DebugTriangle(p0, p1, p2);
								if(input_lmouse_pressed()){
									break_drawCmd_create_hash = dc.hash;
								}
							}
						}
						
					}
				}
			}
		}
	};
	
	
	
	static u32 break_on_cursor = 0;
	
	if(breakstate){
		TextOld(str8_lit("Press ESC to cancel"));
		if(key_pressed(Key_ESCAPE))
			breakstate = BreakNone;
	}
	
	switch (breakstate){
		case BreakNone:{
			if(Button(str8_lit("Break Item on Cursor")) ||
			   key_pressed(Key_B | InputMod_LctrlLshift)){
				breakstate = BreakItem;
			}
			if(Button(str8_lit("Break DrawCmd on Cursor")) ||
			   key_pressed(Key_D | InputMod_LctrlLshift)){
				breakstate = BreakDrawCmd;
			}
			if(Button(str8_lit("Break on Window Begin"))){
				breakstate = BreakWindowBegin;
				
			}
			if(Button(str8_lit("Break on Window Begin"))){
				breakstate = BreakWindowEnd;
			}
		}break;
		case BreakItem:{
			for(UIWindow* w : windows){
				if(WinChildHovered(w)){
					for(UIWindow* c : w->children){
						check_win_items(c);
					}
				}
				else if(WinHovered(w)){
					check_win_items(w);
				}
			}
		}break;
		case BreakDrawCmd:{
			for(UIWindow* w : windows){
				if(WinChildHovered(w)){
					for(UIWindow* c : w->children){
						check_win_drawcmds(c);
					}
				}
				else if(WinHovered(w)){
					check_win_drawcmds(w);
				}
			}
		}break;
		case BreakWindowBegin:{
			for(UIWindow* w : windows){
				MetricsCheckWindowBreaks(w, 1);
			}
		}break;
		case BreakWindowEnd:{
			for(UIWindow* w : windows){
				MetricsCheckWindowBreaks(w, 0);
			}
		}break;
	}
	
	if(frame_skip && break_on_cursor){
		for(UIWindow* w : windows){
			if(WinChildHovered(w)){
				for(UIWindow* c : w->children){
					if(break_on_cursor == 1)
						check_win_items(c);
					else{
						check_win_drawcmds(c);
					}
				}
			}
			else if(WinHovered(w)){
				if(break_on_cursor == 1)
					check_win_items(w);
				else{
					check_win_drawcmds(w);
				}
			}
		}
		TextOld(str8_lit("Press ESC to cancel"));
		if(key_pressed(Key_ESCAPE)) break_on_cursor = 0;
		PreventInputs;
	}
	if(break_on_cursor){
		frame_skip = 1;
	}
}

UIWindow* DisplayMetrics(){DPZoneScoped;
	using namespace UI;
	
	persist UIWindow* debugee = nullptr;
	
	UIWindow* myself = 0; //pointer returned for drawing
	
	persist UIWindow* slomo = windows.data[0];
	persist UIWindow* quick = windows.data[0];
	persist UIWindow* mostitems = windows.data[0];
	persist UIWindow* longname = windows.data[0];
	
	arrayT<UIWindow*> winsorted;
	for(UIWindow* win : windows){
		//TODO optional metrics filter or allow text filtering 
		//if(!(win->name == "METRICS")){
		if(win->render_time > slomo->render_time)     slomo = win;
		if(win->render_time < quick->render_time)     quick = win;
		if(win->items_count > mostitems->items_count) mostitems = win;
		if(win->name.count > longname->name.count)   longname = win;
		winsorted.add(win);
		//}
	}
	
	bubble_sort(winsorted, [](UIWindow* win1, UIWindow* win2){return *win1->name.str > *win2->name.str; });
	
	Begin(str8_lit("METRICS"), Vec2(DeshWindow->width, DeshWindow->height) - Vec2(300,500), Vec2(300, 500));
	myself = curwin;
	//WinSetHovered(curwin);
	
	BeginRow(str8_lit("Metrics_General_Stats"), 2, 0, UIRowFlags_AutoSize);
	RowSetupColumnAlignments({ {1, 0.5}, {0, 0.5} });
	TextOld(str8_lit("FPS: "), UITextFlags_NoWrap); TextF(str8_lit("%d"), (int)(1000.0/DeshTime->deltaTime));
	EndRow();
	
	if(BeginHeader(str8_lit("UI Stats"))){
		
		BeginRow(str8_lit("Metrics_UI_Stats"), 2, 0, UIRowFlags_AutoSize);
		RowSetupColumnAlignments({ {1, 0.5}, {0, 0.5} });
		
		TextOld(str8_lit("Windows: "));      TextF(str8_lit("%d"), ui_stats.windows);
		TextOld(str8_lit("Items: "));        TextF(str8_lit("%d"), ui_stats.items);
		TextOld(str8_lit("DrawCmds: "));     TextF(str8_lit("%d"), ui_stats.draw_cmds);
		TextOld(str8_lit("Vertices: "));     TextF(str8_lit("%d"), ui_stats.vertices);
		TextOld(str8_lit("Indices: "));      TextF(str8_lit("%d"), ui_stats.indices);
		TextOld(str8_lit("Global Hover: ")); TextOld((StateHasFlag(UISGlobalHovered)) ? str8_lit("true") : str8_lit("false"));
		TextOld(str8_lit("input state: "));
		switch (inputState){
			case ISNone:                  TextOld(str8_lit("None"));                    break;
			case ISScrolling:             TextOld(str8_lit("Scrolling"));               break;
			case ISResizing:              TextOld(str8_lit("Resizing"));                break;
			case ISDragging:              TextOld(str8_lit("Dragging"));                break;
			case ISPreventInputs:         TextOld(str8_lit("Prevent Inputs"));          break;
			case ISExternalPreventInputs: TextOld(str8_lit("External Prevent Inputs")); break;
		}
		TextOld(str8_lit("input upon: ")); TextOld((inputupon) ? inputupon->name : str8_lit("none"));
		
		EndRow();		
		EndHeader();
	}
	
	if(BeginHeader(str8_lit("Windows"))){
		
		{//window stats (maybe put this in a header?)
			string slomotext = toStr("Slowest Render:");
			string quicktext = toStr("Fastest Render:");
			string mostitext = toStr("Most Items: "); 
			
			persist f32 sw = CalcTextSize(longname->name).x;
			persist f32 fw = CalcTextSize(str8{(u8*)slomotext.str, (s64)slomotext.count}).x + 5;
			
			PushVar(UIStyleVar_RowItemAlign, vec2{ 0, 0.5 });
			BeginRow(str8_lit("MetricsWindowStatsAlignment"), 3, 11, UIRowFlags_AutoSize);
			RowSetupColumnWidths({ fw, sw, 55 });
			
			TextOld(str8{(u8*)slomotext.str, (s64)slomotext.count});
			TextOld(slomo->name);
			if(Button(str8_lit("select"))) debugee = slomo;
			
			TextOld(str8{(u8*)quicktext.str, (s64)quicktext.count});
			TextOld(quick->name);
			if(Button(str8_lit("select"))) debugee = quick;
			
			TextOld(str8{(u8*)mostitext.str, (s64)mostitext.count});
			TextOld(mostitems->name);
			if(Button(str8_lit("select"))) debugee = mostitems;
			
			PopVar();
			EndRow();
		}
		
		persist b32 showChildren = 0;
		
		Checkbox(str8_lit("show children"), &showChildren);
		
		SetNextWindowSize(Vec2(MAX_F32, 300));
		BeginChild(str8_lit("METRICSWindows"), vec2::ZERO, UIWindowFlags_NoScrollX);
		WinSetHovered(curwin);
		
		PushVar(UIStyleVar_SelectableTextAlign, Vec2(0, 0.5));
		for(UIWindow* window : windows){
			SetNextItemSize(Vec2(MAX_F32, 0));
			string s = toStr(window->name, "; hovered: ", WinHovered(window));
			if(Selectable(str8{(u8*)s.str, (s64)s.count}, window == debugee)){
				debugee = window;
			}
			if(MouseInArea(GetLastItemScreenPos(), GetLastItemSize())){
				DebugRect(window->position, window->dimensions * window->style.globalScale);
			}
			if(showChildren){
				PushLeftIndent(13);
				for(UIWindow* child : window->children){
					SetNextItemSize(Vec2(MAX_F32, 0));
					string c = toStr(child->name, "; hovered: ", WinHovered(child));
					if(Selectable(str8{(u8*)c.str, (s64)c.count}, child == debugee)){
						debugee = child;
					}
					if(MouseInArea(GetLastItemScreenPos(), GetLastItemSize())){
						DebugRect(child->position, child->dimensions * child->style.globalScale);
					}
				}
				PopLeftIndent();
			}
		}
		PopVar();
		WinUnSetHovered(curwin);
		EndChild();
		
		string h = toStr("Hovered: ", (hovered ? hovered->name : str8_lit("None")));
		TextOld(str8{(u8*)h.str, (s64)h.count});
		
		EndHeader();
	}
	
	if(BeginHeader(str8_lit("Breaking"))){
		MetricsBreaking();
		EndHeader();
	}
	
	if(BeginHeader(str8_lit("Cursor Debugging"))){
		MetricsDebugItem();
		TextF(str8_lit("Cursor Pos: (%4d,%4d)"), s32(DeshInput->mouseX), s32(DeshInput->mouseY));
		EndHeader();
	}
	
	Separator(20);
	
	string s = toStr("Selected Window: ", (debugee ? debugee->name : str8_lit("none")));
	TextOld(str8{(u8*)s.str, (s64)s.count});
	
	
	if(debugee){
		if(Button(str8_lit("Set Focused"))){
			SetFocusedWindow(debugee);
		}
		if(BeginHeader(str8_lit("Vars"))){
			BeginRow(str8_lit("MetricsWindowVarAlignment"), 2, style.fontHeight * 1.2, UIRowFlags_AutoSize);
			RowSetupColumnWidths({ CalcTextSize(str8_lit("Max Item Width: ")).x , 10 });
			RowSetupColumnAlignments({{0,0.5},{0,0.5}});
			TextOld(str8_lit("Render Time:"), UITextFlags_NoWrap);    TextF(str8_lit("%gms"), debugee->render_time);
			TextOld(str8_lit("Creation Time:"), UITextFlags_NoWrap);  TextF(str8_lit("%gms"), debugee->creation_time);
			TextOld(str8_lit("Item Count:"), UITextFlags_NoWrap);     TextF(str8_lit("%d"), debugee->items_count);
			TextOld(str8_lit("Position:"), UITextFlags_NoWrap);       TextF(str8_lit("(%g,%g)"), debugee->x,debugee->y);
			TextOld(str8_lit("Dimensions:"), UITextFlags_NoWrap);     TextF(str8_lit("(%g,%g)"), debugee->width,debugee->height);
			TextOld(str8_lit("Scroll:"), UITextFlags_NoWrap);         TextF(str8_lit("(%g,%g)"), debugee->scx,debugee->scy);
			TextOld(str8_lit("Max Scroll:"), UITextFlags_NoWrap);     TextF(str8_lit("(%g,%g)"), debugee->maxScroll.x,debugee->maxScroll.y);
			TextOld(str8_lit("Hovered:"), UITextFlags_NoWrap);        TextF(str8_lit("%s"), (WinHovered(debugee)) ? "true" : "false");
			TextOld(str8_lit("Focused:"), UITextFlags_NoWrap);        TextF(str8_lit("%s"), (WinFocused(debugee)) ? "true" : "false");
			TextOld(str8_lit("Max Item Width:"), UITextFlags_NoWrap); TextF(str8_lit("%g"), MaxItemWidth(debugee));
			EndRow();
			EndHeader();
		}
		
		if(BeginHeader(str8_lit("Items"))){
			SetNextWindowSize(Vec2(MAX_F32, 300));
			BeginChild(str8_lit("METRICSItems"), Vec2(0,0)); {
				forI(UI_WINDOW_ITEM_LAYERS){
					u32 count = 0;
					for(UIItem_old& item : debugee->items[i]){
						string s = toStr(UIItemTypeStrs[item.type], " ", count);
						if(BeginHeader(str8{(u8*)s.str, (s64)s.count})){
							persist f32 frs = CalcTextSize(str8_lit("FilledRectangle")).x;
							BeginRow(str8_lit("121255552525"), 3, style.buttonHeightRelToFont * style.fontHeight);
							RowSetupColumnWidths({ frs, 50, 40 });
							
							for(UIDrawCmd& dc : item.drawCmds){
								TextOld(UIDrawTypeStrs[dc.type]);
								if(MouseInArea(GetLastItemScreenPos(), GetLastItemSize())){
									for(int tri = 0; tri < dc.counts.y; tri += 3){
										vec2
											p0 = item.position * item.style.globalScale + debugee->position + item.style.globalScale * dc.vertices[dc.indices[tri]].pos,
										p1 = item.position * item.style.globalScale + debugee->position + item.style.globalScale * dc.vertices[dc.indices[tri + 1]].pos,
										p2 = item.position * item.style.globalScale + debugee->position + item.style.globalScale * dc.vertices[dc.indices[tri + 2]].pos;
										DebugTriangle(p0, p1, p2);
									}
								}
								if(Button(str8_lit("Create"))){
									break_drawCmd_create_hash = dc.hash;
								}
								if(Button(str8_lit("Draw"))){
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
		
		
		if(debugee->children.count && BeginHeader(str8_lit("Children"))){
			for(UIWindow* c : debugee->children){
				if(Button(c->name)){
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
		
		
		if(BeginHeader(str8_lit("Debug Visuals"))){
			Checkbox(str8_lit("Show Item Boxes"), &showItemBoxes);
			Checkbox(str8_lit("Show Item Cursors"), &showItemCursors);
			Checkbox(str8_lit("Show Item Names"), &showItemNames);
			Checkbox(str8_lit("Show Item Coords"), &showItemCoords);
			Checkbox(str8_lit("Show All DrawCmd Scissors"), &showAllDrawCmdScissors);
			Checkbox(str8_lit("Show All DrawCmd Triangles"), &showDrawCmdTriangles);
			Checkbox(str8_lit("Show Bordered Area"), &showBorderArea);
			Checkbox(str8_lit("Show Margined Area"), &showMarginArea);
			Checkbox(str8_lit("Show ScrollBared Area"), &showScrollBarArea);
			
			
			EndHeader();
		}
		
		if(showItemBoxes){
			forI(UI_WINDOW_ITEM_LAYERS){
				for(UIItem_old& item : debugee->items[i]){
					DebugRect(debugee->position + item.position, item.size);
				}
			}
		}
		
		if(showItemCursors){ 
			forI(UI_WINDOW_ITEM_LAYERS){
				for(UIItem_old& item : debugee->items[i]){
					{
						//UIDrawCmd dc;
						//dc.color = Color_Green;
						//dc.position = debugee->position + item.initialCurPos + item.style.windowMargins - vec2::ONE * 3 / 2.f;
						//dc.dimensions = vec2::ONE * 3;
						//debugCmds.add(dc);
					}
				}
			}
		}
		
		if(showItemNames){
			forI(UI_WINDOW_ITEM_LAYERS){
				for(UIItem_old& item : debugee->items[i]){
					DebugText(debugee->position + item.position, UIItemTypeStrs[item.type]);
				}
			}
		}
		
		if(showItemCoords){
			forI(UI_WINDOW_ITEM_LAYERS){
				for(UIItem_old& item : debugee->items[i]){
					string s = toStr("(",item.position.x,",",item.position.y,")");
					DebugText(debugee->position + item.position, str8{(u8*)s.str, (s64)s.count});
				}
			}
		}
		
		if(showAllDrawCmdScissors){
			for(UIItem_old& item : debugee->preItems){
				for(UIDrawCmd& dc : item.drawCmds){
					DebugRect(dc.scissorOffset, dc.scissorExtent, Color_Red);
				}
			}
			forI(UI_WINDOW_ITEM_LAYERS){
				for(UIItem_old& item : debugee->items[i]){
					for(UIDrawCmd& dc : item.drawCmds){
						DebugRect(dc.scissorOffset, dc.scissorExtent, Color_Yellow);
					}
				}
			}
			for(UIItem_old& item : debugee->postItems){
				for(UIDrawCmd& dc : item.drawCmds){
					DebugRect(dc.scissorOffset, dc.scissorExtent, Color_Green);
				}
			}
			
			for(UIWindow* c : debugee->children){
				for(UIItem_old& item : c->preItems){
					for(UIDrawCmd& dc : item.drawCmds){
						DebugRect(dc.scissorOffset, dc.scissorExtent, Color_Red);
					}
				}
				forI(UI_WINDOW_ITEM_LAYERS){
					for(UIItem_old& item : c->items[i]){
						for(UIDrawCmd& dc : item.drawCmds){
							DebugRect(dc.scissorOffset, dc.scissorExtent, Color_Yellow);
						}
					}
				}
				for(UIItem_old& item : c->postItems){
					for(UIDrawCmd& dc : item.drawCmds){
						DebugRect(dc.scissorOffset, dc.scissorExtent, Color_Green);
					}
				}
			}
			
		}
		
		if(showDrawCmdTriangles){
			for(UIItem_old& item : debugee->preItems){
				vec2 ipos = debugee->position + item.position * item.style.globalScale;
				for(UIDrawCmd& dc : item.drawCmds){
					for(int i = 0; i < dc.counts.y; i += 3){
						DebugTriangle(ipos + dc.vertices[dc.indices[i]].pos * item.style.globalScale,
									  ipos + dc.vertices[dc.indices[i + 1]].pos * item.style.globalScale,
									  ipos + dc.vertices[dc.indices[i + 2]].pos * item.style.globalScale, Color_Green);
					}
				}
			}
			forI(UI_WINDOW_ITEM_LAYERS){
				for(UIItem_old& item : debugee->items[i]){
					vec2 ipos = debugee->position + item.position * item.style.globalScale;
					for(UIDrawCmd& dc : item.drawCmds){
						for(int i = 0; i < dc.counts.y; i += 3){
							DebugTriangle(ipos + dc.vertices[dc.indices[i]].pos * item.style.globalScale,
										  ipos + dc.vertices[dc.indices[i + 1]].pos * item.style.globalScale,
										  ipos + dc.vertices[dc.indices[i + 2]].pos * item.style.globalScale);
						}
					}
				}
			}
			for(UIItem_old& item : debugee->postItems){
				vec2 ipos = debugee->position + item.position * item.style.globalScale;
				for(UIDrawCmd& dc : item.drawCmds){
					for(int i = 0; i < dc.counts.y; i += 3){
						DebugTriangle(ipos + dc.vertices[dc.indices[i]].pos * item.style.globalScale,
									  ipos + dc.vertices[dc.indices[i + 1]].pos * item.style.globalScale,
									  ipos + dc.vertices[dc.indices[i + 2]].pos * item.style.globalScale, Color_Blue);
					}
				}
			}
		}
		
		if(showBorderArea){
			auto b = BorderedArea_old(debugee);
			DebugRect(b.first + debugee->position, b.second);
		}
		if(showMarginArea){
			auto m = MarginedArea_old(debugee);
			DebugRect(m.first + debugee->position, m.second);
		}
		if(showScrollBarArea){
			auto s = ScrollBaredArea_old(debugee);
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
void UI::ShowMetricsWindow(){DPZoneScoped;
	show_metrics = 1;
}

void UI::DemoWindow(){DPZoneScoped;
	Begin(str8_lit("deshiUIDEMO"), vec2::ONE * 300, vec2::ONE * 300);
	
	if(BeginHeader(str8_lit("TextOld"))){
		TextOld(str8_lit("heres some text"));
		
		Separator(7);
		
		TextOld(str8_lit("heres some long text that should wrap if it reaches the end of the window"));
		
		Separator(7);
		
		TextOld(str8_lit("heres some long text that shouldn't wrap when it reaches the end of the window"), UITextFlags_NoWrap);
		
		EndHeader();
	}
	
	Separator(11); /////////////////////////////////////////////////////////////////////////////////////////
	
	if(BeginHeader(str8_lit("Button"))){
		persist str8 str = str8_lit("heres some text");
		if(Button(str8_lit("change text"))){
			str = str8_lit("heres some new text, because you pressed the button");
		}
		TextOld(str);
		
		Separator(7);
		
		persist color col = color(55, 45, 66);
		PushColor(UIStyleCol_ButtonBg, col);
		PushColor(UIStyleCol_ButtonBgActive, col);
		PushColor(UIStyleCol_ButtonBgHovered, col);
		if(Button(str8_lit("true while held"), UIButtonFlags_ReturnTrueOnHold)){
			col.r += 2;
			col.g += 2;
			col.b += 2;
		}
		PopColor(3);
		
		Separator(7);
		
		persist str8 str2 = str8_lit("this will change on button release");
		if(Button(str8_lit("true on release"), UIButtonFlags_ReturnTrueOnRelease)){
			str2 = str8_lit("this was changed on release");
		}
		TextOld(str2);
		
		Separator(7);
		
		TextOld(str8_lit("Style Vars: "));
		
		persist u8 button_border_size_buff[7] = {};
		TextOld(str8_lit("Button Border Size (f32): ")); SameLine();
		if(InputText(str8_lit("demo_button_border_size"), button_border_size_buff, 7, str8_lit("1.0"), UIInputTextFlags_Numerical | UIInputTextFlags_AnyChangeReturnsTrue  | UIInputTextFlags_EnterReturnsTrue)){
			style.buttonBorderSize = strtod((const char*)button_border_size_buff, 0);
		}
		
		persist color button_border_color = style.colors[UIStyleCol_ButtonBorder];
		TextOld(str8_lit("Style Colors: "));
		TextOld(str8_lit("TODO button colors"));
		
		EndHeader();
	}
	
	Separator(11); /////////////////////////////////////////////////////////////////////////////////////////
	
	if(BeginHeader(str8_lit("Slider"))){
		persist f32 sl1 = 0;
		
		Slider(str8_lit("slider1"), &sl1, 0, 100); SameLine(); TextF(str8_lit("%g"), sl1);
		
		EndHeader();
	}
	
	Separator(11); /////////////////////////////////////////////////////////////////////////////////////////
	
	if(BeginHeader(str8_lit("Headers"))){
		TextOld(str8_lit(
						 "Headers automatically indent according to UIStyleVar_IndentAmount\n"
						 "They also automatically adjust their width to the width of the window\n"
						 "TODO(sushi) make a way to turn these off!"
						 ));
		
		Separator(7);
		
		
		if(BeginHeader(str8_lit("Header 1"))){
			TextOld(str8_lit("some text in header 1"));
			if(BeginHeader(str8_lit("Header 2"))){
				TextOld(str8_lit("another nested header"));
				EndHeader();
			}
			EndHeader();
		}
		EndHeader();
	}
	
	Separator(11); /////////////////////////////////////////////////////////////////////////////////////////
	
	if(BeginHeader(str8_lit("Row"))){
		TextOld(str8_lit(
						 "UI::Row() aligns any other item in a row"
						 ));
		
		Separator(7);
		
		BeginRow(str8_lit("Demo_35135"), 3, 15);
		RowSetupRelativeColumnWidths({ 1,1,1 });
		TextOld(str8_lit("some text"));
		Button(str8_lit("a button"));
		Button(str8_lit("another button"));
		EndRow();
		
		Separator(7);
		
		TextOld(str8_lit(
						 "Rows aren't restricted to one Row, you can add as many items as you like as long as the amount of items is divisible by the amount of columns you made the Row with"
						 ));
		
		persist f32 rowyalign = 0.5;
		persist f32 rowxalign = 0.5;
		
		PushVar(UIStyleVar_RowItemAlign, Vec2(rowxalign, rowyalign));
		
		Separator(7);
		
		BeginRow(str8_lit("Demo_3541351"), 2, 30);
		RowSetupColumnWidths({ 100, 100 });
		TextOld(str8_lit("example of")); TextOld(str8_lit("aligning text"));
		TextOld(str8_lit("evenly over")); TextOld(str8_lit("multiple rows"));
		EndRow();
		
		Separator(7);
		
		TextOld(str8_lit("you can change how items are aligned within row cells as well"));
		Slider(str8_lit("slider1"), &rowxalign, 0, 1); SameLine(); TextF(str8_lit("x align %g"), rowxalign);
		Slider(str8_lit("slider2"), &rowyalign, 0, 1); SameLine(); TextF(str8_lit("y align %g"), rowyalign);
		
		PopVar();
		
		Separator(7);
		
		TextOld(str8_lit("Rows also allow you to use either persist or relative column widths"));
		
		Separator(7);
		
		persist f32 scw1 = 60;
		persist f32 scw2 = 60;
		persist f32 scw3 = 60;
		
		persist f32 dcw1 = 1;
		persist f32 dcw2 = 1;
		persist f32 dcw3 = 1;
		
		persist u32 selected = 0;
		if(Selectable(str8_lit("Static Column Widths"), !selected)) selected = 0;
		if(Selectable(str8_lit("Relative column widths"), selected)) selected = 1;
		
		switch (selected){
			case 0:{
				BeginRow(str8_lit("Demo_1351351"), 3, 16);
				RowSetupColumnWidths({ scw1, scw2, scw3 });
				TextOld(str8_lit("text"));
				TextOld(str8_lit("long text"));
				TextOld(str8_lit("text"));
				EndRow();
				
				Slider(str8_lit("demo_scw1"), &scw1, 0, 90); SameLine(); TextF(str8_lit("%g"), scw1);
				Slider(str8_lit("demo_scw2"), &scw2, 0, 90); SameLine(); TextF(str8_lit("%g"), scw2);
				Slider(str8_lit("demo_scw3"), &scw3, 0, 90); SameLine(); TextF(str8_lit("%g"), scw3);
			}break;
			case 1:{
				BeginRow(str8_lit("Demo_66462"), 3, 16);
				RowSetupRelativeColumnWidths({ dcw1, dcw2, dcw3 });
				TextOld(str8_lit("text"));
				TextOld(str8_lit("long text"));
				TextOld(str8_lit("text"));
				EndRow();
				
				Slider(str8_lit("demo_dcw1"), &dcw1, 1, 5); SameLine(); TextF(str8_lit("%g"), dcw1);
				Slider(str8_lit("demo_dcw2"), &dcw2, 1, 5); SameLine(); TextF(str8_lit("%g"), dcw2);
				Slider(str8_lit("demo_dcw3"), &dcw3, 1, 5); SameLine(); TextF(str8_lit("%g"), dcw3);
			}break;
		}
		
		
		EndHeader();
	}
	
	Separator(11); /////////////////////////////////////////////////////////////////////////////////////////
	
	if(BeginHeader(str8_lit("Child Windows"))){
		TextOld(str8_lit("You can nest windows inside another one by using BeginChild"));
		
		BeginChild(str8_lit("demochild"), Vec2(curwin->width - style.indentAmount, 300));
		
		TextOld(str8_lit("Heres some text in the child window"));
		
		Separator(7);
		
		TextOld(str8_lit("Child windows have all the same functionality of base windows, save for a few TODOS"));
		
		Separator(7);
		
		persist Texture* tex = assets_texture_create_from_file_simple(str8_lit("lcdpix.png"));
		
		TextOld(str8_lit("heres a image in the child window:"));
		Image(tex);
		
		
		
		forI(15){
			TextOld(str8_lit("heres a bunch of text in the child window"));
		}
		
		Button(str8_lit("child window button"));
		
		EndChild();
		
		EndHeader();
		
	}
	
	Separator(11); /////////////////////////////////////////////////////////////////////////////////////////
	
	if(BeginHeader(str8_lit("Combos"))){
		TextOld(str8_lit("Combos are a huge TODO right now, but heres how they currently look"));
		
		Separator(7);
		
		persist u32 selected = 0;
		if(BeginCombo(str8_lit("uiDemoCombo"), str8_lit("preview text"))){
			forI(10){
				string s = toStr("selection", i);
				if(Selectable(str8{(u8*)s.str, (s64)s.count}, selected == i)){
					selected = i;
				}
			}
			EndCombo();
		}
		
		EndHeader();
	}
	
	Separator(11); /////////////////////////////////////////////////////////////////////////////////////////
	
	if(BeginHeader(str8_lit("Style Variables"))){
		TextOld(str8_lit("Adjusting these will adjust the base style variables of UI"));
		Separator(7);
		
		TextOld(str8_lit("Window Padding (vec2)"));
		Slider(str8_lit("demo_wpx"), &style.windowMargins.x, 0, 100);
		SameLine();
		Slider(str8_lit("demo_wpy"), &style.windowMargins.y, 0, 100);
		
		TextOld(str8_lit("Item Spacing (vec2)"));
		Slider(str8_lit("demo_isx"), &style.itemSpacing.x, 0, 100);
		SameLine();
		Slider(str8_lit("demo_isy"), &style.itemSpacing.y, 0, 100);
		
		EndHeader();
	}
	
	End();
}

//@Init
//initializes core UI with an invisible working window covering the entire screen
//also initializes styles
void UI::Init(){DPZoneScoped;
	DeshiStageInitStart(DS_UI, DS_ASSETS, "Attempted to initialize UI module before initializing Assets module");
	
	curwin = new UIWindow();
	curwin->name = str8_lit("base");
	curwin->position = Vec2(0,0);
	curwin->dimensions = Vec2(DeshWindow->width, DeshWindow->height);
	
	//load font
	style.font = assets_font_create_from_file_bdf(str8_lit("gohufont-11.bdf"));
	Assert(style.font != assets_font_null());
	
	//push default color scheme
	//this is never meant to be popped
	PushColor(UIStyleCol_Border,    0xaaaaaaff);
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
	PushVar(UIStyleVar_WindowMargins,             Vec2(10, 10));
	PushVar(UIStyleVar_WindowBorderSize,          1);
	PushVar(UIStyleVar_ButtonBorderSize,          1);
	PushVar(UIStyleVar_TitleBarHeight,            style.fontHeight * 1.2f);
	PushVar(UIStyleVar_TitleTextAlign,            Vec2(1, 0.5f));
	PushVar(UIStyleVar_ItemSpacing,               Vec2(1, 1));
	PushVar(UIStyleVar_ScrollAmount,              Vec2(10, 10));
	PushVar(UIStyleVar_CheckboxSize,              Vec2(10, 10));
	PushVar(UIStyleVar_CheckboxFillPadding,       2);
	PushVar(UIStyleVar_InputTextTextAlign,        Vec2(0.f,  0.5f));
	PushVar(UIStyleVar_ButtonTextAlign,           Vec2(0.5f, 0.5f));
	PushVar(UIStyleVar_HeaderTextAlign,           Vec2(0.f,  0.5f));
	PushVar(UIStyleVar_SelectableTextAlign,       Vec2(0.5f, 0.5f));
	PushVar(UIStyleVar_TabTextAlign,              Vec2(0.5f, 0.5f));
	PushVar(UIStyleVar_ButtonHeightRelToFont,     1.3f);
	PushVar(UIStyleVar_HeaderHeightRelToFont,     1.3f);
	PushVar(UIStyleVar_InputTextHeightRelToFont,  1.3f);
	PushVar(UIStyleVar_CheckboxHeightRelToFont,   1.3f);
	PushVar(UIStyleVar_SelectableHeightRelToFont, 1.3f);
	PushVar(UIStyleVar_TabHeightRelToFont,        1.3f);
	PushVar(UIStyleVar_RowItemAlign,              Vec2(0.5f, 0.5f));
	PushVar(UIStyleVar_ScrollBarYWidth,           5);
	PushVar(UIStyleVar_ScrollBarXHeight,          5);
	PushVar(UIStyleVar_IndentAmount,              12);
	PushVar(UIStyleVar_TabSpacing,                5);
	PushVar(UIStyleVar_FontHeight,                (f32)style.font->max_height);
	PushVar(UIStyleVar_WindowSnappingTolerance,   Vec2(15,15));                
	
	
	PushScale(Vec2(1, 1));
	
	initColorStackSize = colorStack.count;
	initStyleStackSize = varStack.count;
	
	windows.add(str8_lit("base"), curwin);
	//windowStack.add(curwin);
	
	DeshiStageInitEnd(DS_UI);
}

//in our final draw system, this is the function that primarily does the work
//of figuring out how each draw call will be sent to the renderer
inline void DrawItem(UIItem_old& item, UIWindow* window){DPZoneScoped;
	
	vec2 winpos = Vec2(window->x, window->y);
	vec2 winsiz = Vec2(window->width, window->height) * window->style.globalScale;
	vec2 winScissorOffset = window->visibleRegionStart;
	vec2 winScissorExtent = window->visibleRegionSize * window->style.globalScale;
	
	UIWindow* parent = window->parent;
	
	vec2 itempos = window->position + item.position * item.style.globalScale;
	vec2 itemsiz = item.size;
	int i = 0;
	UIDrawCmd* lastdc = 0;
	for(UIDrawCmd& drawCmd : item.drawCmds){
		BreakOnDrawCmdDraw;
		//NOTE this expects vertex positions to be in item space
		forI(drawCmd.counts.x) drawCmd.vertices[i].pos = floor((drawCmd.vertices[i].pos * item.style.globalScale + itempos));
		
		vec2 dcse = (drawCmd.useWindowScissor ? winScissorExtent : drawCmd.scissorExtent * item.style.globalScale);
		vec2 dcso = (drawCmd.useWindowScissor ? winScissorOffset : itempos + drawCmd.scissorOffset);
		
		//modify the scissor offset and extent according to the kind of window we are drawing
		switch (window->type){
			case UIWindowType_PopOut:
			case UIWindowType_Normal:{
				dcso = Min(winpos + winScissorExtent - dcse, Max(winpos, dcso));
				dcse += Min(dcso - winpos, vec2::ZERO);
				if(drawCmd.useWindowScissor)
					dcse += Min(winpos, vec2::ZERO);
			}break;
			case UIWindowType_Child:{
				dcso = Min(winScissorOffset + winScissorExtent - dcse, Max(dcso, winScissorOffset));
				dcse += Min(dcso - winScissorOffset, vec2::ZERO);
				if(drawCmd.useWindowScissor)
					dcse += Min(winScissorOffset, vec2::ZERO);
				
			}break;
		}
		
		dcse = ClampMin(dcse, vec2::ZERO);
		dcso = ClampMin(dcso, vec2::ZERO);
		
		//Assert(dcse.x != 0 && dcse.y != 0);
		Assert(!isinf(dcse.x) && !isinf(dcse.y));
		render_set_active_surface_idx(drawCmd.render_surface_target_idx);
		render_start_cmd2(window->layer, drawCmd.tex, dcso, dcse);
		render_add_vertices2(window->layer, drawCmd.vertices, drawCmd.counts.x, drawCmd.indices, drawCmd.counts.y);
		
		//if((input_mouse_position() - drawCmd.vertices[0].pos).mag() < 4){
		//	DebugCircle(drawCmd.vertices[0].pos, 4, Color_Green);
		//	if(input_lmouse_pressed()){
		//		break_drawCmd_draw_hash=drawCmd.hash;
		//	}
		//}
		//else{
		//	DebugCircle(drawCmd.vertices[0].pos, 4);
		//}
		
		drawCmd.scissorExtent = dcse;
		drawCmd.scissorOffset = dcso;
		lastdc = &drawCmd;
		i++;
	}
	
	render_set_active_surface_idx(0);
}

inline void DrawWindow(UIWindow* p, UIWindow* parent = 0){DPZoneScoped;
	Stopwatch winren = start_stopwatch();
	
	if(WinHovered(p) && !(p->flags & UIWindowFlags_DontSetGlobalHoverFlag))
		StateAddFlag(UISGlobalHovered);
	
	//draw pre cmds first
	for(UIItem_old& item : p->preItems){
		DrawItem(item, p);
	}
	
	
	forI(UI_WINDOW_ITEM_LAYERS){
		for(UIItem_old& item : p->items[i]){
			if(item.type == UIItemType_Window){
				item.child->position = p->position + item.position * item.style.globalScale;
				DrawWindow(item.child, p);
				WinUnSetBegan(item.child);
				continue;
			}
			DrawItem(item, p);
		}
	}
	
	
	//draw post items, such as scroll bars or context menus
	for(UIItem_old& item : p->postItems){
		DrawItem(item, p);
	}
	
	//after we draw everything to do with the base window we then draw its popouts
	for(UIItem_old& item : p->popOuts){
		item.child->position = p->position + item.position * item.style.globalScale;
		DrawWindow(item.child, p);
		WinUnSetBegan(item.child);
	}
	
	p->render_time = peek_stopwatch(winren);
	
	//when compiling for debug we defer this to after the metrics window
#ifndef BUILD_INTERNAL
	p->preItems.clear();
	p->postItems.clear();
	forI(UI_WINDOW_ITEM_LAYERS){
		p->items[i].clear();
	}
#else
	p->render_time = peek_stopwatch(winren);
	p->items_count = 0;
	forI(UI_WINDOW_ITEM_LAYERS){
		p->items_count += p->items[i].count;
	}
#endif
	
	
	
}

void CleanUpWindow(UIWindow* window){DPZoneScoped;
	window->preItems.clear();
	window->postItems.clear();
	window->popOuts.clear();
	forI(UI_WINDOW_ITEM_LAYERS){
		window->items[i].clear();
	}
	for(UIWindow* c : window->children){
		CleanUpWindow(c);
	}
}

//for checking that certain things were taken care of eg, popping colors/styles/windows
void UI::Update(){DPZoneScoped;
	//there should only be default stuff in the stacks
	Assert(!windowStack.count, 
		   "Frame ended with hanging windows in the stack, make sure you call End() if you call Begin()!");
	
	
	//TODO(sushi) impl this for other stacks
	if(varStack.count != initStyleStackSize){
		printf("Frame ended with hanging vars in the stack, make sure you pop vars if you push them!\nVars were:\n");
		
		for(u32 i = varStack.count - 1; i > initStyleStackSize - 1; i--)
			printf("%s\n", styleVarStr[varStack[i].var].str);
		
		Assert(0);
	}
	
	Assert(colorStack.count == initColorStackSize, 
		   "Frame ended with hanging colors in the stack, make sure you pop colors if you push them!");
	
	Assert(leftIndentStack.count == 1, "Forgot to call End for an indenting Begin!");
	Assert(rightIndentStack.count == 1, "Forgot to call End for an indenting Begin!");
	Assert(drawTargetStack.count == 1, "Forgot to pop a draw target!");
	Assert(!StateHasFlag(UISTabBegan), "Forgot to call EndTab for a BeginTab");
	Assert(!StateHasFlag(UISTabBarBegan), "Forgot to call EndTabBar for a BeginTabBar");
	
	forI(UIItemType_COUNT)
		Assert(itemFlags[i] == 0, "Forgot to clear an item's default flags!");
	
	
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
	
	if(show_metrics){
		DisplayMetrics();
		show_metrics = 0;
	}
	
	ui_stats = { 0 };
	hovered = 0;
	StateRemoveFlag(UISGlobalHovered);
	MarginPositionOffset = vec2::ZERO;
	MarginSizeOffset = vec2::ZERO;
	
	
	//windows input checking functions
	CheckWindowsForFocusInputs();
	CheckForHoveredWindow();
	
	if(inputupon) CheckWindowForScrollingInputs(inputupon);
	if(inputupon) CheckWindowForResizingInputs(inputupon);
	if(inputupon) CheckWindowForDragInputs(inputupon);
	
	
	//reset cursor to default if no item decided to set it 
	if(!StateHasFlag(UISCursorSet)){
		if(StateHasFlag(UISGlobalHovered)){
			window_set_cursor_type(DeshWindow, CursorType_Arrow);
		}
	}else{
		StateRemoveFlag(UISCursorSet);
	}
	
	
	//draw windows in order 
	for(UIWindow* p : windows){
		DrawWindow(p);
		WinUnSetBegan(p);
		p->items_count = 0;
	}
	
	
	//it should be safe to do this any time the mouse is released
	if(input_lmouse_released()){ AllowInputs; }
	
	
	//we defer window item clearing to after the metrics window is drawn
	//in debug builds
#ifdef BUILD_INTERNAL
	for(UIWindow* p : windows){
		CleanUpWindow(p);
	}
#endif
	
	//draw all debug commands if there are any
	for(UIDrawCmd& drawCmd : debugCmds){
		render_start_cmd2(render_decoration_layer_index(), drawCmd.tex, vec2::ZERO, Vec2(DeshWindow->width, DeshWindow->height));
		render_add_vertices2(render_decoration_layer_index(), drawCmd.vertices, drawCmd.counts.x, drawCmd.indices, drawCmd.counts.y);
	}
	debugCmds.clear();
}

void UI::DrawDebugRect(vec2 pos, vec2 size, color col)             {DPZoneScoped; DebugRect(pos, size, col); }
void UI::DrawDebugRectFilled(vec2 pos, vec2 size, color col)       {DPZoneScoped; DebugRectFilled(pos, size, col); }
void UI::DrawDebugCircle(vec2 pos, f32 radius, color col)          {DPZoneScoped; DebugCircle(pos, radius, col); }
void UI::DrawDebugCircleFilled(vec2 pos, f32 radius, color col)    {DPZoneScoped; DebugCircleFilled(pos, radius, col); }
void UI::DrawDebugLine(vec2 pos1, vec2 pos2, color col)            {DPZoneScoped; DebugLine(pos1, pos2, col); }
void UI::DrawDebugTriangle(vec2 p0, vec2 p1, vec2 p2, color color){DPZoneScoped; DebugTriangle(p0, p1, p2, color); }
