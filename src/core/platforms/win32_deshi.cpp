/*Index:
@win32_vars
@win32_helpers
@win32_init_time
@win32_init_window
@win32_update_window
@win32_update_time
@win32_update_input
@win32_stopwatch
@win32_io
@win32_module
@win32_clipboard
@win32_threading
*/


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @win32_vars
local s64 deshi__win32_perf_count_frequency;


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @win32_helpers
//NOTE(delle) ideally we'd do this with a static C array, but C++ doesn't support out of order array init
FORCE_INLINE KeyCode win32_vkcode_to_key(s32 vk){
	switch(vk){
		case 'A': return Key_A; case 'B': return Key_B; case 'C': return Key_C; case 'D': return Key_D; case 'E': return Key_E;
		case 'F': return Key_F; case 'G': return Key_G; case 'H': return Key_H; case 'I': return Key_I; case 'J': return Key_J;
		case 'K': return Key_K; case 'L': return Key_L; case 'M': return Key_M; case 'N': return Key_N; case 'O': return Key_O;
		case 'P': return Key_P; case 'Q': return Key_Q; case 'R': return Key_R; case 'S': return Key_S; case 'T': return Key_T;
		case 'U': return Key_U; case 'V': return Key_V; case 'W': return Key_W; case 'X': return Key_X; case 'Y': return Key_Y;
		case 'Z': return Key_Z;
		case '0': return Key_0; case '1': return Key_1; case '2': return Key_2; case '3': return Key_3; case '4': return Key_4;
		case '5': return Key_5; case '6': return Key_6; case '7': return Key_7; case '8': return Key_8; case '9': return Key_9;
		case VK_F1: return Key_F1; case VK_F2:  return Key_F2;  case VK_F3:  return Key_F3;  case VK_F4:  return Key_F4;
		case VK_F5: return Key_F5; case VK_F6:  return Key_F6;  case VK_F7:  return Key_F7;  case VK_F8:  return Key_F8;
		case VK_F9: return Key_F9; case VK_F10: return Key_F10; case VK_F11: return Key_F11; case VK_F12: return Key_F12;
		case VK_UP: return Key_UP; case VK_DOWN: return Key_DOWN; case VK_LEFT: return Key_LEFT; case VK_RIGHT: return Key_RIGHT;
		case VK_ESCAPE:    return Key_ESCAPE;     case VK_OEM_3:     return Key_TILDE;        case VK_TAB:        return Key_TAB;
		case VK_CAPITAL:   return Key_CAPSLOCK;   case VK_LSHIFT:    return Key_LSHIFT;       case VK_LCONTROL:   return Key_LCTRL;
		case VK_LMENU:     return Key_LALT;       case VK_BACK:      return Key_BACKSPACE;    case VK_RETURN:     return Key_ENTER;
		case VK_RSHIFT:    return Key_RSHIFT;     case VK_RCONTROL:  return Key_RCTRL;        case VK_RMENU:      return Key_RALT;
		case VK_OEM_MINUS: return Key_MINUS;      case VK_OEM_PLUS:  return Key_EQUALS;       case VK_OEM_4:      return Key_LBRACKET;
		case VK_OEM_6:     return Key_RBRACKET;   case VK_OEM_2:     return Key_FORWARDSLASH; case VK_OEM_1:      return Key_SEMICOLON;
		case VK_OEM_7:     return Key_APOSTROPHE; case VK_OEM_COMMA: return Key_COMMA;        case VK_OEM_PERIOD: return Key_PERIOD;
		case VK_OEM_5:     return Key_BACKSLASH;  case VK_SPACE:     return Key_SPACE;        case VK_INSERT:     return Key_INSERT;
		case VK_DELETE:    return Key_DELETE;     case VK_HOME:      return Key_HOME;         case VK_END:        return Key_END;
		case VK_PRIOR:     return Key_PAGEUP;     case VK_NEXT:      return Key_PAGEDOWN;     case VK_PAUSE:      return Key_PAUSEBREAK;
		case VK_SCROLL:    return Key_SCROLLLOCK; case VK_LWIN:      return Key_LMETA;        case VK_RWIN:       return Key_RMETA;
		case VK_SNAPSHOT:  return Key_PRINTSCREEN;
		case VK_NUMPAD0: return Key_NP0; case VK_NUMPAD1: return Key_NP1; case VK_NUMPAD2: return Key_NP2;
		case VK_NUMPAD3: return Key_NP3; case VK_NUMPAD4: return Key_NP4; case VK_NUMPAD5: return Key_NP5;
		case VK_NUMPAD6: return Key_NP6; case VK_NUMPAD7: return Key_NP7; case VK_NUMPAD8: return Key_NP8;
		case VK_NUMPAD9: return Key_NP9;
		case VK_MULTIPLY: return Key_NPMULTIPLY; case VK_DIVIDE:  return Key_NPDIVIDE; case VK_ADD:     return Key_NPPLUS;
		case VK_SUBTRACT: return Key_NPMINUS;    case VK_DECIMAL: return Key_NPPERIOD; case VK_NUMLOCK: return Key_NUMLOCK;
		default: return Key_NONE;
	}
}

void Win32LogLastError(const char* func_name, b32 crash_on_error = false, const char* custom = ""){DPZoneScoped;
	LPVOID msg_buffer;
	DWORD error = ::GetLastError();
	::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK,
				  0, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&msg_buffer, 0, 0);
	LogfE("win32", "%s failed with error %d: %s. %s", func_name, (u32)error, (const char*)msg_buffer, custom);
	::LocalFree(msg_buffer);
	if(crash_on_error){
		Assert(!"assert before exit so we can stack trace in debug mode");
		::ExitProcess(error);
	}
}

//@Resize
void WinResized(Window* win, s32 width, s32 height, b32 minimized){DPZoneScoped;
	win->width = width;
	win->height = height;
	win->cwidth = width;
	win->cheight = height - win->titlebarheight;
	win->centerX = win->width/2;
	win->centerY = win->height/2;
	win->dimensions = vec2(win->cwidth, win->cheight);
	win->cx = win->borderthickness;
	win->cy = win->borderthickness + win->titlebarheight;
	win->minimized = minimized;
	win->resized = true;
}

void Win32GetMonitorInfo(HWND handle, int* screen_w = 0, int* screen_h = 0, int* working_x = 0, int* working_y = 0, int* working_w = 0, int* working_h = 0){
	HMONITOR monitor = ::MonitorFromWindow(handle, MONITOR_DEFAULTTONEAREST);
	if(monitor){
		MONITORINFO monitor_info = {sizeof(MONITORINFO)};
		if(::GetMonitorInfo(monitor, &monitor_info)){
			if(screen_w) *screen_w = monitor_info.rcMonitor.right  - monitor_info.rcMonitor.left;
			if(screen_h) *screen_h = monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top;
			if(working_x) *working_x = monitor_info.rcWork.left;
			if(working_y) *working_y = monitor_info.rcWork.top;
			if(working_w) *working_w = monitor_info.rcWork.right  - monitor_info.rcWork.left;
			if(working_h) *working_h = monitor_info.rcWork.bottom - monitor_info.rcWork.top;
		}else{
			Win32LogLastError("GetMonitorInfo");
		}
	}else{
		Win32LogLastError("MonitorFromWindow");
	}
}

enum HitTest_ : s32 {
	HitTest_TBorder,
	HitTest_BBorder,
	HitTest_RBorder,
	HitTest_LBorder,
	HitTest_Titlebar,
};

//win32's callback function 
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){DPZoneScoped;
	Window* win = (Window*)::GetWindowLongPtr(hwnd, GWLP_USERDATA);
	switch (msg){
		case WM_CREATE:{ }break;
		case WM_SIZE:{ if(win) WinResized(win, LOWORD(lParam), HIWORD(lParam), wParam == SIZE_MINIMIZED); }break;
		case WM_CLOSE:
		case WM_DESTROY:
		case WM_QUIT:{
			if(win){
				win->closeWindow = true;
				return 0;
			}
		}break;
		case WM_MOVE:{ ////////////////////////////////////////////////////////////// Window Moved
			const s32 x = LOWORD(lParam);
			const s32 y = HIWORD(lParam);
			if(win){ win->x = x; win->y = y; }
		}break;
		case WM_MOUSEMOVE:{ ///////////////////////////////////////////////////////// Mouse Moved
			const s32 xPos = GET_X_LPARAM(lParam);
			const s32 yPos = GET_Y_LPARAM(lParam);
			DeshInput->realMouseX = xPos - f64(win->borderthickness);
			DeshInput->realMouseY = yPos - f64(win->titlebarheight + win->titlebarheight);
			POINT p = { xPos, yPos };
			::ClientToScreen((HWND)win->handle, &p);
			DeshInput->realScreenMouseX = p.x;
			DeshInput->realScreenMouseY = p.y;
		}break;
		case WM_MOUSEHOVER:{ //////////////////////////////////////////////////////// Mouse Hovers
			//TODO
		}break;
		case WM_MOUSEWHEEL:{ //////////////////////////////////////////////////////// Mouse Scrolled
			const s32 zDelta = GET_WHEEL_DELTA_WPARAM(wParam) / (f64)WHEEL_DELTA;
			DeshInput->realScrollY = zDelta;
		}break;
		case WM_MOUSEHWHEEL:{ /////////////////////////////////////////////////////// Mouse H Scrolled
			const s32 zDelta = GET_WHEEL_DELTA_WPARAM(wParam) / (f64)WHEEL_DELTA;
			DeshInput->realScrollY = zDelta;
		}break;
		/////////////////////////////////////////////////////// Mouse Button Down
		case WM_LBUTTONDOWN:{ DeshInput->realKeyState[Mouse_LEFT]   = true; ::SetCapture((HWND)win->handle); }break;
		case WM_RBUTTONDOWN:{ DeshInput->realKeyState[Mouse_RIGHT]  = true; ::SetCapture((HWND)win->handle); }break;
		case WM_MBUTTONDOWN:{ DeshInput->realKeyState[Mouse_MIDDLE] = true; ::SetCapture((HWND)win->handle); }break;
		case WM_XBUTTONDOWN:{ DeshInput->realKeyState[(GET_XBUTTON_WPARAM(wParam) == XBUTTON1 ? Mouse_4 : Mouse_5)] = true; return true; }break;
		/////////////////////////////////////////////////////// Mouse Button Up
		case WM_LBUTTONUP:  { DeshInput->realKeyState[Mouse_LEFT]   = false; ::ReleaseCapture(); }break;
		case WM_RBUTTONUP:  { DeshInput->realKeyState[Mouse_RIGHT]  = false; ::ReleaseCapture(); }break;
		case WM_MBUTTONUP:  { DeshInput->realKeyState[Mouse_MIDDLE] = false; ::ReleaseCapture(); }break;
		case WM_XBUTTONUP:  { DeshInput->realKeyState[(GET_XBUTTON_WPARAM(wParam) == XBUTTON1 ? Mouse_4 : Mouse_5)] = false; return true; }break;
		case WM_KEYUP:        //////////////////////////////////////////////////////// Key Down/Up
		case WM_SYSKEYUP: 
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:{ 
			u16 vcode = LOWORD(wParam);
			u16 scancode = (HIWORD(lParam) & (KF_EXTENDED | 0xff));
			b32 upFlag = (HIWORD(lParam) & KF_UP) == KF_UP;              // transition-state flag, 1 on keyup
			b32 repeatFlag = (HIWORD(lParam) & KF_REPEAT) == KF_REPEAT;  // previous key-state flag, 1 on autorepeat
			u16 repeatCount = LOWORD(lParam);
			
			//these are probably unused for now
			b32 altDownFlag  = (HIWORD(lParam) & KF_ALTDOWN)  == KF_ALTDOWN;   // ALT key was pressed
			b32 dlgModeFlag  = (HIWORD(lParam) & KF_DLGMODE)  == KF_DLGMODE;   // dialog box is active
			b32 menuModeFlag = (HIWORD(lParam) & KF_MENUMODE) == KF_MENUMODE;  // menu is active
			
			//check modifier keys //TODO(delle) check if this is needed considering win32_vkcode_to_key
			DeshInput->realKeyState[Key_LSHIFT] = ::GetKeyState(VK_LSHIFT) & 0x8000;
			DeshInput->realKeyState[Key_RSHIFT] = ::GetKeyState(VK_RSHIFT) & 0x8000;
			DeshInput->realKeyState[Key_LCTRL]  = ::GetKeyState(VK_LCONTROL) & 0x8000;
			DeshInput->realKeyState[Key_RCTRL]  = ::GetKeyState(VK_RCONTROL) & 0x8000;
			DeshInput->realKeyState[Key_LALT]   = ::GetKeyState(VK_LMENU) & 0x8000;
			DeshInput->realKeyState[Key_RALT]   = ::GetKeyState(VK_RMENU) & 0x8000;
			DeshInput->realKeyState[Key_LMETA]  = ::GetKeyState(VK_LWIN) & 0x8000;
			DeshInput->realKeyState[Key_RMETA]  = ::GetKeyState(VK_RWIN) & 0x8000;
			DeshInput->capsLock   = (::GetKeyState(VK_CAPITAL) & 0x8000);
			DeshInput->numLock    = (::GetKeyState(VK_NUMLOCK) & 0x8000);
			DeshInput->scrollLock = (::GetKeyState(VK_SCROLL ) & 0x8000);
			
			//get key from vcode
			KeyCode key = win32_vkcode_to_key(vcode);
			if(key != Key_NONE){
				DeshInput->realKeyState[key] = upFlag;
				
#if LOG_INPUTS
					string out = toStr(KeyCodeStrings[key], (upFlag ? " released" : " pressed"));
					if(DeshInput->realKeyState[Key_LSHIFT]){out += " + LSHIFT";}
					if(DeshInput->realKeyState[Key_RSHIFT]){out += " + RSHIFT";}
					if(DeshInput->realKeyState[Key_LCTRL]) {out += " + LCTRL";}
					if(DeshInput->realKeyState[Key_RCTRL]) {out += " + RCTRL";}
					if(DeshInput->realKeyState[Key_LALT])  {out += " + LALT";}
					if(DeshInput->realKeyState[Key_RALT])  {out += " + RALT";}
					Log("input", out); 
#endif
			}
		}break;
		case WM_CHAR:{ ////////////////////////////////////////////////////////////// Char From Key 
			if(!iscntrl(LOWORD(wParam))){ //NOTE skip control characters
				DeshInput->charIn[DeshInput->realCharCount++] = LOWORD(wParam);
			}
		}break;
		case WM_INPUT:{ ///////////////////////////////////////////////////////////// Raw Input
			UINT size = 0;
			HRAWINPUT ri = (HRAWINPUT)lParam;
			RAWINPUT* data = NULL;
			//get size of rawinput
			::GetRawInputData(ri, RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER));
			data = (RAWINPUT*)memory_talloc(size);
			
			if(::GetRawInputData(ri, RID_INPUT, data, &size, sizeof(RAWINPUTHEADER)) == (u32)-1){
				//LogE("WINDOW-WIN32", "failed to retrieve raw input data in WndProc case WM_INPUT");
				break;
			}
			
			RAWMOUSE mdata = data->data.mouse;
			s32 dx, dy;
			if(HasFlag(mdata.usFlags, MOUSE_MOVE_ABSOLUTE)){
				dx = mdata.lLastX - DeshInput->realMouseX;
				dy = mdata.lLastY - DeshInput->realMouseY;
			}else{
				dx = mdata.lLastX;
				dy = mdata.lLastY;
			}
			
			DeshInput->realMouseX += dx; DeshInput->realMouseY += dy;
			
			//TODO maybe implement raw key inputs from this
			RAWKEYBOARD kdata = data->data.keyboard;
			RAWHID      hdata = data->data.hid; //TODO human interface device input
		}break;
		case WM_NCHITTEST:{
			s32  xPos = GET_X_LPARAM(lParam);
			s32  yPos = GET_Y_LPARAM(lParam);
			s32  x = win->x, y = win->y;
			s32  width = win->width, height = win->height;
			s32  cx = win->cx, cy = win->cy;
			s32  cwidth = win->cwidth, cheight = win->cheight;
			s32  tbh = win->titlebarheight;
			s32  bt = win->borderthickness;
			vec2 mp = input_mouse_position() + vec2(bt, tbh+bt*2);
			u32  decor = win->decorations;
			b32  hitset = 0;
			if(Math::PointInRectangle(mp, vec2(cx, cy),                  vec2(cwidth, cheight)))                   return HTCLIENT;  
			if(Math::PointInRectangle(mp, vec2(bt, bt),                  vec2(width - 2*bt, win->titlebarheight))) return HTCAPTION;  
			if(Math::PointInRectangle(mp, vec2::ZERO,                    vec2(bt, bt)))                            return HTTOPLEFT;	  
			if(Math::PointInRectangle(mp, vec2(0, height - bt),          vec2(bt, bt)))                            return HTBOTTOMLEFT;  
			if(Math::PointInRectangle(mp, vec2(width - bt, 0),           vec2(bt, bt)))                            return HTTOPRIGHT;	  
			if(Math::PointInRectangle(mp, vec2(width - bt, height - bt), vec2(bt, bt)))                            return HTBOTTOMRIGHT; 
			if(Math::PointInRectangle(mp, vec2::ZERO,                    vec2(width, bt)))                         return HTTOP;		  
			if(Math::PointInRectangle(mp, vec2(0, height - bt),          vec2(width, bt)))                         return HTBOTTOM;       
			if(Math::PointInRectangle(mp, vec2::ZERO,                    vec2(bt, height)))                        return HTLEFT;		  
			if(Math::PointInRectangle(mp, vec2(width - bt, 0),           vec2(bt, height)))                        return HTRIGHT;		  
		}
	}
	if(win){
		win->active = (::GetForegroundWindow() == (HWND)win->handle);
	}
	return ::DefWindowProcA(hwnd, msg, wParam, lParam);
}



//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @Window API
#define DESHI_WND_CLASSNAME_A "_DESHI_"

void Window::Init(const char* _name, s32 width, s32 height, s32 x, s32 y, DisplayMode displayMode){DPZoneScoped;
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// @win32_init_time
	DeshiStageInitStart(DS_TIME, DS_MEMORY, "Attempted to initialize Time module before initializing Memory module");
	
	//get the perf counter frequency for timers
	LARGE_INTEGER perf_count_frequency_result;
	::QueryPerformanceFrequency(&perf_count_frequency_result);
	deshi__win32_perf_count_frequency = perf_count_frequency_result.QuadPart;
	
	DeshiStageInitEnd(DS_TIME);
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// @win32_init_window
	DeshiStageInitStart(DS_WINDOW, DS_MEMORY, "Attempted to initialize Window module before initializing Memory module");
	
	//get console's handle
	instance = ::GetModuleHandle(NULL);
	
	//TODO load and set icon
	//HICON icon = LoadImageA(NULL, "data/textures/deshi_icon.png", IMAGE_ICON, LR_DEFAULTSIZE, LR_DEFAULTSIZE, LR_DEFAULTCOLOR);;
	
	//// register window class ////
	WNDCLASSA wc;
	wc.        style = CS_OWNDC; //https://docs.microsoft.com/en-us/windows/win32/winmsg/window-class-styles
	wc.  lpfnWndProc = WndProc;
	wc.   cbClsExtra = 0; // The number of extra bytes to allocate following the window-class structure. The system initializes the bytes to zero.
	wc.   cbWndExtra = 0; // The number of extra bytes to allocate following the window instance. The system initializes the bytes to zero.
	wc.    hInstance = (HINSTANCE)instance;
	wc.        hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.      hCursor = LoadCursor(NULL, IDC_ARROW); //TODO implement custom cursors
	wc.hbrBackground = NULL;
	wc. lpszMenuName = NULL;
	wc.lpszClassName = DESHI_WND_CLASSNAME_A;
	if(::RegisterClassA(&wc) == NULL) Win32LogLastError("RegisterClassA", true); 
	
	//// create window ////
	//https://docs.microsoft.com/en-us/windows/win32/winmsg/window-styles
#if DESHI_OPENGL
	handle = ::CreateWindowA(DESHI_WND_CLASSNAME_A, _name, WS_CLIPCHILDREN|WS_CLIPSIBLINGS, 0, 0, 0, 0, NULL, NULL, (HINSTANCE)instance, NULL);
#else
	handle = ::CreateWindowA(DESHI_WND_CLASSNAME_A, _name, 0, 0, 0, 0, 0, NULL, NULL, (HINSTANCE)instance, NULL);
#endif
	if(handle == NULL) Win32LogLastError("CreateWindowA", true);
	//set WndProc user data to be a pointer to this window
	::SetWindowLongPtr((HWND)handle, GWLP_USERDATA, (LONG_PTR)this);
	dc = ::GetDC((HWND)handle);
	
	//// get initial input values ////
	POINT mp = { 0 };
	GetCursorPos(&mp);
	DeshInput->realMouseX = mp.x - x;
	DeshInput->realMouseY = mp.y - y;
	DeshInput->capsLock   = (GetKeyState(VK_CAPITAL) & 0x8000);
	DeshInput->numLock    = (GetKeyState(VK_NUMLOCK) & 0x8000);
	DeshInput->scrollLock = (GetKeyState(VK_SCROLL ) & 0x8000);
	
	//// setup raw input ////
	RAWINPUTDEVICE rid;
	rid.usUsagePage = 0x01;
	rid.    usUsage = 0x02;
	rid.    dwFlags = 0; //set this to RIDEV_INPUTSINK to update mouse pos even when window isnt focused
	rid. hwndTarget = NULL;
	
	if(::RegisterRawInputDevices(&rid, 1, sizeof(rid)) == NULL) Win32LogLastError("RegisterRawInputDevices");
	
	//// get monitor info ////
	int work_x = 0, work_y = 0, work_w = 0, work_h = 0;
	Win32GetMonitorInfo((HWND)handle, &screenWidth, &screenHeight, &work_x, &work_y, &work_w, &work_h);
	
	//// setup default window pos/size ////
	if(x == 0xFFFFFFFF || y == 0xFFFFFFFF){
		x = work_x + ((work_w -  width) / 2);
		y = work_y + ((work_h - height) / 2);
	}
	
	//// update window to requested properties ////
	UpdateDisplayMode(displayMode);
	//UpdateDecorations(Decoration_TitlebarFull | Decoration_Borders);
	//titlebarheight = 5;
	UpdateDecorations(Decoration_SystemDecorations);
	titlebarheight = 0;
	::SetWindowPos((HWND)handle, 0, x, y, width, height, 0);
	name = _name;
	renderer_surface_index = 0; ///main win is always first surface
	
	DeshiStageInitEnd(DS_WINDOW);
}

//returns nullptr if the function fails to make the child;
Window* Window::MakeChild(const char* _name, s32 width, s32 height, s32 x, s32 y){DPZoneScoped;
	AssertDS(DS_WINDOW, "Attempt to make a child window without initializing window first");
	if(child_count == max_child_windows){ LogE("WINDOW-WIN32", "Window failed to make a child window: max child windows reached."); return 0; }
	Stopwatch t_s = start_stopwatch();
	
	//TODO make global window counter
	
	Window* child = (Window*)memalloc(sizeof(Window));
	//TODO remove all of this code and just call Init on the child when i decide how to handle the main init erroring
	
	child->instance = ::GetModuleHandle(NULL);
	
	//make and register window class //TODO reuse above window class to not pollute atom table
	WNDCLASSA wc;
	wc.        style = 0; //https://docs.microsoft.com/en-us/windows/win32/winmsg/window-class-styles
	wc.  lpfnWndProc = WndProc;
	wc.   cbClsExtra = 0; // The number of extra bytes to allocate following the window-class structure. The system initializes the bytes to zero.
	wc.   cbWndExtra = 0; // The number of extra bytes to allocate following the window instance. The system initializes the bytes to zero.
	wc.    hInstance = (HINSTANCE)child->instance;
	wc.        hIcon = ::LoadIcon(NULL, IDI_APPLICATION);
	wc.      hCursor = ::LoadCursor(NULL, IDC_ARROW); //TODO implement custom cursors
	wc.hbrBackground = NULL;
	wc. lpszMenuName = NULL;
	wc.lpszClassName = _name;
	
	if(::RegisterClassA(&wc) == NULL){ 
		LogE("WINDOW-WIN32", "Window failed to register WNDCLASS for child window");
		Win32LogLastError("RegisterClassA"); 
		memzfree(child);
		return 0;
	}
	
	//create window
	//https://docs.microsoft.com/en-us/windows/win32/winmsg/window-styles
	child->handle = ::CreateWindowA(_name, _name,0,0,0,0,0, NULL, NULL, (HINSTANCE)instance, NULL);
	if(child->handle == NULL){
		LogE("WINDOW-WIN32", "Windows failed to create child window");
		Win32LogLastError("CreateWindowA", true);
		memzfree(child);
		return 0;
	}
	//set WndProc user data to be a pointer to this window
	::SetWindowLongPtr((HWND)child->handle, GWLP_USERDATA, (LONG_PTR)child);
	child->dc = ::GetDC((HWND)child->handle);
	
	//// get monitor info ////
	int work_x = 0, work_y = 0, work_w = 0, work_h = 0;
	Win32GetMonitorInfo((HWND)handle, &screenWidth, &screenHeight, &work_x, &work_y, &work_w, &work_h);
	
	//// setup default window pos/size ////
	if(x == 0xFFFFFFFF || y == 0xFFFFFFFF){
		x = work_x + ((work_w -  width) / 2);
		y = work_y + ((work_h - height) / 2);
	}
	
	children[child_count++] = child;
	child->name = _name;
	child->UpdateDisplayMode(displayMode);
	child->UpdateDecorations(Decoration_TitlebarFull | Decoration_Borders);
	child->titlebarheight = 5;
	SetWindowPos((HWND)child->handle, 0, x, y, width, height, 0);
	
	LogS("deshi", "Finished child window initialization in ", peek_stopwatch(t_s), "ms");
	return child;
}

//TODO options for decoration colors
void DrawDecorations(Window* win){DPZoneScoped;
	using namespace Render;
	s32 x=win->x, y=win->y;
	s32 width = win->width, height = win->height;
	s32 cwidth = win->cwidth, cheight = win->cheight;
	u32 decor = win->decorations;
	b32 hitset = 0;
	persist Font* decorfont = Storage::CreateFontFromFileBDF("gohufont-11.bdf").second;
	SetSurfaceDrawTargetByWindow(win);
	StartNewTwodCmd(GetZZeroLayerIndex(), 0, vec2::ZERO, vec2(width, height));
	
	if(Math::PointInRectangle(input_mouse_position(), vec2::ZERO, vec2(width, height))) win->hittest = HitTestClient;
	
	//minimal titlebar takes precedence over all other title bar flags
	if(HasFlag(decor, Decoration_MinimalTitlebar)){
		win->titlebarheight = 5;
		FillRect2D(vec2::ZERO, vec2(width, win->titlebarheight), Color_White);
		if(Math::PointInRectangle(input_mouse_position(), vec2::ZERO, vec2(width, win->titlebarheight))){ win->hittest = HitTestTitle; hitset = 1; }
	}else{
		if(HasFlag(decor, Decoration_Titlebar)){
			win->titlebarheight = 20;
			FillRect2D(vec2::ZERO, vec2(width, win->titlebarheight), color(60,60,60));
		}
		if(HasFlag(decor, Decoration_TitlebarTitle)){
			//!Incomplete
		}
	}
	
	if(HasFlag(decor, Decoration_MouseBorders)){
		vec2 mp = input_mouse_position();
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
		
		if(!hitset && Math::PointInRectangle(mp, lbpos, lbsiz)){win->hittest = HitTestLeft;   hitset = 1;}
		else if(Math::PointInRectangle(mp, rbpos, rbsiz))      {win->hittest = HitTestRight;  hitset = 1;}
		else if(Math::PointInRectangle(mp, bbpos, bbsiz))      {win->hittest = HitTestBottom; hitset = 1;}
		
		FillRect2D(vec2::ZERO, vec2(borderSize, height), lcol);
		FillRect2D(vec2(width - borderSize, 0), vec2(borderSize, height), rcol);
		FillRect2D(vec2(0, height - borderSize), vec2(width, borderSize), bcol);
	}else if(HasFlag(decor, Decoration_Borders)){
		win->borderthickness = 2;
		f32 borderSize = win->borderthickness;
		vec2 
			tbpos = vec2::ZERO,
		lbpos = vec2::ZERO,
		rbpos = vec2(width - borderSize, 0),
		bbpos = vec2(0, height - borderSize),
		tbsiz = vec2(width, borderSize),
		lbsiz = vec2(borderSize, height),
		rbsiz = vec2(borderSize, height),
		bbsiz = vec2(width, borderSize);
		color 
			tcol = color(133,133,133),
		bcol = color(133,133,133),
		rcol = color(133,133,133),
		lcol = color(133,133,133);
		
		FillRect2D(lbpos, lbsiz, lcol);
		FillRect2D(rbpos, rbsiz, rcol);
		FillRect2D(bbpos, bbsiz, bcol);
		FillRect2D(tbpos, tbsiz, tcol);
	}
#if LOG_INPUTS
	Log("", win->hittest);
#endif
}

void Window::Update(){DPZoneScoped;
	//-/////////////////////////////////////////////
	//// @update_window
	Stopwatch update_stopwatch = start_stopwatch();
	
	resized = false;
	
	//iterate through all window messages 
	MSG msg;
	while(::PeekMessageA(&msg, (HWND)handle, 0, 0, PM_REMOVE)){
		if(msg.message == WM_QUIT || msg.message == WM_CLOSE || msg.message == WM_DESTROY){
			closeWindow = true;
		}else{
			::TranslateMessage(&msg);
			::DispatchMessageA(&msg);
		}
	}
	
	//update children (this should maybe be done by whoever creates it instead of the parent)
	forI(child_count) children[i]->Update();
	
	if(decorations != Decoration_SystemDecorations) DrawDecorations(this);
	hittest = HitTestNone;
	if(cursorMode == CursorMode_FirstPerson) SetCursorPos(centerX, centerY /*+ titlebarheight*/);
	
	DeshTime->windowTime = reset_stopwatch(&update_stopwatch);
	//-/////////////////////////////////////////////
	//// @update_time
	DeshTime->prevDeltaTime = DeshTime->deltaTime;
	DeshTime->deltaTime = start_stopwatch() - peek_stopwatch(DeshTime->stopwatch);
	DeshTime->stopwatch = start_stopwatch();
	DeshTime->totalTime += DeshTime->deltaTime;
	DeshTime->frame++;
	
	DeshTime->timeTime = reset_stopwatch(&update_stopwatch);
	//-/////////////////////////////////////////////
	//// @update_input
	//caches input values so they are consistent thru the frame
	memcpy(&DeshInput->oldKeyState, &DeshInput->newKeyState,  sizeof(b32)*MAX_KEYBOARD_KEYS);
	memcpy(&DeshInput->newKeyState, &DeshInput->realKeyState, sizeof(b32)*MAX_KEYBOARD_KEYS);
	
	if(!memcmp(DeshInput->newKeyState, DeshInput->zero, MAX_KEYBOARD_KEYS)){
		reset_stopwatch(&DeshInput->time_since_key_hold);
		DeshInput->newKeyState[0] = 1;
		DeshInput->anyKeyDown = 0;
	}else{
		DeshInput->time_key_held = peek_stopwatch(DeshInput->time_since_key_hold);
		DeshInput->anyKeyDown = 1;
	}
	
	if(!DeshInput->realCharCount){
		reset_stopwatch(&DeshInput->time_since_char_hold);
	}else{
		DeshInput->time_char_held = peek_stopwatch(DeshInput->time_since_char_hold);
	}
	
	DeshInput->mouseX = DeshInput->realMouseX;
	DeshInput->mouseY = DeshInput->realMouseY;
	DeshInput->screenMouseX = DeshInput->realScreenMouseX;
	DeshInput->screenMouseY = DeshInput->realScreenMouseY;
	DeshInput->scrollY = DeshInput->realScrollY;
	DeshInput->realScrollY = 0;
	DeshInput->charCount = DeshInput->realCharCount;
	DeshInput->realCharCount = 0;
	
	DeshTime->inputTime = peek_stopwatch(update_stopwatch);
}

b32 Window::ShouldClose(){DPZoneScoped;
	DPFrameMark;
	return closeWindow;
}

void Window::Close(){DPZoneScoped;
	closeWindow = true;
}

void Window::Cleanup(){DPZoneScoped;
	::DestroyWindow((HWND)handle);
}

void Window::UpdateDisplayMode(DisplayMode dm){DPZoneScoped;
	switch (dm){
		case DisplayMode_Windowed:{
			if(displayMode != DisplayMode_Windowed){
				HWND hwnd = (HWND)handle;
				u32 style = ::GetWindowLongA(hwnd, GWL_STYLE);
				::SetWindowLongA(hwnd, GWL_STYLE, AddFlag(style, WS_OVERLAPPEDWINDOW));
				::SetWindowPos(hwnd, HWND_TOP, restoreX, restoreY, restoreW, restoreH, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
				resized = true;
			}
			displayMode = dm;
		}break;
		case DisplayMode_Fullscreen:{
			if(displayMode != DisplayMode_Fullscreen){
				restoreX = x;
				restoreY = y;
				restoreW = width;
				restoreH = height;
				HWND hwnd = (HWND)handle;
				u32 style = ::GetWindowLongA(hwnd, GWL_STYLE);
				MONITORINFO mi = { sizeof(MONITORINFO) };
				WINDOWPLACEMENT wp = { 0 };
				//TODO add a way to choose what monitor the window is fullscreened to
				b32 failed = false;
				if(!GetWindowPlacement(hwnd, &wp)){ Win32LogLastError("GetWindowPlacement"); failed = true; }
				if(!GetMonitorInfo(::MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &mi)){ Win32LogLastError("GetMonitorInfo"); failed = true; }
				
				if(!failed){
					::SetWindowLongA(hwnd, GWL_STYLE, RemoveFlag(style, WS_OVERLAPPEDWINDOW));
					RECT mr = mi.rcMonitor;
					::SetWindowPos(hwnd, HWND_TOP, mr.left, mr.top, mr.right - mr.left, mr.bottom - mr.top, SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
				}
				resized = true;
			}
			displayMode = dm;
		}break;
		case DisplayMode_Borderless:{
			if(displayMode != DisplayMode_Borderless){
				HWND hwnd = (HWND)handle;
				u32 style = ::GetWindowLongA(hwnd, GWL_STYLE);
				::SetWindowLongA(hwnd, GWL_STYLE, RemoveFlag(style, WS_OVERLAPPEDWINDOW));
				::SetWindowPos(hwnd, HWND_TOP, restoreX, restoreY, restoreW, restoreH, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
				resized = true;
			}
			displayMode = dm;
		}break;
	}
}

void Window::UpdateCursorMode(CursorMode mode){DPZoneScoped;
	if(mode == this->cursorMode){return;}
	cursorMode = mode;
	
	switch(mode){
		case(CursorMode_Default):default:{
			SetCursor(CursorType_Arrow); //TODO setup a restore cursor?
			::ClipCursor(0);
		}break;
		case(CursorMode_FirstPerson):{
			//set cursor to middle of screen
			SetCursorPos(centerX, centerY /*+ titlebarheight*/);
			
			//hide cursor
			SetCursor(CursorType_Hidden);
			
			//restrict cursor to client rect
			RECT clip_rect;
			::GetClientRect((HWND)handle, &clip_rect);
			::ClientToScreen((HWND)handle, (POINT*)&clip_rect.left); //NOTE transforms .top as well
			::ClientToScreen((HWND)handle, (POINT*)&clip_rect.right);
			::ClipCursor(&clip_rect);
		}break;
		case(CursorMode_Hidden):{
			SetCursor(CursorType_Hidden);
			::ClipCursor(0);
		}break;
	}
#if 0
	Log("window", "Setting cursor mode to ", mode);
#endif
}

void Window::UpdateDecorations(Decoration _decorations){DPZoneScoped;
	decorations = _decorations;
	HWND hwnd = (HWND)handle;
	u32 style = ::GetWindowLongA(hwnd, GWL_STYLE);
	if(decorations == Decoration_SystemDecorations){
		::SetWindowLongA(hwnd, GWL_STYLE, AddFlag(style, WS_OVERLAPPEDWINDOW));
		titlebarheight = 0;
		borderthickness = 0;
	}else{
		::SetWindowLongA(hwnd, GWL_STYLE, RemoveFlag(style, WS_OVERLAPPEDWINDOW)); 
	}
	::SetWindowPos(hwnd, 0, x, y, width, height, SWP_NOMOVE | SWP_NOOWNERZORDER);
}

void Window::SetCursorPos(f64 _x, f64 _y){DPZoneScoped;
#if 0
	Logf("window", "SetCursorPos old(%f,%f) new(%f,%f)", DeshInput->mouseX, DeshInput->mouseY, _x, _y);
#endif
	
	POINT p = {LONG(_x), LONG(_y + 2.f*titlebarheight)};
	::ClientToScreen((HWND)handle, &p);
	::SetCursorPos(p.x, p.y);
	
	DeshInput->mouseX = _x;
	DeshInput->mouseY = _y;
}

void Window::SetCursorPosScreen(f64 _x, f64 _y){
#if 0
	Logf("window", "SetCursorPosScreen old(%f,%f) new(%f,%f)", DeshInput->screenMouseX, DeshInput->screenMouseY, _x, _y);
#endif
	
	::SetCursorPos(_x, _y);
	DeshInput->screenMouseX = _x;
	DeshInput->screenMouseY = _y;
}

void Window::SetCursor(CursorType cursor_type){
	switch(cursor_type){
		case CursorType_Arrow:{
			::SetCursor(LoadCursor(0, IDC_ARROW));
		}break;
		case CursorType_HResize:{
			::SetCursor(LoadCursor(0, IDC_SIZEWE));
		}break;
		case CursorType_VResize:{
			::SetCursor(LoadCursor(0, IDC_SIZENS));
		}break;
		case CursorType_RightDiagResize:{
			::SetCursor(LoadCursor(0, IDC_SIZENESW));
		}break;
		case CursorType_LeftDiagResize:{
			::SetCursor(LoadCursor(0, IDC_SIZENWSE));
		}break;
		case CursorType_Hand:{
			::SetCursor(LoadCursor(0, IDC_HAND));
		}break;
		case CursorType_IBeam:{
			::SetCursor(LoadCursor(0, IDC_IBEAM));
		}break;
		case CursorType_Hidden:{
			::SetCursor(0);
		}break;
		default:{
			LogE("window","Unknown cursor type: ", cursor_type);
			::SetCursor(LoadCursor(0, IDC_ARROW));
		}break;
	}
#if 0
	Log("window", "Setting cursor type to ", cursor_type);
#endif
}

void Window::UpdateRawInput(b32 rawInput){DPZoneScoped;
	WarnFuncNotImplemented("");
}

void Window::UpdateResizable(b32 resizable){DPZoneScoped;
	WarnFuncNotImplemented("");
}

void Window::GetScreenSize(s32& _width, s32& _height){DPZoneScoped;
	_width = screenWidth; _height = screenHeight;
}

void Window::GetWindowSize(s32& _width, s32& _height){DPZoneScoped;
	_width = width; _height = height;	
}

void Window::GetClientSize(s32& _width, s32& _height){DPZoneScoped;
	_width = cwidth; _height = cheight;
}

vec2 Window::GetClientAreaPosition(){DPZoneScoped;
	return vec2(0, titlebarheight);
}

vec2 Window::GetClientAreaDimensions(){DPZoneScoped;
	return vec2(width, height - titlebarheight);
}

void Window::UpdateTitle(const char* title){DPZoneScoped;
	::SetWindowTextA((HWND)handle, title);
}

void Window::ShowWindow(u32 child){DPZoneScoped;
	if(child != npos){
		::ShowWindow((HWND)children[child]->handle, SW_SHOWNORMAL);
	}else{
		::ShowWindow((HWND)handle, SW_SHOWNORMAL);
	}
}

void Window::HideWindow(u32 child){DPZoneScoped;
	if(child != npos){
		::ShowWindow((HWND)children[child]->handle, SW_HIDE);
	}else{
		::ShowWindow((HWND)handle, SW_HIDE);
	}
}

void Window::CloseConsole(){DPZoneScoped;
	//TODO funciton for making a new console 
	::FreeConsole();
}

void Window::SwapBuffers(){
	::SwapBuffers((HDC)dc);
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @win32_stopwatch
Stopwatch
start_stopwatch(){
	LARGE_INTEGER current;
	QueryPerformanceCounter(&current);
	return current.QuadPart;
}

f64
peek_stopwatch(Stopwatch watch){
	return 1000.0 * ((f64)(start_stopwatch() - watch) / (f64)deshi__win32_perf_count_frequency);
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @win32_io
local b32 io__crash_on_error = false;
//TODO maybe use a common buffer for string filename operations

cstring read_file(const File& file){
	if(!file.handle){ LogE("IO", "attempted to read a file that's not initialized"); return {0,0}; }
	cstring out;
	out.str = (char*)memalloc(file.bytes_size);
	out.count = file.bytes_size;
	
	u32 bytes_read = 0;
	if(!ReadFile(file.handle, out.str, file.bytes_size, (LPDWORD)&bytes_read, 0)){
		Win32LogLastError("ReadFile", io__crash_on_error);
		return {0,0};
	}

	if(bytes_read != file.bytes_size) LogW("IO-WIN32", "ReadFile failed to read the entire file '", get_file_name((File)file), "' \nfile's size is ", file.bytes_size, " but ", bytes_read, " were read");

	return out;
}

//TODO binary file loading when needed
//initializes a FileReader for a given File
FileReader init_reader(const File& file){
	FileReader fr;
	if(!file.handle){ LogE("IO", "attempted to initalize a FileReader on a file that's not initialized"); fr.failed = true; return fr; }
	if(!HasFlag(file.flags, FileAccess_Read)){ LogE("IO", "attempted to initialize a FileReader on a file that doesn't have read access"); fr.failed = true; return fr; }
	fr.raw.str = (char*)memalloc(file.bytes_size); //TODO eventually arena file data allocations
	fr.raw.count = file.bytes_size;
	
	u32 bytes_read = 0;
	if(!ReadFile(file.handle, fr.raw.str, file.bytes_size, (LPDWORD)&bytes_read, 0)){
		Win32LogLastError("ReadFile", io__crash_on_error);
		fr.failed = true; return fr;
	}
	if(bytes_read != file.bytes_size) LogW("IO-WIN32", "ReadFile failed to read the entire file '", get_file_name((File)file), "' \nfile's size is ", file.bytes_size, " but ", bytes_read, " were read");
	
	//gather lines
	//TODO maybe make a way to disable this
	//maybe make a function gather_lines or something and only cache lines when thats called
	//char* start = fr.raw.str, end = 0;
	//cstring raw = fr.raw;
	//forI(bytes_read + 1){
	//	if(raw[0] == '\n' || raw[0] == '\0'){
	//		fr.lines.add({ start, u32(raw.str - start) });
	//		start = raw.str + 1;
	//	}
	//	advance(&raw);
	//}
	//if(!fr.lines.count) fr.lines.add(fr.raw);
	
	fr.file = &file;
	fr.read = { fr.raw.str, 0 };
	return fr;
}

void end_reader(FileReader& reader){
	memzfree(reader.raw.str);
	defer{ CloseHandle(reader.file->handle); };
}

File open_file(const char* path, FileAccessFlags flags){
	Assert(flags, "attempt to open_file without specifing access flags");
	File file;
	file.flags = flags;
	
	DWORD access = (HasFlag(flags, FileAccess_Write) ? GENERIC_WRITE : 0) | (HasFlag(flags, FileAccess_Read) ? GENERIC_READ : 0);
	file.handle = CreateFileA(path, access, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if(GetLastError() != ERROR_FILE_EXISTS && GetLastError()) Win32LogLastError("CreateFileA", io__crash_on_error, path);
	
	//read data of opened file
	BY_HANDLE_FILE_INFORMATION data;
	ULARGE_INTEGER time;
	ULARGE_INTEGER size;
	
	if(!GetFileInformationByHandle(file.handle, &data)) Win32LogLastError("GetFileInformationByHandle", io__crash_on_error, path);
	
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
	if(pathlength > MAX_FILEPATH_SIZE) LogW("IO", "file path for '", path, "' has a length greater than MAX_FILEPATH_SIZE\npath length was ", pathlength);
	
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

void close_file(File* file){
	CloseHandle(file->handle);
}

//TODO(delle) search filters
array<File>
get_directory_files(const char* directory){
	array<File> result(deshi_temp_allocator);
	if(directory == 0) return result;
	if(directory[0] == '\0') return result;
	
	string pattern(directory); //TODO add allocator to string
	pattern += (pattern[pattern.count-1] == '/' || pattern[pattern.count-1] == '\\') ? "*" : "/*";
	WIN32_FIND_DATAA data; HANDLE next;
	ULARGE_INTEGER size;   ULARGE_INTEGER time;
	
	next = FindFirstFileA(pattern.str, &data);
	if(next == INVALID_HANDLE_VALUE || !(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)){
		Win32LogLastError("FindFirstFileA", io__crash_on_error, directory);
		FindClose(next);
		return result;
	}
	
	while(next != INVALID_HANDLE_VALUE){
		if((strcmp(data.cFileName, ".") == 0) || (strcmp(data.cFileName, "..") == 0)){
			if(FindNextFileA(next, &data) == 0) break;
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
		while (short_len && data.cFileName[short_len--] != '.'); //remove extension from short
		file.path_length = path_len + name_len;
		file.name_length = name_len;
		file.short_length = short_len + 1;
		file.ext_length = name_len - short_len - 1;
		Assert(file.path_length < MAX_FILEPATH_SIZE);
		CopyMemory(file.path, pattern.str, pattern.count - 1);
		CopyMemory(file.path + path_len, data.cFileName, name_len);
		CopyMemory(file.name, data.cFileName, name_len);
		
		result.add(file);
		if(FindNextFileA(next, &data) == 0) break;
	}
	DWORD error = GetLastError();
	if(error != ERROR_NO_MORE_FILES){
		Win32LogLastError("FindNextFileA", io__crash_on_error, directory);
	}
	FindClose(next);
	
	return result;
}


//TODO(delle) add safety checks so deletion only happens within the data folder
void
delete_file(const char* _filepath){
	if(_filepath == 0) return;
	if(_filepath[0] == '\0') return;
	
	string filepath(_filepath, deshi_temp_allocator);
	if(filepath[filepath.count-1] == '/' || filepath[filepath.count-1] == '\\'){
		filepath[filepath.count-1] = '\0';
		filepath.count--;
	}
	
	WIN32_FIND_DATAA data;
	HANDLE next = FindFirstFileA(filepath.str, &data);
	if(next == INVALID_HANDLE_VALUE){
		Win32LogLastError("FindFirstFileA", io__crash_on_error, filepath.str);
		return;
	}
	
	//if directory, recursively delete all files and directories
	if(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
		array<File> dir_files = get_directory_files(filepath.str);
		forE(dir_files) delete_file(it->path);
		BOOL success = RemoveDirectoryA(filepath.str);
		if(!success) Win32LogLastError("RemoveDirectoryA", io__crash_on_error, filepath.str);
	}else{
		BOOL success = DeleteFileA(filepath.str);
		if(!success) Win32LogLastError("DeleteFile", io__crash_on_error, filepath.str);
	}
	FindClose(next);
}

b32
file_exists(const char* _filepath){
	if(_filepath == 0) return false;
	if(_filepath[0] == '\0') return false;
	
	string filepath(_filepath, deshi_temp_allocator);
	if(filepath[filepath.count-1] == '/' || filepath[filepath.count-1] == '\\'){
		filepath[filepath.count-1] = '\0';
		filepath.count--;
	}
	
	WIN32_FIND_DATAA data;
	HANDLE handle = FindFirstFileA(filepath.str, &data);
	if(handle != INVALID_HANDLE_VALUE){
		FindClose(handle);
		return true;
	}
	return false;
}

void
rename_file(const char* old_filepath, const char* new_filepath){
	if(old_filepath == 0) return;
	if(old_filepath[0] == '\0') return;
	
	string filepath(old_filepath, deshi_temp_allocator);
	if(filepath[filepath.count-1] == '/' || filepath[filepath.count-1] == '\\'){
		filepath[filepath.count-1] = '\0';
		filepath.count--;
	}
	
	BOOL success = MoveFileA(filepath.str, new_filepath);
	if(!success) Win32LogLastError("MoveFileA", io__crash_on_error, filepath.str);
}

cstring
absolute_path(const char* relative_path){
	if(relative_path == 0) return cstring{};
	if(relative_path[0] == '\0') return cstring{};
	
	string filepath(relative_path, deshi_temp_allocator);
	if(filepath[filepath.count-1] == '/' || filepath[filepath.count-1] == '\\'){
		filepath[filepath.count-1] = '\0';
		filepath.count--;
	}
	
	cstring result{};
	result.str   = (char*)memory_talloc(MAX_PATH*sizeof(char));
	result.count = (upt)GetFullPathNameA(filepath.str, MAX_PATH, result.str, 0);
	if(!result.count){
		Win32LogLastError("GetFullPathNameA", io__crash_on_error, filepath.str);
		return cstring{};
	}
	return result;
}

File*
file_info(const char* _filepath){
	if(_filepath == 0) return 0;
	if(_filepath[0] == '\0') return 0;
	
	string filepath(_filepath, deshi_temp_allocator);
	if(filepath[filepath.count-1] == '/' || filepath[filepath.count-1] == '\\'){
		filepath[filepath.count-1] = '\0';
		filepath.count--;
	}
	
	WIN32_FIND_DATAA data;
	HANDLE next = FindFirstFileA(filepath.str, &data);
	defer{ FindClose(next); };
	if(next == INVALID_HANDLE_VALUE){
		Win32LogLastError("FindFirstFileA", io__crash_on_error, filepath.str);
		return 0;
	}
	
	ULARGE_INTEGER size, time;
	File* result = (File*)memory_talloc(sizeof(File));
	result->handle = next;
	time.LowPart = data.ftCreationTime.dwLowDateTime; time.HighPart = data.ftCreationTime.dwHighDateTime;
	result->time_creation = WindowsTimeToUnixTime(time.QuadPart);
	time.LowPart = data.ftLastAccessTime.dwLowDateTime; time.HighPart = data.ftLastAccessTime.dwHighDateTime;
	result->time_last_access = WindowsTimeToUnixTime(time.QuadPart);
	time.LowPart = data.ftLastWriteTime.dwLowDateTime; time.HighPart = data.ftLastWriteTime.dwHighDateTime;
	result->time_last_write = WindowsTimeToUnixTime(time.QuadPart);
	size.LowPart = data.nFileSizeLow; size.HighPart = data.nFileSizeHigh;
	result->bytes_size = size.QuadPart;
	result->is_directory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
	
	Assert(filepath.count < MAX_FILEPATH_SIZE);
	result->path_length = filepath.count;
	CopyMemory(result->path, filepath.str, filepath.count);
	result->name_length = strlen(data.cFileName);
	CopyMemory(result->name, data.cFileName, result->name_length);
	result->short_length = result->name_length;
	
	if(result->is_directory){
		result->ext_length = 0;
	}else{
		while(result->short_length && data.cFileName[result->short_length--] != '.'); //remove extension from short
		result->ext_length = result->name_length - result->short_length - 1;
	}
	return result;
}

cstring
read_entire_file(const char* filepath){
	File file = open_file(filepath, FileAccess_Read);
	if(!file.handle) return cstring{};
	defer{ close_file(&file); };
	
	cstring result{};
	result.str   = (char*)memory_talloc(file.bytes_size);
	result.count = file.bytes_size;
	
	u32 bytes_read = 0;
	if(!ReadFile(file.handle, result.str, file.bytes_size, (LPDWORD)&bytes_read, 0)){
		Win32LogLastError("ReadFile", io__crash_on_error);
		return cstring{};
	}
	if(bytes_read != file.bytes_size){
		LogW("IO-WIN32", "ReadFile failed to read the entire file '", get_file_name(file), "'.\n"
			 "File's size is ",file.bytes_size,", but ",bytes_read," were read");
	}
	
	return result;
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @win32_module
void*
platform_load_module(const char* module_path){
	return LoadLibraryA(module_path);
}

void
platform_free_module(void* module){
	FreeLibrary((HMODULE)module);
}

platform_symbol
platform_get_module_symbol(void* module, const char* symbol_name){
	return (platform_symbol)GetProcAddress((HMODULE)module, symbol_name);
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @win32_clipboard
//!ref: Dear ImGui imgui.cpp@GetClipboardTextFn_DefaultImpl
str8
platform_get_clipboard(){
	str8 result{};
	
	if(::OpenClipboard(NULL) == NULL){
		Win32LogLastError("OpenClipboard");
		return result;
	}
	
	HANDLE wchar_buffer_handle = ::GetClipboardData(CF_UNICODETEXT);
	if(wchar_buffer_handle == NULL){
		Win32LogLastError("GetClipboardData");
		::CloseClipboard();
		return result;
	}
	
	if(const WCHAR* wchar_buffer = (const WCHAR*)::GlobalLock(wchar_buffer_handle)){
		result.count = (s64)::WideCharToMultiByte(CP_UTF8, 0, wchar_buffer, -1, NULL, 0, NULL, NULL);
		result.str   = (u8*)memory_talloc(result.count);
		::WideCharToMultiByte(CP_UTF8, 0, wchar_buffer, -1, (LPSTR)result.str, (int)result.count, NULL, NULL);
	}else{
		Win32LogLastError("GlobalLock");
	}
	::GlobalUnlock(wchar_buffer_handle);
    ::CloseClipboard();
	return result;
}

//!ref: Dear ImGui imgui.cpp@SetClipboardTextFn_DefaultImpl
void
platform_set_clipboard(str8 text){
	if(::OpenClipboard(NULL) == NULL){
		Win32LogLastError("OpenClipboard");
		return;
	}
	
	int wchar_buffer_length = ::MultiByteToWideChar(CP_UTF8, 0, (const char*)text.str, text.count*sizeof(u8), NULL, 0);
	HGLOBAL wchar_buffer_handle = ::GlobalAlloc(GMEM_MOVEABLE, (SIZE_T)wchar_buffer_length*sizeof(WCHAR));
	if(wchar_buffer_handle == NULL){
		Win32LogLastError("GlobalAlloc");
		::CloseClipboard();
		return;
	}
	
	WCHAR* wchar_buffer = (WCHAR*)::GlobalLock(wchar_buffer_handle);
    ::MultiByteToWideChar(CP_UTF8, 0, (const char*)text.str, text.count*sizeof(u8), wchar_buffer, wchar_buffer_length);
    ::GlobalUnlock(wchar_buffer_handle);
    ::EmptyClipboard();
    if(::SetClipboardData(CF_UNICODETEXT, wchar_buffer_handle) == NULL){
		Win32LogLastError("SetClipboardData");
        ::GlobalFree(wchar_buffer_handle);
	}
    ::CloseClipboard();
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @win32_threading
mutex::
mutex(){DPZoneScoped;
	if(handle = CreateMutex(NULL, FALSE, NULL); !handle)
		Win32LogLastError("CreateMutex");
}

mutex::
~mutex(){DPZoneScoped;//NOTE a mutex is not released on scope end, use scopedlock for this
	CloseHandle(handle);
}

void mutex::
lock(){DPZoneScoped;
	DWORD waitResult = WaitForSingleObject(handle, INFINITE);
	switch(waitResult){
		case WAIT_ABANDONED:{
			//TODO maybe have an option somewhere to suppress this error
			LogE("threading-win32", "Attempted to lock an abandoned mutex. This mutex was not released by the thread that owned it before the thread terminated.");
		}break;
		//TODO set up our own error reporting once i figure out what error codes are what for this
		case WAIT_FAILED:{ Win32LogLastError("CreateMutex"); Assert(0); }break;
	}
	is_locked = 1;
}

b32 mutex::
try_lock(){DPZoneScoped;
	DWORD waitResult = WaitForSingleObject(handle, 0);
	switch(waitResult){
		case WAIT_ABANDONED:{
			//TODO maybe have an option somewhere to suppress this error
			LogE("threading-win32", "Locking an abandoned mutex. This mutex was not released by the thread that owned it before the thread terminated.");
		}break;
		case WAIT_TIMEOUT:{
			return false;
		}break;
		//TODO set up our own error reporting once i figure out what error codes are what for this
		case WAIT_FAILED:{ Win32LogLastError("CreateMutex"); Assert(0); }break;
	}
	is_locked = 1;
	return true;
}

b32 mutex::
try_lock_for(u64 milliseconds){DPZoneScoped;
	DWORD waitResult = WaitForSingleObject(handle, milliseconds);
	switch(waitResult){
		case WAIT_ABANDONED:{
			//TODO maybe have an option somewhere to suppress this error
			LogE("threading-win32", "Locking an abandoned mutex. This mutex was not released by the thread that owned it before the thread terminated.");
		}break;
		case WAIT_TIMEOUT:{
			return false;
		}break;
		//TODO set up our own error reporting once i figure out what error codes are what for this
		case WAIT_FAILED:{ Win32LogLastError("CreateMutex"); Assert(0); }break;
	}
	is_locked = 1;
	return true;
}

void mutex::
unlock(){DPZoneScoped;
	if(!ReleaseMutex(handle)){ Win32LogLastError("ReleaseMutex"); Assert(0); }
}


condition_variable::condition_variable(){DPZoneScoped;
	//NOTE i have to use std mem here in the case that condvar is used before memory is initialized (eg. a condvar global var)
	cvhandle = malloc(sizeof(CONDITION_VARIABLE));
	cshandle = malloc(sizeof(CRITICAL_SECTION));
	InitializeCriticalSection((CRITICAL_SECTION*)cshandle);
	InitializeConditionVariable((CONDITION_VARIABLE*)cvhandle);
}

condition_variable::~condition_variable(){DPZoneScoped;
	free(cvhandle); free(cshandle);
}

void condition_variable::
notify_one(){DPZoneScoped;
	WakeConditionVariable((CONDITION_VARIABLE*)cvhandle);
}

void condition_variable::
notify_all(){DPZoneScoped;
	WakeAllConditionVariable((CONDITION_VARIABLE*)cvhandle);
}

void condition_variable::
wait(){DPZoneScoped;
	EnterCriticalSection((CRITICAL_SECTION*)cshandle);
	if(!SleepConditionVariableCS((CONDITION_VARIABLE*)cvhandle, (CRITICAL_SECTION*)cshandle, INFINITE)){
		Win32LogLastError("SleepConditionVariableCS");
	}
	LeaveCriticalSection((CRITICAL_SECTION*)cshandle);
}

void condition_variable::
wait_for(u64 milliseconds){DPZoneScoped;
	EnterCriticalSection((CRITICAL_SECTION*)cshandle);
	SleepConditionVariableCS((CONDITION_VARIABLE*)cvhandle, (CRITICAL_SECTION*)cshandle, milliseconds);
	LeaveCriticalSection((CRITICAL_SECTION*)cshandle);	
}

// template<typename FuncToRun, typename... FuncArgs>
// DWORD WINAPI threadfunc_stub(LPVOID in){DPZoneScoped;
// 	upt o = 2;
// 	tuple<Thread*, FuncToRun, FuncArgs...>* intup = (tuple<Thread*, FuncToRun, FuncArgs...>*)in;
// 	intup->get<0>()->threadfunc<FuncToRun, FuncArgs...>(intup->get<1>(), (*((FuncArgs*)&intup->raw[intup->offsets[o++]]), ...));
// 	return 0;
// }

// template<typename FuncToRun, typename... FuncArgs>
// void Thread::threadfunc(FuncToRun f, FuncArgs... args){DPZoneScoped;
// 	SetName(str16_from_str8(comment.str));
// 	while(state!=ThreadState_Close){
// 		if(state==ThreadState_CallFunction){
// 			while(functioncalls--){f(deref_if_ptr(args)...);}
// 		}
// 		manager_return_callback(this);
// 		ChangeState(ThreadState_Sleep);
// 		calling_thread_cv.notify_all();
// 		if(state==ThreadState_Sleep) thread_cv.wait();
// 	}
// 	calling_thread_cv.notify_all();
// }

// template<typename FuncToRun, typename... FuncArgs>
// void Thread::SetFunction(FuncToRun f, FuncArgs...args){DPZoneScoped;
//     using tup = tuple<Thread*, FuncToRun, FuncArgs...>;
// 	memfree(tuple_handle);
// 	CloseAndJoin(); ChangeState(ThreadState_Initializing);
// 	tuple_handle = memalloc(sizeof(tup));
// 	(*(tup*)tuple_handle) = tup(this, f, args...); 
// 	CreateThread(0, 0, threadfunc_stub<FuncToRun, FuncArgs...>, tuple_handle, 0, 0);
// }


// void Thread::SetName(str16 name){
// 	if(FAILED(SetThreadDescription(handle, (LPCWSTR)name.str))){
// 		//TODO handle whatever errors this may throw
// 	}
// }

#ifdef BUILD_SLOW
#define WorkerLog(message) Log("thread", "worker ", me, ": ", message)
#else
#define WorkerLog(message)
#endif
//infinite loop worker thread
// all this does is check for a job from ThreadManager's job_ring and if it finds one it runs it immediately
// after running the job it continues looking for more jobs
// if it can't find any jobs to run it just goes to sleep
void deshi__thread_worker(Thread* me){DPZoneScoped;
	ThreadManager* man = DeshThreadManager;
	WorkerLog("spawned");
	while(!me->close){
		while(man->job_ring.count){//lock and retrieve a job from ThreadManager
			WorkerLog("looking to take a job from job ring");
			ThreadJob tj;
			//TODO look into how DOOM3 does this w/o locking, I don't understand currently how they atomically do this 
			{scopedlock jrl(man->job_ring_lock);
				//check once more that there are jobs since the locking thread could have taken the last one
				//im sure theres a better way to do this
				if(!man->job_ring.count) break; 
				WorkerLog("locked job ring and taking a job");
				//take the job and remove it from the ring
				tj = man->job_ring[0];
				man->job_ring.remove(1);
			}
			//run the function
			WorkerLog("running job");
			me->running = true;
			tj.ThreadFunction(tj.data);
			me->running = false;
			WorkerLog("finished running job");
		}
		//when there are no more jobs go to sleep until notified again by thread manager
		//NOTE this may hang!
		WorkerLog("going to sleep");
		man->idle.wait();
		WorkerLog("woke up");
	}
	WorkerLog("closing");
	DebugBreakpoint;
}

DWORD WINAPI deshi__thread_worker__win32_stub(LPVOID in){DPZoneScoped;
	deshi__thread_worker((Thread*)in);
	return 0;
}

void ThreadManager::init(u32 max_jobs){
	AssertDS(DS_MEMORY, "Attempt to init ThreadManager without loading Memory first");
	job_ring.init(max_jobs, deshi_allocator);
}

void ThreadManager::spawn_thread(){DPZoneScoped;
	Thread* t = (Thread*)memalloc(sizeof(Thread));
	threads.add(t);
	CreateThread(0, 0, deshi__thread_worker__win32_stub, (void*)t, 0, 0);
}

void ThreadManager::close_all_threads(){
	forI(threads.count) threads[i]->close = true;
	wake_threads(0);
	threads.clear();
}

void ThreadManager::add_job(ThreadJob job){
	job_ring.add(job);
}

void ThreadManager::add_jobs(carray<ThreadJob> jobs){
	forI(jobs.count) job_ring.add(jobs[i]);
}

void ThreadManager::cancel_all_jobs(){
	job_ring.clear();
} 

 void ThreadManager::wake_threads(u32 count){
	 if(!threads.count){ LogW("Thread", "Attempt to use wake_threads without spawning any threads first"); }
	 else if(!count) idle.notify_all(); 
	 else{
		 forI(count) idle.notify_one();
	 }
}
