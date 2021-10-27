//color pallete 
//current palette:
//https://lospec.com/palette-list/slso8
//TODO(sushi, Ui) implement menu style file loading sort of stuff yeah
//TODO(sushi, Ui) standardize what UI element each color belongs to
local struct {
	color          midnight_blue = color(0x0d2b45ff); //midnight blue
	color         dark_grey_blue = color(0x203c56ff); //dark gray blue
	color            purple_gray = color(0x544e68ff); //purple gray
	color              pink_gray = color(0x8d697aff); //pink gray
	color        bleached_orange = color(0xd08159ff); //bleached orange
	color bright_bleached_orange = color(0xffaa5eff); //above but brighter
	color             skin_white = color(0xffd4a3ff); //skin white
	color      bright_skin_white = color(0xffecd6ff); //even whiter skin
	color             near_black = color(0x141414ff); //almost black
} colors;

//global styling
UIStyle style;

//for color stack, saves what element was changed and what it's old color was 
struct ColorMod {
	UIStyleCol element;
	color oldCol;
};

static constexpr u32 CHAR_SIZE = sizeof(CHAR);


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
	{2, offsetof(UIStyle, rowCellPadding)},
	{2, offsetof(UIStyle, rowItemAlign)},
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
local map<const char*, bool>             dropDowns;   //stores known dropdowns and if they are open or not
local array<UIWindow*>                   windowStack; //window stack which allow us to use windows like we do colors and styles
local array<ColorMod>                    colorStack; 
local array<VarMod>                      varStack; 
local array<Font*>                       fontStack;

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

//the last item worked on
//this is always set in EndItem()
//its purpose is to solve a problem caused by custom items, since items all get merged into one item
//when we're making a custom item, and i want the user to be able to still get information about the last item made
//for the group, we need to store a copy of it here
local UIItem lastitem;


//helper defines


#define workingWinPositionPlusTitlebar vec2(curwin->x, curwin->y + ((curwin->flags & UIWindowFlags_NoTitleBar) ? 0 :style.titleBarHeight));
#define workingWinSizeMinusTitlebar    vec2(curwin->width, curwin->height - ((curwin->flags & UIWindowFlags_NoTitleBar) ? 0 : style.titleBarHeight));
#define workingHasFlag(flag) (curwin->flags & flag)
#define HasFlag(flag) (flags & flag)


//helper functions


//this calculates text taking into account newlines, BUT NOT WRAPPING
//useful for sizing a window to fit some text
vec2 UI::CalcTextSize(cstring text){
	vec2 result = vec2{0, f32(style.font->max_height)};
	f32 line_width = 0;
	switch(style.font->type){
		case FontType_BDF: case FontType_NONE:{
			while(text){
				if(*text.str == '\n'){
					result.y += style.font->max_height;
					line_width = 0;
				}
				line_width += style.font->max_width;
				if(line_width > result.x) result.x = line_width;
				advance(&text,1);
			}
		}break;
		case FontType_TTF:{
			while(text){
				if(*text.str == '\n'){
					result.y += style.font->max_height;
					line_width = 0;
				}
				//line_width += ((stbtt_bakedchar*)style.font->ttf_bake)[*text.str-32].xadvance;
				if(line_width > result.x) result.x = line_width;
				advance(&text,1);
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
	else if(moveCursor) curwin->cursor = vec2{ 0, itemmade->position.y + itemmade->size.y + style.itemSpacing.y };
}

//function for getting the position of a new item based on style, so the long string of additions
//is centralized for new additions, if ever made, and so that i dont have to keep writing it :)
inline vec2 PositionForNewItem() {
	return curwin->cursor + (style.windowPadding);// -curwin->scroll);
}



void UI::SetNextItemActive() {
	NextActive = 1;
}

UIStyle& UI::GetStyle(){
	return style;
}

//the following 4 functions should probably error out sofly, rather than asserting

//returns the cursor to the same line as the previous and moves it to the right by the 
//width of the object
void UI::SameLine(){
	Assert(curwin->items.count, "Attempt to sameline an item creating any items!");
	curwin->cursor.y = curwin->items.last->initialCurPos.y;
	curwin->cursor.x += curwin->items.last->size.x + style.itemSpacing.x;
}

vec2 UI::GetLastItemPos() {
	Assert(curwin->items.count, "Attempt to get last item position without creating any items!");
	return curwin->items.last->position;
}

vec2 UI::GetLastItemSize() {
	Assert(curwin->items.count, "Attempt to get last item size without creating any items!");
	return curwin->items.last->size;
}

vec2 UI::GetLastItemScreenPos() {
	Assert(curwin->items.count, "Attempt to get last item position without creating any items!");
	return curwin->position + curwin->items.last->position;
}

//internal last item getter, returns nullptr if there are none
inline UIItem* GetLastItem() {
	return curwin->items.last;
}

//helper for making any new UIItem, since now we must work with item pointers internally
//this function also decides if we are working with a new item or continuing to work on a previous
inline UIItem* BeginItem(UIItemType type) {
	if (!custom_item)
		curwin->items.add(UIItem{ type, curwin->cursor, style });
	return GetLastItem();
}

inline void EndItem(UIItem* item) {
	//copy the last made item to lastitem, so we can look back at it independently of custom item nonsense
	//maybe only do this is we're making a custom item
	lastitem = *item;
}

//@BeginRow
//  a row is a collection of columns used to align a number of items nicely
//  you are required to specify the width of each column when using Row, as it removes the complicated
//  nature of having to figure this out after the fact. row height and width are not necessarily a problem
//  and can be determined dynamically
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
	UIDrawCmd drawCmd{ UIDrawType_Rectangle };
	drawCmd.position = vec2::ZERO;
	drawCmd.dimensions = dimen;
	drawCmd.color = color;
	
	item.position = pos;
	item.size = dimen;
	
	item.drawCmds.add(drawCmd);
	curwin->items.add(item);
}

void UI::RectFilled(vec2 pos, vec2 dimen, color color) {
	UIItem       item{ UIItemType_Abstract, curwin->cursor, style };
	UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
	drawCmd.position = vec2::ZERO;
	drawCmd.dimensions = dimen;
	drawCmd.color = color;
	
	item.position = pos;
	item.size = dimen;
	
	item.drawCmds.add(drawCmd);
	curwin->items.add(item);
}


//@Line


void UI::Line(vec2 start, vec2 end, float thickness, color color){
	UIItem       item{ UIItemType_Abstract, curwin->cursor, style };
	UIDrawCmd drawCmd{ UIDrawType_Line };
	drawCmd. position = start;
	drawCmd.position2 = end;
	drawCmd.thickness = thickness;
	drawCmd.    color = color;
	
	item.position = vec2{ Min(drawCmd.position.x, drawCmd.position2.x), Min(drawCmd.position.y, drawCmd.position2.y) };
	item.    size = vec2{ Max(drawCmd.position.x, drawCmd.position2.x), Max(drawCmd.position.y, drawCmd.position2.y) } - item.position;
	
	item.drawCmds.add(drawCmd);
	curwin->items.add(item);
}



//@Items



void UI::SetNextItemSize(vec2 size) {
	NextItemSize = size;
}


//@Text

//internal function for actually making and adding the drawCmd
local void TextCall(const char* text, vec2 pos, color color, UIItem* item) {
	UIDrawCmd drawCmd{ UIDrawType_Text };
	drawCmd.text = string(text); 
	drawCmd.position = pos;
	drawCmd.color = color;
	//drawCmd.scissorOffset = -item.position;
	//drawCmd.scissorExtent = UI::CalcTextSize(text);
	drawCmd.font = style.font;
	
	item->drawCmds.add(drawCmd);
}

//main function for wrapping, where position is starting position of text relative to the top left of the window
//this function also decides if text is to be wrapped or not, and if not simply calls TextEx (to clean up the Text() functions)
local void TextW(const char* in, vec2 pos, color color, bool nowrap, bool move_cursor = true) {
	
	using namespace UI;
	UIItem* item = BeginItem(UIItemType_Text);
	item->position = (move_cursor) ? PositionForNewItem() : pos;
	
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
		
		//max characters we can place 
		u32 maxChars = floor(((curwin->width - style.windowPadding.x) - workcur.x) / style.font->max_width);
		
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
				//this is now always handled by CalcItemSize becuase we now support
				//non monospace fonts, however we should probably handle finding width 
				//during all of this not after.

				//we have to get max string length to determine item's width here
				//item->size.x = Max(style.font->max_width * t.count, item->size.x);
				
				TextCall(t.str, workcur, color, item);
				workcur.y += style.fontHeight + style.itemSpacing.y;
			}
		}
		
		item->size.y = workcur.y - curwin->position.y;
		if (NextItemSize.x != -1)
			item->size = NextItemSize;
		
		CalcItemSize(item);
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

void UI::Text(const char* text, UITextFlags flags) {
	TextW(text, curwin->cursor, style.colors[UIStyleCol_Text], HasFlag(UITextFlags_NoWrap));
}

void UI::Text(const char* text, vec2 pos, UITextFlags flags) {
	TextW(text, pos, style.colors[UIStyleCol_Text], HasFlag(UITextFlags_NoWrap), 0);
}

void UI::Text(const char* text, color color, UITextFlags flags) {
	TextW(text, curwin->cursor, color, HasFlag(UITextFlags_NoWrap));
}

void UI::Text(const char* text, vec2 pos, color color, UITextFlags flags) {
	TextW(text, pos, color, HasFlag(UITextFlags_NoWrap), 0);
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


bool ButtonCall(const char* text, vec2 pos, color color, bool move_cursor = 1) {
	UIItem* item = BeginItem(UIItemType_Button);
	item->size = (NextItemSize.x != -1) ? NextItemSize : vec2(Min(curwin->width, 50), style.font->max_height * 1.3);
	item->position = pos;
	AdvanceCursor(item, move_cursor);
	
	{//background
		UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
		drawCmd.position = vec2::ZERO;
		drawCmd.dimensions = item->size;
		drawCmd.color = color;
		item->drawCmds.add(drawCmd);
	}
	
	{//border
		UIDrawCmd drawCmd{ UIDrawType_Rectangle }; //inst 58
		drawCmd.color = style.colors[UIStyleCol_Border];
		drawCmd.position = vec2::ZERO;
		drawCmd.dimensions = item->size;
		drawCmd.scissorOffset = -vec2::ONE * 2;
		drawCmd.scissorExtent = curwin->dimensions + vec2::ONE * 2;
		item->drawCmds.add(drawCmd);
	}
	
	{//text
		UIDrawCmd drawCmd{ UIDrawType_Text };
		drawCmd.color = style.colors[UIStyleCol_Text];
		drawCmd.position = 
			vec2((item->size.x - UI::CalcTextSize(text).x) * style.buttonTextAlign.x,
				 (style.font->max_height * 1.3 - style.font->max_height) * style.buttonTextAlign.y);
		//drawCmd.scissorOffset = item->position;
		drawCmd.scissorExtent = item->size;
		drawCmd.text = string(text);
		drawCmd.font = style.font;
		item->drawCmds.add(drawCmd);
	}
	
	//TODO(sushi) add a flag for prevent button presses when window is not focused
	if (/*curwin->focused &&*/ Math::PointInRectangle(DeshInput->mousePos, curwin->position + pos, item->size) && DeshInput->LMousePressed()) return true;
	else return false;
}

bool UI::Button(const char* text) {
	return ButtonCall(text, PositionForNewItem(), style.colors[UIStyleCol_FrameBg]);
}

bool UI::Button(const char* text, vec2 pos) {
	return ButtonCall(text, pos, style.colors[UIStyleCol_FrameBg], 0);
}

bool UI::Button(const char* text, color color) {
	return ButtonCall(text, PositionForNewItem(), color);
}

bool UI::Button(const char* text, vec2 pos, color color) {
	return ButtonCall(text, pos, color, 0);
}

void UI::Checkbox(string label, bool* b) {
	UIItem* item = BeginItem(UIItemType_Checkbox);
	
	vec2 boxpos = PositionForNewItem();
	vec2 boxsiz = style.checkboxSize;
	
	item->position = boxpos;
	item->size = boxsiz;
	item->size.x += style.itemSpacing.x + CalcTextSize(label).x;
	
	AdvanceCursor(item);
	
	{//box
		UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
		drawCmd.position = vec2{ 0,0 };
		drawCmd.dimensions = boxsiz;
		drawCmd.color = style.colors[UIStyleCol_FrameBg];
		
		item->drawCmds.add(drawCmd);
	}
	
	//fill if true
	int fillPadding = style.checkboxFillPadding;
	if (*b) {
		UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
		drawCmd.position = boxsiz * vec2(fillPadding / boxsiz.x, fillPadding / boxsiz.y);
		drawCmd.dimensions = boxsiz * (vec2::ONE - 2 * vec2(fillPadding / boxsiz.x, fillPadding / boxsiz.y));
		drawCmd.color = style.colors[UIStyleCol_FrameBg] * 0.7;
		
		item->drawCmds.add(drawCmd);
	}
	
	{//label
		UIDrawCmd drawCmd{ UIDrawType_Text };
		drawCmd.position = vec2(boxsiz.x + style.itemSpacing.x, (boxsiz.y - style.font->max_height) * 0.5);
		drawCmd.text = label;
		drawCmd.color = style.colors[UIStyleCol_Text];
		drawCmd.font = style.font;
		
		item->drawCmds.add(drawCmd);
	}
	
	
	
	if (DeshInput->LMousePressed() && Math::PointInRectangle(DeshInput->mousePos, curwin->position + item->position, boxsiz))
		*b = !*b;
	
}

void UI::DropDown(const char* label, const char* options[], u32 options_count, u32& selected) {
	UIItem* item = BeginItem(UIItemType_DropDown);
	
	bool isOpen = false;
	if (!dropDowns.has(label)) {
		dropDowns.add(label);
		dropDowns[label] = false;
	}
	else {
		isOpen = dropDowns[label];
	}
	
	
	item->position = PositionForNewItem();
	item->size = (NextItemSize.x == -1) ? vec2{ curwin->width - 2 * style.windowPadding.x, 20 } : NextItemSize;
	
	AdvanceCursor(item);
	
	//main bar, drawn regardless of if the box is open
	
	{//background
		UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
		drawCmd.position = vec3{ 0,0 };
		drawCmd.dimensions = item->size;
		drawCmd.color = style.colors[UIStyleCol_FrameBg];
		item->drawCmds.add(drawCmd);
	}
	
	{//selected text
		UIDrawCmd drawCmd{ UIDrawType_Text };
		drawCmd.position = vec3{ 10, (item->size.y - style.font->max_height) * 0.5f };
		drawCmd.color = style.colors[UIStyleCol_Text];
		drawCmd.text = string(options[selected]);
		drawCmd.font = style.font;
		item->drawCmds.add(drawCmd);
	}
	
	vec2 openBoxPos = vec2{ (item->size.x - item->size.y / 2) * 0.95f, item->size.y / 4 };
	vec2 openBoxSize = vec2{ item->size.y / 2, item->size.y / 2 };
	
	{//open box
		UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
		drawCmd.position = openBoxPos;
		drawCmd.dimensions = openBoxSize;
		drawCmd.color = style.colors[UIStyleCol_FrameBg] * 0.4;
		item->drawCmds.add(drawCmd);
	}
	
	if (curwin->focused && 
		DeshInput->LMousePressed() &&
		Math::PointInRectangle(DeshInput->mousePos, curwin->position + item->position + openBoxPos, openBoxSize)) {
		dropDowns[label] = !dropDowns[label];
	}
	
	if(isOpen) {
		//find what box the mouse is over
		
		vec2 sp = item->position.yAdd(item->size.y);
		vec2 mp = DeshInput->mousePos - (curwin->position + sp);
		f32 height = item->size.y * options_count;
		f32 width = item->size.x;
		u32 mo = -1;
		
		if (curwin->focused && Math::PointInRectangle(mp, vec2::ZERO, vec2{ width, height })) {
			mo = floor(mp.y / height * options_count);
			if (DeshInput->LMousePressed()) {
				selected = mo;
			}
		}
		
		for (int i = 0; i < options_count; i++) {
			{//selection boxes
				UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
				drawCmd.position = vec2{ 0, item->size.y * (i + 1) };
				drawCmd.dimensions = item->size;
				drawCmd.color = style.colors[UIStyleCol_FrameBg] * ((i == selected || i == mo) ? 0.5 : 1);
				item->drawCmds.add(drawCmd);
				
			}
			
			{//underline
				UIDrawCmd drawCmd{ UIDrawType_Line };
				drawCmd.position = vec2{ 0,(item->size.y * (i + 2))};
				drawCmd.position2 = vec2{ item->size.x, (item->size.y * (i + 2))};
				drawCmd.color = Color_Black;
				drawCmd.thickness = 1;
				item->drawCmds.add(drawCmd);
			}
			
			{//selection texts
				UIDrawCmd drawCmd{ UIDrawType_Text };
				drawCmd.position = vec3{ 10, (item->size.y - style.font->max_height) * 0.5f + (i + 1) * item->size.y };
				drawCmd.color = style.colors[UIStyleCol_Text];
				drawCmd.text = options[i];
				drawCmd.font = style.font;
				item->drawCmds.add(drawCmd);
			}
		}
		
		
		
	}
	
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
		dim = vec2(Math::clamp(100.f, 0.f, Math::clamp(curwin->width - 2.f*style.windowPadding.x, 1.f, FLT_MAX)), 1.3f*style.font->max_height);
	}
	
	item->size = dim;
	item->position = position;
	
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
	
	if (NextActive || DeshInput->KeyPressed(MouseButton::LEFT)) {
		if (NextActive || Math::PointInRectangle(DeshInput->mousePos, curwin->position + GetLastItem()->position, dim)) {
			activeId = state->id;
			NextActive = 0;
		}
		else if (activeId == state->id) activeId = -1;
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
	if (activeId == state->id) {
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
		UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
		drawCmd.position = vec2::ZERO;
		drawCmd.dimensions = dim;
		drawCmd.color = Color_DarkGrey;
		
		item->drawCmds.add(drawCmd);
	}
	
	vec2 textStart =
		vec2((dim.x - charCount * style.font->max_width) * style.inputTextTextAlign.x,
			 (style.font->max_height * 1.3 - style.font->max_height) * style.inputTextTextAlign.y);
	
	{//text
		UIDrawCmd drawCmd{ UIDrawType_Text };
		drawCmd.position = textStart;
		drawCmd.text = string(buff);
		drawCmd.color = style.colors[UIStyleCol_Text];
		drawCmd.font = style.font;
		
		item->drawCmds.add(drawCmd);
	}
	
	//TODO(sushi, Ui) impl different text cursors
	if (activeId == state->id) {//cursor
		UIDrawCmd drawCmd{ UIDrawType_Line };
		drawCmd.position = textStart + vec2(state->cursor * style.font->max_width, 0);
		drawCmd.position2 = textStart + vec2(state->cursor * style.font->max_width, style.font->max_height - 1);
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
	curwin->items.add(UIItem{ UIItemType_Custom, curwin->cursor, style });
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

void UI::PushVar(UIStyleVar idx, float nuStyle) {
	Assert(uiStyleVarTypes[idx].count == 1, "Attempt to use a float on a vec2 style variable!");
	float* p = (float*)((u8*)&style + uiStyleVarTypes[idx].offset);
	varStack.add(VarMod(idx, *p));
	*p = nuStyle;
}

void UI::PushVar(UIStyleVar idx, vec2 nuStyle) {
	Assert(uiStyleVarTypes[idx].count == 2, "Attempt to use a float on a vec2 style variable!");
	vec2* p = (vec2*)((u8*)&style + uiStyleVarTypes[idx].offset);
	varStack.add(VarMod(idx, *p));
	*p = nuStyle;
}

void UI::PushFont(Font* font) { 
	fontStack.add(style.font);
	style.font = font;
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
			float* p = (float*)((u8*)&style + type.offset);
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


//Windows


//begins a window with a name, position, and dimensions along with some optional flags
//if begin window is called with a name that was already called before it will work with
//the data that window previously had
void UI::Begin(const char* name, vec2 pos, vec2 dimensions, UIWindowFlags flags) {
	Assert(!rowInProgress, "Attempted to begin a window with a Row in progress! (Did you forget to call EndRow()?");
	
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
	if(Math::PointInRectangle(mp, curwin->position, curwin->dimensions)){
		curwin->hovered = 1;
	}
	else {
		curwin->hovered = 0;
	}
	
	//check if window's title is hovered (if were drawing it), so we can check for window movement by mouse later
	if (!(curwin->flags & UIWindowFlags_NoTitleBar)) {
		if(Math::PointInRectangle(mp, curwin->position, vec2(curwin->width, style.titleBarHeight))){
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
			curwin->scy = Math::clamp(curwin->scy, 0.f, curwin->maxScroll.y);
		}
		else if (curwin->hovered && DeshInput->ScrollDown()) {
			curwin->scy += style.scrollAmount.y;
			curwin->scy = Math::clamp(curwin->scy, 0.f, curwin->maxScroll.y);
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
	for (UIItem& item : curwin->items) {
		max.x = Max(max.x, (item.position.x + curwin->scx) + item.size.x);
		max.y = Max(max.y, (item.position.y + curwin->scy) + item.size.y);
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
									curwin->y + (style.titleBarHeight - style.font->max_height) * style.titleTextAlign.y);
			drawCmd.color = Color_White;
			drawCmd.scissorExtent = vec2{ curwin->width, style.titleBarHeight };
			drawCmd.scissorOffset = curwin->position;
			drawCmd.font = style.font;
			
			//TODO(sushi, Ui) add title text coloring
			
			base.drawCmds.add(drawCmd); //inst 54
		}
	}
	
	{//draw titlebar minimize button and check for it being clicked
		if (!((curwin->flags & UIWindowFlags_NoMinimizeButton) || (curwin->flags & UIWindowFlags_NoMinimizeButton))) {
			UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
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
	
	UIItem item{ UIItemType_Base, curwin->cursor, style };
	item.position = vec2::ZERO;
	
	vec2 mp = DeshInput->mousePos;
	
	vec2 minSizeForFit = CalcWindowMinSize();
	
	if ((curwin->flags & UIWindowFlags_FitAllElements)) 
		curwin->dimensions = minSizeForFit;
	
	
	//if the window isn't invisible draw things that havent been disabled
	if ((curwin->flags & UIWindowFlags_Invisible) != UIWindowFlags_Invisible) {
		//draw background
		if (!(curwin->flags & UIWindowFlags_NoBackground) && !curwin->minimized) {
			UIDrawCmd drawCmd{ UIDrawType_FilledRectangle }; 
			drawCmd.position = vec2::ZERO;
			drawCmd.dimensions = curwin->dimensions;
			drawCmd.color = style.colors[UIStyleCol_WindowBg];
			
			item.drawCmds.add(drawCmd); 
		}
		
		//draw border
		if (!(curwin->flags & UIWindowFlags_NoBorder) && !curwin->minimized) {
			UIDrawCmd drawCmd{ UIDrawType_Rectangle }; //inst 58
			drawCmd.color = style.colors[UIStyleCol_Border];
			drawCmd.position = vec2::ZERO;
			drawCmd.dimensions = curwin->dimensions;
			drawCmd.scissorOffset = -vec2::ONE * 2;
			drawCmd.scissorExtent = curwin->dimensions + vec2::ONE * 2;
			
			item.drawCmds.add(drawCmd);
		}
	}
	
	curwin->style = style;
	curwin->baseItems.add(item);
	
	//check to see if the elements we have drawn so far have gone beyond the window's size
	//and allow scrolling if it did, as well as define a max scrolling amount
	if (curwin->dimensions.y < minSizeForFit.y)
		curwin->maxScroll.y = minSizeForFit.y - curwin->dimensions.y;
	else
		curwin->maxScroll.y = 0;
	
	//Log("ok", minSizeForFit);
	
	NextWinPos = vec2(-1, 0); NextWinSize = vec2(-1, 0);
	
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
	
	if ((curwin->flags & UIWindowFlags_FitAllElements))
		curwin->dimensions = minSizeForFit;
	
	
	//if the window isn't invisible draw things that havent been disabled
	if ((curwin->flags & UIWindowFlags_Invisible) != UIWindowFlags_Invisible) {
		//draw background
		if (!(curwin->flags & UIWindowFlags_NoBackground) && !curwin->minimized) {
			UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
			drawCmd.position = vec2::ZERO;
			drawCmd.dimensions = curwin->dimensions;
			drawCmd.color = style.colors[UIStyleCol_WindowBg];
			
			item.drawCmds.add(drawCmd);
		}
		
		//draw border
		if (!(curwin->flags & UIWindowFlags_NoBorder) && !curwin->minimized) {
			UIDrawCmd drawCmd{ UIDrawType_Rectangle }; //inst 58
			drawCmd.color = style.colors[UIStyleCol_Border];
			drawCmd.position = vec2::ZERO;
			drawCmd.dimensions = curwin->dimensions;
			drawCmd.scissorOffset = -vec2::ONE * 2;
			drawCmd.scissorExtent = curwin->dimensions + vec2::ONE * 2;
			
			item.drawCmds.add(drawCmd);
		}
	}
	
	curwin->style = style;
	curwin->baseItems.add(item);
	
	//check to see if the elements we have drawn so far have gone beyond the window's size
	//and allow scrolling if it did, as well as define a max scrolling amount
	if (curwin->dimensions.y < minSizeForFit.y)
		curwin->maxScroll.y = minSizeForFit.y - curwin->dimensions.y;
	else
		curwin->maxScroll.y = 0;
	
	NextWinPos = vec2(-1, 0); NextWinSize = vec2(-1, 0);
	
	//update stored window with new window state
	curwin = *windowStack.last;
	windowStack.pop();
}

void UI::SetNextWindowPos(vec2 pos) {
	NextWinPos = pos;
}

void UI::SetNextWindowPos(float x, float y) {
	NextWinPos = vec2(x,y);
}

void UI::SetNextWindowSize(vec2 size) {
	NextWinSize = size.yAdd(style.titleBarHeight);
}

void UI::SetNextWindowSize(float x, float y) {
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

void UI::ShowDebugWindowOf(const char* name) {
	if (UIWindow* debugee = *windows.at(name)) {
		
		persist bool show_drawcall_sizes    = 1;
		persist bool show_drawcall_scissors = 0;
		persist bool show_cursor = 0;
		
		string info =
			TOSTRING("    position: ", debugee->position, "\n") +
			TOSTRING("  dimensions: ", debugee->dimensions, "\n") +
			TOSTRING("      scroll: ", debugee->scroll, "\n") +
			TOSTRING("   maxScroll: ", debugee->maxScroll, "\n") +
			TOSTRING("     hovered: ", (debugee->hovered) ? "true" : "false", "\n") +
			TOSTRING("titleHovered: ", (debugee->titleHovered) ? "true" : "false", "\n") +
			TOSTRING("      cursor: ", debugee->cursor);
		
		PushVar(UIStyleVar_ItemSpacing, vec2(5, 1));
		SetNextWindowSize(CalcTextSize(info) + vec2(style.windowPadding.x * 2, style.windowPadding.y * 2));
		Begin(TOSTRING("#", name, " debug", "#").str, debugee->position + vec2::ONE * 30, debugee->dimensions, UIWindowFlags_FitAllElements);
		
		Text(info.str);
		
		Checkbox("cursor",         &show_cursor);
		Checkbox("Item boxes",     &show_drawcall_sizes);
		Checkbox("Item scissors",  &show_drawcall_scissors);
		
		//if (show_cursor) {
		//	vec2 cursize = vec2::ONE * 2;
		//	UIItem item{ UIItemType_Button, curwin->cursor, style };
		//	UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
		//	drawCmd.position = debugee->position + (debugee->cursor - cursize / 2) - debugee->scroll + debugee->style.windowPadding;
		//	drawCmd.dimensions = cursize;
		//	drawCmd.color = Color_White;
		//	drawCmd.scissorExtent = DeshWindow->dimensions;
		//	drawCmd.trackedForFit = 0;
		//    
		//	curwin->drawCmds.add(drawCmd);
		//}
		
		
		if (show_drawcall_sizes) {
			UIDrawCmd drawCmd{ UIDrawType_Rectangle };
			drawCmd.color = Color_Red;
			drawCmd.scissorExtent = DeshWindow->dimensions;
			drawCmd.trackedForFit = 0;
			
			for (UIItem& i : debugee->items) {
				drawCmd.position = debugee->position + i.position;
				drawCmd.dimensions = i.size;
				
				debugCmds.add(drawCmd);
			}
		}
		
		//if (show_drawcall_scissors) {
		//	UIDrawCmd drawCmd{ UIDrawType_Rectangle };
		//	drawCmd.color = color::GREEN;
		//	drawCmd.scissorExtent = DeshWindow->dimensions;
		//	drawCmd.trackedForFit = 0;
		//	for (UIDrawCmd& d : debugee->drawCmds) {
		//		drawCmd.position = d.scissorOffset;
		//		drawCmd.dimensions = d.scissorExtent;
		//		curwin->drawCmds.add(drawCmd);
		//	}
		//}
		
		End();
		PopVar();
	}
	else {
		LogE("ui","ShowDebugWindowOf() called with unknown window ", name, "!");
	}
}


//initializes core UI with an invisible working window covering the entire screen
//also initializes styles
//the base window should never focus when clicking within it, so any widgets drawn within
//it will not focus if theres a window in front of them.
//I'm not sure how i want to fix it yet
void UI::Init() {
	curwin = new UIWindow();
	curwin->name = "Base";
	curwin->position = vec2(0,0);
	curwin->dimensions = DeshWindow->dimensions;
	
	//load font
	style.font = Storage::CreateFontFromFileBDF("gohufont-11.bdf").second;
	//style.font = Storage::CreateFontFromFileTTF("Paskowy.ttf", 72).second;
	//style.font = Storage::CreateFontFromFileTTF("comixxx4.otf", 36).second;
	
	//push default color scheme
	//this is never meant to be popped
	PushColor(UIStyleCol_Border,   colors.near_black);
	PushColor(UIStyleCol_WindowBg, colors.midnight_blue);
	PushColor(UIStyleCol_TitleBg,  colors.purple_gray);
	PushColor(UIStyleCol_FrameBg,  colors.pink_gray);
	PushColor(UIStyleCol_Text,     Color_White);
	
	//push default style variables
	PushVar(UIStyleVar_WindowBorderSize,    1);
	PushVar(UIStyleVar_TitleBarHeight,      style.font->max_height * 1.2);
	PushVar(UIStyleVar_TitleTextAlign,      vec2(1, 0.5));
	PushVar(UIStyleVar_WindowPadding,       vec2(10, 10));
	PushVar(UIStyleVar_ItemSpacing,         vec2(1, 1));
	PushVar(UIStyleVar_ScrollAmount,        vec2(5, 5));
	PushVar(UIStyleVar_CheckboxSize,        vec2(10, 10));
	PushVar(UIStyleVar_CheckboxFillPadding, 2);
	PushVar(UIStyleVar_InputTextTextAlign,  vec2(0, 0.5));
	PushVar(UIStyleVar_ButtonTextAlign,     vec2(0.5, 0.5));
	PushVar(UIStyleVar_RowItemAlign,        vec2(0.5, 0.5));
	PushVar(UIStyleVar_FontHeight,          20);
	
	initColorStackSize = colorStack.count;
	initStyleStackSize = varStack.count;
	
	windows.add("base", curwin);
	windowStack.add(curwin);
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
	
	
	{ //drag
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
		//window position and size corrected for titlebar 
		vec2 winpos = vec2(p->x, p->y + p->titleBarHeight);
		vec2 winscissor{ Max(0, winpos.x), Max(0, winpos.y) }; //NOTE scissor offset cant be negative
		vec2 winsiz = vec2(p->width, p->height - p->titleBarHeight);
		
		if (p->hovered && !(p->flags & UIWindowFlags_DontSetGlobalHoverFlag))
			globalHovered = 1;
		
		//draw base cmds first
		for (UIItem& item : p->baseItems) {
			vec2 itempos = (item.type == UIItemType_Abstract ? item.position : winpos + item.position);
			vec2 itemsiz = item.size;
			
			for (UIDrawCmd& drawCmd : item.drawCmds) {
				vec2   dcpos = itempos + drawCmd.position;
				vec2  dcpos2 = itempos + drawCmd.position2;
				vec2   dcsiz = drawCmd.dimensions;
				vec2    dcse = drawCmd.scissorExtent;
				vec2    dcso = itempos + drawCmd.scissorOffset;
				dcso.x = Max(0, dcso.x); dcso.y = Max(0, dcso.y); //NOTE scissor offset cant be negative
				color  dccol = drawCmd.color;
				float    dct = drawCmd.thickness;
				
				cstring dctex{drawCmd.text.str,drawCmd.text.count};
				Font*   font = drawCmd.font;
				
				switch (drawCmd.type) {
					case UIDrawType_FilledRectangle: {
						Render::FillRectUI(dcpos, dcsiz, dccol, dcso, dcse);
					}break;
					
					case UIDrawType_Line: {
						Render::DrawLineUI(dcpos, dcpos2, dct, dccol, dcso, dcse);
					}break;
					
					case UIDrawType_Text: {
						if (drawCmd.scissorExtent.x == -1) {
							Render::DrawTextUI(font, dctex, dcpos, dccol, winscissor, winsiz);
						}
						else {
							Render::DrawTextUI(font, dctex, dcpos, dccol, dcso, dcse);
						}
					}break;
					case UIDrawType_Rectangle: {
						if (drawCmd.scissorExtent.x == -1)
							Render::DrawRectUI(dcpos, dcsiz, dccol, winscissor, winsiz);
						else
							Render::DrawRectUI(dcpos, dcsiz, dccol, dcso, dcse);
					}break;
				}
			}
		}
		
		//dont draw non-base draw cmds if we're minimized
		if (!p->minimized) {
			for (UIItem& item : p->items) {
				vec2 itempos = (item.type == UIItemType_Abstract ? item.position : winpos + item.position);
				vec2 itemsiz = item.size;
				
				for (UIDrawCmd& drawCmd : item.drawCmds) {
					vec2   dcpos = itempos + drawCmd.position;
					vec2  dcpos2 = itempos + drawCmd.position2;
					vec2   dcsiz = drawCmd.dimensions;
					vec2    dcse = drawCmd.scissorExtent;
					vec2    dcso = itempos + drawCmd.scissorOffset;
					dcso.x = Max(0, dcso.x); dcso.y = Max(0, dcso.y); //NOTE scissor offset cant be negative
					color  dccol = drawCmd.color;
					float    dct = drawCmd.thickness;
					
					cstring dctex{drawCmd.text.str,drawCmd.text.count};
					Font*   font = drawCmd.font;
					
					switch (drawCmd.type) {
						case UIDrawType_FilledRectangle: {
							if (drawCmd.scissorExtent.x == -1)
								Render::FillRectUI(dcpos, dcsiz, dccol, winscissor, winsiz);
							else
								Render::FillRectUI(dcpos, dcsiz, dccol, dcso, dcse);
							
						}break;
						
						case UIDrawType_Line: {
							if (drawCmd.scissorExtent.x == -1)
								Render::DrawLineUI(dcpos - itempos, dcpos2 - itempos, dct, dccol, winscissor, winsiz);
							else
								Render::DrawLineUI(dcpos - itempos, dcpos2 - itempos, dct, dccol, dcso - itempos, dcse);
						}break;
						
						case UIDrawType_Text: {
							if (drawCmd.scissorExtent.x == -1)
								Render::DrawTextUI(font, dctex, dcpos, dccol, vec2::ONE * ((item.style.font->type != FontType_BDF) ? item.style.fontHeight / item.style.font->max_height : 1), winscissor, winsiz);
							else
								Render::DrawTextUI(font, dctex, dcpos, dccol, dcso, dcse);
						}break;
						
						case UIDrawType_Rectangle: {
							if (drawCmd.scissorExtent.x == -1)
								Render::DrawRectUI(dcpos, dcsiz, dccol, winscissor, winsiz);
							else
								Render::DrawRectUI(dcpos, dcsiz, dccol, dcso, dcse);
						}break;
					}
				}
			}
		}
		p->baseItems.clear();
		p->items.clear();
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
	
	//draw all debug commands if there are any
	
	for (UIDrawCmd& drawCmd : debugCmds) {
		vec2   dcpos = drawCmd.position;
		vec2  dcpos2 = drawCmd.position2;
		vec2   dcsiz = drawCmd.dimensions;
		vec2    dcse = drawCmd.scissorExtent;
		vec2    dcso = drawCmd.scissorOffset;
		dcso.x = Max(0, dcso.x); dcso.y = Max(0, dcso.y); //NOTE scissor offset cant be negative
		color  dccol = drawCmd.color;
		float    dct = drawCmd.thickness;
		cstring dctex{drawCmd.text.str,drawCmd.text.count};
		Font*   font = drawCmd.font;
		
		switch (drawCmd.type) {
			case UIDrawType_FilledRectangle: {
				if (drawCmd.scissorExtent.x == -1)
					Render::FillRectUI(dcpos, dcsiz, dccol, vec2::ZERO, DeshWindow->dimensions);
				else
					Render::FillRectUI(dcpos, dcsiz, dccol, dcso, dcse);
				
			}break;
			
			case UIDrawType_Line: {
				if (drawCmd.scissorExtent.x == -1)
					Render::DrawLineUI(dcpos, dcpos2, dct, dccol, vec2::ZERO, DeshWindow->dimensions);
				else
					Render::DrawLineUI(dcpos, dcpos2, dct, dccol, dcso, dcse);
			}break;
			
			case UIDrawType_Text: {
				if (drawCmd.scissorExtent.x == -1)
					Render::DrawTextUI(font, dctex, dcpos, dccol, vec2::ONE * ((font->type != FontType_BDF) ? style.fontHeight / font->max_height : 1), vec2::ZERO, DeshWindow->dimensions);
				else
					Render::DrawTextUI(font, dctex, dcpos, dccol, dcso, dcse);
			}break;
			
			case UIDrawType_Rectangle: {
				if (drawCmd.scissorExtent.x == -1)
					Render::DrawRectUI(dcpos, dcsiz, dccol, vec2::ZERO, DeshWindow->dimensions);
				else
					Render::DrawRectUI(dcpos, dcsiz, dccol, dcso, dcse);
			}break;
		}
	}
	
	debugCmds.clear();
}
