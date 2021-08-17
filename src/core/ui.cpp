#include "ui.h"
//color pallete 
//current palette:
//https://lospec.com/palette-list/slso8
//TODO(sushi, Ui) implement menu style file loading sort of stuff yeah
//TODO(sushi, Ui) standardize what UI element each color belongs to
local struct {
	color          midnight_blue = color(0x0d2b45); //midnight blue
	color         dark_grey_blue = color(0x203c56); //dark gray blue
	color            purple_gray = color(0x544e68); //purple gray
	color              pink_gray = color(0x8d697a); //pink gray
	color        bleached_orange = color(0xd08159); //bleached orange
	color bright_bleached_orange = color(0xffaa5e); //above but brighter
	color             skin_white = color(0xffd4a3); //skin white
	color      bright_skin_white = color(0xffecd6); //even whiter skin
	color             near_black = color(0x141414); //almost black
} colors;

//global styling
UIStyle style;

//for color stack, saves what element was changed and what it's old color was 
struct ColorMod {
	UIStyleCol element;
	color oldCol;
};

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
local map<string, UIWindow*>        windows;     
local map<string, UIInputTextState> inputTexts;  //stores known input text labels and their state
local array<UIWindow*>              windowStack; //window stack which allow us to use windows like we do colors and styles
local array<ColorMod>               colorStack; 
local array<VarMod>                 varStack; 

local u32 initColorStackSize;
local u32 initStyleStackSize;

//set if any window other than base is hovered
local bool globalHovered = false;
local bool draggingWin = false; //if a user moves their mouse too fast while dragging, the globalHover flag can be set to false

local u32 activeId = -1; //the id of an active widget eg. input text


//row variables
local u32        rowCount = 0;
local UIRowFlags rowFlags = 0;


//helper defines


#define workingWinPositionPlusTitlebar vec2(curwin->x, curwin->y + ((curwin->flags & UIWindowFlags_NoTitleBar) ? 0 :style.titleBarHeight));
#define workingWinSizeMinusTitlebar    vec2(curwin->width, curwin->height - ((curwin->flags & UIWindowFlags_NoTitleBar) ? 0 : style.titleBarHeight));
#define workingHasFlag(flag) (curwin->flags & flag)
#define HasFlag(flag) (flags & flag)

//helper functions


//this calculates text taking into account newlines, BUT NOT WRAPPING
//useful for sizing a window to fit some text
//TODO(sushi, Ui) make a CalcTextSize that takes into account wrapping that would occur in a sized window
inline vec2 UI::CalcTextSize(string text) {
	string stage = text;
	u32 longest = 0;
    u32 nlp = stage.findFirstChar('\n');
	while (nlp != string::npos) {
		if (nlp - 1 > longest)
			longest = nlp - 1;
		stage = stage.substr(nlp + 1);
		nlp = stage.findFirstChar('\n');
	}
	
	if (stage.size > longest)
		longest = stage.size;
	
	return vec2(longest * style.font->width, style.font->height * (text.charCount('\n') + 1));
}

//calculates the items position and size based on its draw commands
//should really only be used when doing this manually is too annoying
inline void CalcItemSize(UIItem& item) {
	using namespace UI;

	vec2 max;
	for (UIDrawCmd& drawCmd : item.drawCmds) {
		switch (drawCmd.type) {
			case UIDrawType_Rectangle:
			case UIDrawType_FilledRectangle: {
				max.x = Max(max.x, drawCmd.position.x + drawCmd.dimensions.x);
				max.y = Max(max.y, drawCmd.position.y + drawCmd.dimensions.y);
			}break;
			case UIDrawType_Line: {
				vec2 ulm{ Min(drawCmd.position.x, drawCmd.position2.x), Min(drawCmd.position.y, drawCmd.position2.y) };
				vec2 lrm{ Max(drawCmd.position.x, drawCmd.position2.x), Max(drawCmd.position.y, drawCmd.position2.y) };
				lrm -= item.position;
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
	item.size = max;
}


void UI::SetNextItemActive() {
	NextActive = 1;
}

UIStyle UI::GetStyle(){
	return style;
}

//the following 3 functions should probably error out sofly, rather than asserting

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
UIItem* GetLastItem() {
	return curwin->items.last;
}


void UI::Row(u32 num_items, UIRowFlags flags){
	rowCount = num_items;
	rowFlags = flags;
}

//internal master cursor controller
//  an attempt to centralize all cursor movement
//  this should be called with a requested new position and the function will decide
//  wether or not the cursor should be moved there or somewhere else based on certain conditions
//  such as if we're placing items in a row
void AdvanceCursor(vec2 nupos){

}


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


//Line


void UI::Line(vec2 start, vec2 end, float thickness, color color){
	UIItem       item{ UIItemType_Abstract, curwin->cursor, style };
	UIDrawCmd drawCmd{ UIDrawType_Line };
	drawCmd. position = curwin->position + start;
	drawCmd.position2 = curwin->position + end;
	drawCmd.thickness = thickness;
	drawCmd.    color = color;
	
	item.position = vec2{ Min(drawCmd.position.x, drawCmd.position2.x), Min(drawCmd.position.y, drawCmd.position2.y) };
	item.    size = vec2{ Max(drawCmd.position.x, drawCmd.position2.x), Max(drawCmd.position.y, drawCmd.position2.y) } - item.position;
	
	item.drawCmds.add(drawCmd);
	curwin->items.add(item);
}


//Text


//internal function for actually making and adding the drawCmd
local void TextCall(string text, vec2 pos, color color, UIItem& item) {
	UIDrawCmd drawCmd{ UIDrawType_Text };
	drawCmd.text = text;
	drawCmd.position = pos;
	drawCmd.color = color;
	drawCmd.scissorOffset = pos;//vec2(curwin->x, curwin->y + ((curwin->flags & UIWindowFlags_NoTitleBar) ? 0 :style.titleBarHeight));
	drawCmd.scissorExtent = UI::CalcTextSize(text);//vec2(curwin->width, curwin->height - ((curwin->flags & UIWindowFlags_NoTitleBar) ? 0 : style.titleBarHeight));
	
	item.drawCmds.add(drawCmd);
}

//main function for wrapping, where position is starting position of text relative to the top left of the window
//TODO(sushi) clean this up 
inline local void WrapText(string text, vec2 pos, color color, bool move_cursor = true) {
	using namespace UI;
	UIItem item{ UIItemType_Text, curwin->cursor, style };

	//we split string by newlines and put them into here 
	//maybe make this into its own function
	array<string> newlined;

	u32 newline = text.findFirstChar('\n');
	if (newline != string::npos && newline != text.size - 1) {
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
	
	item.position = pos;
	//apply window padding if we're not manually positioning text
	if (move_cursor)
		item.position += style.windowPadding - curwin->scroll;

	
	//max characters we can place 
	u32 maxChars = floor(((curwin->width - style.windowPadding.x) - workcur.x) / style.font->width);
	
	//make sure max chars never equals 0
	if (!maxChars) maxChars++;
	
	//wrap each string in newline array
	for (string& t : newlined) {
		//we need to see if the string goes beyond the width of the window and wrap if it does
		if (maxChars < t.size) {
			//if this is true we know item's total width is just maxChars times font width
			item.size.x = maxChars * style.font->width;


			//find closest space to split by
			u32 splitat = t.findLastChar(' ', maxChars);
			string nustr = t.substr(0, (splitat == string::npos) ? maxChars - 1 : splitat);
			TextCall(nustr, workcur, color, item);

			t = t.substr(nustr.size);
			workcur.y += style.font->height + style.itemSpacing.y;

			//continue to wrap if we need to
			while (t.size > maxChars) {
				splitat = t.findLastChar(' ', maxChars);
				nustr = t.substr(0, (splitat == string::npos) ? maxChars - 1 : splitat);
				TextCall(nustr, workcur, color, item);

				t = t.substr(nustr.size);
				workcur.y += style.font->height + style.itemSpacing.y;

				if (!strlen(t.str)) break;
			}

			//write last bit of text
			TextCall(t, workcur, color, item);
			workcur.y += style.font->height + style.itemSpacing.y;

		}
		else {
			//we have to get max string length to determine item's width here
			item.size.x = Max(style.font->width * t.size, item.size.x);

			TextCall(t, workcur, color, item);
			workcur.y += style.font->height + style.itemSpacing.y;
		}
	}
	
	item.size.y = workcur.y - curwin->position.y;

	CalcItemSize(item);
	if (move_cursor) {
		workcur -= style.windowPadding - curwin->scroll;
		curwin->cursor.y += item.size.y + style.itemSpacing.y;
	}

	curwin->items.add(item);
}

//TODO(sushi) make NoWrap also check for newlines
void UI::Text(string text, UITextFlags flags) {
	if (flags & UITextFlags_NoWrap) {
		UIItem item{ UIItemType_Text, curwin->cursor, style };
		TextCall(text, curwin->position + curwin->cursor + style.windowPadding - curwin->scroll, style.colors[UIStyleCol_Text], item);
		curwin->cury += style.font->height + style.itemSpacing.y;
		curwin->curx = 0;

	}
	else {
		WrapText(text, curwin->cursor, style.colors[UIStyleCol_Text]);
	}
}

void UI::Text(string text, vec2 pos, UITextFlags flags) {
	if (flags & UITextFlags_NoWrap) {
		UIItem item{ UIItemType_Text, curwin->cursor, style };
		TextCall(text, curwin->position + pos - curwin->scroll, style.colors[UIStyleCol_Text], item);
	}
	else {
		WrapText(text, pos, style.colors[UIStyleCol_Text], 0);
	}
}

void UI::Text(string text, color color, UITextFlags flags) {
	if (flags & UITextFlags_NoWrap) {
		UIItem item{ UIItemType_Text, curwin->cursor, style };
		TextCall(text, curwin->position + curwin->cursor + style.windowPadding - curwin->scroll, color, item);
		curwin->cury += style.font->height + style.itemSpacing.y;
		curwin->curx = 0;

	}
	else {
		WrapText(text, curwin->cursor, color);
	}
	
}

void UI::Text(string text, vec2 pos, color color, UITextFlags flags) {
	if (flags & UITextFlags_NoWrap) {
		UIItem item{ UIItemType_Text, curwin->cursor, style };
		TextCall(text, curwin->position + pos - curwin->scroll, color, item);
	}
	else {
		WrapText(text, pos, color);
	}
}


//Windows


//begins a window with a name, position, and dimensions along with some optional flags
//if begin window is called with a name that was already called before it will work with
//the data that window previously had
void UI::BeginWindow(string name, vec2 pos, vec2 dimensions, UIWindowFlags flags) {
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
		if (curwin->hovered && DeshInput->KeyPressedAnyMod(MouseButton::SCROLLUP)) {
			curwin->scy -= style.scrollAmount.y;
			Math::clampr(curwin->scy, 0, curwin->maxScroll.y);
		}
		else if (curwin->hovered && DeshInput->KeyPressedAnyMod(MouseButton::SCROLLDOWN)) {
			curwin->scy += style.scrollAmount.y;
			Math::clampr(curwin->scy, 0, curwin->maxScroll.y);
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

//calculates the minimum size a window can be to contain all drawn elements
vec2 CalcWindowMinSize() {
	using namespace UI;
	vec2 max;
	//for (UIDrawCmd& drawCmd : curwin->drawCmds) {
	//	if (drawCmd.trackedForFit) {
	//		float xbase = drawCmd.position.x + curwin->style.windowPadding.x - curwin->position.x + curwin->scroll.x;
	//		float ybase = drawCmd.position.y + curwin->style.windowPadding.x - curwin->position.y + curwin->scroll.y;
	//		switch (drawCmd.type) {
	//			case UIDrawType_Text: {
	//				vec2 textSize = CalcTextSize(drawCmd.text);
	//				max.x = Max(max.x, (xbase) + textSize.x);
	//				max.y = Max(max.y, (ybase) + textSize.y);
	//			}break;
	//			case UIDrawType_FilledRectangle: {
	//				max.x = Max(max.x, (xbase) + drawCmd.dimensions.x);
	//				max.y = Max(max.y, (ybase) + drawCmd.dimensions.y);
	//			}break;
	//			case UIDrawType_Line: {
	//				max.x = Max(max.x, (xbase));
	//				max.y = Max(max.y, (ybase));
	//				max.x = Max(max.x, (drawCmd.position2.x + curwin->style.windowPadding.x - curwin->position.x + curwin->scroll.x));
	//				max.y = Max(max.y, (drawCmd.position2.y + curwin->style.windowPadding.x - curwin->position.y + curwin->scroll.y));
    //                
	//			}break;
	//		}
	//	}
	//}
	return max;
}

#if 0 Old titlebar code for when i reimplement it as its own call
 

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
					drawCmd.position = vec2(
											curwin->x + (curwin->width - curwin->name.size * style.font->width) * style.titleTextAlign.x,
											curwin->y + (style.titleBarHeight - style.font->height) * style.titleTextAlign.y);
					drawCmd.color = color::WHITE;
					drawCmd.scissorExtent = vec2{ curwin->width, style.titleBarHeight };
					drawCmd.scissorOffset = curwin->position;

					//TODO(sushi, Ui) add title text coloring

					base.drawCmds.add(drawCmd); //inst 54
				}
			}

			{//draw titlebar minimize button and check for it being clicked
				if (!((curwin->flags & UIWindowFlags_NoMinimizeButton) || (curwin->flags & UIWindowFlags_NoMinimizeButton))) {
					UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
					drawCmd.position = vec2(
											curwin->x + (curwin->width - curwin->name.size * style.font->width) * 0.01,
											curwin->y + (style.titleBarHeight * 0.5 - 2));
					drawCmd.dimensions = vec2(10, 4);

					if (Math::PointInRectangle(mp, drawCmd.position, drawCmd.dimensions)) {
						drawCmd.color = style.colors[UIStyleCol_TitleBg] * 0.7;
						if (DeshInput->KeyPressedAnyMod(MouseButton::LEFT)) {
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



void UI::EndWindow() {
	Assert(windowStack.size() > 1, "Attempted to end the base window");
	UIItem item{ UIItemType_Base, curwin->cursor, style };
	item.position = vec2::ZERO;

	vec2 mp = DeshInput->mousePos;
    
	if ((curwin->flags & UIWindowFlags_FitAllElements)) {
		curwin->dimensions = CalcWindowMinSize();
	}
    
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
	if (curwin->cury > curwin->height - curwin->titleBarHeight)
		curwin->maxScroll.y = (curwin->cury + style.windowPadding.y * 2) - curwin->height;
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

void UI::SetWindowName(string name) {
	curwin->name = name;
}

bool UI::IsWinHovered() {
	return curwin->hovered;
}

bool UI::AnyWinHovered() {
	return globalHovered || draggingWin;
}

void UI::ShowDebugWindowOf(string name) {
#if 0
	if (UIWindow* debugee = *windows.at(name)) {
		
		persist bool show_drawcall_pos      = 0;
		persist bool show_drawcall_sizes    = 0;
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
		BeginWindow(TOSTRING("#", name, " debug", "#"), debugee->position + vec2::ONE * 30, debugee->dimensions, UIWindowFlags_FitAllElements);
        
		Text(info);
        
		Checkbox("cursor",             &show_cursor);
		Checkbox("drawCall positions", &show_drawcall_pos);
		Checkbox("drawCall sizes",     &show_drawcall_sizes);
		Checkbox("drawCall scissors",  &show_drawcall_scissors);
        
		if (show_cursor) {
			vec2 cursize = vec2::ONE * 2;
			UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
			drawCmd.position = debugee->position + (debugee->cursor - cursize / 2) - debugee->scroll + debugee->style.windowPadding;
			drawCmd.dimensions = cursize;
			drawCmd.color = color::WHITE;
			drawCmd.scissorExtent = DeshWindow->dimensions;
			drawCmd.trackedForFit = 0;
            
			curwin->drawCmds.add(drawCmd);
		}
        
		if (show_drawcall_pos) {
			UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
			drawCmd.dimensions = vec2::ONE * 4;
			drawCmd.color = color::RED;
			drawCmd.scissorExtent = DeshWindow->dimensions;
			drawCmd.trackedForFit = 0;
			for (UIDrawCmd& d : debugee->drawCmds) {
				drawCmd.position = d.position;
				curwin->drawCmds.add(drawCmd);
			}
		}
        
		if (show_drawcall_sizes) {
			UIDrawCmd drawCmd{ UIDrawType_Rectangle };
			drawCmd.color = color::RED;
			drawCmd.scissorExtent = DeshWindow->dimensions;
			drawCmd.trackedForFit = 0;
			for (UIDrawCmd& d : debugee->drawCmds) {
				drawCmd.position = d.position;
				switch(d.type){
					case UIDrawType_Rectangle:
					case UIDrawType_FilledRectangle: {
						drawCmd.dimensions = d.dimensions;
					}break;
                    
					//TODO(sushi) make this work right later
					case UIDrawType_Line: {
						drawCmd.dimensions = d.position2 - d.position;
					}break;
                    
					case UIDrawType_Text: {
						drawCmd.dimensions = CalcTextSize(d.text);
					}break;
				}
				curwin->drawCmds.add(drawCmd);
			}
		}
        
		if (show_drawcall_scissors) {
			UIDrawCmd drawCmd{ UIDrawType_Rectangle };
			drawCmd.color = color::GREEN;
			drawCmd.scissorExtent = DeshWindow->dimensions;
			drawCmd.trackedForFit = 0;
			for (UIDrawCmd& d : debugee->drawCmds) {
				drawCmd.position = d.scissorOffset;
				drawCmd.dimensions = d.scissorExtent;
				curwin->drawCmds.add(drawCmd);
			}
		}
        
		EndWindow();
		PopVar();
	}
	else {
		LOG("UI::ShowDebugWindowOf() called with unknown window ", name, "!");
	}
#endif
	
}

//Push/Pop functions
void UI::PushColor(UIStyleCol idx, color color) {
	//save old color
	colorStack.add(ColorMod{ idx, style.colors[idx] });
	//change to new color
	style.colors[idx] = color;
}

void UI::PushVar(UIStyleVar idx, float nuStyle){
	Assert(uiStyleVarTypes[idx].count == 1, "Attempt to use a float on a vec2 style variable!");
	varStack.add(VarMod(idx, nuStyle));
	float* p = (float*)((u8*)&style + uiStyleVarTypes[idx].offset);
	*p = nuStyle;
}

void UI::PushVar(UIStyleVar idx, vec2 nuStyle) {
	Assert(uiStyleVarTypes[idx].count == 2, "Attempt to use a float on a vec2 style variable!");
	varStack.add(VarMod(idx, nuStyle));
	vec2* p = (vec2*)((u8*)&style + uiStyleVarTypes[idx].offset);
	*p = nuStyle;
}

//we always leave the current color on top of the stack and the previous gets popped
void UI::PopColor(u32 count) {
	//Assert(count < colorStack.size() - 1, "Attempt to pop too many colors!");
	while (count-- > 0) {
		style.colors[(colorStack.last)->element] = colorStack.last->oldCol;
		colorStack.pop();
	}
}

void UI::PopVar(u32 count){
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



void UI::SetNextItemSize(vec2 size) {
	NextItemSize = size;
}

//widget stuff
bool UI::Button(string text) {
	UIDrawCmd drawCmd{ UIDrawType_FilledRectangle }; return true;
}

bool UI::Button(string text, vec2 pos){
	return true;
}

bool UI::Button(string text, color color){
	return true;
}

bool UI::Button(string text, vec2 pos, color color){
	return true;
}


void UI::Checkbox(string label, bool* b) {
	UIItem item{ UIItemType_Checkbox, curwin->cursor, style };

	vec2 boxpos = curwin->position + curwin->cursor + style.windowPadding - curwin->scroll;
	vec2 boxsiz = style.checkboxSize;
    
	item.position = boxpos;
	item.size = boxsiz;

	{//box
		UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
		drawCmd.position = vec2{ 0,0 };
		drawCmd.dimensions = boxsiz;
		drawCmd.color = style.colors[UIStyleCol_FrameBg];
        
		item.drawCmds.add(drawCmd);
	}
    
	//fill if true
	int fillPadding = style.checkboxFillPadding;
	if (*b) {
		UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
		drawCmd.position = boxsiz * vec2(fillPadding / boxsiz.x, fillPadding / boxsiz.y);
		drawCmd.dimensions = boxsiz  * (vec2::ONE - 2 * vec2(fillPadding / boxsiz.x, fillPadding / boxsiz.y));
		drawCmd.color = style.colors[UIStyleCol_FrameBg] * 0.7;
        
		item.drawCmds.add(drawCmd);
	}
    
	{//label
		UIDrawCmd drawCmd{ UIDrawType_Text };
		drawCmd.position = vec2(boxsiz.x + style.itemSpacing.x, (boxsiz.y - style.font->height) * 0.5);
		drawCmd.text = label;
		drawCmd.color = style.colors[UIStyleCol_Text];
        
		item.drawCmds.add(drawCmd);
	}
    
	if (DeshInput->LMousePressed() && Math::PointInRectangle(DeshInput->mousePos, boxpos, boxsiz))
		*b = !*b;

	curwin->cury += boxsiz.y + style.itemSpacing.y;
	curwin->items.add(item);
}


//final input text
bool InputTextCall(string label, string& buffer, u32 maxChars, vec2 position, vec2 dimensions, UIInputTextCallback callback, UIInputTextFlags flags, bool moveCursor) {
	UIItem item{ UIItemType_InputText, curwin->cursor, style };

	UIInputTextState* state;
    
	if (!(state = inputTexts.at(label))) {
		state = inputTexts.atIdx(inputTexts.add(label));
		state->buffer = buffer;
		state->cursor = buffer.size;
		state->id = hash<string>{}(label);
		state->selectStart = 0;
		state->selectEnd = 0;
		state->cursorBlinkTime = 5;
	}
	else {
		state->callback = callback;
		state->buffer = buffer;
	}
    
	if (buffer.size < state->cursor)
		state->cursor = buffer.size;
    
	//data for callback function
	UIInputTextCallbackData data;
	data.flags = flags;
	data.buffer = &buffer;
	data.selectionStart = state->selectStart;
	data.selectionEnd = state->selectEnd;
    
	//check for mouse click or next active to set active 
	if (NextActive || DeshInput->KeyPressedAnyMod(MouseButton::LEFT)) {
		if (NextActive || Math::PointInRectangle(DeshInput->mousePos, position, dimensions)) {
			activeId = state->id;
			NextActive = 0;
		}
		else {
			if (activeId == state->id) activeId = -1;
		}
	}
    
	bool bufferChanged = 0;
	if (activeId == state->id) {
		if (DeshInput->KeyPressedAnyMod(Key::RIGHT) && state->cursor < buffer.size) state->cursor++;
		if (DeshInput->KeyPressedAnyMod(Key::LEFT) && state->cursor > 0) state->cursor--;
        
		data.cursorPos = state->cursor;
        
		//check if the user used up/down keys
		if (DeshInput->KeyPressedAnyMod(Key::UP) && (flags & UIInputTextFlags_CallbackUpDown)) {
			data.eventFlag = UIInputTextFlags_CallbackUpDown;
			data.eventKey = Key::UP;
			callback(&data);
		}
		if (DeshInput->KeyPressedAnyMod(Key::DOWN) && (flags & UIInputTextFlags_CallbackUpDown)) {
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
        
		char charPlaced;
		auto placeKey = [&](u32 i, u32 ins, char toPlace) {
			if (i >= Key::A && i <= Key::Z) {
				if (DeshInput->capsLock || DeshInput->KeyDownAnyMod(Key::LSHIFT) || DeshInput->KeyDownAnyMod(Key::RSHIFT))
					buffer.insert(toPlace, ins);
				else
					buffer.insert(toPlace + 32, ins);
			}
			else if (i >= Key::K0 && i <= Key::K9) {
				if (DeshInput->KeyDownAnyMod(Key::LSHIFT) || DeshInput->KeyDownAnyMod(Key::RSHIFT)) {
					switch (i) {
						case Key::K0: data.character = ')'; buffer.insert(')', ins); break;
						case Key::K1: data.character = '!'; buffer.insert('!', ins); break;
						case Key::K2: data.character = '@'; buffer.insert('@', ins); break;
						case Key::K3: data.character = '#'; buffer.insert('#', ins); break;
						case Key::K4: data.character = '$'; buffer.insert('$', ins); break;
						case Key::K5: data.character = '%'; buffer.insert('%', ins); break;
						case Key::K6: data.character = '^'; buffer.insert('^', ins); break;
						case Key::K7: data.character = '&'; buffer.insert('&', ins); break;
						case Key::K8: data.character = '*'; buffer.insert('*', ins); break;
						case Key::K9: data.character = '('; buffer.insert('(', ins); break;
					}
				}
				else {
					data.character = KeyStringsLiteral[i];
					buffer.insert(KeyStringsLiteral[i], ins);
				}
			}
			else {
				if (DeshInput->KeyDownAnyMod(Key::LSHIFT) || DeshInput->KeyDownAnyMod(Key::RSHIFT)) {
					switch (i) {
						case Key::SEMICOLON:  data.character = ':';  buffer.insert(':', ins);  break;
						case Key::APOSTROPHE: data.character = '"';  buffer.insert('"', ins);  break;
						case Key::LBRACKET:   data.character = '{';  buffer.insert('{', ins);  break;
						case Key::RBRACKET:   data.character = '}';  buffer.insert('}', ins);  break;
						case Key::BACKSLASH:  data.character = '\\'; buffer.insert('\\', ins); break;
						case Key::COMMA:      data.character = '<';  buffer.insert('<', ins);  break;
						case Key::PERIOD:     data.character = '>';  buffer.insert('>', ins);  break;
						case Key::SLASH:      data.character = '?';  buffer.insert('?', ins);  break;
						case Key::MINUS:      data.character = '_';  buffer.insert('_', ins);  break;
						case Key::EQUALS:     data.character = '+';  buffer.insert('+', ins);  break;
						case Key::TILDE:      data.character = '~';  buffer.insert('~', ins);  break;
					}
				}
				else {
					data.character = KeyStringsLiteral[i];
					buffer.insert(KeyStringsLiteral[i], ins);
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
				if (DeshInput->KeyPressedAnyMod(Key::BACKSPACE) && buffer.size > 0 && state->cursor != 0) {
					buffer.erase(--state->cursor);
					bufferChanged = 1;
				}
				else {
					for (int i = 0; i < Key::Key_COUNT; i++) {
						char toPlace = KeyStringsLiteral[i];
						if (DeshInput->KeyPressedAnyMod(i) && buffer.size < maxChars && toPlace != '\0') {
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
					if (DeshInput->KeyDownAnyMod(Key::BACKSPACE) && buffer.size > 0 && state->cursor != 0) {
						buffer.erase(--state->cursor);
						bufferChanged = 1;
					}
					else {
						for (int i = 0; i < Key::Key_COUNT; i++) {
							char toPlace = KeyStringsLiteral[i];
							if (DeshInput->KeyDownAnyMod(i) && buffer.size < maxChars && toPlace != '\0') {
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


	vec2 dim = (dimensions.x == -1) ? vec2(Math::clamp(100, 0, curwin->width - style.windowPadding.x * 2), style.font->height * 1.3) : dimensions;

	item.size = dim;
	item.position = position;

	if(!(flags & UIInputTextFlags_NoBackground)){//text box
		UIDrawCmd drawCmd{ UIDrawType_FilledRectangle };
		drawCmd.position = position;
		drawCmd.dimensions = dim;
		drawCmd.scissorOffset = workingWinPositionPlusTitlebar;
		drawCmd.scissorExtent = workingWinSizeMinusTitlebar;
		drawCmd.color = color::VERY_DARK_GREY;
        
		item.drawCmds.add(drawCmd);
	}

	vec2 textStart = position +
		vec2((dim.x - buffer.size * style.font->width) * style.inputTextTextAlign.x,
			(style.font->height * 1.3 - style.font->height) * style.inputTextTextAlign.y);
    
	{//text
		UIDrawCmd drawCmd{ UIDrawType_Text };
		drawCmd.position = textStart;
		drawCmd.text = buffer;
		drawCmd.color = style.colors[UIStyleCol_Text];
        
		item.drawCmds.add(drawCmd);
	}
    
	//TODO(sushi, Ui) impl different text cursors
	if (activeId == state->id) {//cursor
		UIDrawCmd drawCmd{ UIDrawType_Line };
		drawCmd.position = textStart + vec2(state->cursor * style.font->width, -1);
		drawCmd.position2 = textStart + vec2(state->cursor * style.font->width, style.font->height - 1);
		drawCmd.color =
			color(255, 255, 255,
                  255 * (
                  cos((2 * M_PI) / (state->cursorBlinkTime / 2) * TIMER_END(state->timeSinceTyped) / 1000 -
                  sin((2 * M_PI) / (state->cursorBlinkTime / 2) * TIMER_END(state->timeSinceTyped) / 1000)) + 1) / 2);
		drawCmd.thickness = 1;
        
		item.drawCmds.add(drawCmd);
	}
    
	if (moveCursor) {
		curwin->cursor.y += style.font->height * 1.3 + style.itemSpacing.y;
		curwin->curx = 0;
	}
	curwin->items.add(item);

	if (flags & UIInputTextFlags_EnterReturnsTrue && DeshInput->KeyPressedAnyMod(Key::ENTER) || DeshInput->KeyPressedAnyMod(Key::NUMPADENTER)) {
		return true;
	}
	else if (flags & UIInputTextFlags_AnyChangeReturnsTrue && bufferChanged) {
		return true;
	}
    
	return false;
}

bool UI::InputText(string label, string& buffer, u32 maxChars, UIInputTextFlags flags) {	
	vec2 position = curwin->position + curwin->cursor + style.windowPadding - curwin->scroll;
	position.y -= style.font->height * 1.3 / 4;
	vec2 dimensions = (NextItemSize.x != -1) ? NextItemSize : vec2(Math::clamp(100, 0, curwin->width - style.windowPadding.x * 2), style.font->height * 1.3);
    
	NextItemSize = vec2(-1, 0);
    
	return InputTextCall(label, buffer, maxChars, position, dimensions, nullptr, flags, 1);
}

bool UI::InputText(string label, string& buffer, u32 maxChars, UIInputTextCallback callback, UIInputTextFlags flags){
	vec2 position = curwin->position + curwin->cursor + style.windowPadding - curwin->scroll;
	position.y -= style.font->height * 1.3 / 4;
	vec2 dimensions = (NextItemSize.x != -1) ? NextItemSize : vec2(Math::clamp(100, 0, curwin->width - style.windowPadding.x * 2), style.font->height * 1.3);
	
	NextItemSize = vec2(-1, 0);
    
	return InputTextCall(label, buffer, maxChars, position, dimensions, callback, flags, 1);
}

bool UI::InputText(string label, string& buffer, u32 maxChars, UIInputTextState*& getInputTextState, UIInputTextFlags flags) {
	vec2 position = curwin->position + curwin->cursor + style.windowPadding - curwin->scroll;
	position.y -= style.font->height * 1.3 / 4;
	vec2 dimensions = (NextItemSize.x != -1) ? NextItemSize : vec2(Math::clamp(100, 0, curwin->width - style.windowPadding.x * 2), style.font->height * 1.3);

	NextItemSize = vec2(-1, 0);

	if (InputTextCall(label, buffer, maxChars, position, dimensions, nullptr, flags, 1)) {
		getInputTextState = inputTexts.at(label);
		return true;
	}
	getInputTextState = inputTexts.at(label);
	return false;
}

bool UI::InputText(string label, string& buffer, u32 maxChars, vec2 pos, UIInputTextFlags flags) {
	vec2 dimensions = (NextItemSize.x != -1) ? NextItemSize : vec2(Math::clamp(100, 0, Math::clamp(curwin->width - style.windowPadding.x * 2, 1, FLT_MAX)), style.font->height * 1.3);
	pos += curwin->position - curwin->scroll;

	NextItemSize = vec2(-1, 0);

	return InputTextCall(label, buffer, maxChars, pos, dimensions, nullptr, flags, 0);
}

bool UI::InputText(string label, string& buffer, u32 maxChars, vec2 pos, UIInputTextCallback callback, UIInputTextFlags flags) {
	vec2 dimensions = (NextItemSize.x != -1) ? NextItemSize : vec2(Math::clamp(100, 0, curwin->width - style.windowPadding.x * 2), style.font->height * 1.3);
	pos += curwin->position - curwin->scroll;

	NextItemSize = vec2(-1, 0);

	return InputTextCall(label, buffer, maxChars, pos, dimensions, callback, flags, 0);
}

bool UI::InputText(string label, string& buffer, u32 maxChars, vec2 pos, UIInputTextState*& getInputTextState, UIInputTextFlags flags) {
	vec2 dimensions = (NextItemSize.x != -1) ? NextItemSize : vec2(Math::clamp(100, 0, curwin->width - style.windowPadding.x * 2), style.font->height * 1.3);
	pos += curwin->position - curwin->scroll;

	NextItemSize = vec2(-1, 0);

	if (InputTextCall(label, buffer, maxChars, pos, dimensions, nullptr, flags, 0)) {
		getInputTextState = inputTexts.at(label);
		return true; 
	}
	getInputTextState = inputTexts.at(label);
	return false;
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
	
	//set default style
	
	//load font
	Font white_font{2, 2, 1, "white_block"};
	u8 white_pixels[4] = {255, 255, 255, 255};
	Texture* white_texture = Storage::CreateTextureFromMemory(&white_pixels, "font_white", 2, 2, ImageFormat_BW, TextureType_2D, false, false).second;;
	Render::LoadFont(&white_font, white_texture);
	
	style.font = new Font();
	style.font->load_bdf_font("gohufont-14.bdf");
	Texture* font_texture = Storage::CreateTextureFromMemory(style.font->texture_sheet, "font_gohu", style.font->width, style.font->height * style.font->char_count, ImageFormat_RGBA, TextureType_2D, false, false).second;
	Render::LoadFont(style.font, font_texture);
	
	//push default color scheme
	//this is never meant to be popped
	PushColor(UIStyleCol_Border,   colors.near_black);
	PushColor(UIStyleCol_WindowBg, colors.midnight_blue);
	PushColor(UIStyleCol_TitleBg,  colors.purple_gray);
	PushColor(UIStyleCol_FrameBg,  colors.pink_gray);
	PushColor(UIStyleCol_Text,     color::WHITE);
	
	//push default style variables
	PushVar(UIStyleVar_WindowBorderSize,    1);
	PushVar(UIStyleVar_TitleBarHeight,      style.font->height * 1.2);
	PushVar(UIStyleVar_TitleTextAlign,      vec2(1, 0.5));
	PushVar(UIStyleVar_WindowPadding,       vec2(10, 10));
	PushVar(UIStyleVar_ItemSpacing,         vec2(1, 1));
	PushVar(UIStyleVar_ScrollAmount,        vec2(5, 5));
	PushVar(UIStyleVar_CheckboxSize,        vec2(10, 10));
	PushVar(UIStyleVar_CheckboxFillPadding, 2);
	PushVar(UIStyleVar_InputTextTextAlign,  vec2(0, 0.5));
    
	
	initColorStackSize = colorStack.size();
	initStyleStackSize = varStack.size();
	
	windows.add("base", curwin);
	windowStack.add(curwin);
	
}

//for checking that certain things were taken care of eg, popping colors/styles/windows
void UI::Update() {
	//there should only be default stuff in the stacks
	Assert(windowStack.size() == 1, 
		   "Frame ended with hanging windows in the stack, make sure you call EndWindow() if you call BeginWindow()!");
	
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
		if (!(w->flags & UIWindowFlags_NoFocus)) {
			if (i == windows.count - 1 && w->hovered) {
				break;
			}
			else if (w->hovered && ((w->flags & UIWindowFlags_FocusOnHover) ? 1 : DeshInput->KeyPressedAnyMod(MouseButton::LEFT))) {
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
			DeshInput->KeyPressedAnyMod(MouseButton::LEFT)) {
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
	
	//draw windows in order with their drawCmds
	for (UIWindow* p : windows) {
		//window position and size corrected for titlebar 
		vec2 winpos = vec2(p->x, p->y + p->titleBarHeight);
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
				color  dccol = drawCmd.color;
				string dctex = drawCmd.text;
				float    dct = drawCmd.thickness;

				switch (drawCmd.type) {
					case UIDrawType_FilledRectangle: {
						Render::FillRectUI(dcpos, dcsiz, dccol, dcso, dcse);
					}break;

					case UIDrawType_Line: {
						Render::DrawLineUI(dcpos, dcpos2, dct, dccol, dcso, dcse);
					}break;

					case UIDrawType_Text: {
						if (drawCmd.scissorExtent.x == -1) {
							Render::DrawTextUI(dctex, dcpos, dccol, winpos, winsiz);
						}
						else {
							Render::DrawTextUI(dctex, dcpos, dccol, dcso, dcse);
						}
					}break;
					case UIDrawType_Rectangle: {
						if (drawCmd.scissorExtent.x == -1)
							Render::DrawRectUI(dcpos, dcsiz, dccol, winpos, winsiz);
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
					color  dccol = drawCmd.color;
					string dctex = drawCmd.text;
					float    dct = drawCmd.thickness;

					switch (drawCmd.type) {
						case UIDrawType_FilledRectangle: {
							if (drawCmd.scissorExtent.x == -1)
								Render::FillRectUI(dcpos, dcsiz, dccol, winpos, winsiz);
							else
								Render::FillRectUI(dcpos, dcsiz, dccol, dcso, dcse);

						}break;

						case UIDrawType_Line: {
							if (drawCmd.scissorExtent.x == -1)
								Render::DrawLineUI(dcpos, dcpos2, dct, dccol, winpos, winsiz);
							else
								Render::DrawLineUI(dcpos, dcpos2, dct, dccol, dcso, dcse);
						}break;

						case UIDrawType_Text: {
							if (drawCmd.scissorExtent.x == -1)
								Render::DrawTextUI(dctex, dcpos, dccol, winpos, winsiz);
							else
								Render::DrawTextUI(dctex, dcpos, dccol, dcso, dcse);
						}break;

						case UIDrawType_Rectangle: {
							if (drawCmd.scissorExtent.x == -1)
								Render::DrawRectUI(dcpos, dcsiz, dccol, winpos, winsiz);
							else
								Render::DrawRectUI(dcpos, dcsiz, dccol, dcso, dcse);
						}break;
					}
				}
			}
		}
		p->baseItems.clear();
		p->items.clear();
	}
}
