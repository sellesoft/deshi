#include "ui.h"
//color pallete 
//current palette:
//https://lospec.com/palette-list/slso8
//TODO(sushi, Ui) implement menu style file loading sort of stuff yeah
//TODO(sushi, Ui) standardize what UI element each color belongs to
local struct {
	Color          midnight_blue = Color(0x0d2b45); //midnight blue
	Color         dark_grey_blue = Color(0x203c56); //dark gray blue
	Color            purple_gray = Color(0x544e68); //purple gray
	Color              pink_gray = Color(0x8d697a); //pink gray
	Color        bleached_orange = Color(0xd08159); //bleached orange
	Color bright_bleached_orange = Color(0xffaa5e); //above but brighter
	Color             skin_white = Color(0xffd4a3); //skin white
	Color      bright_skin_white = Color(0xffecd6); //even whiter skin
	Color             near_black = Color(0x141414); //almost black
}colors;

//global styling
struct UIStyle {
	vec2  windowPadding;
	vec2  itemSpacing;
	float windowBorderSize;
	float titleBarHeight;
	vec2  titleTextAlign;
	vec2  scrollAmount;
	Font* font; //this is a pointer until I fix font to not store so much shit
	Color colors[UIStyleCol_COUNT];
} style;

//for color stack, saves what element was changed and what it's old color was 
struct ColorMod {
	UIStyleCol element;
	Color oldCol;
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
	{2, offsetof(UIStyle, scrollAmount)}
};

//this variable defines the space the user is working in when calling UI functions
//windows are primarily a way for the user to easily position things on screen relative to a parent
//and to make detecting where text wraps and other things easier
//by default a window that takes up the entire screen and is invisible is made on init
UIWindow workingWin; 

local vec2 NextWinSize = vec2(-1, 0);
local vec2 NextWinPos  = vec2(-1, 0);


//window map which only stores known windows
//and their order in layers eg. when one gets clicked it gets moved to be first if its set to
local map<string, UIWindow>         windows;     
local map<string, UIInputTextState> inputTexts;  //stores known input text labels and their state
local array<UIWindow>               windowStack; //window stack which allow us to use windows like we do colors and styles
local array<ColorMod>               colorStack; 
local array<VarMod>                 varStack; 

local u32 initColorStackSize;
local u32 initStyleStackSize;

//set if any window other than base is hovered
local bool globalHovered = false;

u32 activeId = -1; //the id of an active widget eg. input text


//helper defines


#define workingWinPositionPlusTitlebar vec2(workingWin.x, workingWin.y + ((workingWin.flags & UIWindowFlags_NoTitleBar) ? 0 :style.titleBarHeight));
#define workingWinSizeMinusTitlebar    vec2(workingWin.width, workingWin.height - ((workingWin.flags & UIWindowFlags_NoTitleBar) ? 0 : style.titleBarHeight));


//helper functions


//this calculates text taking into account newlines, BUT NOT WRAPPING
//useful for sizing a window to fit some text
//TODO(sushi, Ui) make a CalcTextSize that takes into account wrapping that would occur in a sized window
inline local vec2 UI::CalcTextSize(string text) {
	string stage = text;
	u32 longest = 0;
	size_t nlp = stage.find_first_of('\n');
	while (nlp != string::npos) {
		if (nlp - 1 > longest)
			longest = nlp - 1;
		stage = stage.substr(nlp + 1);
		nlp = stage.find_first_of('\n');
	}
	
	if (stage.size > longest)
		longest = stage.size;
	
	return vec2(longest * style.font->width, style.font->height * (text.count('\n') + 1));
}

inline local bool PointInRectangle(vec2 point, vec2 rectPos, vec2 rectDims) {
	return
		point.x >= rectPos.x &&
		point.y >= rectPos.y &&
		point.x <= rectPos.x + rectDims.x &&
		point.y <= rectPos.y + rectDims.y;
}


//rectangle


void UI::RectFilled(f32 x, f32 y, f32 width, f32 height, Color color) {
	UIDrawCmd drawCmd;
	drawCmd.      type = UIDrawType_Rectangle;
	drawCmd.  position = vec2{ workingWin.position.x + x, workingWin.position.y + y };
	drawCmd.dimensions = vec2{ width, height };
	drawCmd.     color = color;
	
	workingWin.drawCmds.add(drawCmd);
}

void UI::RectFilled(vec2 pos, vec2 dimen, Color color) {
	UIDrawCmd drawCmd;
	drawCmd.type = UIDrawType_Rectangle;
	drawCmd.position = pos;
	drawCmd.dimensions = dimen;
	drawCmd.color = color;

	workingWin.drawCmds.add(drawCmd);
}


//Line


void UI::Line(f32 x1, f32 y1, f32 x2, f32 y2, float thickness, Color color) {
	UIDrawCmd drawCmd;
	drawCmd.     type = UIDrawType_Line;
	drawCmd. position = vec2{ workingWin.position.x + x1, workingWin.position.y + y1 };
	drawCmd.position2 = vec2{ workingWin.position.x + x2, workingWin.position.y + y2 };
	drawCmd.thickness = thickness;
	drawCmd.    color = color;
	
	workingWin.drawCmds.add(drawCmd);
}

void UI::Line(vec2 start, vec2 end, float thickness, Color color){
	Render::DrawLineUI(workingWin.position + start, workingWin.position + end, thickness, color);
	UIDrawCmd drawCmd;
	drawCmd.     type = UIDrawType_Line;
	drawCmd. position = workingWin.position + start;
	drawCmd.position2 = workingWin.position + start;
	drawCmd.thickness = thickness;
	drawCmd.    color = color;
	
	workingWin.drawCmds.add(drawCmd);
}


//Text


//internal function for actually making and adding the drawCmd
local void TextCall(string text, vec2 pos, Color color) {
	UIDrawCmd drawCmd;
	drawCmd.type = UIDrawType_Text;
	drawCmd.text = text;
	drawCmd.position = pos;
	drawCmd.color = color;
	drawCmd.scissorOffset = vec2(workingWin.x, workingWin.y + ((workingWin.flags & UIWindowFlags_NoTitleBar) ? 0 :style.titleBarHeight));
	drawCmd.scissorExtent = vec2(workingWin.width, workingWin.height - ((workingWin.flags & UIWindowFlags_NoTitleBar) ? 0 : style.titleBarHeight));
	
	workingWin.drawCmds.add(drawCmd);
}

//main function for wrapping, where position is starting position of text relative to the top left of the window
inline local void WrapText(string text, vec2 pos, Color color, bool move_cursor = true) {
	using namespace UI;
	vec2 workcur = pos;
	
	//apply window padding if we're not manually positioning text
	if (move_cursor) 
		workcur += style.windowPadding - workingWin.scroll;
	
	
	//work out where we're going to draw the text and how much to advance the cursor by
	vec2 textSize = CalcTextSize(text);
	
	//max characters we can place 
	u32 maxChars = floor(((workingWin.width - style.windowPadding.x) - workcur.x) / style.font->width);
	
	//make sure max chars never equals 0
	if (!maxChars) maxChars++;
	
	//we need to see if the string goes beyond the width of the window and wrap if it does
	if (maxChars < text.size) {
		//find closest space to split by
		size_t splitat = text.find_first_of_lookback(' ', maxChars);
		string nustr = text.substr(0, (splitat == string::npos) ? maxChars - 1 : splitat);
		TextCall(nustr, workingWin.position + workcur, color);
		
		text = text.substr(nustr.size);
		workcur.y += style.font->height + style.itemSpacing.y;
		
		//continue to wrap if we need to
		while (text.size > maxChars) {
			splitat = text.find_first_of_lookback(' ', maxChars);
			nustr = text.substr(0, (splitat == string::npos) ? maxChars - 1 : splitat);
			TextCall(nustr, workingWin.position + workcur, color);
			
			text = text.substr(nustr.size);
			workcur.y += style.font->height + style.itemSpacing.y;
			
			if (!strlen(text.str)) break;
		}
		
		//write last bit of text
		TextCall(text, workingWin.position + workcur, color);
		workcur.y += style.font->height + style.itemSpacing.y;
		
	}
	else {
		TextCall(text, workingWin.position + workcur, color);
		workcur.y += style.font->height + style.itemSpacing.y;
	}
	
	if (move_cursor) {
		workcur -= style.windowPadding - workingWin.scroll;
		workingWin.cursor = workcur;
	}
}


void UI::Text(string text, UITextFlags flags) {
	if (flags & UITextFlags_NoWrap) {
		TextCall(text, workingWin.position + workingWin.cursor + style.windowPadding - workingWin.scroll, style.colors[UIStyleCol_Text]);
		workingWin.cury += style.font->height + 1;
	}
	else {
		//we check for \n here and call WrapText on each as if they were separate text calls
		//i could probably do this in wrap text, but i decided to do it here for now
		
		size_t newline = text.find_first_of('\n');
		if (newline != string::npos && newline != text.size - 1) {
			string remainder = text.substr(newline + 1);
			WrapText(text.substr(0, newline - 1), workingWin.cursor, style.colors[UIStyleCol_Text]);
			newline = remainder.find_first_of('\n');
			while (newline != string::npos) {
				WrapText(remainder.substr(0, newline - 1), workingWin.cursor, style.colors[UIStyleCol_Text]);
				remainder = remainder.substr(newline + 1);
				newline = remainder.find_first_of('\n');
			}
			WrapText(remainder, workingWin.cursor, style.colors[UIStyleCol_Text]);
		}
		else {
			WrapText(text, workingWin.cursor, style.colors[UIStyleCol_Text]);
		}
	}
}

void UI::Text(string text, vec2 pos, UITextFlags flags) {
	if (flags & UITextFlags_NoWrap) {
		TextCall(text, workingWin.position + pos - workingWin.scroll, style.colors[UIStyleCol_Text]);
	}
	else {
		size_t newline = text.find_first_of('\n');
		if (newline != string::npos && newline != text.size - 1) {
			string remainder = text.substr(newline + 1);
			WrapText(text.substr(0, newline - 1), pos, style.colors[UIStyleCol_Text], 0);
			newline = remainder.find_first_of('\n');
			while (newline != string::npos) {
				WrapText(remainder.substr(0, newline - 1), pos, style.colors[UIStyleCol_Text], 0);
				remainder = remainder.substr(newline + 1);
				newline = remainder.find_first_of('\n');
			}
			WrapText(remainder, pos, style.colors[UIStyleCol_Text], 0);
		}
		else {
			WrapText(text, pos, style.colors[UIStyleCol_Text], 0);
		}
		
	}
}

void UI::Text(string text, Color color, UITextFlags flags) {
	if (flags & UITextFlags_NoWrap) {
		TextCall(text, workingWin.position + workingWin.cursor + style.windowPadding - workingWin.scroll, color);
		workingWin.cury += style.font->height + 1;
	}
	else {
		size_t newline = text.find_first_of('\n');
		if (newline != string::npos && newline != text.size - 1) {
			string remainder = text.substr(newline + 1);
			WrapText(text.substr(0, newline - 1), workingWin.cursor, color);
			newline = remainder.find_first_of('\n');
			while (newline != string::npos) {
				WrapText(remainder.substr(0, newline - 1), workingWin.cursor, color);
				remainder = remainder.substr(newline + 1);
				newline = remainder.find_first_of('\n');
			}
			WrapText(remainder, workingWin.cursor, color);
		}
		else {
			WrapText(text, workingWin.cursor, color);
		}
	}
	
}

void UI::Text(string text, vec2 pos, Color color, UITextFlags flags) {
	if (flags & UITextFlags_NoWrap) {
		TextCall(text, workingWin.position + pos - workingWin.scroll, color);
	}
	else {
		size_t newline = text.find_first_of('\n');
		if (newline != string::npos && newline != text.size - 1) {
			string remainder = text.substr(newline + 1);
			WrapText(text.substr(0, newline - 1), pos, color);
			newline = remainder.find_first_of('\n');
			while (newline != string::npos) {
				WrapText(remainder.substr(0, newline - 1), pos, color);
				remainder = remainder.substr(newline + 1);
				newline = remainder.find_first_of('\n');
			}
			WrapText(remainder, pos, color);
		}
		else {
			WrapText(text, pos, color);
		}
	}
}


//Windows

//begins a window with a name, position, and dimensions along with some optional flags
//if begin window is called with a name that was already called before it will work with
//the data that window previously had
void UI::BeginWindow(string name, vec2 pos, vec2 dimensions, UIWindowFlags flags) {
	//save previous window on stack
	windowStack.add(workingWin); 
	
	//check if were making a new window or working with one we already know
	if (!windows.has(name)) {
		//make new window if we dont know this one already or if we arent saving it
		workingWin.      name = name; 
		workingWin.  position = pos;
		workingWin.dimensions = dimensions;
		workingWin.    cursor = vec2(0, 0);
		workingWin.     flags = flags;
		windows.add(name, workingWin); 
	}
	else {
		workingWin = *windows.at(name);
		workingWin.cursor = vec2(0, 0);
		if (NextWinPos.x  != -1) workingWin.position   = NextWinPos;
		if (NextWinSize.x != -1) workingWin.dimensions = NextWinSize;
		NextWinPos = vec2(-1, 0); NextWinSize = vec2(-1, 0);
	}
	
	//check if window is hovered
	vec2 mp = DeshInput->mousePos;
	if(PointInRectangle(mp, workingWin.position, workingWin.dimensions)){
		workingWin.hovered = 1;
	}
	else {
		workingWin.hovered = 0;
	}
	
	//check if window's title is hovered (if were drawing it), so we can check for window movement by mouse later
	if (!(workingWin.flags & UIWindowFlags_NoTitleBar)) {
		if(PointInRectangle(mp, workingWin.position, vec2(workingWin.width, style.titleBarHeight))){
			workingWin.titleHovered = 1;
		}
		else {
			workingWin.titleHovered = 0;
		}
	}
	
	//check for scrolling inputs
	if (!(flags & UIWindowFlags_NoScroll)) {
		if (workingWin.hovered && DeshInput->KeyPressedAnyMod(MouseButton::SCROLLUP)) {
			workingWin.scy -= style.scrollAmount.y;
			Math::clampr(workingWin.scy, 0, workingWin.maxScroll.y);
		}
		else if (workingWin.hovered && DeshInput->KeyPressedAnyMod(MouseButton::SCROLLDOWN)) {
			workingWin.scy += style.scrollAmount.y;
			Math::clampr(workingWin.scy, 0, workingWin.maxScroll.y);
		}
	}
	
	
	//if the window isn't invisible draw things that havent been disabled
	if ((flags & UIWindowFlags_Invisible) != UIWindowFlags_Invisible) {
		workingWin.titleBarHeight = style.titleBarHeight;
		//draw background
		if (!(flags & UIWindowFlags_NoBackground) && !workingWin.minimized) {
			UIDrawCmd drawCmd; //inst 29
			drawCmd.      type = UIDrawType_Rectangle;
			drawCmd.  position = workingWin.position;
			drawCmd.dimensions = workingWin.dimensions;
			drawCmd.     color = style.colors[UIStyleCol_WindowBg];
			
			workingWin.baseDrawCmds.add(drawCmd); //inst 35
		}
		
		//draw title bar
		if (!(flags & UIWindowFlags_NoTitleBar)) {
			{
				UIDrawCmd drawCmd; //inst 40
				drawCmd.type = UIDrawType_Rectangle;
				drawCmd.position = workingWin.position;
				drawCmd.dimensions = vec2{ workingWin.width, style.titleBarHeight };
				drawCmd.color = style.colors[UIStyleCol_TitleBg];
				
				workingWin.baseDrawCmds.add(drawCmd); //inst 44
			}
			
			{//draw text if it exists
				if (name.size != 0) {
					UIDrawCmd drawCmd; //inst 46
					drawCmd.type = UIDrawType_Text;
					drawCmd.text = workingWin.name; //inst 48
					drawCmd.position = vec2(
											workingWin.x + (workingWin.width - name.size * style.font->width) * style.titleTextAlign.x,
											workingWin.y + (style.titleBarHeight - style.font->height) * style.titleTextAlign.y);
					drawCmd.color = Color::WHITE;
					
					//TODO(sushi, Ui) add title text coloring
					
					workingWin.baseDrawCmds.add(drawCmd); //inst 54
				}
			}
			
			{//draw titlebar minimize button and check for it being clicked
				if (!((flags & UIWindowFlags_NoMinimizeButton) || (flags & UIWindowFlags_NoMinimizeButton))) {
					UIDrawCmd drawCmd{ UIDrawType_Rectangle };
					drawCmd.position = vec2(
											workingWin.x + (workingWin.width - name.size * style.font->width) * 0.01,
											workingWin.y + (style.titleBarHeight * 0.5 - 2));
					drawCmd.dimensions = vec2(10, 4);
					
					if (mp.x >= drawCmd.position.x &&
						mp.y >= drawCmd.position.y &&
						mp.x <= drawCmd.position.x + drawCmd.dimensions.x &&
						mp.y <= drawCmd.position.y + drawCmd.dimensions.y) {
						drawCmd.color = style.colors[UIStyleCol_TitleBg] * 0.7;
						if (DeshInput->KeyPressedAnyMod(MouseButton::LEFT)) {
							workingWin.minimized = !workingWin.minimized;
						}
					}
					else {
						drawCmd.color = style.colors[UIStyleCol_TitleBg] * 0.3;
					}
					
					
					workingWin.baseDrawCmds.add(drawCmd); //inst 54
				}
			}
			
			//move cursor down by title bar height
			workingWin.cursor.y = style.titleBarHeight;
		}
		else {
			workingWin.titleBarHeight = 0;
		}
		
		//draw border
		if (!(flags & UIWindowFlags_NoBorder) && !workingWin.minimized) {
			UIDrawCmd drawCmd; //inst 58
			drawCmd.type = UIDrawType_Rectangle;
			drawCmd.color = style.colors[UIStyleCol_Border];
			
			//left
			drawCmd.  position = vec2{ workingWin.x - style.windowBorderSize, workingWin.y };
			drawCmd.dimensions = vec2{ style.windowBorderSize, workingWin.height };
			workingWin.baseDrawCmds.add(drawCmd); //inst 64
			
			//right 
			drawCmd.  position = vec2{ workingWin.x + workingWin.width, workingWin.y };
			drawCmd.dimensions = vec2{ style.windowBorderSize, workingWin.height };
			workingWin.baseDrawCmds.add(drawCmd); //inst 71
			
			//top
			drawCmd.  position = vec2{ workingWin.x - style.windowBorderSize, workingWin.y - style.windowBorderSize };
			drawCmd.dimensions = vec2{ workingWin.width + 2 * style.windowBorderSize, style.windowBorderSize };
			workingWin.baseDrawCmds.add(drawCmd); //inst 78
			
			//bottom
			drawCmd.  position = vec2{ workingWin.x - style.windowBorderSize, workingWin.y + workingWin.height };
			drawCmd.dimensions = vec2{ workingWin.width + 2 * style.windowBorderSize, style.windowBorderSize };
			workingWin.baseDrawCmds.add(drawCmd);//inst 85
		}
	}
	if(UIWindow* window = windows.at(name)){
		*window = workingWin;
	}else{
		windows.add(name, workingWin);
	}
}

void UI::EndWindow() {
	Assert(windowStack.size() > 1, "Attempted to end the base window");
	
	//check to see if the elements we have drawn so far have gone beyond the window's size
	//and allow scrolling if it did, as well as define a max scrolling amount
	if (workingWin.cury > workingWin.height)
		workingWin.maxScroll.y = (workingWin.cury + style.windowPadding.y * 2) - workingWin.height;
	else
		workingWin.maxScroll.y = 0;
	
	NextWinPos = vec2(-1, 0); NextWinSize = vec2(-1, 0);
	
	//update stored window with new window state
	//NOTE: I'm not sure if I want to keep doing this like this
	//      or just make workingWin a pointer to a window in the list
	*windows.at(workingWin.name) = workingWin;
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
	workingWin.name = name;
}

//checks if the current working window is hovered
bool UI::IsWinHovered() {
	return workingWin.hovered;
}

void UI::ShowDebugWindowOf(string name) {
	UIWindow* debugee = windows.at(name);
	
	string info = 
		TOSTRING("    position: ", debugee->position,                          "\n") + 
		TOSTRING("  dimensions: ", debugee->dimensions,                        "\n") + 
		TOSTRING("      scroll: ", debugee->scroll,                            "\n") + 
		TOSTRING("   maxScroll: ", debugee->maxScroll,                         "\n") + 
		TOSTRING("     hovered: ", (debugee->hovered) ? "true" : "false",      "\n") + 
		TOSTRING("titleHovered: ", (debugee->titleHovered) ? "true" : "false", "\n") +
		TOSTRING("      cursor: ", debugee->cursor);
	
	SetNextWindowPos(debugee->position + debugee->dimensions.ySet(0).xAdd(10));
	SetNextWindowSize(CalcTextSize(info) + vec2(style.windowPadding.x * 2, style.windowPadding.y * 2));
	BeginWindow(TOSTRING("#", name, " debug", "#"), debugee->position + debugee->dimensions.ySet(0).xAdd(10), debugee->dimensions, UIWindowFlags_NoFocus | UIWindowFlags_NoScroll);
	
	//show info about variables
	Text(info);

	vec2 cursize = vec2::ONE * 3;
	Render::DrawRectUI(debugee->position + (debugee->cursor - cursize / 2) - debugee->scroll, vec2::ONE * 10);
	
	EndWindow();
	
}

//Push/Pop functions
void UI::PushColor(UIStyleCol idx, Color color) {
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



//widget stuff
bool UI::Button(string text) {
	UIDrawCmd drawCmd{ UIDrawType_Rectangle }; return true;
}

bool UI::Button(string text, vec2 pos){
	return true;
}

bool UI::Button(string text, Color color){
	return true;
}

bool UI::Button(string text, vec2 pos, Color color){
	return true;
}


void UI::Checkbox(string label, bool* b) {

	vec2 boxpos = workingWin.position + workingWin.cursor + style.windowPadding - workingWin.scroll;
	vec2 boxsiz = vec2(16, 16);

	{//box
		UIDrawCmd drawCmd{ UIDrawType_Rectangle };
		drawCmd.position = boxpos;
		drawCmd.dimensions = boxsiz;
		drawCmd.color = style.colors[UIStyleCol_FrameBg];

		workingWin.drawCmds.add(drawCmd);
	}

	//fill if true
	int fillPadding = 3;
	if (*b) {
		UIDrawCmd drawCmd{ UIDrawType_Rectangle };
		drawCmd.position = boxpos + boxsiz * vec2(fillPadding / boxsiz.x, fillPadding / boxsiz.y);
		drawCmd.dimensions = boxsiz  * (vec2::ONE - 2 * vec2(fillPadding / boxsiz.x, fillPadding / boxsiz.y));
		drawCmd.color = style.colors[UIStyleCol_FrameBg] * 0.7;

		workingWin.drawCmds.add(drawCmd);
	}

	{//label
		UIDrawCmd drawCmd{ UIDrawType_Text };
		drawCmd.position = boxpos + vec2(boxsiz.x + style.itemSpacing.x, (boxsiz.y - style.font->height) * 0.5);
		drawCmd.text = label;
		drawCmd.color = style.colors[UIStyleCol_Text];

		workingWin.drawCmds.add(drawCmd);
	}

	if (DeshInput->LMousePressed() && PointInRectangle(DeshInput->mousePos, boxpos, boxsiz))
		*b = !*b;

	workingWin.cury += boxsiz.y + style.itemSpacing.y;
}


bool UI::InputText(string label, string& buffer, u32 maxChars, UIInputTextFlags flags) {	

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
		state->buffer = buffer;
	}

	vec2 position = workingWin.position + workingWin.cursor + style.windowPadding - workingWin.scroll;
	vec2 dimensions = vec2(Math::clamp(100, 0, workingWin.x - style.windowPadding.x * 2), style.font->height * 1.3);

	//check for mouse click to set active 
	if (DeshInput->KeyPressedAnyMod(MouseButton::LEFT)) {
		if (PointInRectangle(DeshInput->mousePos, position, dimensions) ) {
			activeId = state->id;
		}
		else {
			if(activeId == state->id) activeId = -1;
		}
	}

	if (activeId == state->id) {
		if (DeshInput->KeyPressedAnyMod(Key::RIGHT) && state->cursor < buffer.size) state->cursor++;
		if (DeshInput->KeyPressedAnyMod(Key::LEFT) && state->cursor > 0) state->cursor--;

		//gather text into buffer from inputs
		//make this only loop when a key has been pressed eventually

		persist TIMER_START(hold);
		persist TIMER_START(throttle);

		//TODO(sushi) make this not count modifier keys
		if (DeshInput->AnyKeyPressed()) { TIMER_RESET(hold); }

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
						case Key::K0: buffer.insert(')', ins); break;
						case Key::K1: buffer.insert('!', ins); break;
						case Key::K2: buffer.insert('@', ins); break;
						case Key::K3: buffer.insert('#', ins); break;
						case Key::K4: buffer.insert('$', ins); break;
						case Key::K5: buffer.insert('%', ins); break;
						case Key::K6: buffer.insert('^', ins); break;
						case Key::K7: buffer.insert('&', ins); break;
						case Key::K8: buffer.insert('*', ins); break;
						case Key::K9: buffer.insert('(', ins); break;
					}
				}
				else {
					buffer.insert(KeyStringsLiteral[i], ins);
				}
			}
			else {
				if (DeshInput->KeyDownAnyMod(Key::LSHIFT) || DeshInput->KeyDownAnyMod(Key::RSHIFT)) {
					switch (i) {
						case Key::SEMICOLON:  buffer.insert(':', ins);  break;
						case Key::APOSTROPHE: buffer.insert('"', ins);  break;
						case Key::LBRACKET:   buffer.insert('{', ins);  break;
						case Key::RBRACKET:   buffer.insert('}', ins);  break;
						case Key::BACKSLASH:  buffer.insert('\\', ins); break;
						case Key::COMMA:      buffer.insert('<', ins);  break;
						case Key::PERIOD:     buffer.insert('>', ins);  break;
						case Key::SLASH:      buffer.insert('?', ins);  break;
						case Key::MINUS:      buffer.insert('_', ins);  break;
						case Key::EQUALS:     buffer.insert('+', ins);  break;
						case Key::TILDE:      buffer.insert('~', ins);  break;
					}
				}
				else {
					buffer.insert(KeyStringsLiteral[i], ins);
				}
			}
			TIMER_RESET(state->timeSinceTyped);
		};

		if (DeshInput->anyKeyDown) {
			if (TIMER_END(hold) < 1000) {
				if (DeshInput->KeyPressedAnyMod(Key::BACKSPACE) && buffer.size > 0 && state->cursor != 0) {
					buffer.erase(--state->cursor);
				}
				else {
					for (int i = 0; i < Key::Key_COUNT; i++) {
						char toPlace = KeyStringsLiteral[i];
						if (DeshInput->KeyPressedAnyMod(i) && buffer.size < maxChars && toPlace != '\0') {
							u32 ins = state->cursor++ - 1;
							placeKey(i, ins, toPlace);
						}
					}
				}
			}
			else {
				if (TIMER_END(throttle) > 50) {
					if (DeshInput->KeyDownAnyMod(Key::BACKSPACE) && buffer.size > 0 && state->cursor != 0) {
						buffer.erase(--state->cursor);
					}
					else {
						for (int i = 0; i < Key::Key_COUNT; i++) {
							char toPlace = KeyStringsLiteral[i];
							if (DeshInput->KeyDownAnyMod(i) && buffer.size < maxChars && toPlace != '\0') {
								u32 ins = state->cursor++ - 1;
								placeKey(i, ins, toPlace);
							}
						}
					}
					TIMER_RESET(throttle);
				}
			}
		}
	}
	
	{//text box
		UIDrawCmd drawCmd{ UIDrawType_Rectangle };
		drawCmd.position = position;
		drawCmd.dimensions = dimensions;
		drawCmd.scissorOffset = workingWinPositionPlusTitlebar;
		drawCmd.scissorExtent = workingWinSizeMinusTitlebar;
		drawCmd.color = Color::VERY_DARK_GREY;

		workingWin.drawCmds.add(drawCmd);
	}

	{//Text
		UIDrawCmd drawCmd{ UIDrawType_Text };
		drawCmd.position = position + vec2(0, (style.font->height * 1.3 - style.font->height) * 0.5);
		drawCmd.text = buffer;
		drawCmd.color = style.colors[UIStyleCol_Text];

		workingWin.drawCmds.add(drawCmd);
	}

	//TODO(sushi, Ui) impl different text cursors
	if(activeId == state->id) {//cursor
		UIDrawCmd drawCmd{ UIDrawType_Line };
		vec2 textStart = position + vec2(0, (style.font->height * 1.3 - style.font->height) * 0.5);
		drawCmd.position  = textStart + vec2(state->cursor * style.font->width, -1);
		drawCmd.position2 = textStart + vec2(state->cursor * style.font->width, style.font->height - 1);
		drawCmd.color = 
			Color(255,255,255, 
			255 * (
			cos((2 * M_PI) / (state->cursorBlinkTime / 2) * TIMER_END(state->timeSinceTyped) / 1000 -
			sin((2 * M_PI) / (state->cursorBlinkTime / 2) * TIMER_END(state->timeSinceTyped) / 1000)) + 1) / 2);
		drawCmd.thickness = 0.5;

		workingWin.drawCmds.add(drawCmd);
	}

	workingWin.cursor.y += style.font->height * 1.3 + style.itemSpacing.y;

	if (DeshInput->KeyPressedAnyMod(Key::ENTER) || DeshInput->KeyPressedAnyMod(Key::NUMPADENTER)) return true;

	return false;
}

bool UI::InputText(string label, string& buffer, vec2 pos, u32 maxChars, UIInputTextFlags flags) {
	return false;
}




//initializes core UI with an invisible working window covering the entire screen
//also initializes styles
//the base window should never focus when clicking within it, so any widgets drawn within
//it will not focus if theres a window in front of them.
//I'm not sure how i want to fix it yet
void UI::Init() {
	workingWin.name = "Base";
	workingWin.position = vec2(0,0);
	workingWin.dimensions = DeshWindow->dimensions;
	
	//set default style
	
	//load font
	Font white_font{2, 2, 1, "white_block"};
	u8 white_pixels[4] = {255, 255, 255, 255};
	Texture* white_texture = Storage::CreateTextureFromMemory(&white_pixels, "font_white", 2, 2, ImageFormat_BW, TextureType_2D, false, false).second;;
	Render::LoadFont(&white_font, white_texture);
	
	style.font = new Font();
	style.font->load_bdf_font("gohufont-11.bdf");
	Texture* font_texture = Storage::CreateTextureFromMemory(style.font->texture_sheet, "font_gohu", style.font->width, style.font->height * style.font->char_count, ImageFormat_RGBA, TextureType_2D, false, false).second;
	Render::LoadFont(style.font, font_texture);
	
	//push default color scheme
	//this is never meant to be popped
	PushColor(UIStyleCol_Border,   colors.near_black);
	PushColor(UIStyleCol_WindowBg, colors.midnight_blue);
	PushColor(UIStyleCol_TitleBg,  colors.purple_gray);
	PushColor(UIStyleCol_FrameBg,  colors.pink_gray);
	PushColor(UIStyleCol_Text,     Color::WHITE);
	
	//push default style variables
	PushVar(UIStyleVar_WindowBorderSize, 1);
	PushVar(UIStyleVar_TitleBarHeight, style.font->height * 1.2);
	PushVar(UIStyleVar_TitleTextAlign, vec2(1, 0.5));
	PushVar(UIStyleVar_WindowPadding,  vec2(10, 10));
	PushVar(UIStyleVar_ItemSpacing,    vec2(1, 1));
	PushVar(UIStyleVar_ScrollAmount,   vec2(5, 5));
	
	initColorStackSize = colorStack.size();
	initStyleStackSize = varStack.size();
	
	windows.add("base", workingWin);
	windowStack.add(workingWin);
	
}

//for checking that certain things were taken care of eg, popping colors/styles/windows
void UI::Update() {
	//there should only be default stuff in the stacks
	Assert(windowStack.size() == 1, 
		   "Frame ended with hanging windows in the stack, make sure you call EndWindow() if you call BeginWindow()!");
	
	Assert(varStack.size() == initStyleStackSize, 
		   "Frame ended with hanging vars in the stack, make sure you pop vars if you push them!");
	
	Assert(colorStack.size() == initColorStackSize, 
		   "Frame ended with hanging colors in the stack, make sure you pop colors if you push them!");
	
	
	//focusing and dragging
	
	//focus
	for (int i = windows.count - 1; i > 0; i--) {
		UIWindow* w = windows.atIdx(i);
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
		UIWindow* focused = windows.atIdx(windows.count-1);
		
		static bool newDrag = true;
		static vec2 mouseOffset = vec2(0, 0);

		static bool shouldDrag = true;

		//dont drag if click didnt happen on title
		if (DeshInput->KeyPressedAnyMod(MouseButton::LEFT) && !focused->titleHovered) shouldDrag = false;

		if (focused->titleHovered && DeshInput->KeyDownAnyMod(MouseButton::LEFT)) {
			if (newDrag) {
				mouseOffset = focused->position - DeshInput->mousePos;
				newDrag = false;
			}
		}
		if (!newDrag && shouldDrag) 
			focused->position = DeshInput->mousePos + mouseOffset;
		if (DeshInput->KeyReleased(MouseButton::LEFT)) {
			newDrag = true; shouldDrag = true;
		}
	}
	
	//draw windows in order with their drawCmds
	for (UIWindow& p : windows) {
		vec2 winCorrectedPos = vec2(p.x, p.y + p.titleBarHeight);
		vec2 winCorrectedSiz = vec2(p.width, p.height - p.titleBarHeight);
		
		//draw base cmds first
		for (UIDrawCmd& drawCmd : p.baseDrawCmds) {
			switch (drawCmd.type) {
				case UIDrawType_Rectangle: {
					Render::FillRectUI(drawCmd.position, drawCmd.dimensions, drawCmd.color, drawCmd.scissorOffset, drawCmd.scissorExtent);
				}break;
				
				case UIDrawType_Line: {
					Render::DrawLineUI(drawCmd.position, drawCmd.position2, drawCmd.thickness, drawCmd.color, drawCmd.scissorOffset, drawCmd.scissorExtent);
				}break;
				
				case UIDrawType_Text: {
					//scissor out the titlebar area as well if we have one
					if (drawCmd.scissorExtent.x == -1) {
						Render::DrawTextUI(drawCmd.text, drawCmd.position, drawCmd.color, winCorrectedPos, winCorrectedSiz);
					}
					else {
						Render::DrawTextUI(drawCmd.text, drawCmd.position, drawCmd.color, drawCmd.scissorOffset, drawCmd.scissorExtent);
					}
				}break;
			}
		}
		
		//dont draw non-base draw cmds if we're minimized
		if (!p.minimized) {
			for (UIDrawCmd& drawCmd : p.drawCmds) {
				switch (drawCmd.type) {
					case UIDrawType_Rectangle: {
						if (drawCmd.scissorExtent.x == -1) 
							Render::FillRectUI(drawCmd.position, drawCmd.dimensions, drawCmd.color, winCorrectedPos, winCorrectedSiz);
						else 
							Render::FillRectUI(drawCmd.position, drawCmd.dimensions, drawCmd.color, drawCmd.scissorOffset, drawCmd.scissorExtent);
						
					}break;
					
					case UIDrawType_Line: {
						if(drawCmd.scissorExtent.x == -1)
							Render::DrawLineUI(drawCmd.position, drawCmd.position2, drawCmd.thickness, drawCmd.color, winCorrectedPos, winCorrectedSiz);
						else
							Render::DrawLineUI(drawCmd.position, drawCmd.position2, drawCmd.thickness, drawCmd.color, drawCmd.scissorOffset, drawCmd.scissorExtent);
					}break;
					
					case UIDrawType_Text: {
						if (drawCmd.scissorExtent.x == -1)
							Render::DrawTextUI(drawCmd.text, drawCmd.position, drawCmd.color, winCorrectedPos, winCorrectedSiz );
						else 
							Render::DrawTextUI(drawCmd.text, drawCmd.position, drawCmd.color, drawCmd.scissorOffset, drawCmd.scissorExtent);
					}break;
				}
			}
		}
		p.baseDrawCmds.clear();
		p.drawCmds.clear();
	}
}

