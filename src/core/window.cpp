local bool _resized = false;
local int _width, _height, _x, _y;
local int opengl_version;

void glfwError(int id, const char* description){
	LogfE("glfw","%d: %s", id, description);
}

GLFWcursor* дефолткурсор;
GLFWcursor* hResizeCursor;
GLFWcursor* vResizeCursor;
GLFWcursor* handCursor;
GLFWcursor* textCursor;



void Window::Init(const char* _name, s32 width, s32 height, s32 x, s32 y, DisplayMode displayMode){
	TIMER_START(t_s);
	
	name = _name;
	glfwSetErrorCallback(&glfwError);
	if(!glfwInit()){ LogE("glfw","Failed to init!"); return; }
	
	monitor = glfwGetPrimaryMonitor();
	if(!monitor) { LogE("glfw","Failed to get the monitor!"); return; }
	int work_xpos, work_ypos, work_width, work_height;
	glfwGetMonitorWorkarea(monitor, &work_xpos, &work_ypos, &work_width, &work_height);
	
	glfwWindowHint(GLFW_RESIZABLE,               GLFW_TRUE); 
	glfwWindowHint(GLFW_FOCUSED,                 GLFW_FALSE);
	glfwWindowHint(GLFW_CENTER_CURSOR,           GLFW_FALSE);
	glfwWindowHint(GLFW_VISIBLE,                 GLFW_FALSE);
	glfwWindowHint(GLFW_FOCUS_ON_SHOW,           GLFW_FALSE);
	glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE);
	
#if   DESHI_VULKAN
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#elif DESHI_OPENGL //DESHI_VULKAN
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if DESHI_MAC
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif //DESHI_MAC
#endif //DESHI_OPENGL
	
	window = glfwCreateWindow(width, height, _name, NULL, NULL);
	if(!window){ LogE("glfw","Failed to create the window!"); glfwTerminate(); return; }
	
#if DESHI_OPENGL
	glfwMakeContextCurrent(window);
	opengl_version = gladLoadGL(glfwGetProcAddress);
	if(opengl_version == 0){ LogE("glad","Failed to load OpenGL!"); glfwTerminate(); return; }
	Logf("glad","Loaded OpenGL %d.%d", GLAD_VERSION_MAJOR(opengl_version), GLAD_VERSION_MINOR(opengl_version));
#endif //DESHI_OPENGL
	
	//set initial window size
	if(x == 0xFFFFFFFF || y == 0xFFFFFFFF){
		glfwSetWindowPos(window, work_width-width, work_height-height);
	}else{
		glfwSetWindowPos(window, work_xpos+x, work_ypos+y);
	}
	
	дефолткурсор = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
	hResizeCursor = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
	vResizeCursor = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
	handCursor = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
	textCursor = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);

	if(!дефолткурсор){ LogE("glfw","Failed to create the cursor!"); glfwTerminate(); return; }
	

	//glfwSetWindowOpacity(window, 0.5);
	
	//load and set icon
	GLFWimage icon; int icon_channels;
	icon.pixels = stbi_load("data/textures/deshi_icon.png", &icon.width, &icon.height, &icon_channels, STBI_rgb_alpha);
	if(icon.pixels){
		glfwSetWindowIcon(window, 1, &icon);
		stbi_image_free(icon.pixels);
	}else{
		LogE("stbi","Failed to load texture: deshi_icon.png; Using default window icon");
	}
	
	//set window's cursor
	glfwSetCursor(window, дефолткурсор);
	
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
	this->cursorMode = CursorMode_Default;
	this->dimensions = vec2(width, height);
	
	this->rawInput = false;
	UpdateRawInput(true); //sets raw input to true if supported
	this->resizable = true;
	
	this->resized = false;
	this->closeWindow = false;
	
	_width = width; _height = height;
	
	UpdateDisplayMode(displayMode);
	
	glfwSetInputMode(window, GLFW_LOCK_KEY_MODS, GLFW_TRUE);
	
	//keyboard mappings
	DeshInput->mapKeys[0x00] = Key::Key_NONE;
	DeshInput->mapKeys['A'] = Key::A; DeshInput->mapKeys['B'] = Key::B; DeshInput->mapKeys['C'] = Key::C;
	DeshInput->mapKeys['D'] = Key::D; DeshInput->mapKeys['E'] = Key::E; DeshInput->mapKeys['F'] = Key::F;
	DeshInput->mapKeys['G'] = Key::G; DeshInput->mapKeys['H'] = Key::H; DeshInput->mapKeys['I'] = Key::I;
	DeshInput->mapKeys['J'] = Key::J; DeshInput->mapKeys['K'] = Key::K; DeshInput->mapKeys['L'] = Key::L; 
	DeshInput->mapKeys['M'] = Key::M; DeshInput->mapKeys['N'] = Key::N; DeshInput->mapKeys['O'] = Key::O;
	DeshInput->mapKeys['P'] = Key::P; DeshInput->mapKeys['Q'] = Key::Q; DeshInput->mapKeys['R'] = Key::R;
	DeshInput->mapKeys['S'] = Key::S; DeshInput->mapKeys['T'] = Key::T; DeshInput->mapKeys['U'] = Key::U;
	DeshInput->mapKeys['V'] = Key::V; DeshInput->mapKeys['W'] = Key::W; DeshInput->mapKeys['X'] = Key::X;
	DeshInput->mapKeys['Y'] = Key::Y; DeshInput->mapKeys['Z'] = Key::Z;
	
	DeshInput->mapKeys[GLFW_KEY_0] = Key::K0; DeshInput->mapKeys[GLFW_KEY_1] = Key::K1;
	DeshInput->mapKeys[GLFW_KEY_2] = Key::K2; DeshInput->mapKeys[GLFW_KEY_3] = Key::K3; 
	DeshInput->mapKeys[GLFW_KEY_4] = Key::K4; DeshInput->mapKeys[GLFW_KEY_5] = Key::K5;
	DeshInput->mapKeys[GLFW_KEY_6] = Key::K6; DeshInput->mapKeys[GLFW_KEY_7] = Key::K7; 
	DeshInput->mapKeys[GLFW_KEY_8] = Key::K8; DeshInput->mapKeys[GLFW_KEY_9] = Key::K9;
	
	DeshInput->mapKeys[GLFW_KEY_F1]  = Key::F1;  DeshInput->mapKeys[GLFW_KEY_F2]  = Key::F2; 
	DeshInput->mapKeys[GLFW_KEY_F3]  = Key::F3;  DeshInput->mapKeys[GLFW_KEY_F4]  = Key::F4; 
	DeshInput->mapKeys[GLFW_KEY_F5]  = Key::F5;  DeshInput->mapKeys[GLFW_KEY_F6]  = Key::F6;
	DeshInput->mapKeys[GLFW_KEY_F7]  = Key::F7;  DeshInput->mapKeys[GLFW_KEY_F8]  = Key::F8; 
	DeshInput->mapKeys[GLFW_KEY_F9]  = Key::F9;  DeshInput->mapKeys[GLFW_KEY_F10] = Key::F10;
	DeshInput->mapKeys[GLFW_KEY_F11] = Key::F11; DeshInput->mapKeys[GLFW_KEY_F12] = Key::F12;
	
	DeshInput->mapKeys[GLFW_KEY_DOWN] = Key::DOWN; DeshInput->mapKeys[GLFW_KEY_UP]    = Key::UP;
	DeshInput->mapKeys[GLFW_KEY_LEFT] = Key::LEFT; DeshInput->mapKeys[GLFW_KEY_RIGHT] = Key::RIGHT;
	
	DeshInput->mapKeys[GLFW_KEY_ESCAPE]        = Key::ESCAPE; 
	DeshInput->mapKeys[GLFW_KEY_GRAVE_ACCENT]  = Key::TILDE;
	DeshInput->mapKeys[GLFW_KEY_TAB]           = Key::TAB; 
	DeshInput->mapKeys[GLFW_KEY_CAPS_LOCK]     = Key::CAPSLOCK;
	DeshInput->mapKeys[GLFW_KEY_LEFT_SHIFT]    = Key::LSHIFT; 
	DeshInput->mapKeys[GLFW_KEY_RIGHT_SHIFT]   = Key::RSHIFT;
	DeshInput->mapKeys[GLFW_KEY_LEFT_CONTROL]  = Key::LCTRL; 
	DeshInput->mapKeys[GLFW_KEY_RIGHT_CONTROL] = Key::RCTRL;
	DeshInput->mapKeys[GLFW_KEY_LEFT_ALT]      = Key::LALT; 
	DeshInput->mapKeys[GLFW_KEY_RIGHT_ALT]     = Key::RALT; 
	DeshInput->mapKeys[GLFW_KEY_BACKSPACE]     = Key::BACKSPACE; 
	DeshInput->mapKeys[GLFW_KEY_ENTER]         = Key::ENTER;
	DeshInput->mapKeys[GLFW_KEY_MINUS]         = Key::MINUS; 
	DeshInput->mapKeys[GLFW_KEY_EQUAL]         = Key::EQUALS;
	DeshInput->mapKeys[GLFW_KEY_LEFT_BRACKET]  = Key::LBRACKET; 
	DeshInput->mapKeys[GLFW_KEY_RIGHT_BRACKET] = Key::RBRACKET;
	DeshInput->mapKeys[GLFW_KEY_SLASH]         = Key::SLASH; 
	DeshInput->mapKeys[GLFW_KEY_SEMICOLON]     = Key::SEMICOLON;
	DeshInput->mapKeys[GLFW_KEY_APOSTROPHE]    = Key::APOSTROPHE; 
	DeshInput->mapKeys[GLFW_KEY_COMMA]         = Key::COMMA;
	DeshInput->mapKeys[GLFW_KEY_PERIOD]        = Key::PERIOD;
	DeshInput->mapKeys[GLFW_KEY_BACKSLASH]     = Key::BACKSLASH;
	DeshInput->mapKeys[GLFW_KEY_SPACE]         = Key::SPACE;
	
	DeshInput->mapKeys[GLFW_KEY_INSERT]  = Key::INSERT; DeshInput->mapKeys[GLFW_KEY_DELETE]      = Key::DELETE;
	DeshInput->mapKeys[GLFW_KEY_HOME]    = Key::HOME;   DeshInput->mapKeys[GLFW_KEY_END]         = Key::END;
	DeshInput->mapKeys[GLFW_KEY_PAGE_UP] = Key::PAGEUP; DeshInput->mapKeys[GLFW_KEY_PAGE_DOWN]   = Key::PAGEDOWN;
	DeshInput->mapKeys[GLFW_KEY_PAUSE]   = Key::PAUSE;  DeshInput->mapKeys[GLFW_KEY_SCROLL_LOCK] = Key::SCROLL;
	
	DeshInput->mapKeys[GLFW_KEY_KP_0] = Key::NUMPAD0; DeshInput->mapKeys[GLFW_KEY_KP_1] = Key::NUMPAD1;
	DeshInput->mapKeys[GLFW_KEY_KP_2] = Key::NUMPAD2; DeshInput->mapKeys[GLFW_KEY_KP_3] = Key::NUMPAD3;
	DeshInput->mapKeys[GLFW_KEY_KP_4] = Key::NUMPAD4; DeshInput->mapKeys[GLFW_KEY_KP_5] = Key::NUMPAD5;
	DeshInput->mapKeys[GLFW_KEY_KP_6] = Key::NUMPAD6; DeshInput->mapKeys[GLFW_KEY_KP_7] = Key::NUMPAD7;
	DeshInput->mapKeys[GLFW_KEY_KP_8] = Key::NUMPAD8; DeshInput->mapKeys[GLFW_KEY_KP_9] = Key::NUMPAD9;
	
	DeshInput->mapKeys[GLFW_KEY_KP_MULTIPLY] = Key::NUMPADMULTIPLY;
	DeshInput->mapKeys[GLFW_KEY_KP_DIVIDE]   = Key::NUMPADDIVIDE;
	DeshInput->mapKeys[GLFW_KEY_KP_ADD]      = Key::NUMPADPLUS; 
	DeshInput->mapKeys[GLFW_KEY_KP_SUBTRACT] = Key::NUMPADMINUS;
	DeshInput->mapKeys[GLFW_KEY_KP_DECIMAL]  = Key::NUMPADPERIOD; 
	DeshInput->mapKeys[GLFW_KEY_KP_ENTER]    = Key::NUMPADENTER;
	DeshInput->mapKeys[GLFW_KEY_NUM_LOCK]    = Key::NUMLOCK;
	
	//mouse mappings
	DeshInput->mapMouse[GLFW_MOUSE_BUTTON_1] = MouseButton::LEFT;
	DeshInput->mapMouse[GLFW_MOUSE_BUTTON_2] = MouseButton::RIGHT;
	DeshInput->mapMouse[GLFW_MOUSE_BUTTON_3] = MouseButton::MIDDLE;
	DeshInput->mapMouse[GLFW_MOUSE_BUTTON_4] = MouseButton::FOUR;
	DeshInput->mapMouse[GLFW_MOUSE_BUTTON_5] = MouseButton::FIVE;
	DeshInput->mapMouse[GLFW_MOUSE_BUTTON_6] = MouseButton::SIX;
	DeshInput->mapMouse[GLFW_MOUSE_BUTTON_7] = MouseButton::SEVEN;
	DeshInput->mapMouse[GLFW_MOUSE_BUTTON_8] = MouseButton::EIGHT;
	
	//event callbacks
	//void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
	glfwSetMouseButtonCallback(window, 
							   [](GLFWwindow* w, int button, int action, int mods)->void{
								   std::map<size_t, u8>::iterator it = DeshInput->mapMouse.find(button);
								   if(it != DeshInput->mapMouse.end()){
									   if(action == GLFW_PRESS){
										   DeshInput->realKeyState[it->second] = true;
										   DeshInput->checkbinds = true;
										   if(DeshInput->logInput) { Log("input","{m", it->second, "|", mods,"}"); }
									   }else if(action == GLFW_RELEASE){
										   DeshInput->realKeyState[it->second] = false;
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
								 DeshInput->mouseFocus = true;
								 DeshInput->realMouseX = xpos - (double)x;
								 DeshInput->realMouseY = ypos - (double)y;
								 DeshInput->realScreenMouseX = xpos;
								 DeshInput->realScreenMouseY = ypos;
							 });
	
	//void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
	glfwSetKeyCallback(window,
					   [](GLFWwindow* window, int key, int scancode, int action, int mods)->void{
						   std::map<size_t, u8>::iterator it = DeshInput->mapKeys.find(key);
						   if (mods & GLFW_MOD_CAPS_LOCK) DeshInput->capsLock = 1;
						   else DeshInput->capsLock = 0;
						   if(it != DeshInput->mapKeys.end()){
							   if(action == GLFW_PRESS){
								   DeshInput->realKeyState[it->second] = true;
								   DeshInput->checkbinds = true;
								   if(DeshInput->logInput) { Log("input","{k", it->second, "|", mods,"}"); }
							   }else if(action == GLFW_RELEASE){
								   DeshInput->realKeyState[it->second] = false;
							   }
						   }
					   });
	
	//void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
	glfwSetScrollCallback(window, 
						  [](GLFWwindow* window, double xoffset, double yoffset)->void{
							  DeshInput->realScrollX = xoffset;
							  DeshInput->realScrollY = yoffset;
						  });
	
	//void function_name(GLFWwindow* window, int width, int height)
	glfwSetFramebufferSizeCallback(window, 
								   [](GLFWwindow* window, int width, int height)->void{
									   if(width != _width || height != _height) _resized = true;
								   });
	
	//TODO(sushi) implement this function for use on InputText()
	//glfwSetCharCallback()
	
	Log("deshi","Finished window initialization in ",TIMER_END(t_s),"ms");
} //Init()

void Window::Update() {
	TIMER_START(t_d);
	glfwPollEvents();
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
	
	glfwGetCursorPos(window, &DeshInput->mouseX, &DeshInput->mouseY);
	if(cursorMode == CursorMode_FirstPerson){ glfwSetCursorPos(window, width/2, height/2); }
	DeshTime->windowTime = TIMER_END(t_d);
}

void Window::Cleanup(){
	glfwTerminate();
}

void Window::UpdateDisplayMode(DisplayMode displayMode){
	if(displayMode == this->displayMode){return;}
	if(this->displayMode == DisplayMode_Windowed || this->displayMode == DisplayMode_Borderless) {
		restoreX = x;  restoreY = y; 
		restoreW = width; restoreH = height;
	}
	
	this->displayMode = displayMode;
	
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);
	switch(displayMode){
		case(DisplayMode_Fullscreen):{
			glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
		}break;
		case(DisplayMode_Borderless): {
			glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_FALSE);
		}break;
		case(DisplayMode_BorderlessFullscreen):{
			glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_FALSE);
			glfwSetWindowMonitor(window, 0, 0, 0, mode->width, mode->height, mode->refreshRate);
		}break;
		case(DisplayMode_Windowed):default:{
			glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_TRUE);
			glfwSetWindowMonitor(window, 0, restoreX, restoreY, restoreW, restoreH, GLFW_DONT_CARE);
		}break;
	}
}

void Window::UpdateCursorMode(CursorMode mode){
	if(mode == this->cursorMode){return;}
	this->cursorMode = mode;
	
	switch(mode){
		case(CursorMode_Default):default:{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}break;
		case(CursorMode_FirstPerson):{
			int w, h;
			glfwGetWindowSize(window, &w, &h);
			glfwSetCursorPos(window, w/2, h/2);
			glfwGetCursorPos(window, &DeshInput->mouseX, &DeshInput->mouseY);
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}break;
		case(CursorMode_Hidden):{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
		}break;
	}
}

void Window::SetCursorPos(vec2 pos){
	glfwSetCursorPos(window, pos.x, pos.y);
}

void Window::SetCursor(CursorType curtype) {
	switch (curtype) {
		case CursorType_Arrow:   glfwSetCursor(window, дефолткурсор);  break;
		case CursorType_HResize: glfwSetCursor(window, hResizeCursor); break;
		case CursorType_VResize: glfwSetCursor(window, vResizeCursor); break;
		case CursorType_Hand:    glfwSetCursor(window, handCursor);    break;
		case CursorType_IBeam:   glfwSetCursor(window, textCursor);    break;
	}
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

void Window::Close(){
	closeWindow = true;
}

void Window::UpdateTitle(const char* title){
	glfwSetWindowTitle(this->window, title);
}

void Window::ShowWindow(){
	glfwShowWindow(window);
}

void Window::HideWindow(){
	glfwHideWindow(window);
}

b32 Window::ShouldClose(){
	return glfwWindowShouldClose(window) || closeWindow;
}