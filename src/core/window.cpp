local bool _resized = false;
local int _width, _height, _x, _y;

void glfwError(int id, const char* description){
	std::cout << description << std::endl;
}

//thanks: https://github.com/OneLoneCoder/olcPixelGameEngine/pull/181
void Window::Init(s32 width, s32 height, s32 x, s32 y, DisplayMode displayMode){
	glfwSetErrorCallback(&glfwError);
	if(!glfwInit()){ PRINTLN("GLFW failed to "); return; }
	
	monitor = glfwGetPrimaryMonitor();
	if(!monitor) { PRINTLN("GLFW failed to get the monitor!"); return; }
	int work_xpos, work_ypos, work_width, work_height;
	glfwGetMonitorWorkarea(monitor, &work_xpos, &work_ypos, &work_width, &work_height);
	
	//TODO(delle,Wi) maybe we should not allow the window to be resizable in-game, but in-engine is fine
	//i think its fine if its resizable in windowed mode during gameplay, i dont see any reason not to allow that
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE); 
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_FOCUSED, GLFW_FALSE);
	glfwWindowHint(GLFW_CENTER_CURSOR, GLFW_FALSE);
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_FALSE);
#if DESHI_OPENGL
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if DESHI_MAC
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif //DESHI_MAC
#endif //DESHI_OPENGL
	
	window = glfwCreateWindow(width, height, "deshi", NULL, NULL);
	if(!window) { PRINTLN("GLFW failed to create the window!"); glfwTerminate(); return; }
	
	//set initial window size
	if(x == 0xFFFFFFFF || y == 0xFFFFFFFF){
		glfwSetWindowPos(window, work_width-width, work_height-height);
	}else{
		glfwSetWindowPos(window, work_xpos+x, work_ypos+y);
	}

	cursor  = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
	if(!cursor){ PRINTLN("GLFW failed to create the cursor!"); glfwTerminate(); return; }
	
	//load and set icon
	GLFWimage icon; int icon_channels;
	icon.pixels = stbi_load("data/textures/deshi_icon.png", &icon.width, &icon.height, &icon_channels, STBI_rgb_alpha);
	if(icon.pixels){
		glfwSetWindowIcon(window, 1, &icon);
	}else{
		PRINTLN("Failed to load texture: deshi_icon.png; Using default window icon");
	}
	
	//set window's cursor
	glfwSetCursor(window, cursor);

	const GLFWvidmode* mode = glfwGetVideoMode(monitor);
	int xpos, ypos;
	glfwGetWindowPos(window, &xpos, &ypos);
	this->x = xpos; this->y = ypos;
	this->width = width; this->height = height;
	this->restoreX = x; this->restoreY = y;
	this->restoreW = width; this->restoreH = height;
	this->screenWidth = mode->width; this->screenHeight = mode->height;
	this->screenRefreshRate = mode->refreshRate;
	this->displayMode = displayMode;
	this->cursorMode = CursorMode::DEFAULT;
	this->dimensions = vec2(width, height);
	
	this->rawInput = false;
	UpdateRawInput(true); //sets raw input to true if supported
	this->resizable = true;
	
	this->resized = false;
	this->closeWindow = false;
	
	_width = width; _height = height;
	
	UpdateDisplayMode(displayMode);
	
	//keyboard mappings
	DengInput->mapKeys[0x00] = Key::Key_NONE;
	DengInput->mapKeys['A'] = Key::A; DengInput->mapKeys['B'] = Key::B; DengInput->mapKeys['C'] = Key::C;
	DengInput->mapKeys['D'] = Key::D; DengInput->mapKeys['E'] = Key::E; DengInput->mapKeys['F'] = Key::F;
	DengInput->mapKeys['G'] = Key::G; DengInput->mapKeys['H'] = Key::H; DengInput->mapKeys['I'] = Key::I;
	DengInput->mapKeys['J'] = Key::J; DengInput->mapKeys['K'] = Key::K; DengInput->mapKeys['L'] = Key::L; 
	DengInput->mapKeys['M'] = Key::M; DengInput->mapKeys['N'] = Key::N; DengInput->mapKeys['O'] = Key::O;
	DengInput->mapKeys['P'] = Key::P; DengInput->mapKeys['Q'] = Key::Q; DengInput->mapKeys['R'] = Key::R;
	DengInput->mapKeys['S'] = Key::S; DengInput->mapKeys['T'] = Key::T; DengInput->mapKeys['U'] = Key::U;
	DengInput->mapKeys['V'] = Key::V; DengInput->mapKeys['W'] = Key::W; DengInput->mapKeys['X'] = Key::X;
	DengInput->mapKeys['Y'] = Key::Y; DengInput->mapKeys['Z'] = Key::Z;
	
	DengInput->mapKeys[GLFW_KEY_0] = Key::K0; DengInput->mapKeys[GLFW_KEY_1] = Key::K1;
	DengInput->mapKeys[GLFW_KEY_2] = Key::K2; DengInput->mapKeys[GLFW_KEY_3] = Key::K3; 
	DengInput->mapKeys[GLFW_KEY_4] = Key::K4; DengInput->mapKeys[GLFW_KEY_5] = Key::K5;
	DengInput->mapKeys[GLFW_KEY_6] = Key::K6; DengInput->mapKeys[GLFW_KEY_7] = Key::K7; 
	DengInput->mapKeys[GLFW_KEY_8] = Key::K8; DengInput->mapKeys[GLFW_KEY_9] = Key::K9;
	
	DengInput->mapKeys[GLFW_KEY_F1]  = Key::F1;  DengInput->mapKeys[GLFW_KEY_F2]  = Key::F2; 
	DengInput->mapKeys[GLFW_KEY_F3]  = Key::F3;  DengInput->mapKeys[GLFW_KEY_F4]  = Key::F4; 
	DengInput->mapKeys[GLFW_KEY_F5]  = Key::F5;  DengInput->mapKeys[GLFW_KEY_F6]  = Key::F6;
	DengInput->mapKeys[GLFW_KEY_F7]  = Key::F7;  DengInput->mapKeys[GLFW_KEY_F8]  = Key::F8; 
	DengInput->mapKeys[GLFW_KEY_F9]  = Key::F9;  DengInput->mapKeys[GLFW_KEY_F10] = Key::F10;
	DengInput->mapKeys[GLFW_KEY_F11] = Key::F11; DengInput->mapKeys[GLFW_KEY_F12] = Key::F12;
	
	DengInput->mapKeys[GLFW_KEY_DOWN] = Key::DOWN; DengInput->mapKeys[GLFW_KEY_UP]    = Key::UP;
	DengInput->mapKeys[GLFW_KEY_LEFT] = Key::LEFT; DengInput->mapKeys[GLFW_KEY_RIGHT] = Key::RIGHT;
	
	DengInput->mapKeys[GLFW_KEY_ESCAPE]        = Key::ESCAPE; 
	DengInput->mapKeys[GLFW_KEY_GRAVE_ACCENT]  = Key::TILDE;
	DengInput->mapKeys[GLFW_KEY_TAB]           = Key::TAB; 
	DengInput->mapKeys[GLFW_KEY_CAPS_LOCK]     = Key::CAPSLOCK;
	DengInput->mapKeys[GLFW_KEY_LEFT_SHIFT]    = Key::LSHIFT; 
	DengInput->mapKeys[GLFW_KEY_RIGHT_SHIFT]   = Key::RSHIFT;
	DengInput->mapKeys[GLFW_KEY_LEFT_CONTROL]  = Key::LCTRL; 
	DengInput->mapKeys[GLFW_KEY_RIGHT_CONTROL] = Key::RCTRL;
	DengInput->mapKeys[GLFW_KEY_LEFT_ALT]      = Key::LALT; 
	DengInput->mapKeys[GLFW_KEY_RIGHT_ALT]     = Key::RALT; 
	DengInput->mapKeys[GLFW_KEY_BACKSPACE]     = Key::BACKSPACE; 
	DengInput->mapKeys[GLFW_KEY_ENTER]         = Key::ENTER;
	DengInput->mapKeys[GLFW_KEY_MINUS]         = Key::MINUS; 
	DengInput->mapKeys[GLFW_KEY_EQUAL]         = Key::EQUALS;
	DengInput->mapKeys[GLFW_KEY_LEFT_BRACKET]  = Key::LBRACKET; 
	DengInput->mapKeys[GLFW_KEY_RIGHT_BRACKET] = Key::RBRACKET;
	DengInput->mapKeys[GLFW_KEY_SLASH]         = Key::SLASH; 
	DengInput->mapKeys[GLFW_KEY_SEMICOLON]     = Key::SEMICOLON;
	DengInput->mapKeys[GLFW_KEY_APOSTROPHE]    = Key::APOSTROPHE; 
	DengInput->mapKeys[GLFW_KEY_COMMA]         = Key::COMMA;
	DengInput->mapKeys[GLFW_KEY_PERIOD]        = Key::PERIOD;
	DengInput->mapKeys[GLFW_KEY_BACKSLASH]     = Key::BACKSLASH;
	DengInput->mapKeys[GLFW_KEY_SPACE]         = Key::SPACE;
	
	DengInput->mapKeys[GLFW_KEY_INSERT]  = Key::INSERT; DengInput->mapKeys[GLFW_KEY_DELETE]      = Key::DELETE;
	DengInput->mapKeys[GLFW_KEY_HOME]    = Key::HOME;   DengInput->mapKeys[GLFW_KEY_END]         = Key::END;
	DengInput->mapKeys[GLFW_KEY_PAGE_UP] = Key::PAGEUP; DengInput->mapKeys[GLFW_KEY_PAGE_DOWN]   = Key::PAGEDOWN;
	DengInput->mapKeys[GLFW_KEY_PAUSE]   = Key::PAUSE;  DengInput->mapKeys[GLFW_KEY_SCROLL_LOCK] = Key::SCROLL;
	
	DengInput->mapKeys[GLFW_KEY_KP_0] = Key::NUMPAD0; DengInput->mapKeys[GLFW_KEY_KP_1] = Key::NUMPAD1;
	DengInput->mapKeys[GLFW_KEY_KP_2] = Key::NUMPAD2; DengInput->mapKeys[GLFW_KEY_KP_3] = Key::NUMPAD3;
	DengInput->mapKeys[GLFW_KEY_KP_4] = Key::NUMPAD4; DengInput->mapKeys[GLFW_KEY_KP_5] = Key::NUMPAD5;
	DengInput->mapKeys[GLFW_KEY_KP_6] = Key::NUMPAD6; DengInput->mapKeys[GLFW_KEY_KP_7] = Key::NUMPAD7;
	DengInput->mapKeys[GLFW_KEY_KP_8] = Key::NUMPAD8; DengInput->mapKeys[GLFW_KEY_KP_9] = Key::NUMPAD9;
	
	DengInput->mapKeys[GLFW_KEY_KP_MULTIPLY] = Key::NUMPADMULTIPLY;
	DengInput->mapKeys[GLFW_KEY_KP_DIVIDE]   = Key::NUMPADDIVIDE;
	DengInput->mapKeys[GLFW_KEY_KP_ADD]      = Key::NUMPADPLUS; 
	DengInput->mapKeys[GLFW_KEY_KP_SUBTRACT] = Key::NUMPADMINUS;
	DengInput->mapKeys[GLFW_KEY_KP_DECIMAL]  = Key::NUMPADPERIOD; 
	DengInput->mapKeys[GLFW_KEY_KP_ENTER]    = Key::NUMPADENTER;
	DengInput->mapKeys[GLFW_KEY_NUM_LOCK]    = Key::NUMLOCK;
	
	//mouse mappings
	DengInput->mapMouse[GLFW_MOUSE_BUTTON_1] = MouseButton::LEFT;
	DengInput->mapMouse[GLFW_MOUSE_BUTTON_2] = MouseButton::RIGHT;
	DengInput->mapMouse[GLFW_MOUSE_BUTTON_3] = MouseButton::MIDDLE;
	DengInput->mapMouse[GLFW_MOUSE_BUTTON_4] = MouseButton::FOUR;
	DengInput->mapMouse[GLFW_MOUSE_BUTTON_5] = MouseButton::FIVE;
	DengInput->mapMouse[GLFW_MOUSE_BUTTON_6] = MouseButton::SIX;
	DengInput->mapMouse[GLFW_MOUSE_BUTTON_7] = MouseButton::SEVEN;
	DengInput->mapMouse[GLFW_MOUSE_BUTTON_8] = MouseButton::EIGHT;
	
	//event callbacks
	//void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
	glfwSetMouseButtonCallback(window, 
							   [](GLFWwindow* w, int button, int action, int mods)->void{
								   std::map<size_t, u8>::iterator it = DengInput->mapMouse.find(button);
								   if(it != DengInput->mapMouse.end()){
									   if(action == GLFW_PRESS){
										   DengInput->realKeyState[it->second] = true;
										   DengInput->checkbinds = true;
										   if(DengInput->logInput) { LOG("{m", it->second, "|", mods,"}"); }
									   }else if(action == GLFW_RELEASE){
										   DengInput->realKeyState[it->second] = false;
									   }
								   }
							   });
	
	//void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
	glfwSetCursorPosCallback(window,
							 [](GLFWwindow* window, double xpos, double ypos)->void{
								 //mouse coords come in screen space, must convert to window space
								 GLFWmonitor* monitor = glfwGetPrimaryMonitor();
								 int x, y, width, height;
								 glfwGetMonitorWorkarea(monitor, &x, &y, &width, &height);
								 DengInput->mouseFocus = true;
								 DengInput->realMouseX = xpos - (double)x;
								 DengInput->realMouseY = ypos - (double)y;
								 DengInput->realScreenMouseX = xpos;
								 DengInput->realScreenMouseY = ypos;
							 });
	
	//void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
	glfwSetKeyCallback(window,
					   [](GLFWwindow* window, int key, int scancode, int action, int mods)->void{
						   std::map<size_t, u8>::iterator it = DengInput->mapKeys.find(key);
						   if(it != DengInput->mapKeys.end()){
							   if(action == GLFW_PRESS){
								   DengInput->realKeyState[it->second] = true;
								   DengInput->checkbinds = true;
								   if(DengInput->logInput) { LOG("{k", it->second, "|", mods,"}"); }
							   }else if(action == GLFW_RELEASE){
								   DengInput->realKeyState[it->second] = false;
							   }
						   }
					   });
	
	//void cursor_enter_callback(GLFWwindow* window, int entered)
	glfwSetCursorEnterCallback(window, 
							   [](GLFWwindow* w, int entered)->void{
								   DengInput->keyFocus = entered;
								   glfwSetCursor(w, g_window->cursor);
							   });
	
	//void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
	glfwSetScrollCallback(window, 
						  [](GLFWwindow* window, double xoffset, double yoffset)->void{
							  DengInput->realScrollX = xoffset;
							  DengInput->realScrollY = yoffset;
						  });
	
	//void function_name(GLFWwindow* window, int width, int height)
	glfwSetFramebufferSizeCallback(window, 
								   [](GLFWwindow* window, int width, int height)->void{
									   if(width != _width || height != _height) _resized = true;
								   });
}//Init

void Window::Update() {
	glfwGetWindowPos(window, &_x, &_y);
	x = _x; y = _y;
	
	resized = false;
	if(_resized) {
		resized = true;
		_resized = false;
	}
	
	glfwGetWindowSize(window, &_width, &_height);
	width = _width; height = _height;
	minimized = (width <= 0 || height <= 0) ? true : false;
	
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);
	screenWidth = mode->width; screenHeight = mode->height;
	screenRefreshRate = mode->refreshRate; 
	
	centerX = width/2; centerY = height/2;
	this->dimensions = vec2(width, height);
	
	glfwGetCursorPos(window, &DengInput->mouseX, &DengInput->mouseY);
	if(cursorMode == CursorMode::FIRSTPERSON){ glfwSetCursorPos(window, width/2, height/2); }
}

void Window::Cleanup(){
	glfwTerminate();
}

void Window::UpdateDisplayMode(DisplayMode displayMode){
	if(displayMode == this->displayMode){return;}
	if(this->displayMode == DisplayMode::WINDOWED){
		restoreX = x;  restoreY = y; 
		restoreW = width; restoreH = height;
	}
	
	this->displayMode = displayMode;
	
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);
	switch(displayMode){
		case(DisplayMode::FULLSCREEN):{
			glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
		}break;
		case(DisplayMode::BORDERLESS):{
			glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_FALSE);
			glfwSetWindowMonitor(window, 0, 0, 0, mode->width, mode->height, mode->refreshRate);
		}break;
		case(DisplayMode::WINDOWED):default:{
			glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_TRUE);
			glfwSetWindowMonitor(window, 0, restoreX, restoreY, restoreW, restoreH, GLFW_DONT_CARE);
		}break;
	}
}

void Window::UpdateCursorMode(CursorMode mode){
	if(mode == this->cursorMode){return;}
	this->cursorMode = mode;
	
	switch(mode){
		case(CursorMode::DEFAULT):default:{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}break;
		case(CursorMode::FIRSTPERSON):{
			int w, h;
			glfwGetWindowSize(window, &w, &h);
			glfwSetCursorPos(window, w/2, h/2);
			glfwGetCursorPos(window, &DengInput->mouseX, &DengInput->mouseY);
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}break;
		case(CursorMode::HIDDEN):{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
		}break;
	}
}

void Window::SetCursorPos(vec2 pos){
	glfwSetCursorPos(window, pos.x, pos.y);
}

void Window::UpdateRawInput(bool rawInput){
	if (glfwRawMouseMotionSupported()){
		this->rawInput = rawInput;
		glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, rawInput);
	}
}

void Window::UpdateResizable(bool resizable){
	this->resizable = resizable;
	glfwSetWindowAttrib(this->window, GLFW_RESIZABLE, resizable);
}

void Window::Close() {
	closeWindow = true;
}

void Window::UpdateTitle(const char* title){
	glfwSetWindowTitle(this->window, title);
}

std::string Window::str(){
	std::string dispMode;
	switch (displayMode) {
		case(DisplayMode::WINDOWED):   { dispMode = "Windowed"; }break;
		case(DisplayMode::BORDERLESS): { dispMode = "Borderless Windowed"; }break;
		case(DisplayMode::FULLSCREEN): { dispMode = "Fullscreen"; }break;
	}
	std::string cursMode;
	switch (cursorMode) {
		case(CursorMode::DEFAULT):     { cursMode = "Default"; }break;
		case(CursorMode::FIRSTPERSON): { cursMode = "First Person"; }break;
		case(CursorMode::HIDDEN):      { cursMode = "Hidden"; }break;
	}
	return TOSTDSTRING("Window Info"
					"\n    Window Position: ", x, ",", y,
					"\n    Window Dimensions: ", width, "x", height,
					"\n    Screen Dimensions: ", screenWidth, "x", screenHeight,
					"\n    Refresh Rate: ", refreshRate,
					"\n    Screen Refresh Rate: ", screenRefreshRate,
					"\n    Display Mode: ", dispMode,
					"\n    Cursor Mode: ", cursMode,
					"\n    Raw Input: ", rawInput,
					"\n    Resizable: ", resizable,
					"\n    Restores: ", restoreX, ",", restoreY, " ", restoreW, "x", restoreH);
}