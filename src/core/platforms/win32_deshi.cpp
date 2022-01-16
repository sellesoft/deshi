﻿local int _width, _height, _x, _y;
local b32 _resized = false;
local b32 block_mouse_pos_change = false;
local RECT winrect = { 0 };

#undef DELETE
map<s32, Key::Key> vkToKey{
	{ 'A', Key::A},{'B', Key::B},{'C', Key::C},{'D', Key::D},{'E', Key::E},{'F', Key::F},{'G', Key::G},{'H', Key::H},{'I', Key::I},{'J', Key::J},{'K', Key::K},{'L', Key::L},{'M', Key::M},{'N', Key::N},{'O', Key::O},{'P', Key::P},{'Q', Key::Q},{'R', Key::R},{'S', Key::S},{'T', Key::T},{'U', Key::U},{'V', Key::V},{'W', Key::W},{'X', Key::X},{'Y', Key::Y},{'Z', Key::Z},
	{'0', Key::K0},{'1', Key::K1},{'2', Key::K2},{'3', Key::K3},{'4', Key::K4},{'5', Key::K5},{'6', Key::K6},{'7', Key::K7},{'8', Key::K8},{ '9', Key::K9 },
	{ VK_F1, Key::F1 },{VK_F2, Key::F2},{VK_F3, Key::F3},{VK_F4, Key::F4},{VK_F5, Key::F5} ,{VK_F6, Key::F6},{VK_F7, Key::F7},{VK_F8, Key::F8},{VK_F9, Key::F9},{VK_F10, Key::F10},{VK_F11, Key::F11},{VK_F12, Key::F12},
	{VK_UP, Key::UP},{VK_DOWN, Key::DOWN},{VK_LEFT, Key::LEFT},{VK_RIGHT, Key::RIGHT},
	{ VK_ESCAPE, Key::ESCAPE },{ VK_OEM_3, Key::TILDE },{ VK_TAB, Key::TAB },{ VK_CAPITAL, Key::CAPSLOCK },{ VK_LSHIFT, Key::LSHIFT },{ VK_LCONTROL, Key::LCTRL },{ VK_LMENU, Key::LALT },
	{ VK_BACK, Key::BACKSPACE },{ VK_RETURN, Key::ENTER },{ VK_RSHIFT, Key::RSHIFT },{ VK_RCONTROL, Key::RCTRL },{ VK_RMENU, Key::RALT },{ VK_OEM_MINUS, Key::MINUS },{ VK_OEM_PLUS, Key::EQUALS },{ VK_OEM_4, Key::LBRACKET },{ VK_OEM_5, Key::RBRACKET },
	{ VK_OEM_2, Key::SLASH },{ VK_OEM_1, Key::SEMICOLON },{ VK_OEM_7, Key::APOSTROPHE },{ VK_OEM_COMMA, Key::COMMA },{ VK_OEM_PERIOD, Key::PERIOD },{ VK_OEM_5, Key::BACKSLASH },{ VK_SPACE, Key::SPACE },
	{ VK_INSERT, Key::INSERT },{ VK_DELETE, Key::DELETE },{ VK_HOME, Key::HOME },{ VK_END, Key::END },{ VK_PRIOR, Key::PAGEUP },{ VK_NEXT, Key::PAGEDOWN },{ VK_PAUSE, Key::PAUSE },{ VK_SCROLL, Key::SCROLL },
	{VK_NUMPAD0, Key::NUMPAD0},{VK_NUMPAD1, Key::NUMPAD1},{VK_NUMPAD2, Key::NUMPAD2},{VK_NUMPAD3, Key::NUMPAD3},{VK_NUMPAD4, Key::NUMPAD4},{VK_NUMPAD5, Key::NUMPAD5},{VK_NUMPAD6, Key::NUMPAD6},{VK_NUMPAD7, Key::NUMPAD7},{VK_NUMPAD8, Key::NUMPAD8},{VK_NUMPAD9, Key::NUMPAD9},
	{ VK_MULTIPLY, Key::NUMPADMULTIPLY },{ VK_DIVIDE, Key::NUMPADDIVIDE },{ VK_ADD, Key::NUMPADPLUS },{ VK_SUBTRACT, Key::NUMPADMINUS },{ VK_DECIMAL, Key::NUMPADPERIOD },{ VK_RETURN, Key::NUMPADENTER },{ VK_NUMLOCK, Key::NUMLOCK },
	{ VK_LWIN, Key::LMETA },{ VK_RWIN, Key::RMETA },
};

void Win32LogLastError(const char* func_name, b32 crash_on_error = false) {
	LPVOID msg_buffer;
	DWORD error = GetLastError();
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK,
		0, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&msg_buffer, 0, 0);
	LogfE("io-win32", "%s failed with error %d: %s", func_name, (u32)error, (const char*)msg_buffer);
	LocalFree(msg_buffer);
	if (crash_on_error) ExitProcess(error);
}

//@Resize
void WinResized(Window* win, s32 width, s32 height, b32 minimized) {
	win->width = width; win->height = height;
	win->cwidth = width; win->cheight = height - win->titlebarheight;
	win->dimensions = vec2(win->cwidth, win->cheight);
	if (minimized) win->minimized = true;
	else win->minimized = false;
	win->resized = true;
}

enum HitTest_ : s32 {
	HitTest_TBorder,
	HitTest_BBorder,
	HitTest_RBorder,
	HitTest_LBorder,
	HitTest_Titlebar,
};

//win32's callback function 
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	Window* win = (Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	Input* in = DeshInput;
	switch (msg) {
		case WM_CREATE: {		}break;
		case WM_SIZE: { if(win) WinResized(win, LOWORD(lParam), HIWORD(lParam), wParam == SIZE_MINIMIZED); }break;
		case WM_MOVE: { ////////////////////////////////////////////////////////////// Window Moved
			const s32 x = LOWORD(lParam);
			const s32 y = HIWORD(lParam);
			
			if (win) { win->x = x; win->y = y; }
		}break;
		case WM_MOUSEMOVE: { ///////////////////////////////////////////////////////// Mouse Moved
			const s32 xPos = GET_X_LPARAM(lParam);
			const s32 yPos = GET_Y_LPARAM(lParam);
			in->realMouseX = xPos;
			in->realMouseY = yPos - f64(win->titlebarheight);
			POINT p = { xPos, yPos };
			ClientToScreen((HWND)win->handle, &p);
			in->realScreenMouseX = p.x; in->realScreenMouseY = p.y;
		}break;
		case WM_MOUSEHOVER: { //////////////////////////////////////////////////////// Mouse Hovers
			//TODO
		}break;
		case WM_MOUSEWHEEL: { //////////////////////////////////////////////////////// Mouse Scrolled
			const s32 zDelta = GET_WHEEL_DELTA_WPARAM(wParam) / (f64)WHEEL_DELTA;
			in->realScrollY = zDelta;
		}break;
		case WM_MOUSEHWHEEL: { /////////////////////////////////////////////////////// Mouse H Scrolled
			const s32 zDelta = GET_WHEEL_DELTA_WPARAM(wParam) / (f64)WHEEL_DELTA;
			in->realScrollY = zDelta;
		}break;
		                       /////////////////////////////////////////////////////// Mouse Button Down
		case WM_LBUTTONDOWN: { in->realKeyState[Key::MBLEFT]   = true; SetCapture((HWND)win->handle); }break;
		case WM_RBUTTONDOWN: { in->realKeyState[Key::MBRIGHT]  = true; SetCapture((HWND)win->handle); }break;
		case WM_MBUTTONDOWN: { in->realKeyState[Key::MBMIDDLE] = true; SetCapture((HWND)win->handle); }break;
		case WM_XBUTTONDOWN: { in->realKeyState[(GET_XBUTTON_WPARAM(wParam) == XBUTTON1 ? Key::MBFOUR : Key::MBFIVE)] = true; return true; }break;
			                   /////////////////////////////////////////////////////// Mouse Button Up
		case WM_LBUTTONUP:   { in->realKeyState[Key::MBLEFT]   = false; ReleaseCapture(); }break;
		case WM_RBUTTONUP:   { in->realKeyState[Key::MBRIGHT]  = false; ReleaseCapture(); }break;
		case WM_MBUTTONUP:   { in->realKeyState[Key::MBMIDDLE] = false; ReleaseCapture(); }break;
		case WM_XBUTTONUP:   { in->realKeyState[(GET_XBUTTON_WPARAM(wParam) == XBUTTON1 ? Key::MBFOUR : Key::MBFIVE)] = false; return true; }break;
		case WM_KEYUP:        //////////////////////////////////////////////////////// Key Down/Up
		case WM_SYSKEYUP: 
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN: { 
			u16 vcode = LOWORD(wParam);
			u16 scancode = (HIWORD(lParam) & (KF_EXTENDED | 0xff));
			b32 upFlag = (HIWORD(lParam) & KF_UP) == KF_UP;              // transition-state flag, 1 on keyup
			b32 repeatFlag = (HIWORD(lParam) & KF_REPEAT) == KF_REPEAT;  // previous key-state flag, 1 on autorepeat
			u16 repeatCount = LOWORD(lParam);

			//these are probably unused for now
			b32 altDownFlag  = (HIWORD(lParam) & KF_ALTDOWN) == KF_ALTDOWN;    // ALT key was pressed
			b32 dlgModeFlag  = (HIWORD(lParam) & KF_DLGMODE) == KF_DLGMODE;    // dialog box is active
			b32 menuModeFlag = (HIWORD(lParam) & KF_MENUMODE) == KF_MENUMODE;  // menu is active

			//check modifier keys
			if (GetKeyState(VK_LSHIFT) & 0x8000)   in->realKeyState[Key::LSHIFT] = true;
			else                                   in->realKeyState[Key::LSHIFT] = false;
			if (GetKeyState(VK_RSHIFT) & 0x8000)   in->realKeyState[Key::RSHIFT] = true;
			else                                   in->realKeyState[Key::RSHIFT] = false;
			if (GetKeyState(VK_LCONTROL) & 0x8000) in->realKeyState[Key::LCTRL] = true;
			else                                   in->realKeyState[Key::LCTRL] = false;
			if (GetKeyState(VK_RCONTROL) & 0x8000) in->realKeyState[Key::RCTRL] = true;
			else                                   in->realKeyState[Key::RCTRL] = false;
			if (GetKeyState(VK_LMENU) & 0x8000)    in->realKeyState[Key::LALT] = true;
			else                                   in->realKeyState[Key::LALT] = false;
			if (GetKeyState(VK_RMENU) & 0x8000)    in->realKeyState[Key::RALT] = true;
			else                                   in->realKeyState[Key::RALT] = false;
			if(GetKeyState(VK_LWIN) & 0x8000)      in->realKeyState[Key::LMETA] = true;
			else 								   in->realKeyState[Key::LMETA] = false;
			if (GetKeyState(VK_RWIN) & 0x8000)	   in->realKeyState[Key::RMETA] = true;
			else								   in->realKeyState[Key::RMETA] = false;

			//get key from vcode
			Key::Key* key = vkToKey.at(vcode);

			if (key) {
				if (upFlag) in->realKeyState[*key] = false;
				else        in->realKeyState[*key] = true;

				if (in->logInput) { 
					string out = toStr(KeyStrings[*key], (upFlag ? " released" : " pressed"));
					if(in->realKeyState[Key::LSHIFT]) {out += " + LSHIFT";}
					if(in->realKeyState[Key::RSHIFT]) {out += " + RSHIFT";}
					if(in->realKeyState[Key::LCTRL])  {out += " + LCTRL";}
					if(in->realKeyState[Key::RCTRL])  {out += " + RCTRL";}
					if(in->realKeyState[Key::LALT])   {out += " + LALT";}
					if(in->realKeyState[Key::RALT])	  {out += " + RALT";}
					Log("input", out); 
				}
			}
		}break;
		case WM_CHAR: { ////////////////////////////////////////////////////////////// Char From Key 
			DeshInput->charIn[DeshInput->realCharCount++] = LOWORD(wParam);
		}break;
		case WM_INPUT: { ///////////////////////////////////////////////////////////// Raw Input
			UINT size = 0;
			HRAWINPUT ri = (HRAWINPUT)lParam;
			RAWINPUT* data = NULL;
			//get size of rawinput
			GetRawInputData(ri, RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER));
			data = (RAWINPUT*)memtalloc(size);

			if (GetRawInputData(ri, RID_INPUT, data, &size, sizeof(RAWINPUTHEADER)) == (u32)-1) {
				//LogE("WINDOW-WIN32", "failed to retrieve raw input data in WndProc case WM_INPUT");
				break;
			}

			RAWMOUSE mdata = data->data.mouse;
			s32 dx, dy;
			if (HasFlag(mdata.usFlags, MOUSE_MOVE_ABSOLUTE)) {
				dx = mdata.lLastX - in->realMouseX;  dy = mdata.lLastY - in->realMouseY;
			}
			else {
				dx = mdata.lLastX; dy = mdata.lLastY;
			}

			in->realMouseX += dx; in->realMouseY += dy;

			//TODO maybe implement raw key inputs from this
			RAWKEYBOARD kdata = data->data.keyboard;
			RAWHID      hdata = data->data.hid; //TODO human interface device input


		}break;
		case WM_NCHITTEST: {
			//from https://stackoverflow.com/questions/7773771/how-do-i-implement-dragging-a-window-using-its-client-area
			//allows us to drag the window from whereever we want in the client area
			if (win->decorations != Decoration_SystemDecorations) {
				if(Math::PointInRectangle(in->mousePos, vec2(0, -f32(win->titlebarheight)), vec2(win->width, win->titlebarheight))) {
					LRESULT hit = DefWindowProc((HWND)win->handle, msg, wParam, lParam);
					if (hit == HTCLIENT) hit = HTCAPTION;
					return hit;
				}
				else {//else test for resizing
					switch (win->hittest) {
						case HitTestTitle:       return HTCAPTION;
						case HitTestLeft:        return HTLEFT;
						case HitTestRight:       return HTRIGHT;
						case HitTestBottom:      return HTBOTTOM;
						case HitTestTop:         return HTTOP;
						case HitTestTopRight:    return HTTOPRIGHT;
						case HitTestTopLeft:     return HTTOPLEFT;
						case HitTestBottomRight: return HTBOTTOMRIGHT;
						case HitTestBottomLeft:  return HTBOTTOMLEFT;
					}
					
				}
			}
		}
	}
	if (win) {
		if (GetForegroundWindow() == (HWND)win->handle) win->active = true;
		else win->active = false;
	}
	return DefWindowProcA(hwnd, msg, wParam, lParam);
}

void Window::Init(const char* _name, s32 width, s32 height, s32 x, s32 y, DisplayMode displayMode) {
	AssertDS(DS_MEMORY, "Attempt to load Console without loading Memory first");
	deshiStage |= DS_WINDOW;
	TIMER_START(t_s);
	
	//get console's handle
	instance = GetModuleHandle(NULL);
	
	//TODO load and set icon
	//HICON icon = LoadImageA(NULL, "data/textures/deshi_icon.png", IMAGE_ICON, LR_DEFAULTSIZE, LR_DEFAULTSIZE, LR_DEFAULTCOLOR);;

	//make and register window class
	WNDCLASSA wc;
	wc.        style = 0; //https://docs.microsoft.com/en-us/windows/win32/winmsg/window-class-styles
	wc.  lpfnWndProc = WndProc;
	wc.   cbClsExtra = 0; // The number of extra bytes to allocate following the window-class structure. The system initializes the bytes to zero.
	wc.   cbWndExtra = 0; // The number of extra bytes to allocate following the window instance. The system initializes the bytes to zero.
	wc.    hInstance = (HINSTANCE)instance;
	wc.        hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.      hCursor = LoadCursor(NULL, IDC_ARROW); //TODO implement custom cursors
	wc.hbrBackground = NULL;
	wc. lpszMenuName = NULL;
	wc.lpszClassName = _name;

	if (!RegisterClassA(&wc)) Win32LogLastError("RegisterClassA", true); 

	//create window
	//https://docs.microsoft.com/en-us/windows/win32/winmsg/window-styles
	handle = CreateWindowA(_name, _name, 0, 0, 0, 0, 0, NULL, NULL, (HINSTANCE)instance, NULL);
	if (!handle) Win32LogLastError("CreateWindowA", true);
	//set WndProc user data to be a pointer to this window
	SetWindowLongPtr((HWND)handle, GWLP_USERDATA, (LONG_PTR)this);

	POINT mp = { 0 };
	GetCursorPos(&mp);

	DeshInput->realMouseX = mp.x - x; DeshInput->realMouseY = mp.y - y;

	RAWINPUTDEVICE rid;
	rid.usUsagePage = 0x01;
	rid.usUsage = 0x02;
	rid.dwFlags = 0; //set this to RIDEV_INPUTSINK to update mouse pos even when window isnt focused
	rid.hwndTarget = NULL;

	if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) Win32LogLastError("RegisterRawInputDevices");

	UpdateDisplayMode(displayMode);
	UpdateDecorations(Decoration_MinimalTitlebar | Decoration_MouseBorders);
	titlebarheight = 5;
	SetWindowPos((HWND)handle, 0, x, y, width, height, 0);

	name = _name;
	renderer_surface_index = 0; ///main win is always first surface

	LogS("deshi", "Finished window initialization in ", TIMER_END(t_s), "ms");
}

//returns nullptr if the function fails to make the child;
Window* Window::MakeChild(const char* _name, s32 width, s32 height, s32 x, s32 y) {
	AssertDS(DS_WINDOW, "Attempt to make a child window without initializing window first");
	if (child_count == max_child_windows) { LogE("WINDOW-WIN32", "Window failed to make a child window: max child windows reached."); return 0; }
	TIMER_START(t_s);

	//TODO make global window counter

	Window* child = (Window*)memalloc(sizeof(Window));
	//TODO remove all of this code and just call Init on the child when i decide how to handle the main init erroring

	child->instance = GetModuleHandle(NULL);

	//make and register window class
	WNDCLASSA wc;
	wc.        style = 0; //https://docs.microsoft.com/en-us/windows/win32/winmsg/window-class-styles
	wc.  lpfnWndProc = WndProc;
	wc.   cbClsExtra = 0; // The number of extra bytes to allocate following the window-class structure. The system initializes the bytes to zero.
	wc.   cbWndExtra = 0; // The number of extra bytes to allocate following the window instance. The system initializes the bytes to zero.
	wc.    hInstance = (HINSTANCE)child->instance;
	wc.        hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.      hCursor = LoadCursor(NULL, IDC_ARROW); //TODO implement custom cursors
	wc.hbrBackground = NULL;
	wc. lpszMenuName = NULL;
	wc.lpszClassName = _name;

	if (!RegisterClassA(&wc)) { 
		LogE("WINDOW-WIN32", "Window failed to register WNDCLASS for child window");
		Win32LogLastError("RegisterClassA"); 
		memzfree(child);
		return 0;
	}

	//create window
	//https://docs.microsoft.com/en-us/windows/win32/winmsg/window-styles
	child->handle = CreateWindowA(_name, _name,0,0,0,0,0, NULL, NULL, (HINSTANCE)instance, NULL);
	if (!child->handle) {
		LogE("WINDOW-WIN32", "Windows failed to create child window");
		Win32LogLastError("CreateWindowA", true);
		memzfree(child);
		return 0;
	}
	//set WndProc user data to be a pointer to this window
	SetWindowLongPtr((HWND)child->handle, GWLP_USERDATA, (LONG_PTR)child);

	children[child_count++] = child;
	child->name = _name;
	child->UpdateDisplayMode(displayMode);
	child->UpdateDecorations(Decoration_MinimalTitlebar | Decoration_MouseBorders);
	child->titlebarheight = 5;
	SetWindowPos((HWND)child->handle, 0, x, y, width, height, 0);

	LogS("deshi", "Finished child window initialization in ", TIMER_END(t_s), "ms");
	return child;
}

void DrawDecorations(Window* win) {
	using namespace Render;
	s32 x=win->x, y=win->y;
	s32 width = win->width, height = win->height;
	s32 cwidth = win->cwidth, cheight = win->cheight;
	u32 decor = win->decorations;
	b32 hitset = 0;
	SetSurfaceDrawTargetByWindow(win);
	StartNewTwodCmd(GetZZeroLayerIndex(), 0, vec2::ZERO, vec2(width, height));

	if(Math::PointInRectangle(DeshInput->mousePos, vec2::ZERO, vec2(width, height))) win->hittest = HitTestClient;

	//minimal titlebar takes precedence over all other title bar flags
	if (HasFlag(decor, Decoration_MinimalTitlebar)) {
		win->titlebarheight = 5;
		FillRect2D(vec2::ZERO, vec2(width, win->titlebarheight), Color_White);
		if (Math::PointInRectangle(DeshInput->mousePos, vec2::ZERO, vec2(width, win->titlebarheight))) { win->hittest = HitTestTitle; hitset = 1; }
	}
	else {
		if (HasFlag(decor, Decoration_Titlebar)) {
			win->titlebarheight = 20;
			FillRect2D(vec2::ZERO, vec2(width, win->titlebarheight), color(60,60,60));
		}
		if (HasFlag(decor, Decoration_TitlebarTitle)) {

		}
	}

	if (HasFlag(decor, Decoration_MouseBorders)) {
		vec2 mp = DeshInput->mousePos;
		f32 distToAppear = 7;
		f32 borderSize = 1;
		vec2 
		lbpos = vec2::ZERO,
		rbpos = vec2(width - borderSize, 0),
		bbpos = vec2(0, height - borderSize),
		lbsiz = vec2(borderSize, height),
		rbsiz = vec2(borderSize, height),
		bbsiz = vec2(width, borderSize);
		color //TODO make these not show if the mouse goes beyond the window
		bcol = color(255,255,255, Remap(Clamp(mp.y, f32(cheight - distToAppear), f32(cheight - borderSize)), 0.f, 255.f, f32(cheight - distToAppear), f32(cheight - borderSize))),
		rcol = color(255,255,255, Remap(Clamp(mp.x, f32(cwidth - distToAppear), f32(cwidth - borderSize)), 0.f, 255.f, f32(cwidth - distToAppear), f32(cwidth - borderSize))),
		lcol = color(255,255,255, Remap(Clamp(cwidth - mp.x, f32(cwidth - distToAppear), f32(cwidth - borderSize)), 0.f, 255.f, f32(cwidth - distToAppear), f32(cwidth - borderSize)));

		if (!hitset && Math::PointInRectangle(mp, lbpos, lbsiz)) {win->hittest = HitTestLeft;   hitset = 1;}
		else if (Math::PointInRectangle(mp, rbpos, rbsiz))       {win->hittest = HitTestRight;  hitset = 1;}
		else if (Math::PointInRectangle(mp, bbpos, bbsiz))       {win->hittest = HitTestBottom; hitset = 1;}

		FillRect2D(vec2::ZERO, vec2(borderSize, height), lcol);
		FillRect2D(vec2(width - borderSize, 0), vec2(borderSize, height), rcol);
		FillRect2D(vec2(0, height - borderSize), vec2(width, borderSize), bcol);


	}

	Log("", win->hittest);
}

void Window::Update() {
	TIMER_START(t_d);

	resized = false;


	//iterate through all window messages 
	MSG msg;
	while (PeekMessageA(&msg, (HWND)handle, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT) { closeWindow = true; }
		else {
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
		}
	}

	//update children (this should maybe be done by whoever creates it instead of the parent)
	forI(child_count) children[i]->Update();

	if(decorations != Decoration_SystemDecorations)
		DrawDecorations(this);

	hittest = HitTestNone;
	if(cursorMode == CursorMode_FirstPerson) ::SetCursorPos(x + width / 2, y + height / 2);
	DeshTime->windowTime = TIMER_END(t_d);
}

void Window::UpdateDisplayMode(DisplayMode dm) {
	switch (dm) {
		case DisplayMode_Windowed: {
			if (displayMode != DisplayMode_Windowed) {
				HWND hwnd = (HWND)handle;
				u32 style = GetWindowLongA(hwnd, GWL_STYLE);
				SetWindowLongA(hwnd, GWL_STYLE, AddFlag(style, WS_OVERLAPPEDWINDOW));
				SetWindowPos(hwnd, HWND_TOP, restoreX, restoreY, restoreW, restoreH, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
				resized = true;
			}
			displayMode = dm;
		}break;
		case DisplayMode_Fullscreen: {
			if (displayMode != DisplayMode_Fullscreen) {
				restoreX = x;
				restoreY = y;
				restoreW = width;
				restoreH = height;
				HWND hwnd = (HWND)handle;
				u32 style = GetWindowLongA(hwnd, GWL_STYLE);
				MONITORINFO mi = { sizeof(MONITORINFO) };
				WINDOWPLACEMENT wp = { 0 };
				//TODO add a way to choose what monitor the window is fullscreened to
				b32 failed = false;
				if (!GetWindowPlacement(hwnd, &wp)) { Win32LogLastError("GetWindowPlacement"); failed = true; }
				if (!GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &mi)) { Win32LogLastError("GetMonitorInfo"); failed = true; }

				if (!failed) {
					SetWindowLongA(hwnd, GWL_STYLE, RemoveFlag(style, WS_OVERLAPPEDWINDOW));
					RECT mr = mi.rcMonitor;
					SetWindowPos(hwnd, HWND_TOP, mr.left, mr.top, mr.right - mr.left, mr.bottom - mr.top, SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
				}
				resized = true;
			}
			displayMode = dm;
		}break;
		case DisplayMode_Borderless: {
			if (displayMode != DisplayMode_Borderless) {
				HWND hwnd = (HWND)handle;
				u32 style = GetWindowLongA(hwnd, GWL_STYLE);
				SetWindowLongA(hwnd, GWL_STYLE, RemoveFlag(style, WS_OVERLAPPEDWINDOW));
				SetWindowPos(hwnd, HWND_TOP, restoreX, restoreY, restoreW, restoreH, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
				resized = true;
			}
			displayMode = dm;
		}break;
	}
}

void Window::UpdateCursorMode(CursorMode mode) {
	WarnFuncNotImplemented("");
}

void Window::UpdateDecorations(Decoration _decorations) {
	decorations = _decorations;
	HWND hwnd = (HWND)handle;
	u32 style = GetWindowLongA(hwnd, GWL_STYLE);
	if (decorations == Decoration_SystemDecorations) {
		SetWindowLongA(hwnd, GWL_STYLE, AddFlag(style, WS_OVERLAPPEDWINDOW));
		titlebarheight = 0;
	}
	else 
		SetWindowLongA(hwnd, GWL_STYLE, RemoveFlag(style, WS_OVERLAPPEDWINDOW)); 
	SetWindowPos(hwnd, 0, 0, 0, width, height, SWP_NOMOVE | SWP_NOOWNERZORDER);
}

void Window::SetCursorPos(vec2 pos) {
	::SetCursorPos(pos.x, pos.y);
}

void Window::SetCursor(CursorType curtype) {
	//TODO ref glfw's createIcon
	//WarnFuncNotImplemented;
}

void Window::UpdateRawInput(b32 rawInput) {
	WarnFuncNotImplemented("");
}

void Window::UpdateResizable(b32 resizable) {
	WarnFuncNotImplemented("");
}

void Window::GetScreenSize(s32& _width, s32& _height) {
	_width = width; _height = height;	
}

void Window::GetClientSize(s32& _width, s32& _height) {
	_width = cwidth; _height = cheight;
 }


vec2 Window::GetClientAreaPosition(){
	return vec2(0, titlebarheight);
}

vec2 Window::GetClientAreaDimensions(){
	return vec2(width, height - titlebarheight);
}


void Window::UpdateTitle(const char* title) {
	SetWindowTextA((HWND)handle, title);
}

void Window::ShowWindow(u32 child) {
	if (child != npos) { ::ShowWindow((HWND)children[child]->handle, SW_SHOWNORMAL); }
	else ::ShowWindow((HWND)handle, SW_SHOWNORMAL);
}

void Window::HideWindow(u32 child) {
	if (child != npos) {::ShowWindow((HWND)children[child]->handle, SW_HIDE);}
	else ::ShowWindow((HWND)handle, SW_HIDE);
}

void Window::CloseConsole() {
	//TODO funciton for making a new console 
	FreeConsole();
}

b32 Window::ShouldClose() {
	return closeWindow;
}

void Window::Close(){
	closeWindow = true;
}

void Window::Cleanup() {
	DestroyWindow((HWND)handle);
}






//////////////////
//// File API ////
//////////////////

//TODO binary file loading when needed
//initializes a FileReader for a given File
FileReader init_reader(const File & file) {
	FileReader fr;
	if (!file.handle) { LogE("IO", "attempted to initalize a FileReader on a file that's not initialized"); fr.failed = true; return fr; }
	if (!HasFlag(file.flags, FileAccess_Read)) { LogE("IO", "attempted to initialize a FileReader on a file that doesn't have read access"); fr.failed = true; return fr; }
	fr.raw.str = (char*)memalloc(file.bytes_size); //TODO eventually arena file data allocations
	fr.raw.count = file.bytes_size;

	u32 bytes_read = 0;
	if (!ReadFile(file.handle, fr.raw.str, file.bytes_size, (LPDWORD)&bytes_read, 0)) {
		Win32LogLastError("ReadFile");
		fr.failed = true; return fr;
	}
	if (bytes_read != file.bytes_size) LogW("IO-WIN32", "ReadFile failed to read the entire file '", get_file_name((File)file), "' \nfile's size is ", file.bytes_size, " but ", bytes_read, " were read");

	//gather lines
	//TODO maybe make a way to disable this
	//maybe make a function gather_lines or something and only cache lines when thats called
	char* start = fr.raw.str, end = 0;
	cstring raw = fr.raw;
	forI(bytes_read + 1) {
		if (raw[0] == '\n' || raw[0] == '\0') {
			fr.lines.add({ start, u32(raw.str - start) });
			start = raw.str + 1;
		}
		advance(&raw);
	}
	if (!fr.lines.count) fr.lines.add(fr.raw);

	fr.file = &file;
	fr.read = { fr.raw.str, 0 };
	return fr;
}


//opens a file if it already exists or creates a new one if it doesnt
//this does not load any data, you must use FileReader to do that!
File open_file(const char* path, FileAccessFlags flags) {
	Assert(flags, "attempt to open_file without specifing access flags");
	File file;
	file.flags = flags;

	DWORD access = (HasFlag(flags, FileAccess_Write) ? GENERIC_WRITE : 0) | (HasFlag(flags, FileAccess_Read) ? GENERIC_READ : 0);
	file.handle = CreateFileA(path, access, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (GetLastError() != ERROR_FILE_EXISTS && GetLastError()) Win32LogLastError("CreateFileA");

	//read data of opened file
	BY_HANDLE_FILE_INFORMATION data;
	ULARGE_INTEGER time;
	ULARGE_INTEGER size;

	if (!GetFileInformationByHandle(file.handle, &data)) Win32LogLastError("GetFileInformationByHandle");

	time.LowPart = data.ftCreationTime.dwLowDateTime; time.HighPart = data.ftCreationTime.dwHighDateTime;
	file.time_creation = WindowsTimeToUnixTime(time.QuadPart);
	time.LowPart = data.ftLastAccessTime.dwLowDateTime; time.HighPart = data.ftLastAccessTime.dwHighDateTime;
	file.time_last_access = WindowsTimeToUnixTime(time.QuadPart);
	time.LowPart = data.ftLastWriteTime.dwLowDateTime; time.HighPart = data.ftLastWriteTime.dwHighDateTime;
	file.time_last_write = WindowsTimeToUnixTime(time.QuadPart);
	size.LowPart = data.nFileSizeLow; size.HighPart = data.nFileSizeHigh;
	file.bytes_size = size.QuadPart;
	file.is_directory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);

	u32 pathlength = GetFinalPathNameByHandleA(file.handle, file.path, MAX_FILEPATH_SIZE, FILE_NAME_NORMALIZED);
	if (pathlength > MAX_FILEPATH_SIZE) LogW("IO", "file path for '", path, "' has a length greater than MAX_FILEPATH_SIZE\npath length was ", pathlength);

	//file.path_length = Min(pathlength, u32(MAX_FILEPATH_SIZE));

	string pathstr(file.path);
	//NOTE when we start using unicode stuff for windows path, ideally store this separate from the path name, or just set up the get name funcs to remove the prefix on call
	//remove \\?\ prefix, however this may cause issues in the future with network paths, so TODO add checking for that
	//see https://stackoverflow.com/questions/31439011/getfinalpathnamebyhandle-result-without-prepended
	pathstr = pathstr.substr(4);
	pathstr.replace('\\', "/");
	strcpy(file.path, pathstr.str);
	file.path_length = pathstr.count;
	pathstr = pathstr.substr(pathstr.findLastChar('/') + 1);
	strcpy(file.name, pathstr.str);
	file.name_length = pathstr.count;
	file.short_length = pathstr.findFirstChar('.');
	file.ext_length = pathstr.count - file.short_length - 1;
	return file;
}

//TODO(delle) search filters
array<File>
get_directory_files(const char* directory) {
	array<File> result(deshi_temp_allocator);
	if (directory == 0) return result;

	string pattern(directory); //TODO add allocator to string
	pattern += (pattern[pattern.count - 1] != '/') ? "/*" : "*";
	WIN32_FIND_DATAA data; HANDLE next;
	ULARGE_INTEGER size;   ULARGE_INTEGER time;

	next = FindFirstFileA(pattern.str, &data);
	if (next == INVALID_HANDLE_VALUE || !(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
		Win32LogLastError("FindFirstFileA");
		LogaE("io-win32", "'$' is not a valid directory.", directory);
		FindClose(next);
		return result;
	}
	while (next != INVALID_HANDLE_VALUE) {
		if ((strcmp(data.cFileName, ".") == 0) || (strcmp(data.cFileName, "..") == 0)) {
			if (FindNextFileA(next, &data) == 0) break;
			continue;
		}

		File file;
		file.handle = next;
		time.LowPart = data.ftCreationTime.dwLowDateTime; time.HighPart = data.ftCreationTime.dwHighDateTime;
		file.time_creation = WindowsTimeToUnixTime(time.QuadPart);
		time.LowPart = data.ftLastAccessTime.dwLowDateTime; time.HighPart = data.ftLastAccessTime.dwHighDateTime;
		file.time_last_access = WindowsTimeToUnixTime(time.QuadPart);
		time.LowPart = data.ftLastWriteTime.dwLowDateTime; time.HighPart = data.ftLastWriteTime.dwHighDateTime;
		file.time_last_write = WindowsTimeToUnixTime(time.QuadPart);
		size.LowPart = data.nFileSizeLow; size.HighPart = data.nFileSizeHigh;
		file.bytes_size = size.QuadPart;
		file.is_directory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);

		u32 path_len = pattern.count - 1;
		u32 name_len = strlen(data.cFileName);
		u32 short_len = name_len;
		while (short_len && data.cFileName[short_len--] != '.');
		file.path_length = path_len + name_len;
		file.name_length = name_len;
		file.short_length = short_len + 1;
		file.ext_length = name_len - short_len - 1;
		Assert(file.path_length < MAX_FILEPATH_SIZE);
		memcpy(file.path, pattern.str, pattern.count - 1);
		memcpy(file.path + path_len, data.cFileName, name_len);
		memcpy(file.name, data.cFileName, name_len);

		result.add(file);
		if (FindNextFileA(next, &data) == 0) break;
	}
	DWORD error = GetLastError();
	if (error != ERROR_NO_MORE_FILES) {
		Win32LogLastError("FindNextFileA");
	}
	FindClose(next);

	return result;
}


//TODO(delle) add safety checks so deletion only happens within the data folder
void
delete_file(const char* filepath) {
	if (filepath == 0) return;

	WIN32_FIND_DATAA data;
	HANDLE next = FindFirstFileA(filepath, &data);
	if (next == INVALID_HANDLE_VALUE) {
		Win32LogLastError("FindFirstFileA");
		return;
	}

	//if directory, recursively delete all files and directories
	if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		array<File> dir_files = get_directory_files(filepath);
		forE(dir_files) delete_file(it->path);
		BOOL success = RemoveDirectoryA(filepath);
		if (!success) Win32LogLastError("RemoveDirectoryA");
	}
	else {
		BOOL success = DeleteFileA(filepath);
		if (!success) Win32LogLastError("DeleteFile");
	}
	FindClose(next);
}

b32
file_exists(const char* filepath) {
	if (filepath == 0) return false;
	WIN32_FIND_DATAA data;
	HANDLE handle = FindFirstFileA(filepath, &data);
	if (handle != INVALID_HANDLE_VALUE) {
		FindClose(handle);
		return true;
	}
	return false;
}

void
rename_file(const char* old_filepath, const char* new_filepath) {
	BOOL success = MoveFileA(old_filepath, new_filepath);
	if (!success) Win32LogLastError("MoveFileA");
}