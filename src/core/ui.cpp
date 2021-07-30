//global styling
struct UIStyle {
	vec2  windowPadding;
	float windowBorderSize;
	float titleBarHeight;
	vec2  titleTextAlign;
	Font  font;
	Color colors[UIStyleCol_COUNT];
} style;

//color pallete 
//current palette:
//https://lospec.com/palette-list/slso8
//TODO(sushi, Ui) implement menu style file loading sort of stuff yeah
//TODO(sushi, Ui) standardize what UI element each color belongs to
struct {
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

static const UIStyleVarType uiStyleVarTypes[] = {
	{2, offsetof(UIStyle, windowPadding)},
	{1, offsetof(UIStyle, windowBorderSize)},
	{1, offsetof(UIStyle, titleBarHeight)},
	{2, offsetof(UIStyle, titleTextAlign)},
};

//this variable defines the space the user is working in when calling UI functions
//windows are primarily a way for the user to easily position things on screen relative to a parent
//and to make detecting where text wraps and other things easier
//by default a window that takes up the entire screen and is invisible
UIWindow workingWin;
 
map<string, UIWindow> windows;     //window map which only stores known windows
array<UIWindow>       windowStack; //window stack which allow us to use windows like we do colors and styles
array<ColorMod>       colorStack; 
array<VarMod>         varStack; 

u32 initColorStackSize;
u32 initStyleStackSize;


//helper functions


vec2 UI::CalcTextSize(string text) {
	return vec2(text.size * style.font.width, style.font.height);
}



void UI::RectFilled(f32 x, f32 y, f32 width, f32 height, Color color) {
	Render::FillRectUI(workingWin.position.x + x, workingWin.position.y + y, width, height, color);
}


//Line


void UI::Line(f32 x1, f32 y1, f32 x2, f32 y2, float thickness, Color color) {
	Render::DrawLineUI(
					   workingWin.position.x + x1, 
					   workingWin.position.y + y1, 
					   workingWin.position.x + x2, 
					   workingWin.position.y + y2, 
					   thickness, color);
}

void UI::Line(vec2 start, vec2 end, float thickness, Color color){
	Render::DrawLineUI(workingWin.position + start, workingWin.position + end, thickness, color);
}


//Text
//every window has a cursor that tracks where some primitives are to be drawn
//the cursor should be moved according to the primitive drawn 


void UI::Text(string text) {
	//work out where we're going to draw the text and how much to advance the cursor by
	vec2 textSize = CalcTextSize(text);

	workingWin.width = 400;
	if (textSize.y > workingWin.width) {

	}

	Render::DrawTextUI(text, workingWin.position + workingWin.cursor, style.colors[UIStyleCol_Text]);


}

void UI::Text(string text, vec2 pos) {
	Render::DrawTextUI(text, workingWin.position + pos, style.colors[UIStyleCol_Text]);
}

void UI::Text(string text, Color color) {
	Render::DrawTextUI(text, workingWin.position + workingWin.cursor, color);
}

void UI::Text(string text, vec2 pos, Color color) {
	Render::DrawTextUI(text, workingWin.position + pos, color);
}


//Windows


bool CheckWindowExists(string name) {
	if (windows.has(name)) {
		return true;
	}
	return false;
}


void UI::BeginWindow(string name, vec2 pos, vec2 dimensions, UIWindowFlags flags) {
	//save previous window on stack
	windowStack.add(workingWin);

	//check if were making a new window or working with one we already know
	if (!windows.has(name)) {
		//make new window if we dont know this one already
		workingWin.      name = name;
		workingWin.  position = pos;
		workingWin.dimensions = dimensions;
		workingWin.    cursor = vec2(0, 0);
		workingWin.     flags = flags;
		windows[name] = workingWin;
	}
	else {
		workingWin = windows[name];
	}

	//if the window isn't invisible draw things that havent been disabled
	if ((flags & UIWindowFlags_Invisible) != UIWindowFlags_Invisible) {
		
		//draw background
		if (!(flags & UIWindowFlags_NoBackground)) 
			Render::FillRectUI(workingWin.position, workingWin.dimensions, style.colors[UIStyleCol_WindowBg]);

		//draw title bar
		if (!(flags & UIWindowFlags_NoTitleBar)) {
			Render::FillRectUI(workingWin.x, workingWin.y, workingWin.width, style.titleBarHeight, style.colors[UIStyleCol_TitleBg]);
			
			//draw text if it exists
			if (name.size != 0) {
				Render::DrawTextUI(
					workingWin.name,
					vec2(
						workingWin.x + (workingWin.width - name.size * style.font.width) * style.titleTextAlign.x,
						workingWin.y + (style.titleBarHeight - style.font.height) * style.titleTextAlign.y));
			}
		}
		
		//draw border
		if (!(flags & UIWindowFlags_NoBorder)) {
			//left
			Render::FillRectUI(
				workingWin.x - style.windowBorderSize, 
				workingWin.y, 
				style.windowBorderSize, 
				workingWin.height, 
				style.colors[UIStyleCol_Border]);
			
			//right 
			Render::FillRectUI(
				workingWin.x + workingWin.width, 
				workingWin.y, 
				style.windowBorderSize, 
				workingWin.height, 
				style.colors[UIStyleCol_Border]);
		
			//top
			Render::FillRectUI(
				workingWin.x - style.windowBorderSize, 
				workingWin.y - style.windowBorderSize, 
				workingWin.width + 2 * style.windowBorderSize, 
				style.windowBorderSize, 
				style.colors[UIStyleCol_Border]);

			//bottom
			Render::FillRectUI(
				workingWin.x - style.windowBorderSize, 
				workingWin.y + dimensions.y, 
				workingWin.width + 2 * style.windowBorderSize, 
				style.windowBorderSize, 
				style.colors[UIStyleCol_Border]);

		}

	}
}

void UI::EndWindow() {
	Assert(windowStack.size() > 1, "Attempted to end the base window");
	//update stored window with new window state
	//NOTE: I'm not sure if I want to keep doing this like this
	//      or just make workingWin a pointer to a window in the list
	windows[workingWin.name] = workingWin;
	workingWin = *windowStack.last;
	windowStack.pop();
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




//initializes core UI with an invisible working window covering the entire screen
//also initializes styles
void UI::Init() {
	workingWin.name = "Base";
	workingWin.position = vec2(0,0);
	workingWin.dimensions = DengWindow->dimensions;
	
	//set default style
	//TODO(sushi, Ui) set up the initial stack once i have pushing and popping it set up 
	
	//load font
	style.font.load_bdf_font("gohufont-11.bdf");

	Render::CreateFont(
		Render::LoadTexture(
			style.font.texture_sheet, style.font.width, style.font.height * style.font.char_count, 0),
			style.font.width, style.font.height, style.font.char_count);
  
	//push default color scheme
	//this is never meant to be popped
	PushColor(UIStyleCol_Border,   colors.near_black);
	PushColor(UIStyleCol_WindowBg, colors.midnight_blue);
	PushColor(UIStyleCol_TitleBg,  colors.purple_gray);
	PushColor(UIStyleCol_Text,     Color::WHITE);

	//push default style variables
	PushVar(UIStyleVar_WindowBorderSize, 1);
	PushVar(UIStyleVar_TitleBarHeight, style.font.height * 1.5);
	PushVar(UIStyleVar_TitleTextAlign, vec2(0, 0.5));

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



}

