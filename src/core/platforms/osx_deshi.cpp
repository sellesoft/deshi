local b32 _resized = false;
local int _width, _height, _x, _y;
local int opengl_version;
local b32 block_mouse_pos_change = false;


void glfwError(int id, const char* description){
	LogfE("glfw","%d: %s", id, description);
}

GLFWcursor* дефолткурсор;
GLFWcursor* hResizeCursor;
GLFWcursor* vResizeCursor;
GLFWcursor* rDiagResizeCursor;
GLFWcursor* lDiagResizeCursor;
GLFWcursor* handCursor;
GLFWcursor* textCursor;


void Window::Init(str8 _name, s32 width, s32 height, s32 x, s32 y, DisplayMode displayMode){
	AssertDS(DS_MEMORY, "Attempt to load Console without loading Memory first");
	deshiStage |= DS_WINDOW;

	TIMER_START(t_s);

	name = str8_copy(_name, deshi_allocator); //!Leak
	glfwSetErrorCallback(&glfwError);
	if(!glfwInit()){ LogE("glfw","Failed to init!"); return; }

	monitor = glfwGetPrimaryMonitor();
	if(!monitor) { LogE("glfw","Failed to get the monitor!"); return; }
	int work_xpos, work_ypos, work_width, work_height;
	glfwGetMonitorWorkarea(monitor, &work_xpos, &work_ypos, &work_width, &work_height);

	glfwWindowHint(GLFW_RESIZABLE,               GLFW_TRUE);
	//glfwWindowHint(GLFW_FOCUSED,                 GLFW_FALSE); //TODO(delle) make this a config
	glfwWindowHint(GLFW_CENTER_CURSOR,           GLFW_FALSE);
	glfwWindowHint(GLFW_VISIBLE,                 GLFW_FALSE);
	glfwWindowHint(GLFW_FOCUS_ON_SHOW,           GLFW_TRUE);
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

	window = glfwCreateWindow(width, height, (const char*)_name.str, NULL, NULL);
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

	GLFWimage image;
	image.width = 16;
	image.height = 16;

	image.pixels = (u8*)defaultcur;
	дефолткурсор = glfwCreateCursor(&image, 0, 0); //glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
	image.pixels = (u8*)hresizecur;
	hResizeCursor = glfwCreateCursor(&image, 8, 8); //glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
	image.pixels = (u8*)vresizecur;
	vResizeCursor = glfwCreateCursor(&image, 8, 8); //glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
	image.pixels = (u8*)rightdiagresizecur;
	rDiagResizeCursor = glfwCreateCursor(&image, 8, 8);
	image.pixels = (u8*)leftdiagresizecur;
	lDiagResizeCursor = glfwCreateCursor(&image, 8, 8);
	image.pixels = (u8*)handcur;
	handCursor = glfwCreateCursor(&image, 8, 8); //glfwCreateStandardCursor(GLFW_HAND_CURSOR);
	image.pixels = (u8*)textcur;
	textCursor = glfwCreateCursor(&image, 8, 8); //glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);

	if(!дефолткурсор){ LogE("glfw","Failed to create the cursor!"); glfwTerminate(); return; }


	//glfwSetWindowOpacity(window, 0.5);

	//load and set icon
	GLFWimage icon; int icon_channels;
	icon.pixels = stbi_load("data/textures/deshi_icon.png", &icon.width, &icon.height, &icon_channels, STBI_rgb_alpha);
	if(icon.pixels){
		deshi__memory_allocinfo_set(icon.pixels, cstring_lit("deshi_icon.png"), 0);
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
	this->dimensions = vec2((f32)width, (f32)height);

	this->rawInput = false;
	UpdateRawInput(true); //sets raw input to true if supported
	this->resizable = true;

	this->resized = false;
	this->closeWindow = false;

	_width = width; _height = height;

	UpdateDisplayMode(displayMode);

	glfwSetInputMode(window, GLFW_LOCK_KEY_MODS, GLFW_TRUE);

	//keyboard mappings
	DeshInput->mapKeys[0x00] = KeyCode_NONE;
	DeshInput->mapKeys['A'] = Key_A; DeshInput->mapKeys['B'] = Key_B; DeshInput->mapKeys['C'] = Key_C;
	DeshInput->mapKeys['D'] = Key_D; DeshInput->mapKeys['E'] = Key_E; DeshInput->mapKeys['F'] = Key_F;
	DeshInput->mapKeys['G'] = Key_G; DeshInput->mapKeys['H'] = Key_H; DeshInput->mapKeys['I'] = Key_I;
	DeshInput->mapKeys['J'] = Key_J; DeshInput->mapKeys['K'] = Key_K; DeshInput->mapKeys['L'] = Key_L;
	DeshInput->mapKeys['M'] = Key_M; DeshInput->mapKeys['N'] = Key_N; DeshInput->mapKeys['O'] = Key_O;
	DeshInput->mapKeys['P'] = Key_P; DeshInput->mapKeys['Q'] = Key_Q; DeshInput->mapKeys['R'] = Key_R;
	DeshInput->mapKeys['S'] = Key_S; DeshInput->mapKeys['T'] = Key_T; DeshInput->mapKeys['U'] = Key_U;
	DeshInput->mapKeys['V'] = Key_V; DeshInput->mapKeys['W'] = Key_W; DeshInput->mapKeys['X'] = Key_X;
	DeshInput->mapKeys['Y'] = Key_Y; DeshInput->mapKeys['Z'] = Key_Z;

	DeshInput->mapKeys[GLFW_KEY_0] = Key_K0; DeshInput->mapKeys[GLFW_KEY_1] = Key_K1;
	DeshInput->mapKeys[GLFW_KEY_2] = Key_K2; DeshInput->mapKeys[GLFW_KEY_3] = Key_K3;
	DeshInput->mapKeys[GLFW_KEY_4] = Key_K4; DeshInput->mapKeys[GLFW_KEY_5] = Key_K5;
	DeshInput->mapKeys[GLFW_KEY_6] = Key_K6; DeshInput->mapKeys[GLFW_KEY_7] = Key_K7;
	DeshInput->mapKeys[GLFW_KEY_8] = Key_K8; DeshInput->mapKeys[GLFW_KEY_9] = Key_K9;

	DeshInput->mapKeys[GLFW_KEY_F1]  = Key_F1;  DeshInput->mapKeys[GLFW_KEY_F2]  = Key_F2;
	DeshInput->mapKeys[GLFW_KEY_F3]  = Key_F3;  DeshInput->mapKeys[GLFW_KEY_F4]  = Key_F4;
	DeshInput->mapKeys[GLFW_KEY_F5]  = Key_F5;  DeshInput->mapKeys[GLFW_KEY_F6]  = Key_F6;
	DeshInput->mapKeys[GLFW_KEY_F7]  = Key_F7;  DeshInput->mapKeys[GLFW_KEY_F8]  = Key_F8;
	DeshInput->mapKeys[GLFW_KEY_F9]  = Key_F9;  DeshInput->mapKeys[GLFW_KEY_F10] = Key_F10;
	DeshInput->mapKeys[GLFW_KEY_F11] = Key_F11; DeshInput->mapKeys[GLFW_KEY_F12] = Key_F12;

	DeshInput->mapKeys[GLFW_KEY_DOWN] = Key_DOWN; DeshInput->mapKeys[GLFW_KEY_UP]    = Key_UP;
	DeshInput->mapKeys[GLFW_KEY_LEFT] = Key_LEFT; DeshInput->mapKeys[GLFW_KEY_RIGHT] = Key_RIGHT;

	DeshInput->mapKeys[GLFW_KEY_ESCAPE]        = Key_ESCAPE;
	DeshInput->mapKeys[GLFW_KEY_GRAVE_ACCENT]  = Key_TILDE;
	DeshInput->mapKeys[GLFW_KEY_TAB]           = Key_TAB;
	DeshInput->mapKeys[GLFW_KEY_CAPS_LOCK]     = Key_CAPSLOCK;
	DeshInput->mapKeys[GLFW_KEY_LEFT_SHIFT]    = Key_LSHIFT;
	DeshInput->mapKeys[GLFW_KEY_RIGHT_SHIFT]   = Key_RSHIFT;
	DeshInput->mapKeys[GLFW_KEY_LEFT_CONTROL]  = Key_LCTRL;
	DeshInput->mapKeys[GLFW_KEY_RIGHT_CONTROL] = Key_RCTRL;
	DeshInput->mapKeys[GLFW_KEY_LEFT_ALT]      = Key_LALT;
	DeshInput->mapKeys[GLFW_KEY_RIGHT_ALT]     = Key_RALT;
	DeshInput->mapKeys[GLFW_KEY_BACKSPACE]     = Key_BACKSPACE;
	DeshInput->mapKeys[GLFW_KEY_ENTER]         = Key_ENTER;
	DeshInput->mapKeys[GLFW_KEY_MINUS]         = Key_MINUS;
	DeshInput->mapKeys[GLFW_KEY_EQUAL]         = Key_EQUALS;
	DeshInput->mapKeys[GLFW_KEY_LEFT_BRACKET]  = Key_LBRACKET;
	DeshInput->mapKeys[GLFW_KEY_RIGHT_BRACKET] = Key_RBRACKET;
	DeshInput->mapKeys[GLFW_KEY_SLASH]         = Key_SLASH;
	DeshInput->mapKeys[GLFW_KEY_SEMICOLON]     = Key_SEMICOLON;
	DeshInput->mapKeys[GLFW_KEY_APOSTROPHE]    = Key_APOSTROPHE;
	DeshInput->mapKeys[GLFW_KEY_COMMA]         = Key_COMMA;
	DeshInput->mapKeys[GLFW_KEY_PERIOD]        = Key_PERIOD;
	DeshInput->mapKeys[GLFW_KEY_BACKSLASH]     = Key_BACKSLASH;
	DeshInput->mapKeys[GLFW_KEY_SPACE]         = Key_SPACE;

	DeshInput->mapKeys[GLFW_KEY_INSERT]  = Key_INSERT; DeshInput->mapKeys[GLFW_KEY_DELETE]      = Key_DELETE;
	DeshInput->mapKeys[GLFW_KEY_HOME]    = Key_HOME;   DeshInput->mapKeys[GLFW_KEY_END]         = Key_END;
	DeshInput->mapKeys[GLFW_KEY_PAGE_UP] = Key_PAGEUP; DeshInput->mapKeys[GLFW_KEY_PAGE_DOWN]   = Key_PAGEDOWN;
	DeshInput->mapKeys[GLFW_KEY_PAUSE]   = Key_PAUSE;  DeshInput->mapKeys[GLFW_KEY_SCROLL_LOCK] = Key_SCROLL;

	DeshInput->mapKeys[GLFW_KEY_KP_0] = Key_NUMPAD0; DeshInput->mapKeys[GLFW_KEY_KP_1] = Key_NUMPAD1;
	DeshInput->mapKeys[GLFW_KEY_KP_2] = Key_NUMPAD2; DeshInput->mapKeys[GLFW_KEY_KP_3] = Key_NUMPAD3;
	DeshInput->mapKeys[GLFW_KEY_KP_4] = Key_NUMPAD4; DeshInput->mapKeys[GLFW_KEY_KP_5] = Key_NUMPAD5;
	DeshInput->mapKeys[GLFW_KEY_KP_6] = Key_NUMPAD6; DeshInput->mapKeys[GLFW_KEY_KP_7] = Key_NUMPAD7;
	DeshInput->mapKeys[GLFW_KEY_KP_8] = Key_NUMPAD8; DeshInput->mapKeys[GLFW_KEY_KP_9] = Key_NUMPAD9;

	DeshInput->mapKeys[GLFW_KEY_KP_MULTIPLY] = Key_NUMPADMULTIPLY;
	DeshInput->mapKeys[GLFW_KEY_KP_DIVIDE]   = Key_NUMPADDIVIDE;
	DeshInput->mapKeys[GLFW_KEY_KP_ADD]      = Key_NUMPADPLUS;
	DeshInput->mapKeys[GLFW_KEY_KP_SUBTRACT] = Key_NUMPADMINUS;
	DeshInput->mapKeys[GLFW_KEY_KP_DECIMAL]  = Key_NUMPADPERIOD;
	DeshInput->mapKeys[GLFW_KEY_KP_ENTER]    = Key_NUMPADENTER;
	DeshInput->mapKeys[GLFW_KEY_NUM_LOCK]    = Key_NUMLOCK;

	//mouse mappings
	DeshInput->mapMouse[GLFW_MOUSE_BUTTON_1] = Mouse_LEFT;
	DeshInput->mapMouse[GLFW_MOUSE_BUTTON_2] = Mouse_RIGHT;
	DeshInput->mapMouse[GLFW_MOUSE_BUTTON_3] = Mouse_MIDDLE;
	DeshInput->mapMouse[GLFW_MOUSE_BUTTON_4] = Mouse_FOUR;
	DeshInput->mapMouse[GLFW_MOUSE_BUTTON_5] = Mouse_FIVE;
	DeshInput->mapMouse[GLFW_MOUSE_BUTTON_6] = Mouse_SIX;
	DeshInput->mapMouse[GLFW_MOUSE_BUTTON_7] = Mouse_SEVEN;
	DeshInput->mapMouse[GLFW_MOUSE_BUTTON_8] = Mouse_EIGHT;

	//event callbacks
	//void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
	glfwSetMouseButtonCallback(window,
							   [](GLFWwindow* w, int button, int action, int mods)->void{
								   std::map<size_t, u8>::iterator it = DeshInput->mapMouse.find(button);
								   if(it != DeshInput->mapMouse.end()){
									   if(action == GLFW_PRESS){
										   DeshInput->realKeyState[it->second] = true;
										   DeshInput->checkbinds = true;
#if LOG_INPUTS
										   Log("input","{m", it->second, "|", mods,"}");
										   #endif
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
#if LOG_INPUTS
								   Log("input","{k", it->second, "|", mods,"}");
								   #endif
							   }else if(action == GLFW_RELEASE){
								   DeshInput->realKeyState[it->second] = false;
							   }
						   }
					   });

	//void character_callback(GLFWwindow* window, unsigned int codepoint)
	glfwSetCharCallback(window,
						[](GLFWwindow* window, unsigned int codepoint)->void{
							DeshInput->charIn[DeshInput->realCharCount++] = codepoint;
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

	//void window_focus_callback(GLFWwindow* window, int focused)
	glfwSetWindowFocusCallback(window,
							   [](GLFWwindow* window, int focused) {
								   if (focused) block_mouse_pos_change = false;
								   else block_mouse_pos_change = true;
							   });


	//TODO(sushi) implement this function for use on InputText()
	// //void windo
	//

	LogS("deshi","Finished window initialization in ",peek_stopwatch(t_s),"ms");
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
	this->dimensions = vec2((f32)width, (f32)height);

	if(!block_mouse_pos_change)
		glfwGetCursorPos(window, &DeshInput->mouseX, &DeshInput->mouseY);
	if(cursorMode == CursorMode_FirstPerson){ glfwSetCursorPos(window, width/2, height/2); }
	DeshTime->windowTime = peek_stopwatch(t_d);
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
		case CursorType_Arrow:           glfwSetCursor(window, дефолткурсор);      break;
		case CursorType_HResize:         glfwSetCursor(window, hResizeCursor);     break;
		case CursorType_VResize:         glfwSetCursor(window, vResizeCursor);     break;
		case CursorType_RightDiagResize: glfwSetCursor(window, rDiagResizeCursor); break;
		case CursorType_LeftDiagResize:  glfwSetCursor(window, lDiagResizeCursor); break;
		case CursorType_Hand:            glfwSetCursor(window, handCursor);        break;
		case CursorType_IBeam:           glfwSetCursor(window, textCursor);        break;
	}
}

void Window::UpdateRawInput(b32 rawInput){
	if (glfwRawMouseMotionSupported()){
		this->rawInput = rawInput;
		glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, rawInput);
	}
}

void Window::UpdateResizable(b32 resizable){
	this->resizable = resizable;
	glfwSetWindowAttrib(this->window, GLFW_RESIZABLE, resizable);
}

void Window::Close(){
	closeWindow = true;
}

void Window::UpdateTitle(str8 title){
	glfwSetWindowTitle(this->window, title.str);
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
