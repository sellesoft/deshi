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

local vec2 NextWinSize  = vec2(-1, 0);
local vec2 NextWinPos   = vec2(-1, 0);
local vec2 NextItemSize = vec2(-1, 0);
local b32 NextActive   = 0;

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
local b32   rowInProgress;

//misc state vars
local u32 currlayer       = floor(UI_WINDOW_ITEM_LAYERS / 2.f);
local b32 globalHovered   = false; //set if any window other than base is hovered
local b32 cursorWasSet    = 0; //set when something changes the window's cursor
local UIWindow* inputupon = 0; //set to a window who has drag, scrolling, or resizing inputs being used on it 

enum WinInputState_ {
	ISNone,
	ISScrolling,
	ISDragging,
	ISResizing,
	ISPreventInputs,
}; typedef u32 WinInputState;

WinInputState inputState = ISNone;

//helper defines


#define HasFlag(flag) (flags & flag)
#define HasFlags(flag) ((flags & flag) != flag) //used on flag enums that are collections of flags
#define WinHasFlag(flag) (curwin->flags & flag)
#define WinHasFlags(flag) ((curwin->flags & flag) == flag) 
#define DrawCmdScreenPos(pos) pos + item->position + curwin->position 
#define ItemScreenPos item->position + curwin->position 

#define globalIndent *indentStack.last

#define CanTakeInput      inputState == ISNone
#define PreventInputs     inputState = ISPreventInputs
#define AllowInputs       inputState = ISNone;      inputupon = 0;
#define SetResizingInput  inputState = ISResizing;  inputupon = window;
#define SetDraggingInput  inputState = ISDragging;  inputupon = window;
#define SetScrollingInput inputState = ISScrolling; inputupon = window; window->beingScrolled = 1;
#define WinResizing       inputState == ISResizing
#define WinDragging       inputState == ISDragging
#define WinScrolling      inputState == ISScrolling
#define ItemCanTakeInput  !(WinResizing || WinDragging || WinScrolling)

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
	return Math::PointInRectangle(DeshInput->mousePos, item->position + curwin->position, item->size);
}

inline b32 isLocalAreaHovered(vec2 pos, vec2 size, UIItem* item) {
	return Math::PointInRectangle(DeshInput->mousePos, pos + item->position + curwin->position, size);
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
	if (rowInProgress) {
		//abstract item types (lines, rectangles, etc.) are not row'd, for now
		if (itemmade->type != UIItemType_Abstract) {
			row.items.add(itemmade);
			
			f32 height = row.height;
			f32 width;
			//determine if the width is relative to the size of the item or not
			if (row.columnWidths[(row.items.count - 1) % row.columns].second == true)
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
	else if (moveCursor) curwin->cursor = vec2{ 0, itemmade->position.y + itemmade->size.y + style.itemSpacing.y - style.windowPadding.y + curwin->scy } ;
}

//function for getting the position of a new item based on style, so the long string of additions
//is centralized for new additions, if ever made, and so that i dont have to keep writing it :)
inline vec2 PositionForNewItem() {
	return curwin->cursor + (style.windowPadding - curwin->scroll) + vec2(globalIndent, 0);
}



void UI::SetNextItemActive() {
	NextActive = 1;
}

UIStyle& UI::GetStyle(){
	return style;
}

UIWindow* UI::GetWindow() {
    return curwin;
}

//the following 4 functions should probably error out sofly, rather than asserting

//returns the cursor to the same line as the previous and moves it to the right by the 
//width of the object
void UI::SameLine(){
	//Assert(curwin->items.count, "Attempt to sameline an item creating any items!");
	if (curwin->items[currlayer].last) {
		curwin->cursor.y = curwin->items[currlayer].last->initialCurPos.y;
		curwin->cursor.x += curwin->items[currlayer].last->initialCurPos.x + curwin->items[currlayer].last->size.x + style.itemSpacing.x;
	}
}

vec2 UI::GetLastItemPos() {
	//Assert(curwin->items.count, "Attempt to get last item position without creating any items!");
	return curwin->items[currlayer].last->position;
}

vec2 UI::GetLastItemSize() {
	//Assert(curwin->items.count, "Attempt to get last item size without creating any items!");
	return curwin->items[currlayer].last->size;
}

vec2 UI::GetLastItemScreenPos() {
	//Assert(curwin->items.count, "Attempt to get last item position without creating any items!");
	return curwin->position + curwin->items[currlayer].last->position;
}

u32 UI::GetCenterLayer() {
	return UI_CENTER_LAYER;
}

//internal last item getter, returns nullptr if there are none
inline UIItem* GetLastItem(u32 layeroffset = 0) {
	return curwin->items[currlayer + layeroffset].last;
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
		curwin->items[currlayer + layeroffset].add(UIItem{ type, curwin->cursor, style });
	}
	curwin->items_count++;
	return GetLastItem(layeroffset);
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
	Assert(!rowInProgress, "Attempted to start a new Row without finishing one already in progress!");
	//TODO(sushi) when we have more row flags, check for mutually exclusive flags here
	row.columns = columns;
	row.flags = flags;
	row.height = rowHeight;
	rowInProgress = 1;
	row.position = PositionForNewItem();
	forI(columns) row.columnWidths.add({ 0.f,false });
}

void UI::EndRow() {
	Assert(rowInProgress, "Attempted to a end a row without calling BeginRow() first!");
	Assert(row.items.count % row.columns == 0, "Attempted to end a Row without giving the correct amount of items!");
	
	curwin->cursor = vec2{ 0, row.position.y  + row.yoffset + style.itemSpacing.y - style.windowPadding.y + curwin->scroll.y };
	
	row.items.clear();
	row.columnWidths.clear();
	row = UIRow{ 0 };
	rowInProgress = 0;
}

//this function sets up a static column width for a specified column that does not respect the size of the object
void UI::RowSetupColumnWidth(u32 column, f32 width) {
	Assert(rowInProgress, "Attempted to set a column's width with no Row in progress!");
	Assert(column <= row.columns && column >= 1, "Attempted to set a column who doesn't exists width!");
	row.columnWidths[column - 1] = { width, false };
}

//this function sets up static column widths that do not respect the size of the item at all
void UI::RowSetupColumnWidths(array<f32> widths){
	Assert(rowInProgress, "Attempted to pass column widths without first calling BeginRow()!");
	Assert(widths.count == row.columns, "Passed in the wrong amount of column widths for in progress Row");
	forI(row.columns)
		row.columnWidths[i] = { widths[i], false };
}

//see the function below for what this does
void UI::RowSetupRelativeColumnWidth(u32 column, f32 width) {
	Assert(rowInProgress, "Attempted to set a column's width with no Row in progress!");
	Assert(column <= row.columns && column >= 1, "Attempted to set a column who doesn't exists width!");
	row.columnWidths[column - 1] = { width, true };
}

//this function sets it so that column widths are relative to the size of the item the cell holds
//meaning it should be passed something like 1.2 or 1.3, indicating that the column should have a width of 
//1.2 * width of the item
void UI::RowSetupRelativeColumnWidths(array<f32> widths) {
	Assert(rowInProgress, "Attempted to pass column widths without first calling BeginRow()!");
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
	curwin->items[currlayer].add(item);
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
	curwin->items[currlayer].add(item);
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
	curwin->items[currlayer].add(item);
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
	curwin->items[currlayer].add(item);
	
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
	curwin->items[currlayer].add(item);
}


//@Items


void UI::SetNextItemSize(vec2 size) {
	NextItemSize = size;
}


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
				f32 maxw = curwin->width - 2 * style.windowPadding.x - globalIndent + curwin->scx;
				f32 currlinew = 0;
				
				for (string& t : newlined) {
					for (int i = 0; i < t.count; i++) {
						currlinew += font->GetPackedChar(t[i])->xadvance * wscale;
						
						if (currlinew >= maxw) {
							
							//find closest space to split by, if none we just split the word
							u32 lastspc = t.findLastChar(' ', i);
							string nustr = t.substr(0, (lastspc == string::npos) ? i - 1 : lastspc);
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
				u32 maxChars = floor(((curwin->width - 2 * style.windowPadding.x + curwin->scx) - workcur.x - globalIndent) / style.font->max_width);
				
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

	AdvanceCursor(item);
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
	TextW(text, PositionForNewItem(), style.colors[UIStyleCol_Text], HasFlag(UITextFlags_NoWrap));
}

void UI::Text(const char* text, vec2 pos, UITextFlags flags) {
	TextW(text, pos, style.colors[UIStyleCol_Text], HasFlag(UITextFlags_NoWrap), 0);
}

void UI::Text(const wchar_t* text, UITextFlags flags){
	TextW(text, PositionForNewItem(), style.colors[UIStyleCol_Text], HasFlag(UITextFlags_NoWrap));
}

void UI::Text(const wchar_t* text, vec2 pos, UITextFlags flags){
	TextW(text, pos, style.colors[UIStyleCol_Text], HasFlag(UITextFlags_NoWrap), 0);
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

b32 UI::Button(const char* text, vec2 pos, UIButtonFlags flags) {
	UIItem* item = BeginItem(UIItemType_Button);
	item->size = (NextItemSize.x != -1) ? NextItemSize : vec2(Min(curwin->width, Max(50.f, CalcTextSize(text).x * 1.1f)), style.fontHeight * style.buttonHeightRelToFont);
	item->position = pos;
	AdvanceCursor(item);

	b32 active = ItemCanTakeInput && Math::PointInRectangle(DeshInput->mousePos, curwin->position + item->position, item->size * style.globalScale);

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
		if (HasFlag(UIButtonFlags_ReturnTrueOnHold))
			if (DeshInput->LMouseDown()) { PreventInputs; return true; }
			else return false;
		if (HasFlag(UIButtonFlags_ReturnTrueOnRelease))
			if (DeshInput->LMouseReleased()) return true;
			else return false;
		if (DeshInput->LMousePressed()){ PreventInputs; return true;}
	}
	else return false;
}

b32 UI::Button(const char* text, UIButtonFlags flags) {
	return Button(text, PositionForNewItem(), flags);
}



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
	
	b32 bgactive = ItemCanTakeInput && isItemHovered(item);
	b32 fiactive = ItemCanTakeInput && isLocalAreaHovered(fillpos, fillsiz, item);
	
	
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
	
	
	
	if (DeshInput->LMousePressed() && Math::PointInRectangle(DeshInput->mousePos, curwin->position + item->position * style.globalScale, boxsiz * style.globalScale)) {
		*b = !*b;
		PreventInputs;
	}
}

local b32 combo_open = 0;
local u32 selectables_added = 0;
local UIItem* comboItem = 0; //global, so endcombo can share this with begincombo, and so selectable can read how it should be placed 
//a combo is built by selectables called within its Begin/End
b32 UI::BeginCombo(const char* label, const char* prev_val, vec2 pos) {
	comboItem = BeginItem(UIItemType_Combo, 1);
	comboItem->position = pos;
	comboItem->size = (NextItemSize.x != -1 ? NextItemSize : CalcTextSize(prev_val) * 1.5);
	
	AdvanceCursor(comboItem);
	
	if (!combos.has(label)) {
		combos.add(label);
		combos[label] = false;
	}
	
	b32& open = combos[label];
	
	b32 active = ItemCanTakeInput && isItemHovered(comboItem);
	if (active && DeshInput->LMousePressed()) { 
		open = !open; 
		PreventInputs;
	}
	
	combo_open = open;
	
	{//background
		UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
		drawCmd.color = style.colors[(active ? (DeshInput->LMouseDown() ? UIStyleCol_SelectableBgActive : UIStyleCol_SelectableBgHovered) : UIStyleCol_SelectableBg)];
		drawCmd.position = vec2::ZERO;
		drawCmd.dimensions = comboItem->size;
		comboItem->drawCmds.add(drawCmd);
		
	}
	
	{//text
		UIDrawCmd drawCmd{ UIDrawType_Text };
		drawCmd.position =
			vec2((comboItem->size.x - CalcTextSize(prev_val).x) * style.buttonTextAlign.x,
				 (style.fontHeight * style.buttonHeightRelToFont - style.fontHeight) * style.buttonTextAlign.y);
		drawCmd.text = string(prev_val);
		drawCmd.color = style.colors[UIStyleCol_Text];
		drawCmd.scissorOffset = vec2::ZERO;
		drawCmd.scissorExtent = comboItem->size;
		drawCmd.useWindowScissor = false;
		drawCmd.font = style.font;
		comboItem->drawCmds.add(drawCmd);
	}
	
	return combos[label];
}

b32 UI::BeginCombo(const char* label, const char* prev_val) {
	return BeginCombo(label, prev_val, PositionForNewItem());
}



void UI::EndCombo() {
	Assert(comboItem, "attempt to end a combo without starting one");
	
	comboItem = 0;
	combo_open = 0;
	selectables_added = 0;
	
}

b32 UI::Selectable(const char* label, vec2 pos, b32 selected) {
	
	b32 clicked = 0;
	if (comboItem && combo_open) {
		//if a combo has been started and is open, we attach the selectables to it
		selectables_added++;
		b32 active = ItemCanTakeInput && Math::PointInRectangle(DeshInput->mousePos, curwin->position + comboItem->position.yAdd(comboItem->size.y * selectables_added), comboItem->size);
		if (active && DeshInput->LMousePressed()) { 
			clicked = true; 
			PreventInputs;
		}
		
		{//background
			UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
			drawCmd.position = vec2(0, comboItem->size.y * selectables_added);
			drawCmd.dimensions = comboItem->size;
			if(selected)
				drawCmd.color = style.colors[(active && DeshInput->LMouseDown() ? UIStyleCol_SelectableBgActive : UIStyleCol_SelectableBgHovered)];
			else
				drawCmd.color = style.colors[(active ? (DeshInput->LMouseDown() ? UIStyleCol_SelectableBgActive : UIStyleCol_SelectableBgHovered) : UIStyleCol_SelectableBg)];
			comboItem->drawCmds.add(drawCmd);
		}
		
		{//text
			UIDrawCmd drawCmd{ UIDrawType_Text };
			drawCmd.position =
				vec2((comboItem->size.x - CalcTextSize(label).x) * style.buttonTextAlign.x,
					 (style.fontHeight * style.buttonHeightRelToFont - style.fontHeight) * style.buttonTextAlign.y + comboItem->size.y * selectables_added);
			drawCmd.text = string(label);
			drawCmd.color = style.colors[UIStyleCol_Text];
			drawCmd.scissorOffset = vec2(0, comboItem->size.y * selectables_added);
			drawCmd.scissorExtent = comboItem->size;
			drawCmd.useWindowScissor = false;
			drawCmd.font = style.font;
			comboItem->drawCmds.add(drawCmd);
		}
	}
	else {
		//else the selectable is just its own item
		UIItem* item = BeginItem(UIItemType_Selectable, 0);
		item->size = (NextItemSize.x != -1 ? NextItemSize : CalcTextSize(label) * 1.5);
		item->position = pos;
		
		AdvanceCursor(item);
		
		b32 active = ItemCanTakeInput && Math::PointInRectangle(DeshInput->mousePos, curwin->position + item->position.yAdd(item->size.y * selectables_added), item->size);
		if (active && DeshInput->LMousePressed()) { 
			clicked = true; 
			PreventInputs;
		}

		{//background
			UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
			drawCmd.position = vec2(0, item->size.y * selectables_added);
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
				vec2((item->size.x - CalcTextSize(label).x) * style.buttonTextAlign.x,
					 (style.fontHeight * style.buttonHeightRelToFont - style.fontHeight) * style.buttonTextAlign.y + item->size.y * selectables_added);
			drawCmd.text = string(label);
			drawCmd.color = style.colors[UIStyleCol_Text];
			drawCmd.scissorOffset = vec2(0, item->size.y * selectables_added);
			drawCmd.scissorExtent = item->size;
			drawCmd.useWindowScissor = false;
			drawCmd.font = style.font;
			item->drawCmds.add(drawCmd);
		}
	}
	
	return clicked;
}

b32 UI::Selectable(const char* label, b32 selected) {
	return Selectable(label, PositionForNewItem(), selected);
}

b32 UI::BeginHeader(const char* label) {
	UIItem* item = BeginItem(UIItemType_Header);
	
	b32* open = 0;
	if (!headers.has(label)) {
		headers.add(label);
		headers[label] = false;
	}
	open = &headers[label];
	
	item->position = PositionForNewItem();
	item->size = (NextItemSize.x == -1 ?
				  vec2(curwin->width - style.windowPadding.x * 2 - globalIndent, style.fontHeight * style.headerHeightRelToFont) :
				  NextItemSize);
	

	AdvanceCursor(item);
	
	b32 active = ItemCanTakeInput && isItemHovered(item);
	
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
	item->size = (NextItemSize.x == -1 ?
				  vec2{ curwin->width * (1.f / 3), 10 } :
				  NextItemSize);
	
	AdvanceCursor(item);
	
	b32 active = ItemCanTakeInput && isItemHovered(item);
	
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
	item->size = vec2(curwin->width - 2 * style.windowPadding.x - globalIndent, height);
	
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
	
	vec2 dim;
	if (flags & UIInputTextFlags_FitSizeToText) {
		dim = UI::CalcTextSize(string(buff));
	}
	else if (NextItemSize.x != -1) {
		dim = NextItemSize;
		NextItemSize = vec2{ -1,0 };
	}
	else {
		dim = vec2(Math::clamp(100.f, 0.f, Math::clamp(curwin->width - 2.f*style.windowPadding.x, 1.f, FLT_MAX)), style.inputTextHeightRelToFont*style.fontHeight);
	}
	
	item->size = dim;
	item->position = position;
	
	b32 hovered = ItemCanTakeInput && isItemHovered(item);
	
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
	
	b32 active = ItemCanTakeInput && (activeId == state->id);
	if (NextActive || DeshInput->KeyPressed(MouseButton::LEFT)) {
		if (NextActive || hovered) {
			activeId = state->id;
			NextActive = 0;
		}
		else if (active) activeId = -1;
	}
	
	if (hovered) { DeshWindow->SetCursor(CursorType_IBeam); cursorWasSet = 1; }
	
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
			if (i >= Key::A && i <= Key::Z && !HasFlag(UIInputTextFlags_Numerical)) {
				if (DeshInput->capsLock || DeshInput->ShiftDown())
					insert(toPlace, ins);
				else
					insert(toPlace + 32, ins);
			}
			else if (i >= Key::K0 && i <= Key::K9) {
				if (DeshInput->ShiftDown() && !HasFlag(UIInputTextFlags_Numerical)) {
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
				if (DeshInput->ShiftDown() && !HasFlag(UIInputTextFlags_Numerical)) {
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
					if (HasFlag(UIInputTextFlags_Numerical) && KeyStringsLiteral[i] == '.') {
						data.character = '.';
						insert('.', ins);
					}
					else if(!HasFlag(UIInputTextFlags_Numerical)) {
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
		vec2((dim.x - charCount * style.font->max_width) * style.inputTextTextAlign.x,
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
		drawCmd.position = textStart + vec2(state->cursor * style.font->max_width * style.fontHeight / style.font->aspect_ratio / style.font->max_width, 0);
		drawCmd.position2 = textStart + vec2(state->cursor * style.font->max_width * style.fontHeight / style.font->aspect_ratio / style.font->max_width, style.fontHeight - 1);
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
	pos += curwin->position - curwin->scroll;
	
	return InputTextCall(label, buffer, buffSize, pos, nullptr, flags, 0);
}

b32 UI::InputText(const char* label, char* buffer, u32 buffSize, vec2 pos, UIInputTextCallback callback, UIInputTextFlags flags) {
	pos += curwin->position - curwin->scroll;
	
	return InputTextCall(label, buffer, buffSize, pos, callback, flags, 0);
}

b32 UI::InputText(const char* label, char* buffer, u32 buffSize, vec2 pos, UIInputTextState*& getInputTextState, UIInputTextFlags flags) {
	pos += curwin->position - curwin->scroll;
	
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
	layerStack.add(currlayer);
	currlayer = layer;
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
		currlayer = *layerStack.last;
		layerStack.pop();
	}
}


//@Windows


//window input helper funcs 

//this function is checked in UI::Update, while the other 3 are checked per window
void CheckWindowsForFocusInputs() {
	for (int i = windows.count - 1; i > 0; i--) {
		UIWindow* w = *windows.atIdx(i);
		w->focused = 0;
		if (!(w->flags & UIWindowFlags_NoFocus)) {
			if (i == windows.count - 1 && w->hovered) {
				w->focused = 1;
				break;
			}
			else if (w->hovered && ((w->flags & UIWindowFlags_FocusOnHover) ? 1 : DeshInput->LMousePressed())) {
				(*windows.data.last)->hovered = 0;
				w->focused = 1;
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

		b32& latch = window->latch;
		static vec2 vl1, vl2, vl3;

	
		b32 mpres = DeshInput->LMousePressed();
		b32 mdown = DeshInput->LMouseDown();
		b32 mrele = DeshInput->LMouseReleased();
		
		//get which side is active
		WinActiveSide& activeSide = window->activeSide;
		constexpr f32 boundrysize = 2;
		
		if (!mdown) {
			if (Math::PointInRectangle(mp, window->position.yAdd(-boundrysize), vec2(window->width, boundrysize)))
				activeSide = wTop;
			else if (Math::PointInRectangle(mp, window->position.yAdd(window->height), vec2(window->width, boundrysize)))
				activeSide = wBottom;
			else if (Math::PointInRectangle(mp, window->position, vec2(boundrysize, window->height)))
				activeSide = wLeft;
			else if (Math::PointInRectangle(mp, window->position.xAdd(window->width), vec2(boundrysize, window->height)))
				activeSide = wRight;
			else activeSide = wNone;
		}
		
		if (mpres && !latch && activeSide != wNone) {
			window->latch = 1;
			vl1 = mp;
			vl2 = window->dimensions;
			vl3 = window->scroll;
			SetResizingInput;
		}

		if (mrele) {
			latch = 0;
			AllowInputs;
		}
		
		switch (activeSide) {
			case wTop: {
				DeshWindow->SetCursor(CursorType_VResize); cursorWasSet = 1;
				if (mdown) {
					window->position.y = mp.y;
					window->dimensions = vl2.yAdd(vl1.y - mp.y);
					window->scy = Clamp(window->scy, 0.f, window->maxScroll.y);
				}
			}break;
			case wBottom: {
				DeshWindow->SetCursor(CursorType_VResize); cursorWasSet = 1;
				if (mdown) {
					window->dimensions = vl2.yAdd(mp.y - vl1.y);
					window->scy = Clamp(window->scy, 0.f, window->maxScroll.y);
				}
			}break;
			case wLeft: {
				DeshWindow->SetCursor(CursorType_HResize); cursorWasSet = 1;
				if (mdown) {
					window->position.x = mp.x;
					window->dimensions = vl2.xAdd(vl1.x - mp.x);
					window->scx = Clamp(window->scx, 0.f, window->maxScroll.x);
				}
			}break;
			case wRight: {
				DeshWindow->SetCursor(CursorType_HResize); cursorWasSet = 1;
				if (mdown) {
					window->dimensions = vl2.xAdd(mp.x - vl1.x);
					window->scx = Clamp(window->scx, 0.f, window->maxScroll.x);
				}
			}break;
			case wNone:break;
		}
	}
}

//TODO(sushi) PLEASE clean this shit up
void CheckWindowForScrollingInputs(UIWindow* window, b32 fromChild = 0) {
	//mouse wheel inputs
	//if this is a child window and it cant scroll, redirect the scrolling inputs to the parent
	if (window->parent && window->hovered && window->maxScroll.x == 0 && window->maxScroll.y == 0) {
		CheckWindowForScrollingInputs(window->parent, 1);
		return;
	}
	if ((window->hovered || fromChild) && DeshInput->ScrollUp()) {
		window->scy -= style.scrollAmount.y;
		window->scy = Math::clamp(window->scy, 0.f, window->maxScroll.y);
		return;
	}
	if ((window->hovered || fromChild) && DeshInput->ScrollDown()) {
		window->scy += style.scrollAmount.y;
		window->scy = Math::clamp(window->scy, 0.f, window->maxScroll.y);
		return;
	}

	if (CanTakeInput || WinScrolling) {
		static b32 vscroll = 0, hscroll = 0;
		static vec2 offset;
		static b32 initial = true;
		u32 flags = window->flags;

		b32 mdown = DeshInput->LMouseDown();
		b32 mrele = DeshInput->LMouseReleased();

		if (!hscroll && !HasFlag(UIWindowFlags_NoScrollY)) {
			f32 scrollbarheight = (window->dimensions.x < window->minSizeForFit.x ? window->height - style.scrollBarXHeight : window->height);
			f32 draggerheight = scrollbarheight * scrollbarheight / window->minSizeForFit.y;
			vec2 draggerpos(window->dimensions.x - style.scrollBarYWidth, (scrollbarheight - draggerheight) * window->scy / window->maxScroll.y);

			b32 scbgactive = window->hovered && Math::PointInRectangle(DeshInput->mousePos,
				window->position.xAdd(window->dimensions.x - style.scrollBarYWidth),
				vec2(style.scrollBarYWidth, scrollbarheight));

			b32 scdractive = window->hovered && Math::PointInRectangle(DeshInput->mousePos,
				draggerpos + window->position,
				vec2(style.scrollBarYWidth, draggerheight));

			
			if (scdractive && DeshInput->LMouseDown() || !initial) {
				if (initial) {
					offset = draggerpos - DeshInput->mousePos;
					initial = false;
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
				window->beingScrolled = 0;
				inputupon = 0;
				AllowInputs;
			}

		}
		if (!vscroll && !HasFlag(UIWindowFlags_NoScrollX)) {
			f32 scrollbarwidth = (window->dimensions.y < window->minSizeForFit.y ? window->width - style.scrollBarYWidth : window->width);
			f32 draggerwidth = scrollbarwidth * scrollbarwidth / window->minSizeForFit.x;
			vec2 draggerpos((scrollbarwidth - draggerwidth) * window->scx / window->maxScroll.x, window->dimensions.y - style.scrollBarXHeight);

			b32 scbgactive = window->hovered && Math::PointInRectangle(DeshInput->mousePos,
				window->position.yAdd(scrollbarwidth - style.scrollBarXHeight),
				vec2(scrollbarwidth, style.scrollBarXHeight));

			b32 scdractive = window->hovered && Math::PointInRectangle(DeshInput->mousePos,
				draggerpos + window->position,
				vec2(draggerwidth, style.scrollBarXHeight));

			if (scdractive && DeshInput->LMouseDown() || !initial) {
				if (initial) {
					offset = draggerpos - DeshInput->mousePos;
					initial = false;
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
				window->beingScrolled = 0;
				AllowInputs;
			}
		}
	}
}

void CheckWindowForDragInputs(UIWindow* window, b32 fromChild = 0) {
	if (CanTakeInput || WinDragging) { //drag
		//if this is a child window check the uppermost parent instead
		if (window->parent && window->hovered) { 
			CheckWindowForDragInputs(window->parent, 1); 
			return;
		}
		
		b32& beingDragged = window->beingDragged;
		static vec2 mouseOffset = vec2(0, 0);

		if (
			!(window->flags & UIWindowFlags_NoMove) &&
			(window->hovered || fromChild) &&
			DeshInput->KeyPressed(MouseButton::LEFT)) {
			SetDraggingInput;
			mouseOffset = window->position - DeshInput->mousePos;
			beingDragged = true;
		}
		if (beingDragged) {
			window->position = DeshInput->mousePos + mouseOffset;
		}
		if (DeshInput->KeyReleased(MouseButton::LEFT)) {
			beingDragged = false;
			AllowInputs;
		}
	}
}



//begins a window with a name, position, and dimensions along with some optional flags
//if begin window is called with a name that was already called before it will work with
//the data that window previously had
TIMER_START(wincreate);
void UI::Begin(const char* name, vec2 pos, vec2 dimensions, UIWindowFlags flags) {
	Assert(!rowInProgress, "Attempted to begin a window with a Row in progress! (Did you forget to call EndRow()?");
	TIMER_RESET(wincreate);
	//save previous window on stack
	windowStack.add(curwin);
	
	//check if were making a new window or working with one we already know
	if (windows.has(name)) {
		curwin = windows[name];
		curwin->cursor = vec2(0, 0);
		if (NextWinPos.x != -1) curwin->position = NextWinPos;
		if (NextWinSize.x != -1) curwin->dimensions = NextWinSize;
		NextWinPos = vec2(-1, 0); NextWinSize = vec2(-1, 0);
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
}

void UI::BeginChild(const char* name, vec2 dimensions, UIWindowFlags flags) {
	windowStack.add(curwin);
	
	UIWindow* parent = curwin;
	
	UIItem* item = BeginItem(UIItemType_ChildWin);
	
	//TODO(sushi) add custom positioning for child windows
	item->position = PositionForNewItem();
	
	//check if were making a new child or working with one we already know
	if (parent->children.has(name)) {
		item->size = parent->children[name]->dimensions;
		AdvanceCursor(item);
		
		curwin = parent->children[name];
		curwin->cursor = vec2(0, 0);
		if (NextWinPos.x != -1) curwin->position = NextWinPos;
		if (NextWinSize.x != -1) curwin->dimensions = NextWinSize;
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
		
		parent->children.add(name, curwin);
	}
	
	indentStack.add(0);
	item->child = curwin;
	curwin->parent = parent;
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
			max.x = Max(max.x, (item.position.x + curwin->scx) + item.size.x);
			max.y = Max(max.y, (item.position.y + curwin->scy) + item.size.y);
		}
	}
	return max + style.windowPadding;
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

void UI::End() {
	Assert(windowStack.size() > 1, "Attempted to end the base window");
	Assert(!rowInProgress, "Attempted to end a window with a Row in progress! (Did you forget to call EndRow()?");
	
	UIItem* preitem = BeginItem(UIItemType_PreItems);
	UIItem* postitem = BeginItem(UIItemType_PostItems);
	
	preitem->position = vec2::ZERO;
	postitem->position = vec2::ZERO;
	
	vec2 mp = DeshInput->mousePos;
	
	curwin->minSizeForFit = CalcWindowMinSize();
	vec2 minSizeForFit = curwin->minSizeForFit;
	
	if (WinHasFlag(UIWindowFlags_FitAllElements)) 
		curwin->dimensions = minSizeForFit;

	b32 yCanScroll = curwin->dimensions.y < minSizeForFit.y;
	b32 xCanScroll = curwin->dimensions.x < minSizeForFit.x;


	//check if window is hovered, or if its children are hovered
	if (Math::PointInRectangle(mp, curwin->position, curwin->dimensions * style.globalScale)) {
		curwin->hovered = 1;
		for (UIWindow* c : curwin->children) {
			vec2 scrollBarAdjust = vec2((yCanScroll ? style.scrollBarYWidth : 0), (xCanScroll ? style.scrollBarXHeight : 0));
			vec2 visRegionStart = vec2(Max(curwin->x, c->x), Max(curwin->y, c->y));
			vec2 visRegionEnd = vec2(Min(curwin->x + curwin->width - (yCanScroll ? style.scrollBarYWidth : 0), c->x + c->width), Min(curwin->y + curwin->height - (xCanScroll ? style.scrollBarXHeight : 0), c->y + c->height));
			vec2 childVisibleRegion = visRegionEnd - visRegionStart;
			DebugCircle(visRegionStart, 3);
			c->visibleRegionStart = visRegionStart;
			c->visibleRegionSize = childVisibleRegion;
			if (Math::PointInRectangle(mp, visRegionStart, childVisibleRegion * style.globalScale)) {
				
				curwin->hovered = 0;
				break;
			}
		}
	} else curwin->hovered = 0;

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
		curwin->maxScroll.y = minSizeForFit.y - curwin->dimensions.y + (curwin->dimensions.x < minSizeForFit.x ? style.scrollBarXHeight : 0);
		if (!WinHasFlag(UIWindowFlags_NoScrollBarY)) {
			f32 scrollbarheight = (curwin->dimensions.x < minSizeForFit.x ? curwin->height - style.scrollBarXHeight : curwin->height);
			f32 draggerheight = scrollbarheight * scrollbarheight / minSizeForFit.y;
			vec2 draggerpos(curwin->dimensions.x - style.scrollBarYWidth, (scrollbarheight - draggerheight) * curwin->scy / curwin->maxScroll.y);

			b32 scbgactive = Math::PointInRectangle(DeshInput->mousePos,
				curwin->position.xAdd(curwin->dimensions.x - style.scrollBarYWidth),
				vec2(style.scrollBarYWidth, scrollbarheight));

			b32 scdractive = Math::PointInRectangle(DeshInput->mousePos,
				draggerpos + curwin->position,
				vec2(style.scrollBarYWidth, draggerheight));

			{//scroll bg
				UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
				drawCmd.color = style.colors[UIStyleCol_ScrollBarBg]; //TODO(sushi) add active/hovered scrollbarbg colors
				drawCmd.position = vec2(curwin->dimensions.x - style.scrollBarYWidth, 0);
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
			if (curwin->dimensions.x < minSizeForFit.x) {
				UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
				drawCmd.color = style.colors[UIStyleCol_WindowBg];
				drawCmd.position = vec2(curwin->dimensions.x - style.scrollBarYWidth, scrollbarheight);
				drawCmd.dimensions = vec2(style.scrollBarYWidth, style.scrollBarXHeight);
				postitem->drawCmds.add(drawCmd);
			}
		}
	}
	else curwin->maxScroll.y = 0;


	//do the same but for x
	if (!WinHasFlag(UIWindowFlags_NoScrollX) && curwin->dimensions.x < minSizeForFit.x) {
		curwin->maxScroll.x = minSizeForFit.x - curwin->dimensions.x + (yCanScroll ? style.scrollBarYWidth : 0);
		if (!WinHasFlag(UIWindowFlags_NoScrollBarX)) {
			f32 scrollbarwidth = (yCanScroll ? curwin->width - style.scrollBarYWidth : curwin->width);
			f32 draggerwidth = scrollbarwidth * curwin->dimensions.x / minSizeForFit.x;
			vec2 draggerpos((scrollbarwidth - draggerwidth) * curwin->scx / curwin->maxScroll.x, curwin->dimensions.y - style.scrollBarXHeight);

			b32 scbgactive = Math::PointInRectangle(DeshInput->mousePos,
				curwin->position.yAdd(scrollbarwidth - style.scrollBarXHeight),
				vec2(scrollbarwidth, style.scrollBarXHeight));

			b32 scdractive = Math::PointInRectangle(DeshInput->mousePos,
				draggerpos + curwin->position,
				vec2(draggerwidth, style.scrollBarXHeight));

			{//scroll bg
				UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
				drawCmd.color = style.colors[UIStyleCol_ScrollBarBg]; //TODO(sushi) add active/hovered scrollbarbg colors
				drawCmd.position = vec2(0, curwin->dimensions.y - style.scrollBarXHeight);
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
		if (!WinHasFlag(UIWindowFlags_NoBackground) && !curwin->minimized) {
			UIDrawCmd drawCmd{ UIDrawType_FilledRectangle}; 
			drawCmd.position = vec2::ZERO;
			drawCmd.dimensions = curwin->dimensions;
			drawCmd.color = style.colors[UIStyleCol_WindowBg];
			
			preitem->drawCmds.add(drawCmd); 
		}
		
		//draw border
		if (!WinHasFlag(UIWindowFlags_NoBorder) && !curwin->minimized) {
			UIDrawCmd drawCmd{ UIDrawType_Rectangle}; //inst 58
			drawCmd.color = style.colors[UIStyleCol_Border];
			drawCmd.position = vec2::ONE;
			drawCmd.dimensions = curwin->dimensions - vec2::ONE;
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

void UI::EndChild() {
	Assert(windowStack.size() > 1, "Attempted to end the base window");
	Assert(!rowInProgress, "Attempted to end a window with a Row in progress! (Did you forget to call EndRow()?)");
	
	UIItem* preitem = BeginItem(UIItemType_PreItems);
	UIItem* postitem = BeginItem(UIItemType_PostItems);
	
	vec2 mp = DeshInput->mousePos;
	
	vec2 minSizeForFit = CalcWindowMinSize();
	curwin->minSizeForFit = minSizeForFit;

	if (WinHasFlag(UIWindowFlags_FitAllElements))
		curwin->dimensions = minSizeForFit;

	b32 yCanScroll = curwin->dimensions.y < minSizeForFit.y;
	b32 xCanScroll = curwin->dimensions.x < minSizeForFit.x;

	//check if window is hovered, or if its children are hovered
	if (Math::PointInRectangle(mp, curwin->visibleRegionStart, curwin->visibleRegionSize * style.globalScale)) {
		
		DebugRect(curwin->visibleRegionStart, curwin->visibleRegionSize);

		curwin->hovered = 1;
		for (UIWindow* c : curwin->children) {
			vec2 scrollBarAdjust = vec2((yCanScroll ? style.scrollBarYWidth : 0), (xCanScroll ? style.scrollBarXHeight : 0));
			vec2 visRegionStart = vec2(Max(curwin->x, c->x), Max(curwin->y, c->y));
			vec2 visRegionEnd = vec2(Min(curwin->x + curwin->width, c->x + c->width), Min(curwin->y + curwin->height, c->y + c->height)) - scrollBarAdjust;
			vec2 childVisibleRegion = visRegionEnd - visRegionStart;
			c->visibleRegionStart = visRegionStart;
			c->visibleRegionSize = childVisibleRegion;
			if (Math::PointInRectangle(mp, visRegionStart, childVisibleRegion * style.globalScale)) {
				curwin->hovered = 0;
				break;
			}
		}
	}
	else curwin->hovered = 0;

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
		curwin->maxScroll.y = minSizeForFit.y - curwin->dimensions.y + (curwin->dimensions.x < minSizeForFit.x ? style.scrollBarXHeight : 0);
		if (!WinHasFlag(UIWindowFlags_NoScrollBarY)) {
			f32 scrollbarheight = (curwin->dimensions.x < minSizeForFit.x ? curwin->height - style.scrollBarXHeight : curwin->height);
			f32 draggerheight = scrollbarheight * scrollbarheight / minSizeForFit.y;
			vec2 draggerpos(curwin->dimensions.x - style.scrollBarYWidth, (scrollbarheight - draggerheight) * curwin->scy / curwin->maxScroll.y);

			b32 scbgactive = Math::PointInRectangle(DeshInput->mousePos,
				curwin->position.xAdd(curwin->dimensions.x - style.scrollBarYWidth),
				vec2(style.scrollBarYWidth, scrollbarheight));

			b32 scdractive = Math::PointInRectangle(DeshInput->mousePos,
				draggerpos + curwin->position,
				vec2(style.scrollBarYWidth, draggerheight));

			{//scroll bg
				UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
				drawCmd.color = style.colors[UIStyleCol_ScrollBarBg]; //TODO(sushi) add active/hovered scrollbarbg colors
				drawCmd.position = vec2(curwin->dimensions.x - style.scrollBarYWidth, 0);
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
			if (curwin->dimensions.x < minSizeForFit.x) {
				UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
				drawCmd.color = style.colors[UIStyleCol_WindowBg];
				drawCmd.position = vec2(curwin->dimensions.x - style.scrollBarYWidth, scrollbarheight);
				drawCmd.dimensions = vec2(style.scrollBarYWidth, style.scrollBarXHeight);
				postitem->drawCmds.add(drawCmd);
			}
		}
	}
	else curwin->maxScroll.y = 0;


	//do the same but for x
	if (!WinHasFlag(UIWindowFlags_NoScrollX) && curwin->dimensions.x < minSizeForFit.x) {
		curwin->maxScroll.x = minSizeForFit.x - curwin->dimensions.x + (curwin->dimensions.y < minSizeForFit.y ? style.scrollBarYWidth : 0);
		if (!WinHasFlag(UIWindowFlags_NoScrollBarX)) {
			f32 scrollbarwidth = (curwin->dimensions.y < minSizeForFit.y ? curwin->width - style.scrollBarYWidth : curwin->width);
			f32 draggerwidth = scrollbarwidth * curwin->dimensions.x / minSizeForFit.x;
			vec2 draggerpos((scrollbarwidth - draggerwidth) * curwin->scx / curwin->maxScroll.x, curwin->dimensions.y - style.scrollBarXHeight);

			b32 scbgactive = Math::PointInRectangle(DeshInput->mousePos,
				curwin->position.yAdd(scrollbarwidth - style.scrollBarXHeight),
				vec2(scrollbarwidth, style.scrollBarXHeight));

			b32 scdractive = Math::PointInRectangle(DeshInput->mousePos,
				draggerpos + curwin->position,
				vec2(draggerwidth, style.scrollBarXHeight));

			{//scroll bg
				UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
				drawCmd.color = style.colors[UIStyleCol_ScrollBarBg]; //TODO(sushi) add active/hovered scrollbarbg colors
				drawCmd.position = vec2(0, curwin->dimensions.y - style.scrollBarXHeight);
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
		if (!(curwin->flags & UIWindowFlags_NoBackground) && !curwin->minimized) {
			UIDrawCmd drawCmd{ UIDrawType_FilledRectangle};
			drawCmd.position = vec2::ZERO;
			drawCmd.dimensions = curwin->dimensions;
			drawCmd.color = style.colors[UIStyleCol_WindowBg];
			
			preitem->drawCmds.add(drawCmd);
		}
		
		//draw border
		if (!WinHasFlag(UIWindowFlags_NoBorder) && !curwin->minimized) {
			UIDrawCmd drawCmd{ UIDrawType_Rectangle}; //inst 58
			drawCmd.color = style.colors[UIStyleCol_Border];
			drawCmd.position = vec2::ONE;
			drawCmd.dimensions = curwin->dimensions - vec2::ONE;
			postitem->drawCmds.add(drawCmd);
		}
	}
	
	curwin->style = style;
	
	NextWinPos = vec2(-1, 0); NextWinSize = vec2(-1, 0);
	
	indentStack.pop();

	//update stored window with new window state
	curwin = *windowStack.last;
	windowStack.pop();
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

void UI::SetWindowName(const char* name) {
	curwin->name = name;
}

b32 UI::IsWinHovered() {
	return curwin->hovered;
}

b32 UI::AnyWinHovered() {
	return globalHovered || inputState != ISNone;
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
		if (!(win->name == "METRICS")) {
			if (win->render_time > slomo->render_time)     slomo = win;
			if (win->render_time < quick->render_time)     quick = win;
			if (win->items_count > mostitems->items_count) mostitems = win;
			if (win->name.count > longname->name.count)   longname = win;
			winsorted.add(win);
		}
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

		Text(str1);            Text(toStr(globalHovered).str);
		Text("input state: ");
		switch (inputState) {
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
		if (BeginHeader("Window Vars")) {
			BeginRow(2, style.fontHeight * 1.2);
			RowSetupColumnWidths({ CalcTextSize("Max Scroll: ").x , 10 });

			Text("Position: ");   Text(toStr(debugee->position).str);
			Text("Dimensions: "); Text(toStr(debugee->dimensions).str);
			Text("Scroll: ");     Text(toStr(debugee->scroll).str);
			Text("Max Scroll: "); Text(toStr(debugee->maxScroll).str);
			Text("Hovered: ");    Text(toStr(debugee->hovered).str);
			Text("Focused: ");    Text(toStr(debugee->focused).str);

			EndRow();

			if (BeginHeader("Items")) {
				BeginChild("METRICSItems", vec2(curwin->dimensions.x - style.windowPadding.x * 2, 300)); {
					forI(UI_WINDOW_ITEM_LAYERS) {
						for (UIItem& item : debugee->items[i]) {
							Text(UIItemTypeStrs[item.type]);
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

			EndHeader();
		}

		static b32 showItemBoxes = false;
		static b32 showItemCursors = false;
		
		if (BeginHeader("Window Debug Visuals")) {
			Checkbox("Show Item Boxes", &showItemBoxes);
			Checkbox("Show Item Cursors", &showItemCursors);

			EndHeader();
		}

		if (showItemBoxes) {
			forI(UI_WINDOW_ITEM_LAYERS) {
				for (UIItem& item : debugee->items[i]) {
					{
						UIDrawCmd dc{ UIDrawType_Rectangle };
						dc.color = Color_Red;
						dc.position = debugee->position + item.position;
						dc.dimensions = item.size;
						debugCmds.add(dc);
					}
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

	//Text("heres some long text to test if this shit will scroll right or not", UITextFlags_NoWrap);

	//BeginRow(3, tex->height);
	//RowSetupRelativeColumnWidths({ 1,1,1 });
	//forI(9) {
	//	Image(tex);
	//}
	//EndRow();

	if (BeginHeader("Text")) {
		Text("heres some text");

		Separator(7);

		Text("heres some long text that should wrap if it reaches the end of the window");

		Separator(7);

		Text("heres some long text that shouldn't wrap when it reaches the end of the window", UITextFlags_NoWrap);

		EndHeader();
	}

	Separator(11);

	if (BeginHeader("Button")) {
		static string str = "heres some text";
		if (Button("change text")) {
			str = "heres some new text, because you pressed the button";
		}
		Text(str.str);

		Separator(7);

		static color col = color(55, 45, 66);
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

		static string str2 = "this will change on button release";
		if (Button("true on release", UIButtonFlags_ReturnTrueOnRelease)) {
			str2 = "this was changed on release";
		}
		Text(str2.str);


		EndHeader();
	}

	Separator(11);

	if (BeginHeader("Slider")) {
		static f32 sl1 = 0;

		Slider("slider1", &sl1, 0, 100); SameLine(); Text(toStr(sl1).str);

		EndHeader();
	}

	Separator(11);

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

	Separator(11);

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

		static f32 rowyalign = 0.5;
		static f32 rowxalign = 0.5;

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



		Text("Rows also allow you to use either static or relative column widths");

		Separator(7);

		static f32 scw1 = 60;
		static f32 scw2 = 60;
		static f32 scw3 = 60;

		static f32 dcw1 = 1;
		static f32 dcw2 = 1;
		static f32 dcw3 = 1;

		static u32 selected = 0;
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

				Slider("scw1", &scw1, 0, 90); SameLine(); Text(toStr(scw1).str);
				Slider("scw2", &scw2, 0, 90); SameLine(); Text(toStr(scw2).str);
				Slider("scw3", &scw3, 0, 90); SameLine(); Text(toStr(scw3).str);
			}break;
			case 1: {
				BeginRow(3, 16);
				RowSetupRelativeColumnWidths({ dcw1, dcw2, dcw3 });
				Text("text");
				Text("long text");
				Text("text");
				EndRow();

				Slider("dcw1", &dcw1, 1, 5); SameLine(); Text(toStr(dcw1).str);
				Slider("dcw2", &dcw2, 1, 5); SameLine(); Text(toStr(dcw2).str);
				Slider("dcw3", &dcw3, 1, 5); SameLine(); Text(toStr(dcw3).str);
			}break;
		}


		EndHeader();
	}

	if (BeginHeader("Child Windows")) {
		Text("You can nest windows inside another one by using BeginChild");

		BeginChild("demochild", vec2(curwin->width, 300));

		Text("Heres some text in the child window");
		
		Separator(7);

		Text("Child windows have all the same functionality of base windows, save for a few TODOS");

		Separator(7);

		static Texture* tex = Storage::CreateTextureFromFile("lcdpix.png").second;

		Text("heres a image in the child window:");
		Image(tex);

		EndChild();

		EndHeader();

	}

	End();
}


//initializes core UI with an invisible working window covering the entire screen
//also initializes styles
//the base window should never focus when clicking within it, so any widgets drawn within
//it will not focus if theres a window in front of them.
//I'm not sure how i want to fix it yet
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
	//style.font = Storage::CreateFontFromFileTTF("gohufont-11.ttf", 11).second;
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
	PushVar(UIStyleVar_WindowBorderSize,         1);
	PushVar(UIStyleVar_TitleBarHeight,           style.fontHeight * 1.2);
	PushVar(UIStyleVar_TitleTextAlign,           vec2(1, 0.5));
	PushVar(UIStyleVar_WindowPadding,            vec2(10, 10));
	PushVar(UIStyleVar_ItemSpacing,              vec2(1, 1));
	PushVar(UIStyleVar_ScrollAmount,             vec2(5, 5));
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
	windowStack.add(curwin);
	
	Log("deshi","Finished UI initialization in ",TIMER_END(t_s),"ms");
}


inline void DrawCmd(UIDrawCmd& drawCmd, UIItem& item, vec2 itempos, vec2 itemsiz, vec2 winpos, vec2 winsiz, vec2 winScissorOffset, vec2 winScissorExtent, vec2 winParentScissorOffset, vec2 winParentScissorExtent, u32 winlayer) {
	vec2   dcpos = itempos + drawCmd.position * item.style.globalScale;
	vec2  dcpos2 = itempos + drawCmd.position2 * item.style.globalScale;
	vec2   dcsiz = drawCmd.dimensions * item.style.globalScale;
	vec2    dcse = (drawCmd.useWindowScissor ? winScissorExtent : drawCmd.scissorExtent * item.style.globalScale);
	vec2    dcso = (drawCmd.useWindowScissor ? winScissorOffset : itempos + drawCmd.scissorOffset);
	f32      dct = drawCmd.thickness;
	u32      dcl = winlayer;
	u32    dcsub = drawCmd.subdivisions;
	color  dccol = drawCmd.color;

	dcpos.x = floor(dcpos.x); dcpos.y = floor(dcpos.y);
	dcpos2.x = floor(dcpos2.x); dcpos2.y = floor(dcpos2.y);

	if (!winParentScissorExtent.x) {
		dcso.x = Max(winpos.x, dcso.x); dcso.y = Max(winpos.y, dcso.y); //force all items to stay within their windows
		dcso.x = Min(winpos.x + winScissorExtent.x - dcse.x, dcso.x); dcso.y = Min(winpos.y + winScissorExtent.y - dcse.y, dcso.y);
		if (drawCmd.useWindowScissor && winpos.x < 0) dcse.x += winpos.x; //if the window's pos goes negative, the scissor extent needs to adjust itself
		if (drawCmd.useWindowScissor && winpos.y < 0) dcse.y += winpos.y;
		dcse.x = Max(0.f, dcse.x); dcse.y = Max(0.f, dcse.y);
		dcso.x = Max(0.0f, dcso.x); dcso.y = Max(0.0f, dcso.y); //NOTE scissor offset cant be negative
	}
	else {
		//if this is a child window we must make sure it stays within its parents scissors
		dcso.x = Max(Max(winParentScissorOffset.x, winpos.x), dcso.x); 
		dcso.y = Max(Max(winParentScissorOffset.y, winpos.y), dcso.y); //force all items to stay within their windows
		dcso.x = Min(winParentScissorOffset.x + winParentScissorExtent.x - dcse.x, dcso.x); 
		dcso.y = Min(winParentScissorOffset.y + winParentScissorExtent.y - dcse.y, dcso.y);
		if (drawCmd.useWindowScissor && winpos.x - winParentScissorOffset.x < 0) dcse.x += winpos.x - winParentScissorOffset.x; //if the window's pos goes negative, the scissor extent needs to adjust itself
		if (drawCmd.useWindowScissor && winpos.y - winParentScissorOffset.y < 0) dcse.y += winpos.y - winParentScissorOffset.y;
		dcse.x = Max(0.f, dcse.x); dcse.y = Max(0.f, dcse.y);
		dcso.x = Max(0.0f, dcso.x); dcso.y = Max(0.0f, dcso.y); //NOTE scissor offset cant be negative
	}
	Texture* dctex = drawCmd.tex;

	cstring dctext{ drawCmd.text.str,drawCmd.text.count };
	wcstring wdctext{ drawCmd.wtext.str, drawCmd.wtext.count };

	Font* font = drawCmd.font;

	switch (drawCmd.type) {
		case UIDrawType_FilledRectangle: {
			Render::FillRect2D(dcpos, dcsiz, dccol, dcl, dcso, dcse);
		}break;
		case UIDrawType_Rectangle: {
			Render::DrawRect2D(dcpos, dcsiz, dccol, dcl, dcso, dcse);
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

inline void DrawWindow(UIWindow* p, UIWindow* parent = 0) {
	TIMER_START(winren);
	vec2 winPos;
	vec2 winSiz;
	vec2 winScissorOffset;
	vec2 winScissorExtent;
	vec2 localWinPos; //for children

	if (parent) {
		f32 xch = (parent->x + parent->width) - p->x;
		f32 ych = (parent->y + parent->height) - p->y;
		winPos = vec2(p->x, p->y + p->titleBarHeight);
		winSiz = vec2(p->width, p->height - p->titleBarHeight) * p->style.globalScale;
		f32 extentClipx = Clamp(winPos.x - parent->position.x, winPos.x - parent->position.x, 0.f);
		f32 extentClipy = Clamp(winPos.y - parent->position.y, winPos.y - parent->position.y, 0.f);
		winScissorOffset = { Max(parent->position.x,winPos.x), Max(parent->position.y, winPos.y) }; //NOTE scissor offset cant be negative
		winScissorExtent = { Min(winSiz.x, Clamp(xch, 0, xch) + extentClipx), Min(winSiz.y, Clamp(ych, 0, ych)+extentClipy) };
	
	}
	else {
		winPos = vec2(p->x, p->y + p->titleBarHeight);
		winSiz = vec2(p->width, p->height - p->titleBarHeight) * p->style.globalScale;
		winScissorOffset = { Max(0.0f,winPos.x), Max(0.0f, winPos.y) }; //NOTE scissor offset cant be negative
		winScissorExtent = winSiz;
	}

	//winscissor *= p->style.globalScale;

	if (p->hovered && !(p->flags & UIWindowFlags_DontSetGlobalHoverFlag))
		globalHovered = 1;

	//draw base cmds first
	for (UIItem& item : p->preItems) {
		vec2 itempos = winPos + item.position * item.style.globalScale;
		vec2 itemsiz = item.size;
		for (UIDrawCmd& drawCmd : item.drawCmds) {
			DrawCmd(drawCmd, item, itempos, itemsiz, winPos, winSiz, winScissorOffset, winScissorExtent, (parent ? parent->position : vec2::ZERO), (parent ? parent->dimensions : vec2::ZERO), p->windowlayer);
		}
	}

	//dont draw non-base draw cmds if we're minimized
	if (!p->minimized) {
		forI(UI_WINDOW_ITEM_LAYERS) {
			for (UIItem& item : p->items[i]) {
				if (item.type == UIItemType_ChildWin) {
					item.child->position = p->position + item.position * item.style.globalScale;
					DrawWindow(item.child, p);
					continue;
				}
				vec2 itempos = winPos + item.position * item.style.globalScale;
				vec2 itemsiz = item.size;
				for (UIDrawCmd& drawCmd : item.drawCmds) {
					DrawCmd(drawCmd, item, itempos, itemsiz, winPos, winSiz, winScissorOffset, winScissorExtent, (parent ? parent->position : vec2::ZERO), (parent ? parent->dimensions : vec2::ZERO), p->windowlayer);
				}
			}
		}

		//draw post items, such as scroll bars
		for (UIItem& item : p->postItems) {
			vec2 itempos = winPos + item.position * item.style.globalScale;
			vec2 itemsiz = item.size;
			for (UIDrawCmd& drawCmd : item.drawCmds) {
				DrawCmd(drawCmd, item, itempos, itemsiz, winPos, winSiz, winScissorOffset, winScissorExtent, (parent ? parent->position : vec2::ZERO), (parent ? parent->dimensions : vec2::ZERO), p->windowlayer);
			}
		}
	}

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
	Assert(windowStack.size() == 1, 
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

	globalHovered = 0;
	
	
	//windows input checking functions
	CheckWindowsForFocusInputs();

	 
	if (inputupon) CheckWindowForScrollingInputs(inputupon);
	if (inputupon) CheckWindowForResizingInputs(inputupon);
	if (inputupon) CheckWindowForDragInputs(inputupon);
	
	
	//reset cursor to default if no item decided to set it 
	if (!cursorWasSet) DeshWindow->SetCursor(CursorType_Arrow);
	cursorWasSet = 0;
	
	//draw windows in order 
	for (UIWindow* p : windows) {
		DrawWindow(p);
	}
	
	if (show_metrics) {
		DrawWindow(DisplayMetrics());
		show_metrics = 0;
	}

	//it should be safe to do this any time the mouse is released...
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
				Render::DrawRect2D(dcpos, dcsiz, dccol, dcl, dcso, dcse);
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
}
