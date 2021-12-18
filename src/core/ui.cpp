#include "ui.h"
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
local bool NextActive   = 0;

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

local array<UIDrawCmd> debugCmds; //debug draw cmds that are always drawn last

local u32 initColorStackSize;
local u32 initStyleStackSize;

//set if any window other than base is hovered
local bool globalHovered = false;
local bool draggingWin = false; //if a user moves their mouse too fast while dragging, the globalHover flag can be set to false

local u32 activeId = -1; //the id of an active widget eg. input text

//row variables
local UIRow row;
local bool  rowInProgress;

//set when we are in progress of making a custom item, see BeginCustomItem() in ui.h for more info on what that is
local b32 custom_item = 0;

local u32 currlayer = floor(UI_WINDOW_ITEM_LAYERS / 2.f);

//set when an item need to supress window dragging
local b32 drag_override = 0;

//misc state vars
local b32 combo_active = 0; //set when BeginCombo is called

//helper defines


#define HasFlag(flag) (flags & flag)
#define HasFlags(flag) ((flags & flag) != flag) //used on flag enums that are collections of flags
#define WinHasFlag(flag) (curwin->flags & flag)
#define WinHasFlags(flag) ((curwin->flags & flag) != flag) 
#define DrawCmdScreenPos(pos) pos + item->position + curwin->position 
#define ItemScreenPos item->position + curwin->position 


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

inline bool isItemHovered(UIItem* item) {
	return Math::PointInRectangle(DeshInput->mousePos, item->position + curwin->position, item->size);
}

inline bool isLocalAreaHovered(vec2 pos, vec2 size, UIItem* item) {
	return Math::PointInRectangle(DeshInput->mousePos, pos + item->position + curwin->position, size);
}

//internal master cursor controller
//  this is an attempt to centralize what happens at the end of each item function
// 
//  i expect this to fall through at some point, as not all items are created equal and may need to
//  have different things happen after its creation, which could be handled as special cases within
//  the function itself.
inline void AdvanceCursor(UIItem* itemmade, bool moveCursor = 1) {
	
	//if a row is in progress, we must reposition the item to conform to row style variables
	//this means that you MUST ensure that this happens before any interactions with the item are calculated
	//for instance in the case of Button, this must happen before you check that the user has clicked it!
	if (rowInProgress && !custom_item) {
		//abstract item types (lines, rectangles, etc.) are not row'd, for now
		if (itemmade->type != UIItemType_Abstract) {
			row.items.add(itemmade);
			
			f32 height = row.height;
			f32 width;
			//determine if the width is relative to the size of the item or not
			if (row.columnWidths[row.items.count - 1].second == true)
				width = itemmade->size.x * row.columnWidths[row.items.count - 1].first;
			else
				width = row.columnWidths[row.items.count - 1].first;
			
			itemmade->position.y = row.position.y + (height - itemmade->size.y) * itemmade->style.rowItemAlign.y;
			itemmade->position.x = row.position.x + row.xoffset + (width - itemmade->size.x) * itemmade->style.rowItemAlign.x;
			
			row.xoffset += width;
			
			//we dont need to handle moving the cursor here, because the final position of the cursor after a row is handled in EndRow()
		}
	}
	else if (moveCursor) curwin->cursor = vec2{ 0, itemmade->position.y + itemmade->size.y + style.itemSpacing.y - style.windowPadding.y } + curwin->scroll;
}

//function for getting the position of a new item based on style, so the long string of additions
//is centralized for new additions, if ever made, and so that i dont have to keep writing it :)
inline vec2 PositionForNewItem() {
	return curwin->cursor + (style.windowPadding - curwin->scroll);
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
	curwin->cursor.y = curwin->items[currlayer].last->initialCurPos.y;
	curwin->cursor.x += curwin->items[currlayer].last->size.x + style.itemSpacing.x;
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
	if (!custom_item) {
		if (type == UIItemType_Base) {
			curwin->baseItems.add(UIItem{ type, curwin->cursor, style });
			return curwin->baseItems.last;
		}
		else {
			curwin->items[currlayer + layeroffset].add(UIItem{ type, curwin->cursor, style });
		}
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
	Assert(row.items.count == row.columns, "Attempted to end a Row without giving the correct amount of items!");
	
	curwin->cursor = vec2{ 0, row.position.y + row.height + style.itemSpacing.y - style.windowPadding.y + curwin->scroll.y };
	
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
	drawCmd.position = start;
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
	drawCmd.position2 = vec2( subdivisions, 0 );
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
	drawCmd.position2 = vec2( subdivisions, 0 );
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
local void TextW(const char* in, vec2 pos, color color, bool nowrap, bool move_cursor = true) {
	
	using namespace UI;
	UIItem* item = BeginItem(UIItemType_Text);
	item->position = (move_cursor ? PositionForNewItem() : pos);
	
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
				u32 maxChars = floor(((curwin->width - 2 * style.windowPadding.x) - workcur.x) / style.font->max_width);
				
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
						
						t = t.substr(nustr.count);
						workcur.y += style.fontHeight + style.itemSpacing.y;
						
						//continue to wrap if we need to
						while (t.count > maxChars) {
							splitat = t.findLastChar(' ', maxChars);
							nustr = t.substr(0, (splitat == string::npos) ? maxChars - 1 : splitat);
							TextCall(nustr.str, workcur, color, item);
							
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
local void TextW(const wchar_t* in, vec2 pos, color color, bool nowrap, bool move_cursor = true) {
	
	using namespace UI;
	UIItem* item = BeginItem(UIItemType_Text);
	item->position = (move_cursor ? PositionForNewItem() : pos);
	
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
	TextW(text, curwin->cursor, style.colors[UIStyleCol_Text], HasFlag(UITextFlags_NoWrap));
}

void UI::Text(const char* text, vec2 pos, UITextFlags flags) {
	TextW(text, pos, style.colors[UIStyleCol_Text], HasFlag(UITextFlags_NoWrap), 0);
}

void UI::Text(const wchar_t* text, UITextFlags flags){
	TextW(text, curwin->cursor, style.colors[UIStyleCol_Text], HasFlag(UITextFlags_NoWrap));
}

void UI::Text(const wchar_t* text, vec2 pos, UITextFlags flags){
	TextW(text, pos, style.colors[UIStyleCol_Text], HasFlag(UITextFlags_NoWrap), 0);
}


void UI::TextF(const char* fmt, ...) {
	string s;
	va_list argptr;
	va_start(argptr, fmt);
	s.count  = vsnprintf(nullptr, 0, fmt, argptr);
	s.str   = (char*)malloc(s.count+1);
	s.space = s.count+1;
	vsnprintf(s.str, s.count+1, fmt, argptr);
	va_end(argptr);
	TextW(s.str, curwin->cursor, style.colors[UIStyleCol_Text], false);
}


bool ButtonCall(const char* text, vec2 pos, bool move_cursor = 1) {
	UIItem* item = BeginItem(UIItemType_Button);
	item->size = (NextItemSize.x != -1) ? NextItemSize : vec2(Min(curwin->width, 50.0f), style.fontHeight * style.buttonHeightRelToFont);
	item->position = pos;
	AdvanceCursor(item, move_cursor);
	
	bool active = Math::PointInRectangle(DeshInput->mousePos, curwin->position + item->position, item->size * style.globalScale);
	
	{//background
		UIDrawCmd drawCmd{ UIDrawType_FilledRectangle};
		drawCmd.position = vec2::ZERO;
		drawCmd.dimensions = item->size;
		drawCmd.color = style.colors[(active ? (DeshInput->LMouseDown() ? UIStyleCol_ButtonBgActive : UIStyleCol_ButtonBgHovered) : UIStyleCol_ButtonBg)];
		item->drawCmds.add(drawCmd);
	} 
	
	{//border
		UIDrawCmd drawCmd{ UIDrawType_Rectangle}; //inst 58
		drawCmd.color = style.colors[UIStyleCol_ButtonBorder];
		drawCmd.position = vec2::ZERO;
		drawCmd.dimensions = item->size;
		item->drawCmds.add(drawCmd);
	}
	
	{//text
		UIDrawCmd drawCmd{ UIDrawType_Text};
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
	
	//TODO(sushi) add a flag for prevent button presses when window is not focused
	if (/*curwin->focused &&*/ active && DeshInput->LMousePressed()) return true;
	else return false;
}

bool UI::Button(const char* text) {
	return ButtonCall(text, PositionForNewItem());
}

bool UI::Button(const char* text, vec2 pos) {
	return ButtonCall(text, pos, 0);
}

void UI::Checkbox(string label, bool* b) {
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
	
	b32 bgactive = isItemHovered(item);
	b32 fiactive = isLocalAreaHovered(fillpos, fillsiz, item);
	
	
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
	
	
	
	if (DeshInput->LMousePressed() && Math::PointInRectangle(DeshInput->mousePos, curwin->position + item->position * style.globalScale, boxsiz * style.globalScale))
		*b = !*b;
	
}

UIItem* comboItem = 0; //global, so endcombo can share this with begincombo, and so selectable can read how it should be placed 
bool UI::BeginCombo(const char* label, const char* prev_val, vec2 pos) {
	comboItem = BeginItem(UIItemType_Combo, 1);
	comboItem->position = pos;

	if (!combos.has(label)) {
		combos.add(label);
		combos[label] = false;
	}


	return combos[label];
}

bool UI::BeginCombo(const char* label, const char* prev_val) {
	return BeginCombo(label, prev_val, PositionForNewItem());
}



void UI::EndCombo() {
	Assert(comboItem, "attempt to end a combo without starting one");




}

bool SelectableCall(const char* label, vec2 pos, b32 selected) {
	UIItem* item = BeginItem(UIItemType_Selectable, 0);

	return 0;
}

bool UI::Selectable(const char* label, b32 selected) {

	return 0;
}

bool UI::Selectable(const char* label, vec2 pos, b32 selected) {

	return 0;
}

bool UI::Header(const char* label) {
	UIItem* item = BeginItem(UIItemType_Header);
	
	b32* open = 0;
	if (!headers.has(label)) {
		headers.add(label);
		headers[label] = false;
	}
	open = &headers[label];
	
	item->position = PositionForNewItem();
	item->size = (NextItemSize.x == -1 ?
				  vec2(curwin->width - style.windowPadding.x, style.fontHeight * style.headerHeightRelToFont) :
				  NextItemSize);
	
	AdvanceCursor(item);
	
	b32 active = isItemHovered(item);
	
	if (active && DeshInput->LMousePressed()) *open = !*open;
	
	f32 buttonrad = item->size.y / 4;
	
	vec2 bgpos = vec2{ buttonrad * 2 + 5, 0 };
	vec2 bgdim = vec2{ 
		item->size.x - bgpos.x - style.windowPadding.x, 
		item->size.y };
	
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
		drawCmd.position2 = vec2{ 30.f, 0 };
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
	
	b32 active = isItemHovered(item);
	
	
	if (active && DeshInput->LMousePressed()) {
		drag_override = 1;
		sliders[label] = 1;
	}
	if (being_moved && DeshInput->LMouseDown()) {
		f32 ratio = (DeshInput->mousePos.x - item->position.x - curwin->position.x) / item->size.x;
		*val = ratio * (val_max - val_min) + val_min;
	}
	if (DeshInput->LMouseReleased()) {
		drag_override = 0;
		sliders[label] = 0;
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



//@InputText


//final input text
bool InputTextCall(const char* label, char* buff, u32 buffSize, vec2 position, UIInputTextCallback callback, UIInputTextFlags flags, bool moveCursor) {
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
	
	b32 hovered = isItemHovered(item);
	
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
	
	b32 active = (activeId == state->id);
	if (NextActive || DeshInput->KeyPressed(MouseButton::LEFT)) {
		if (NextActive || Math::PointInRectangle(DeshInput->mousePos, curwin->position + GetLastItem()->position, dim)) {
			activeId = state->id;
			NextActive = 0;
		}
		else if (active) activeId = -1;
	}
	
	if (charCount < state->cursor)
		state->cursor = charCount;
	
	//data for callback function
	UIInputTextCallbackData data;
	data.flags = flags;
	data.buffer = buff;
	data.selectionStart = state->selectStart;
	data.selectionEnd = state->selectEnd;
	
	bool bufferChanged = 0;
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
		
		//
		//
		//
		//IMPORTANT TODO
		// replace this using glfw's Char callback function.
		//
		//
		//
		//
		
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

bool UI::InputText(const char* label, char* buffer, u32 buffSize, UIInputTextFlags flags) {
	vec2 position = PositionForNewItem();
	
	return InputTextCall(label, buffer, buffSize, position, nullptr, flags, 1);
}

bool UI::InputText(const char* label, char* buffer, u32 buffSize, UIInputTextCallback callback, UIInputTextFlags flags) {
	vec2 position = PositionForNewItem();
	
	return InputTextCall(label, buffer, buffSize, position, callback, flags, 1);
}

bool UI::InputText(const char* label, char* buffer, u32 buffSize, UIInputTextState*& getInputTextState, UIInputTextFlags flags) {
	vec2 position = PositionForNewItem();
	
	if (InputTextCall(label, buffer, buffSize, position, nullptr, flags, 1)) {
		getInputTextState = inputTexts.at(label);
		return true;
	}
	getInputTextState = inputTexts.at(label);
	return false;
}

bool UI::InputText(const char* label, char* buffer, u32 buffSize, vec2 pos, UIInputTextFlags flags) {
	pos += curwin->position - curwin->scroll;
	
	return InputTextCall(label, buffer, buffSize, pos, nullptr, flags, 0);
}

bool UI::InputText(const char* label, char* buffer, u32 buffSize, vec2 pos, UIInputTextCallback callback, UIInputTextFlags flags) {
	pos += curwin->position - curwin->scroll;
	
	return InputTextCall(label, buffer, buffSize, pos, callback, flags, 0);
}

bool UI::InputText(const char* label, char* buffer, u32 buffSize, vec2 pos, UIInputTextState*& getInputTextState, UIInputTextFlags flags) {
	pos += curwin->position - curwin->scroll;
	
	if (InputTextCall(label, buffer, buffSize, pos, nullptr, flags, 0)) {
		getInputTextState = inputTexts.at(label);
		return true;
	}
	getInputTextState = inputTexts.at(label);
	return false;
}


//@Utilities


void UI::BeginCustomItem(){
	//make the custom item, add it and set curitem to it
	curwin->items[currlayer].add(UIItem{ UIItemType_Custom, curwin->cursor, style });
	custom_item = 1;
}

void UI::EndCustomItem() {
	custom_item = 0;
	//we always calc the item's size here because it is arbitrary
	CalcItemSize(GetLastItem());
	AdvanceCursor(GetLastItem());
}

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
	
	//check if window is hovered
	vec2 mp = DeshInput->mousePos;
	if(Math::PointInRectangle(mp, curwin->position, curwin->dimensions * style.globalScale)){
		curwin->hovered = 1;
	}
	else {
		curwin->hovered = 0;
	}
	
	//check if window's title is hovered (if were drawing it), so we can check for window movement by mouse later
	if (!(curwin->flags & UIWindowFlags_NoTitleBar)) {
		if(Math::PointInRectangle(mp, curwin->position, vec2(curwin->width, style.titleBarHeight) * style.globalScale)){
			curwin->titleHovered = 1;
		}
		else {
			curwin->titleHovered = 0;
		}
	}
	
	//check for scrolling inputs
	if (!(flags & UIWindowFlags_NoScroll)) {
		if (curwin->hovered && DeshInput->ScrollUp()) {
			curwin->scy -= style.scrollAmount.y;
			curwin->scy = Math::clamp(curwin->scy, 0.f, curwin->maxScroll.y);
		}
		else if (curwin->hovered && DeshInput->ScrollDown()) {
			curwin->scy += style.scrollAmount.y;
			curwin->scy = Math::clamp(curwin->scy, 0.f, curwin->maxScroll.y);
		}
	}
	//if (!(curwin->flags & UIWindowFlags_NoTitleBar)) {
	//	curwin->cursor.y = style.titleBarHeight;
	//	curwin->titleBarHeight = style.titleBarHeight;
	//}
	//else {
	//	curwin->titleBarHeight = 0;
	//}
}

void UI::BeginChild(const char* name, vec2 dimensions, UIWindowFlags flags) {
	windowStack.add(curwin);
	
	UIWindow* parent = curwin;
	
	UIItem* item = BeginItem(UIItemType_ChildWin);
	
	//TODO(sushi) add custom positioning for child windows
	item->position = PositionForNewItem();
	item->size = dimensions;
	
	AdvanceCursor(item);
	
	//check if were making a new child or working with one we already know
	if (parent->children.has(name)) {
		curwin = parent->children[name];
		curwin->cursor = vec2(0, 0);
		//if (NextWinPos.x != -1) curwin->position = NextWinPos;
		//if (NextWinSize.x != -1) curwin->dimensions = NextWinSize;
		//NextWinPos = vec2(-1, 0); NextWinSize = vec2(-1, 0);
	}
	else {
		vec2 parentNewPos = PositionForNewItem();
		
		curwin = new UIWindow();
		
		curwin->scroll = vec2(0, 0);
		curwin->name = name;
		curwin->position = parentNewPos + parent->position;
		curwin->dimensions = dimensions;
		curwin->cursor = vec2(0, 0);
		curwin->flags = flags;
		
		parent->children.add(name, curwin);
	}
	
	//check if window is hovered
	vec2 mp = DeshInput->mousePos;
	if (Math::PointInRectangle(mp, curwin->position, curwin->dimensions)) {
		curwin->hovered = 1;
	}
	else {
		curwin->hovered = 0;
	}
	
	//check if window's title is hovered (if were drawing it), so we can check for window movement by mouse later
	if (!(curwin->flags & UIWindowFlags_NoTitleBar)) {
		if (Math::PointInRectangle(mp, curwin->position, vec2(curwin->width, style.titleBarHeight))) {
			curwin->titleHovered = 1;
		}
		else {
			curwin->titleHovered = 0;
		}
	}
	
	//check for scrolling inputs
	if (!(flags & UIWindowFlags_NoScroll)) {
		if (curwin->hovered && DeshInput->ScrollUp()) {
			curwin->scy -= style.scrollAmount.y;
			curwin->scy = Clamp(curwin->scy, 0.f, curwin->maxScroll.y);
		}
		else if (curwin->hovered && DeshInput->ScrollDown()) {
			curwin->scy += style.scrollAmount.y;
			curwin->scy = Clamp(curwin->scy, 0.f, curwin->maxScroll.y);
		}
	}
	
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
	
	UIItem* item = BeginItem(UIItemType_Base);
	item->position = vec2::ZERO;
	
	vec2 mp = DeshInput->mousePos;
	
	vec2 minSizeForFit = CalcWindowMinSize();
	
	if (WinHasFlag(UIWindowFlags_FitAllElements)) 
		curwin->dimensions = minSizeForFit;
	
	
	//if the window isn't invisible draw things that havent been disabled
	if (WinHasFlag(UIWindowFlags_Invisible) != UIWindowFlags_Invisible) {
		//draw background
		if (!WinHasFlag(UIWindowFlags_NoBackground) && !curwin->minimized) {
			UIDrawCmd drawCmd{ UIDrawType_FilledRectangle}; 
			drawCmd.position = vec2::ZERO;
			drawCmd.dimensions = curwin->dimensions;
			drawCmd.color = style.colors[UIStyleCol_WindowBg];
			
			item->drawCmds.add(drawCmd); 
		}
		
		//draw border
		if (!WinHasFlag(UIWindowFlags_NoBorder) && !curwin->minimized) {
			UIDrawCmd drawCmd{ UIDrawType_Rectangle}; //inst 58
			drawCmd.color = style.colors[UIStyleCol_Border];
			drawCmd.position = vec2::ZERO;
			drawCmd.dimensions = curwin->dimensions + vec2::ONE;
			drawCmd.scissorOffset = -vec2::ONE * 2;
			drawCmd.scissorExtent = curwin->dimensions + vec2::ONE * 5;
			drawCmd.useWindowScissor = false;
			item->drawCmds.add(drawCmd);
		}
	}
	
	
	//check to see if the elements we have drawn so far have gone beyond the window's size
	//and allow scrolling if it did, as well as define a max scrolling amount
	//also draw the scroll bar if allowed
	if (!WinHasFlag(UIWindowFlags_NoScrollY) && curwin->dimensions.y < minSizeForFit.y) {
		curwin->maxScroll.y = minSizeForFit.y - curwin->dimensions.y;
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

			static vec2 offset;
			static b32 initial = true;
			if (scdractive && DeshInput->LMouseDown() || !initial) {
				if (initial) {
					offset = draggerpos - DeshInput->mousePos;
					initial = false;
				}
				drag_override = 1;

				draggerpos.y = DeshInput->mousePos.y + offset.y;
				draggerpos.y = Clamp(draggerpos.y, 0, scrollbarheight - draggerheight);

				curwin->scy = draggerpos.y * curwin->maxScroll.y / (scrollbarheight - draggerheight);
				curwin->scy = Clamp(curwin->scy, 0.f, curwin->maxScroll.y);
			}
			if (DeshInput->LMouseReleased()) {
				initial = true;
				drag_override = 0;
			}

			{//scroll bg
				UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
				drawCmd.color = style.colors[UIStyleCol_ScrollBarBg]; //TODO(sushi) add active/hovered scrollbarbg colors
				drawCmd.position = vec2(curwin->dimensions.x - style.scrollBarYWidth, 0);
				drawCmd.dimensions = vec2(style.scrollBarYWidth, scrollbarheight);
				drawCmd.layerOffset = 1;
				item->drawCmds.add(drawCmd);
			}

			{//scroll dragger
				UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
				drawCmd.color = style.colors[(scdractive ? ((DeshInput->LMouseDown() || !initial) ? UIStyleCol_ScrollBarDraggerActive : UIStyleCol_ScrollBarDraggerHovered) : UIStyleCol_ScrollBarDragger)];
				drawCmd.position = draggerpos;
				drawCmd.dimensions = vec2(style.scrollBarYWidth, draggerheight);
				drawCmd.layerOffset = 1;
				item->drawCmds.add(drawCmd);
			}

			//if both scroll bars are active draw a little square to obscure the empty space 
			if(curwin->dimensions.x < minSizeForFit.x){
				UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
				drawCmd.color = style.colors[UIStyleCol_WindowBg];
				drawCmd.position = vec2(curwin->dimensions.x - style.scrollBarYWidth, scrollbarheight);
				drawCmd.dimensions = vec2(style.scrollBarYWidth, style.scrollBarXHeight);
				drawCmd.layerOffset = 1;
				item->drawCmds.add(drawCmd);
			}
		}
	} else curwin->maxScroll.y = 0;

	//do the same but for x
	if (!WinHasFlag(UIWindowFlags_NoScrollX) && curwin->dimensions.x < minSizeForFit.x) {
		curwin->maxScroll.x = minSizeForFit.x - curwin->dimensions.x;
		if (!WinHasFlag(UIWindowFlags_NoScrollBarX)) {
			f32 scrollbarwidth = curwin->dimensions.x - style.scrollBarYWidth;
			f32 draggerwidth = scrollbarwidth * curwin->dimensions.x / minSizeForFit.x;
			vec2 draggerpos((curwin->dimensions.x - draggerwidth) * curwin->scx / curwin->maxScroll.x, curwin->dimensions.y - style.scrollBarXHeight);

			b32 scbgactive = Math::PointInRectangle(DeshInput->mousePos,
				curwin->position.yAdd(scrollbarwidth - style.scrollBarXHeight),
				vec2(scrollbarwidth, style.scrollBarXHeight));

			b32 scdractive = Math::PointInRectangle(DeshInput->mousePos,
				draggerpos + curwin->position,
				vec2(draggerwidth, style.scrollBarXHeight));

			static vec2 offset;
			static b32 initial = true;
			if (scdractive && DeshInput->LMouseDown() || !initial) {
				if (initial) {
					offset = draggerpos - DeshInput->mousePos;
					initial = false;
				}
				drag_override = 1;

				draggerpos.x = DeshInput->mousePos.x + offset.x;
				

				curwin->scx = draggerpos.x * curwin->maxScroll.x / (scrollbarwidth - draggerwidth);
				curwin->scx = Clamp(curwin->scx, 0.f, curwin->maxScroll.x);
			}
			if (DeshInput->LMouseReleased()) {
				initial = true;
				drag_override = 0;
			}

			draggerpos.x = Clamp(draggerpos.x, 0, scrollbarwidth - draggerwidth);

			{//scroll bg
				UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
				drawCmd.color = style.colors[UIStyleCol_ScrollBarBg]; //TODO(sushi) add active/hovered scrollbarbg colors
				drawCmd.position = vec2(0, curwin->dimensions.y - style.scrollBarXHeight);
				drawCmd.dimensions = vec2(scrollbarwidth, style.scrollBarXHeight);
				drawCmd.layerOffset = 1;
				item->drawCmds.add(drawCmd);
			}

			{//scroll dragger
				UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
				drawCmd.color = style.colors[(scdractive ? ((DeshInput->LMouseDown() || !initial) ? UIStyleCol_ScrollBarDraggerActive : UIStyleCol_ScrollBarDraggerHovered) : UIStyleCol_ScrollBarDragger)];
				drawCmd.position = draggerpos;
				drawCmd.dimensions = vec2(draggerwidth, style.scrollBarXHeight);
				drawCmd.layerOffset = 1;
				item->drawCmds.add(drawCmd);
			}
		}
	} else curwin->maxScroll.x = 0;

	if (!WinHasFlag(UIWindowFlags_NoScrollX) && curwin->dimensions.x < minSizeForFit.x)
		curwin->maxScroll.x = minSizeForFit.x - curwin->dimensions.x;
	else
		curwin->maxScroll.x = 0;

	
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

	UIItem item{ UIItemType_Base, curwin->cursor, style };
	item.position = vec2::ZERO;
	
	vec2 mp = DeshInput->mousePos;
	
	vec2 minSizeForFit = CalcWindowMinSize();
	
	if (WinHasFlag(UIWindowFlags_FitAllElements))
		curwin->dimensions = minSizeForFit;
	
	
	//if the window isn't invisible draw things that havent been disabled
	if (WinHasFlags(UIWindowFlags_Invisible)) {
		//draw background
		if (!(curwin->flags & UIWindowFlags_NoBackground) && !curwin->minimized) {
			UIDrawCmd drawCmd{ UIDrawType_FilledRectangle};
			drawCmd.position = vec2::ZERO;
			drawCmd.dimensions = curwin->dimensions;
			drawCmd.color = style.colors[UIStyleCol_WindowBg];
			
			item.drawCmds.add(drawCmd);
		}
		
		//draw border
		if (!WinHasFlag(UIWindowFlags_NoBorder) && !curwin->minimized) {
			UIDrawCmd drawCmd{ UIDrawType_Rectangle}; //inst 58
			drawCmd.color = style.colors[UIStyleCol_Border];
			drawCmd.position = vec2::ZERO;
			drawCmd.dimensions = curwin->dimensions;
			drawCmd.scissorOffset = -vec2::ONE * 2;
			drawCmd.scissorExtent = curwin->dimensions + vec2::ONE * 2;
			drawCmd.useWindowScissor = false;
			item.drawCmds.add(drawCmd);
		}
	}
	
	
	
	//check to see if the elements we have drawn so far have gone beyond the window's size
	//and allow scrolling if it did, as well as define a max scrolling amount
	//also draw the scroll bar if allowed
	if (!WinHasFlag(UIWindowFlags_NoScrollY) && curwin->dimensions.y < minSizeForFit.y) {
		curwin->maxScroll.y = minSizeForFit.y - curwin->dimensions.y;
		if (!WinHasFlag(UIWindowFlags_NoScrollBarY)) {
			//f32 draggerwidth = curwin->dimensions.y * curwin->dimensions.y / minSizeForFit.y;

		}
	} else curwin->maxScroll.y = 0;
	
	if (!WinHasFlag(UIWindowFlags_NoScrollX) && curwin->dimensions.x < minSizeForFit.x)
		curwin->maxScroll.x = minSizeForFit.x - curwin->dimensions.x;
	else
		curwin->maxScroll.x = 0;


	curwin->style = style;
	curwin->baseItems.add(item);

	
	NextWinPos = vec2(-1, 0); NextWinSize = vec2(-1, 0);
	
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

bool UI::IsWinHovered() {
	return curwin->hovered;
}

bool UI::AnyWinHovered() {
	return globalHovered || draggingWin;
}

UIWindow* DisplayMetrics() {
	using namespace UI;
	
	UIWindow* debugee = nullptr;
	UIWindow* myself = 0; //pointer returned for drawing
	
	UIWindow* slomo     = *windows.atIdx(0);
	UIWindow* quick     = *windows.atIdx(0);
	UIWindow* mostitems = *windows.atIdx(0);
	UIWindow* longname  = *windows.atIdx(0);
	
	array<char*> names;
	for(UIWindow* win : windows) {
		if (!(win->name == "METRICS")) {
			if (win->render_time > slomo->render_time)     slomo = win;
			if (win->render_time < quick->render_time)     quick = win;
			if (win->items_count > mostitems->items_count) mostitems = win;
			if (win->name.count  > longname->name.count)   longname = win;
			names.add(win->name.str);
		}
	}

	
	Begin("METRICS", vec2::ZERO, vec2(300, 500));
	myself = curwin;
	
	Text(TOSTRING("Active Windows: ", windowStack.count).str);
	
	string slomotext = TOSTRING("Slowest Render:");
	string quicktext = TOSTRING("Fastest Render:");
	string mostitext = TOSTRING("Most Items: "); 
	
	static f32 sw = CalcTextSize(longname->name).x;
	static f32 fw = CalcTextSize(slomotext).x + 5;
	
	BeginRow(3, 11);
	RowSetupColumnWidths({ fw, sw, 55 });
	PushVar(UIStyleVar_RowItemAlign, vec2{ 0, 0.5 });
	Text(slomotext.str);
	PopVar();
	Text(slomo->name.str);
	if (Button("select")) debugee = slomo;
	EndRow();
	
	BeginRow(3, 11);
	RowSetupColumnWidths({ fw, sw, 55 });
	PushVar(UIStyleVar_RowItemAlign, vec2{ 0, 0.5 });
	Text(quicktext.str);
	PopVar();
	Text(quick->name.str);
	if (Button("select")) debugee = quick;
	EndRow();
	
	BeginRow(3, 11);
	RowSetupColumnWidths({ fw, sw, 55 });
	PushVar(UIStyleVar_RowItemAlign, vec2{ 0, 0.5 });
	Text(mostitext.str);
	PopVar();
	Text(mostitems->name.str);
	if (Button("select")) debugee = mostitems;
	EndRow();
	
	
	static u32 selected = 0;
	//if (DropDown("windowstochoosefrom", names.data, windows.count, selected)) {
	//	debugee = *windows.atIdx(selected);
	//}
	
	End();
	
	//PopColor(5);
	
	return myself;
	
}

//this just sets a flag to show the window at the very end of the frame, so we can gather all data
//about windows incase the user tries to call this before making all their windows
bool show_metrics = 0;
void UI::ShowMetricsWindow() {
	show_metrics = 1;
}


//initializes core UI with an invisible working window covering the entire screen
//also initializes styles
//the base window should never focus when clicking within it, so any widgets drawn within
//it will not focus if theres a window in front of them.
//I'm not sure how i want to fix it yet
void UI::Init() {
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
	PushColor(UIStyleCol_ScrollBarBg, Color_VeryDarkCyan);
	PushColor(UIStyleCol_ButtonBg,    Color_VeryDarkCyan);
	PushColor(UIStyleCol_CheckboxBg,  Color_VeryDarkCyan);
	PushColor(UIStyleCol_HeaderBg,    color(0, 100, 100, 255));
	PushColor(UIStyleCol_SliderBg,    Color_VeryDarkCyan);
	PushColor(UIStyleCol_InputTextBg, Color_DarkCyan);
	
	//active backgrounds
	PushColor(UIStyleCol_ScrollBarBgActive, Color_VeryDarkCyan);
	PushColor(UIStyleCol_ButtonBgActive,    Color_Cyan);
	PushColor(UIStyleCol_CheckboxBgActive,  Color_Cyan);
	PushColor(UIStyleCol_HeaderBgActive,    color(0, 255, 255, 255));
	PushColor(UIStyleCol_SliderBgActive,    Color_Cyan);
	PushColor(UIStyleCol_InputTextBgActive, Color_DarkCyan);
	
	//hovered backgrounds
	PushColor(UIStyleCol_ScrollBarBgHovered, Color_VeryDarkCyan);
	PushColor(UIStyleCol_ButtonBgHovered,    Color_DarkCyan);
	PushColor(UIStyleCol_CheckboxBgHovered,  Color_DarkCyan);
	PushColor(UIStyleCol_HeaderBgHovered,    color(0, 128, 128, 255));
	PushColor(UIStyleCol_SliderBgHovered,    Color_DarkCyan);
	PushColor(UIStyleCol_InputTextBgHovered, Color_DarkCyan);
	
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
	PushVar(UIStyleVar_ScrollBarYWidth,          10);
	PushVar(UIStyleVar_ScrollBarXHeight,         10);
	PushVar(UIStyleVar_FontHeight,               style.font->max_height);
	
	PushScale(vec2(1, 1));
	
	initColorStackSize = colorStack.count;
	initStyleStackSize = varStack.count;
	
	windows.add("base", curwin);
	windowStack.add(curwin);
	
	Log("deshi","Finished UI initialization in ",TIMER_END(t_s),"ms");
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
	
	globalHovered = 0;
	
	//focusing and dragging
	
	//focus
	for (int i = windows.count - 1; i > 0; i--) {
		UIWindow* w = *windows.atIdx(i);
		w->focused = 0;
		if (!(w->flags & UIWindowFlags_NoFocus)) {
			if (i == windows.count - 1 && w->hovered) {
				w->focused = 1;
				break;
			}
			else if (w->hovered && ((w->flags & UIWindowFlags_FocusOnHover) ? 1 : DeshInput->KeyPressed(MouseButton::LEFT))) {
				(*windows.data.last)->hovered = 0;
				w->focused = 1;
				for (int move = i; move < windows.count - 1; move++)
					windows.swap(move, move + 1);
				break;
			}
		}
	}
	
	
	if (!drag_override) { //drag
		UIWindow* focused = *windows.atIdx(windows.count-1);
		
		static bool newDrag = true;
		static vec2 mouseOffset = vec2(0, 0);
		
		if (
			!(focused->flags & UIWindowFlags_NoMove) &&
			focused->hovered &&
			DeshInput->KeyPressed(MouseButton::LEFT)) {
			draggingWin = 1;
			mouseOffset = focused->position - DeshInput->mousePos;
			newDrag = false;
		}
		if (!newDrag) 
			focused->position = DeshInput->mousePos + mouseOffset;
		if (DeshInput->KeyReleased(MouseButton::LEFT)) {
			newDrag = true;
			draggingWin = 0;
		}
	}
	
	auto draw_window = [&](UIWindow* p) {
		TIMER_START(winren);
		//window position and size corrected for titlebar 
		vec2 winpos = vec2(p->x, p->y + p->titleBarHeight);
		vec2 winscissor{ Max(0.0f, winpos.x), Max(0.0f, winpos.y) } ; //NOTE scissor offset cant be negative
		vec2 winsiz = vec2(p->width, p->height - p->titleBarHeight) * p->style.globalScale;
		
		//winscissor *= p->style.globalScale;
		
		if (p->hovered && !(p->flags & UIWindowFlags_DontSetGlobalHoverFlag))
			globalHovered = 1;
		
		//lol nested lambdas
		//i dont like having this entire thing in 2 different places
		//could maybe be made an inlined function instead
		auto draw_cmds = [&](UIDrawCmd& drawCmd, UIItem& item, vec2 itempos, vec2 itemsiz) {
			vec2   dcpos = itempos + drawCmd.position * item.style.globalScale;
			vec2  dcpos2 = itempos + drawCmd.position * item.style.globalScale;
			vec2   dcsiz = drawCmd.dimensions * item.style.globalScale;
			vec2    dcse = (drawCmd.useWindowScissor ? winsiz : drawCmd.scissorExtent * item.style.globalScale);
			vec2    dcso = (drawCmd.useWindowScissor ? winscissor : itempos + drawCmd.scissorOffset);
			color  dccol = drawCmd.color;
			f32      dct = drawCmd.thickness;
			u32      dcl = p->windowlayer;
			u32     dclo = drawCmd.layerOffset;

			dcpos.x = floor(dcpos.x); dcpos.y = floor(dcpos.y);
			dcpos2.x = floor(dcpos2.x); dcpos2.y = floor(dcpos2.y);
			
			dcso.x = Max(winpos.x, dcso.x); dcso.y = Max(winpos.y, dcso.y); //force all items to stay within their windows
			dcso.x = Min(winpos.x + winsiz.x - dcse.x, dcso.x); dcso.y = Min(winpos.y + winsiz.y - dcse.y, dcso.y);
			if (drawCmd.useWindowScissor && winpos.x < 0) dcse.x += winpos.x; //if the window's pos goes negative, the scissor extent needs to adjust itself
			if (drawCmd.useWindowScissor && winpos.y < 0) dcse.y += winpos.y;
			dcse.x = Max(0.f, dcse.x); dcse.y = Max(0.f, dcse.y);
			dcso.x = Max(0.0f, dcso.x); dcso.y = Max(0.0f, dcso.y); //NOTE scissor offset cant be negative



			Texture* dctex = drawCmd.tex;

			cstring dctext{ drawCmd.text.str,drawCmd.text.count };
			wcstring wdctext{ drawCmd.wtext.str, drawCmd.wtext.count };

			Font* font = drawCmd.font;

			switch (drawCmd.type) {
				case UIDrawType_FilledRectangle: {
					Render::FillRect2D(dcpos, dcsiz, dccol, dcl + dclo, dcso, dcse);
				}break;

				case UIDrawType_Line: {
					Render::DrawLine2D(dcpos - item.position, dcpos2 - item.position, dct, dccol, dcl + dclo, dcso, dcse);
				}break;
				case UIDrawType_Text: {
					vec2 scale = vec2::ONE * item.style.fontHeight / item.style.font->max_height * item.style.globalScale;
					Render::DrawText2D(font, dctext, dcpos, dccol, scale, dcl + dclo, dcso, dcse);
				}break;
				case UIDrawType_WText: {
					vec2 scale = vec2::ONE * item.style.fontHeight / item.style.font->max_height * item.style.globalScale;
					Render::DrawText2D(font, wdctext, dcpos, dccol, scale, dcl + dclo, dcso, dcse);
				}break;
				case UIDrawType_Rectangle: {
					Render::DrawRect2D(dcpos, dcsiz, dccol, dcl + dclo, dcso, dcse);
				}break;
				case UIDrawType_Image: {
					Render::DrawTexture2D(dctex, dcpos, dcsiz, 0, dct, dcl + dclo, dcso, dcse);
				}break;
			}
		};

		//draw base cmds first
		for (UIItem& item : p->baseItems) {
			vec2 itempos = winpos + item.position * item.style.globalScale;//item.type == UIItemType_Abstract ? item.position : (winpos + item.position);
			vec2 itemsiz = item.size;
			
			for (UIDrawCmd& drawCmd : item.drawCmds) {
				draw_cmds(drawCmd, item, itempos, itemsiz);
			}
		}
		
		//dont draw non-base draw cmds if we're minimized
		if (!p->minimized) {
			forI(UI_WINDOW_ITEM_LAYERS) {
				for (UIItem& item : p->items[i]) {
					vec2 itempos = winpos + item.position * item.style.globalScale;//(item.type == UIItemType_Abstract ? item.position : winpos + item.position * item.style.globalScale);
					vec2 itemsiz = item.size;
					
					for (UIDrawCmd& drawCmd : item.drawCmds) {
						draw_cmds(drawCmd, item, itempos, itemsiz);
					}
				}
			}
		}
		//a lot of the following is debug related and can be totally removed, or moved into
		//#if stuff to make sure it doesnt ship in a release build
		p->baseItems.clear();
		p->items_count = 0;
		forI(UI_WINDOW_ITEM_LAYERS) {
			p->items_count += p->items[i].count;
			p->items[i].clear();
		}
		p->render_time = TIMER_END(winren);
	};
	
	//draw windows in order with their drawCmds
	for (UIWindow* p : windows) {
		draw_window(p);
		
		//NOTE children are drawn last for now, this should be changed to respect the order of items and children windows
		//     later!!!!
		for (UIWindow* c : p->children) {
			draw_window(c);
		}
	}
	
	if (show_metrics) {
		draw_window(DisplayMetrics());
		show_metrics = 0;
	}
	
	//draw all debug commands if there are any
	
	for (UIDrawCmd& drawCmd : debugCmds) {
		vec2   dcpos = drawCmd.position;
		vec2  dcpos2 = drawCmd.position2;
		vec2   dcsiz = drawCmd.dimensions;
		vec2    dcse = drawCmd.scissorExtent;
		vec2    dcso = drawCmd.scissorOffset;
		dcso.x = Max(0.0f, dcso.x); dcso.y = Max(0.0f, dcso.y); //NOTE scissor offset cant be negative
		color  dccol = drawCmd.color;
		f32    dct = drawCmd.thickness;
		cstring dctex{drawCmd.text.str,drawCmd.text.count};
		u32      dcl = 5;
		
		Font*   font = drawCmd.font;
		
		switch (drawCmd.type) {
			case UIDrawType_FilledRectangle: {
				if (drawCmd.useWindowScissor)
					Render::FillRect2D(dcpos, dcsiz, dccol, dcl, vec2::ZERO, DeshWindow->dimensions);
				else
					Render::FillRect2D(dcpos, dcsiz, dccol, dcl, dcso, dcse);
				
			}break;
			
			case UIDrawType_Line: {
				if (drawCmd.useWindowScissor)
					Render::DrawLine2D(dcpos, dcpos2, dct, dccol, dcl, vec2::ZERO, DeshWindow->dimensions);
				else
					Render::DrawLine2D(dcpos, dcpos2, dct, dccol, dcl, dcso, dcse);
			}break;
			
			case UIDrawType_Text: {
				vec2 scale = vec2::ONE * ((font->type != FontType_BDF) ? style.fontHeight / font->max_height : 1);
				if (drawCmd.useWindowScissor)
					Render::DrawText2D(font, dctex, dcpos, dccol, scale, dcl, vec2::ZERO, DeshWindow->dimensions);
				else
					Render::DrawText2D(font, dctex, dcpos, dccol, scale, dcl, dcso, dcse);
			}break;
			
			case UIDrawType_Rectangle: {
				if (drawCmd.useWindowScissor)
					Render::DrawRect2D(dcpos, dcsiz, dccol, dcl,  vec2::ZERO, DeshWindow->dimensions);
				else
					Render::DrawRect2D(dcpos, dcsiz, dccol, dcl, dcso, dcse);
			}break;
		}
	}
	
	
	
	debugCmds.clear();
}
