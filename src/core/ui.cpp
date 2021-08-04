

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
local map<string, UIWindow> windows;     
local array<UIWindow>       windowStack; //window stack which allow us to use windows like we do colors and styles
local array<ColorMod>       colorStack; 
local array<VarMod>         varStack; 

local u32 initColorStackSize;
local u32 initStyleStackSize;

//set if any window other than base is hovered
local bool globalHovered = false;


//helper functions

//this calculates text taking into account newlines, BUT NOT WRAPPING
//useful for sizing a window to fit some text
//TODO(sushi, Ui) make a CalcTextSize that takes into account wrapping that would occur in a sized window
inline local  vec2 UI::CalcTextSize(string text) {
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


//rectangle


void UI::RectFilled(f32 x, f32 y, f32 width, f32 height, Color color) {
	UIDrawCmd drawCmd;
	drawCmd.      type = UIDrawType_Rectangle;
	drawCmd.  position = vec2{ workingWin.position.x + x, workingWin.position.y + y };
	drawCmd.dimensions = vec2{ width, height };
	drawCmd.     color = color;

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
	drawCmd.scissorOffset = vec2(workingWin.x, workingWin.y + style.titleBarHeight);
	drawCmd.scissorExtent = vec2(workingWin.width, workingWin.height - style.titleBarHeight);

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
		windows[name] = workingWin; 
	}
	else {
		workingWin = windows[name];
		workingWin.cursor = vec2(0, 0);
		if (NextWinPos.x  != -1) workingWin.position   = NextWinPos;
		if (NextWinSize.x != -1) workingWin.dimensions = NextWinSize;
		NextWinPos = vec2(-1, 0); NextWinSize = vec2(-1, 0);
	}
	
	//check if window is hovered
	vec2 mp = DengInput->mousePos;
	if (mp.x >= workingWin.position.x &&
		mp.y >= workingWin.position.y &&
		mp.x <= workingWin.position.x + workingWin.dimensions.x &&
		mp.y <= workingWin.position.y + workingWin.dimensions.y) {
		workingWin.hovered = 1;
	}
	else {
		workingWin.hovered = 0;
	}

	//check if window's title is hovered (if were drawing it), so we can check for window movement by mouse later
	if (!(workingWin.flags & UIWindowFlags_NoTitleBar)) {
		if (mp.x >= workingWin.position.x &&
			mp.y >= workingWin.position.y &&
			mp.x <= workingWin.position.x + workingWin.dimensions.x &&
			mp.y <= workingWin.position.y + style.titleBarHeight) {
			workingWin.titleHovered = 1;
		}
		else {
			workingWin.titleHovered = 0;
		}
	}

	//check for scrolling inputs
	if (!(flags & UIWindowFlags_NoScroll)) {
		if (workingWin.hovered && DengInput->KeyPressedAnyMod(MouseButton::SCROLLUP)) {
			workingWin.scy -= style.scrollAmount.y;
			Math::clamp(workingWin.scy, 0, workingWin.maxScroll.y);
		}
		else if (workingWin.hovered && DengInput->KeyPressedAnyMod(MouseButton::SCROLLDOWN)) {
			workingWin.scy += style.scrollAmount.y;
			Math::clamp(workingWin.scy, 0, workingWin.maxScroll.y);
		}
	}


	//if the window isn't invisible draw things that havent been disabled
	if ((flags & UIWindowFlags_Invisible) != UIWindowFlags_Invisible) {
		
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
						if (DengInput->KeyPressedAnyMod(MouseButton::LEFT)) {
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
	windows[name] = workingWin;
}

void UI::EndWindow() {
	Assert(windowStack.size() > 1, "Attempted to end the base window");
	
	//check to see if the elements we have drawn so far have gone beyond the window's size
	//and allow scrolling if it did, as well as define a max scrolling amount
	if (workingWin.cury > workingWin.height)
		workingWin.maxScroll.y = (workingWin.cury + style.windowPadding.y) - workingWin.height;
	else
		workingWin.maxScroll.y = 0;

	NextWinPos = vec2(-1, 0); NextWinSize = vec2(-1, 0);

	//update stored window with new window state
	//NOTE: I'm not sure if I want to keep doing this like this
	//      or just make workingWin a pointer to a window in the list
	windows[workingWin.name] = workingWin;
	workingWin = *windowStack.last;
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
	UIWindow* debugee = &windows[name];
	
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

	Text(info);

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
	UIDrawCmd drawCmd{ UIDrawType_Rectangle };
}

bool UI::Button(string text, vec2 pos){

}

bool UI::Button(string text, Color color){

}

bool UI::Button(string text, vec2 pos, Color color){

}




//initializes core UI with an invisible working window covering the entire screen
//also initializes styles
//the base window should never focus when clicking within it, so any widgets drawn within
//it will not focus if theres a window in front of them.
//I'm not sure how i want to fix it yet
void UI::Init() {
	workingWin.name = "Base";
	workingWin.position = vec2(0,0);
	workingWin.dimensions = DengWindow->dimensions;
	
	//set default style

	//load font
	Font white_font{2, 2, 1, "white_block"};
	u8 white_pixels[4] = {255, 255, 255, 255};
	Texture* white_texture = Storage::CreateTextureFromMemory(&white_pixels, "font_white", 2, 2, ImageFormat_BW, TextureType_2D, false, false).second;;
	Render::LoadFont(&white_font, white_texture);
	
	style.font = new Font();
	style.font->load_bdf_font("gohufont-11.bdf");
	Texture* font_texture = Storage::CreateTextureFromMemory(style.font->texture_sheet, "font_gohu", style.font->width, style.font->height * style.font->char_count, ImageFormat_BW, TextureType_2D, false, false).second;
	Render::LoadFont(style.font, font_texture);
	
	//push default color scheme
	//this is never meant to be popped
	PushColor(UIStyleCol_Border,   colors.near_black);
	PushColor(UIStyleCol_WindowBg, colors.midnight_blue);
	PushColor(UIStyleCol_TitleBg,  colors.purple_gray);
	PushColor(UIStyleCol_Text,     Color::WHITE);
	
	//push default style variables
	PushVar(UIStyleVar_WindowBorderSize, 1);
	PushVar(UIStyleVar_TitleBarHeight, style.font->height * 1.2);
	PushVar(UIStyleVar_TitleTextAlign, vec2(0, 0.5));
	PushVar(UIStyleVar_WindowPadding,  vec2(10, 10));
	PushVar(UIStyleVar_ItemSpacing,    vec2(1, 1));
	PushVar(UIStyleVar_ScrollAmount,   vec2(5, 5));

	initColorStackSize = colorStack.size();
	initStyleStackSize = varStack.size();
	
	windows["base"] = workingWin;
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
		UIWindow& w = windows[i];
		if (i == windows.count - 1 && w.hovered) {
			break;
		}
		else if(w.hovered && ((w.flags & UIWindowFlags_FocusOnHover) ? 1 : DengInput->KeyPressedAnyMod(MouseButton::LEFT))) {
			for (int move = i; move < windows.count - 1; move++)
				windows.swap(move, move + 1);
			break;
		}
	}

	
	{ //drag
		UIWindow* focused = &windows[windows.count - 1];
		
		static bool newDrag = true;
		static vec2 mouseOffset = vec2(0, 0);

		if (focused->titleHovered && DengInput->KeyDownAnyMod(MouseButton::LEFT)) {
			if (newDrag) {
				mouseOffset = focused->position - DengInput->mousePos;
				newDrag = false;
			}
		}
		if (!newDrag) 
			focused->position = DengInput->mousePos + mouseOffset;
		if (DengInput->KeyReleased(MouseButton::LEFT))
			newDrag = true;
	}

	//draw windows in order with their drawCmds
	for (UIWindow& p : windows) {


		//draw base cmds first
		for (UIDrawCmd& drawCmd : p.baseDrawCmds) {
			switch (drawCmd.type) {
			case UIDrawType_Rectangle: {
				Render::FillRectUI(drawCmd.position, drawCmd.dimensions, drawCmd.color);
			}break;

			case UIDrawType_Line: {
				Render::DrawLineUI(drawCmd.position, drawCmd.position2, drawCmd.thickness, drawCmd.color);
			}break;

			case UIDrawType_Text: {
				//scissor out the titlebar area as well if we have one
				if (drawCmd.scissorExtent.x == -1) {
					//TODO(sushi, Ui) fix scissor clamping
					//vec2 scissorOffset = vec2(
					//	Clamp(p.second.position.x, 0, INFINITY),
					//	Clamp(p.second.position.y, 0, INFINITY));
					Render::DrawTextUI(drawCmd.text, drawCmd.position, p.position, p.dimensions, drawCmd.color);
				}
				else {
					Render::DrawTextUI(drawCmd.text, drawCmd.position, drawCmd.scissorOffset, drawCmd.scissorExtent, drawCmd.color);
				}
			}break;
			}
		}

		//dont draw non-base draw cmds if we're minimized
		if (!p.minimized) {
			for (UIDrawCmd& drawCmd : p.drawCmds) {
				switch (drawCmd.type) {
				case UIDrawType_Rectangle: {
					Render::FillRectUI(drawCmd.position, drawCmd.dimensions, drawCmd.color);
				}break;

				case UIDrawType_Line: {
					Render::DrawLineUI(drawCmd.position, drawCmd.position2, drawCmd.thickness, drawCmd.color);
				}break;

				case UIDrawType_Text: {
					//scissor out the titlebar area as well if we have one
					if (drawCmd.scissorExtent.x == -1) {
						//TODO(sushi, Ui) fix scissor clamping
						//vec2 scissorOffset = vec2(
						//	Clamp(p.second.position.x, 0, INFINITY),
						//	Clamp(p.second.position.y, 0, INFINITY));
						Render::DrawTextUI(drawCmd.text, drawCmd.position, p.position, p.dimensions, drawCmd.color);
					}
					else {
						Render::DrawTextUI(drawCmd.text, drawCmd.position, drawCmd.scissorOffset, drawCmd.scissorExtent, drawCmd.color);
					}
				}break;
				}
			}
		}
		p.baseDrawCmds.clear();
		p.drawCmds.clear();
	}
}

