//main cursor that determines where things will be draw on screen
//its position is updated whenever we draw something to the screen whether it be
//text, a box, a window, etc.
vec2 ui_cursor = vec2::ZERO;

//global styling
struct {
	vec2  windowPadding;
	float windowBorderSize;
	float titleBarHeight;
	Font  font;
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

UIWindow workingWin; //by default a window that takes up the entire screen
 
array<UIWindow> windows;    //window stack
array<ColorMod> colorStack; 
array<VarMod>   styleStack; 

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


void UI::Text(string text) {
	Render::DrawTextUI(text, ui_cursor, style.colors[UIStyleCol_Text]);
}

void UI::Text(string text, vec2 pos) {
	Render::DrawTextUI(text, workingWin.position + pos, style.colors[UIStyleCol_Text]);
}

void UI::Text(string text, Color color) {
	Render::DrawTextUI(text, ui_cursor, color);
}

void UI::Text(string text, vec2 pos, Color color) {
	Render::DrawTextUI(text, workingWin.position + pos, color);
}


//Windows

bool CheckWindowExists(string name) {

}


void UI::BeginWindow(string name, vec2 pos, vec2 dimensions, UIWindowFlags flags) {
	//save previous window
	windows.add(workingWin);

	workingWin.name = name;
	workingWin.position = pos;
	workingWin.dimensions = dimensions;
	workingWin.flags = flags;

	//if the window isn't invisible draw things that havent been disabled
	if ((flags & UIWindowFlags_Invisible) != UIWindowFlags_Invisible) {
		
		//draw background
		if (!(flags & UIWindowFlags_NoBackground)) 
			Render::FillRectUI(pos.x, pos.y, dimensions.x, dimensions.y, style.colors[UIStyleCol_WindowBg]);

		//draw title bar
		if (!(flags & UIWindowFlags_NoTitleBar)) {
			Render::FillRectUI(pos.x, pos.y, dimensions.x, style.titleBarHeight, style.colors[UIStyleCol_TitleBg]);
			if (name.size != 0) {
				Render::DrawTextUI(name, vec2(dimensions.x + 2, dimensions.y + 3));
			}
		}

		//draw border
		if (!(flags & UIWindowFlags_NoBorder)) {
			//left
			Render::FillRectUI(pos.x - style.windowBorderSize, pos.y, style.windowBorderSize, dimensions.y, style.colors[UIStyleCol_Border]);
			
			//right 
			Render::FillRectUI(pos.x + dimensions.x, pos.y, style.windowBorderSize, dimensions.y, style.colors[UIStyleCol_Border]);
		
			//top
			Render::FillRectUI(pos.x - style.windowBorderSize, pos.y - style.windowBorderSize, dimensions.x + 2 * style.windowBorderSize, style.windowBorderSize, style.colors[UIStyleCol_Border]);

			//bottom
			Render::FillRectUI(pos.x - style.windowBorderSize, pos.y + dimensions.y, dimensions.x + 2 * style.windowBorderSize, style.windowBorderSize, style.colors[UIStyleCol_Border]);

		}

	}



}

void UI::EndWindow() {
	Assert(windows.size() > 1, "Attempted to end the base window");
	workingWin = *windows.last;
	windows.pop();
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
	style.font.load_bdf_font("gohufont-14.bdf");

	style.windowBorderSize = 10.f;
	style.titleBarHeight = style.font.height + 6;

	style.colors[UIStyleCol_Border]   = Color::GREY;
	style.colors[UIStyleCol_WindowBg] = Color::VERY_DARK_CYAN;
	style.colors[UIStyleCol_TitleBg]  = Color::RED;
}

