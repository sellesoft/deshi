/* deshi Win32 Platform Backend
Index:
@win32_vars
@win32_helpers
@win32_callback
@win32_platform
@win32_stopwatch
@win32_file
@win32_modules
@win32_clipboard
@win32_threading
@win32_window
*/


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @win32_vars
local s64 win32_perf_count_frequency;
local wchar_t* win32_file_data_folder;
local s32 win32_file_data_folder_len;
local HINSTANCE win32_console_instance;


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @win32_helpers
//NOTE(delle) ideally we'd do this with a static C array, but C++ doesn't support out of order array init
//NOTE(delle) FORCE_INLINE because it's only used in one place
FORCE_INLINE KeyCode
win32_vkcode_to_key(s32 vk){
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
		case VK_NUMPAD0: return Key_NP0; case VK_NUMPAD1: return Key_NP1; case VK_NUMPAD2: return Key_NP2; case VK_NUMPAD3: return Key_NP3;
		case VK_NUMPAD4: return Key_NP4; case VK_NUMPAD5: return Key_NP5; case VK_NUMPAD6: return Key_NP6; case VK_NUMPAD7: return Key_NP7;
		case VK_NUMPAD8: return Key_NP8; case VK_NUMPAD9: return Key_NP9;
		case VK_MULTIPLY: return Key_NPMULTIPLY; case VK_DIVIDE:  return Key_NPDIVIDE; case VK_ADD:     return Key_NPPLUS;
		case VK_SUBTRACT: return Key_NPMINUS;    case VK_DECIMAL: return Key_NPPERIOD; case VK_NUMLOCK: return Key_NUMLOCK;
		default: return Key_NONE;
	}
}

local void
win32_log_last_error(const char* func_name, b32 crash_on_error = false, str8 custom = str8{}){DPZoneScoped;
	LPVOID msg_buffer;
	DWORD error = ::GetLastError();
	::FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
					 0, error, MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), (LPSTR)&msg_buffer, 0, 0);
	LogE("win32",func_name," failed with error ",(u32)error,": ",(const char*)msg_buffer," ",custom);
	::LocalFree(msg_buffer);
	if(crash_on_error){
		Assert(!"assert before exit so we can stack trace in debug mode");
		::ExitProcess(error);
	}
}

//makes the path absolute, does not strip trailing slash, converts forwardslashes to backslashes
local wchar_t*
win32_path_from_str8(str8 path, b32 prefix, s64 extra_chars = 0, s32* out_len = 0){
	wchar_t* wpath = wchar_from_str8(path, 0, deshi_temp_allocator);
	
	s64 full_wpath_length = (s64)::GetFullPathNameW((LPCWSTR)wpath, 0, 0, 0);
	if(full_wpath_length == 0){
		win32_log_last_error("GetFullPathNameW", file_crash_on_error, path);
		if(file_crash_on_error){
			Assert(!"assert before exit so we can stack trace in debug mode");
			::ExitProcess(1);
		}
		return 0;
	}
	
	wchar_t* result = 0;
	if(prefix){ // "\\?\"
		result = (wchar_t*)memory_talloc((full_wpath_length+4+extra_chars)*sizeof(wchar_t));
		result[0] = L'\\'; result[1] = L'\\'; result[2] = L'?'; result[3] = L'\\';
		::GetFullPathNameW((LPCWSTR)wpath, (DWORD)full_wpath_length, (LPWSTR)(result+4), 0);
		if(out_len) *out_len = full_wpath_length+4-1; //NOTE(delle) -1 b/c it includes \0 in length
	}else{
		result = (wchar_t*)memory_talloc((full_wpath_length+0+extra_chars)*sizeof(wchar_t));
		::GetFullPathNameW((LPCWSTR)wpath, (DWORD)full_wpath_length, (LPWSTR)(result+0), 0);
		if(out_len) *out_len = full_wpath_length+0-1; //NOTE(delle) -1 b/c it includes \0 in length
	}
	return result;
}

local void
win32_monitor_info(HWND handle, int* screen_w = 0, int* screen_h = 0, int* working_x = 0, int* working_y = 0, int* working_w = 0, int* working_h = 0){
	HMONITOR monitor = ::MonitorFromWindow(handle, MONITOR_DEFAULTTONEAREST);
	if(monitor){
		MONITORINFO monitor_info = {sizeof(MONITORINFO)};
		if(::GetMonitorInfoW(monitor, &monitor_info)){
			if(screen_w) *screen_w = monitor_info.rcMonitor.right  - monitor_info.rcMonitor.left;
			if(screen_h) *screen_h = monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top;
			if(working_x) *working_x = monitor_info.rcWork.left;
			if(working_y) *working_y = monitor_info.rcWork.top;
			if(working_w) *working_w = monitor_info.rcWork.right  - monitor_info.rcWork.left;
			if(working_h) *working_h = monitor_info.rcWork.bottom - monitor_info.rcWork.top;
		}else{
			win32_log_last_error("GetMonitorInfoW");
		}
	}else{
		win32_log_last_error("MonitorFromWindow");
	}
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @win32_callback
LRESULT CALLBACK
win32_window_callback(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){DPZoneScoped;
	Window* window = (Window*)::GetWindowLongPtrW(hwnd, GWLP_USERDATA);
	switch(msg){
		case WM_SIZE:{ ////////////////////////////////////////////////////////////// Window Resized
			if(window){
				window->dimensions.x = LOWORD(lParam);
				window->dimensions.y = HIWORD(lParam);// - window->titlebar_height;
				window->dimensions_decorated.x = LOWORD(lParam);
				window->dimensions_decorated.y = HIWORD(lParam);
				window->center    = vec2i{(s32)LOWORD(lParam)/2, (s32)HIWORD(lParam)/2};
				window->minimized = (wParam == SIZE_MINIMIZED);
				window->resized   = true;
			}else{ Assert(!"WM_SIZE passed to win32 HWND that does not have matching deshi Window"); }
		}break;
		
		case WM_MOVE:{ ////////////////////////////////////////////////////////////// Window Moved
			if(window){
				window->x = LOWORD(lParam);
				window->y = HIWORD(lParam);
				
				RECT rect;
				::GetWindowRect((HWND)window->handle, &rect);
				window->position_decorated.x = rect.left;
				window->position_decorated.y = rect.top;
				window->dimensions_decorated.x = rect.right  - rect.left;
				window->dimensions_decorated.y = rect.bottom - rect.top;
			}else{ Assert(!"WM_MOVE passed to win32 HWND that does not have matching deshi Window"); }
		}break;
		
		case WM_CLOSE:{ ///////////////////////////////////////////////////////////// Close Window (signal to destroy window)
			if(window){
				window->close_window += 1;
				if(platform_fast_exit || window->close_window > 1){
					::DestroyWindow(hwnd);
				}else{
					return 0;
				}
			}else{ Assert(!"WM_CLOSE passed to win32 HWND that does not have matching deshi Window"); }
		}break;
		
		case WM_DESTROY:{ /////////////////////////////////////////////////////////// Destroy Window
			if(window){
				Assert(window != &window_helper, "WM_DESTROY should not be called on helper_window");
				
				forI(window_windows.count){
					if(window == window_windows[i]){
						memory_zfree(window);
						window_windows.remove_unordered(i);
						if(window_windows.count){
							window_windows[i]->index = i;
							break;
						}else{
							return 0;
						}
					}
				}
			}else{ Assert(!"WM_DESTROY passed to win32 HWND that does not have matching deshi Window"); }
		}break;
		
		/*
		case WM_NCHITTEST:{ ///////////////////////////////////////////////////////// Window Decoration Clicked
			if(window){
			s32  xPos = GET_X_LPARAM(lParam);
			s32  yPos = GET_Y_LPARAM(lParam);
			s32  x = window->x, y = window->y;
			s32  width = window->width, height = window->height;
			s32  cx = window->cx, cy = window->cy;
			s32  cwidth = window->cwidth, cheight = window->cheight;
			s32  tbh = window->titlebarheight;
			s32  bt = window->borderthickness;
			vec2 mp = input_mouse_position() + vec2(bt, tbh+bt*2);
			u32  decor = window->decorations;
			b32  hitset = 0;
			if(Math::PointInRectangle(mp, vec2(cx, cy),                  vec2(cwidth, cheight)))                   return HTCLIENT;
			if(Math::PointInRectangle(mp, vec2(bt, bt),                  vec2(width - 2*bt, window->titlebarheight))) return HTCAPTION;
			if(Math::PointInRectangle(mp, vec2::ZERO,                    vec2(bt, bt)))                            return HTTOPLEFT;
			if(Math::PointInRectangle(mp, vec2(0, height - bt),          vec2(bt, bt)))                            return HTBOTTOMLEFT;
			if(Math::PointInRectangle(mp, vec2(width - bt, 0),           vec2(bt, bt)))                            return HTTOPRIGHT;
			if(Math::PointInRectangle(mp, vec2(width - bt, height - bt), vec2(bt, bt)))                            return HTBOTTOMRIGHT;
			if(Math::PointInRectangle(mp, vec2::ZERO,                    vec2(width, bt)))                         return HTTOP;
			if(Math::PointInRectangle(mp, vec2(0, height - bt),          vec2(width, bt)))                         return HTBOTTOM;
			if(Math::PointInRectangle(mp, vec2::ZERO,                    vec2(bt, height)))                        return HTLEFT;
				if(Math::PointInRectangle(mp, vec2(width - bt, 0),           vec2(bt, height)))                        return HTRIGHT;
			}else{ Assert(!"WM_NCHITTEST passed to win32 HWND that does not have matching deshi Window"); }
		}
		*/
		
		case WM_MOUSEMOVE:{ ///////////////////////////////////////////////////////// Mouse Moved
			if(window){
				const s32 xPos = GET_X_LPARAM(lParam);
				const s32 yPos = GET_Y_LPARAM(lParam);
				DeshInput->realMouseX = xPos;// - f64(window->borderthickness);
				DeshInput->realMouseY = yPos;// - f64(window->titlebarheight + window->titlebarheight);
				POINT p = { xPos, yPos };
				::ClientToScreen((HWND)window->handle, &p);
				DeshInput->realScreenMouseX = p.x;
				DeshInput->realScreenMouseY = p.y;
			}else{ Assert(!"WM_NCHITTEST passed to win32 HWND that does not have matching deshi Window"); }
		}break;
		
		case WM_MOUSEWHEEL:{ //////////////////////////////////////////////////////// Mouse Scrolled
			const s32 zDelta = GET_WHEEL_DELTA_WPARAM(wParam) / (f64)WHEEL_DELTA;
			DeshInput->realScrollY = zDelta;
		}break;
		
		case WM_MOUSEHWHEEL:{ /////////////////////////////////////////////////////// Mouse H Scrolled
			const s32 zDelta = GET_WHEEL_DELTA_WPARAM(wParam) / (f64)WHEEL_DELTA;
			DeshInput->realScrollY = zDelta;
		}break;
		
		///////////////////////////////////////////////////////////////////////////// Mouse Button Down
		case WM_LBUTTONDOWN:{ DeshInput->realKeyState[Mouse_LEFT]   = true;  if(window) ::SetCapture((HWND)window->handle); }break;
		case WM_RBUTTONDOWN:{ DeshInput->realKeyState[Mouse_RIGHT]  = true;  if(window) ::SetCapture((HWND)window->handle); }break;
		case WM_MBUTTONDOWN:{ DeshInput->realKeyState[Mouse_MIDDLE] = true;  if(window) ::SetCapture((HWND)window->handle); }break;
		case WM_XBUTTONDOWN:{ DeshInput->realKeyState[(GET_XBUTTON_WPARAM(wParam) == XBUTTON1 ? Mouse_4 : Mouse_5)] = true; return true; }break;
		
		///////////////////////////////////////////////////////////////////////////// Mouse Button Up
		case WM_LBUTTONUP:  { DeshInput->realKeyState[Mouse_LEFT]   = false; if(window) ::ReleaseCapture(); }break;
		case WM_RBUTTONUP:  { DeshInput->realKeyState[Mouse_RIGHT]  = false; if(window) ::ReleaseCapture(); }break;
		case WM_MBUTTONUP:  { DeshInput->realKeyState[Mouse_MIDDLE] = false; if(window) ::ReleaseCapture(); }break;
		case WM_XBUTTONUP:  { DeshInput->realKeyState[(GET_XBUTTON_WPARAM(wParam) == XBUTTON1 ? Mouse_4 : Mouse_5)] = false; return true; }break;
		
		///////////////////////////////////////////////////////////////////////////// Key Down/Up
		case WM_KEYUP:
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
			DeshInput->capsLock   = ::GetKeyState(VK_CAPITAL) & 0x8000;
			DeshInput->numLock    = ::GetKeyState(VK_NUMLOCK) & 0x8000;
			DeshInput->scrollLock = ::GetKeyState(VK_SCROLL ) & 0x8000;
			
			//get key from vcode
			KeyCode key = win32_vkcode_to_key(vcode);
			if(key != Key_NONE){
				DeshInput->realKeyState[key] = !upFlag;
				
#if LOG_INPUTS
				str8_builder s; str8_builder_init(&s, KeyCodeStrings[key], deshi_temp_allocator);
				str8_builder_append(&s, (upFlag) ? str8l(" released") : str8l(" pressed"));
				if(DeshInput->realKeyState[Key_LSHIFT]) str8_builder_append(&s, str8l(" + LSHIFT"));
				if(DeshInput->realKeyState[Key_RSHIFT]) str8_builder_append(&s, str8l(" + RSHIFT"));
				if(DeshInput->realKeyState[Key_LCTRL])  str8_builder_append(&s, str8l(" + LCTRL"));
				if(DeshInput->realKeyState[Key_RCTRL])  str8_builder_append(&s, str8l(" + RCTRL"));
				if(DeshInput->realKeyState[Key_LALT])   str8_builder_append(&s, str8l(" + LALT"));
				if(DeshInput->realKeyState[Key_RALT])   str8_builder_append(&s, str8l(" + RALT"));
				Log("input", str8_builder_peek(&s)); 
#endif
			}
		}break;
		
		case WM_CHAR:{ ////////////////////////////////////////////////////////////// Char From Key (UTF-16)
			if(!iscntrl(LOWORD(wParam))){ //NOTE skip control characters
				//!ref: https://github.com/cmuratori/refterm/blob/main/refterm_example_terminal.c@ProcessMessages()
				persist wchar_t inputPrevChar; //used for UTF-16 surrogate pairs
				wchar_t wch = (wchar_t)wParam;
				wchar_t chars[2];
				int char_count = 0;
				
				if(IS_HIGH_SURROGATE(wch)){
					inputPrevChar = wch;
				}else if(IS_LOW_SURROGATE(wch)){
					if(IS_SURROGATE_PAIR(inputPrevChar, wch)){
						chars[0] = inputPrevChar;
						chars[1] = wch;
						char_count = 2;
					}
					inputPrevChar = 0;
				}else{
					chars[0] = wch;
					char_count = 1;
				}
				
				if(char_count){
					DeshInput->realCharCount += 
						WideCharToMultiByte(CP_UTF8, 0, chars, char_count,
											(LPSTR)(DeshInput->charIn + DeshInput->realCharCount),
											ArrayCount(DeshInput->charIn) - DeshInput->realCharCount, 0, 0);
				}
			}
		}break;
		
		case WM_INPUT:{ ///////////////////////////////////////////////////////////// Raw Input
			//TODO(delle) raw input
		}break;
	}
	if(window && ::GetForegroundWindow() == (HWND)window->handle){
		window_active = window;
	}
	return ::DefWindowProcW(hwnd, msg, wParam, lParam);
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @win32_platform
#define DESHI_HELPER_WNDCLASSNAME L"_DESHI_HELPER_WINDOW_"
#define DESHI_RENDER_WNDCLASSNAME L"_DESHI_RENDER_WINDOW_"
void
platform_init(){DPZoneScoped;
	DeshiStageInitStart(DS_PLATFORM, DS_MEMORY, "Attempted to initialize Platform module before initializing the Memory module");
	
	//// init time ////
	//get the perf counter frequency for timers
	LARGE_INTEGER perf_count_frequency_result;
	::QueryPerformanceFrequency(&perf_count_frequency_result);
	win32_perf_count_frequency = perf_count_frequency_result.QuadPart;
	DeshTime->stopwatch = start_stopwatch();
	
	//// init file ////
	wchar_t* wpath = win32_path_from_str8(str8_lit("data/"), false, 0, &win32_file_data_folder_len);
	win32_file_data_folder = (wchar_t*)memory_alloc((win32_file_data_folder_len+1)*sizeof(wchar_t));
	CopyMemory(win32_file_data_folder, wpath, win32_file_data_folder_len*sizeof(wchar_t));
	
	file_files = array<File*>(deshi_allocator);
	
	//create data directories
	file_create(str8_lit("data/"));
	file_create(str8_lit("data/cfg/"));
	file_create(str8_lit("data/temp/"));
	
	
	//// init window ////
	//get console's handle made from libc main()
	win32_console_instance = ::GetModuleHandleW(0);
	
	//register helper window class
	WNDCLASSEXW helper_window_class{};
	helper_window_class.cbSize        = sizeof(WNDCLASSEXW);
	helper_window_class.style         = CS_OWNDC;
	helper_window_class.lpfnWndProc   = win32_window_callback;
	helper_window_class.hInstance     = win32_console_instance;
	helper_window_class.lpszClassName = DESHI_HELPER_WNDCLASSNAME;
	if(!::RegisterClassExW(&helper_window_class)) win32_log_last_error("RegisterClassExW", true);
	
	//register render window class
	WNDCLASSEXW render_window_class{sizeof(WNDCLASSEXW)};
	render_window_class.cbSize        = sizeof(WNDCLASSEXW);
	render_window_class.style         = CS_OWNDC;
	render_window_class.lpfnWndProc   = win32_window_callback;
	render_window_class.hInstance     = win32_console_instance;
	render_window_class.hIcon         = ::LoadIcon(0, IDI_APPLICATION);
	render_window_class.hCursor       = ::LoadCursor(0, IDC_ARROW);
	render_window_class.lpszClassName = DESHI_RENDER_WNDCLASSNAME;
	if(!::RegisterClassExW(&render_window_class)) win32_log_last_error("RegisterClassExW", true);
	
	//create helper window
	window_helper.handle = ::CreateWindowExW(WS_EX_OVERLAPPEDWINDOW|WS_EX_NOACTIVATE, DESHI_HELPER_WNDCLASSNAME,
											 L"_deshi_helper_window_", WS_CLIPSIBLINGS|WS_CLIPCHILDREN,
											 0,0, 1,1, 0, 0, win32_console_instance, 0);
	if(!window_helper.handle) win32_log_last_error("CreateWindowExW", true);
	window_helper.dc = GetDC((HWND)window_helper.handle);
	::ShowWindow((HWND)window_helper.handle, SW_HIDE);
	MSG msg; while(::PeekMessageW(&msg, (HWND)window_helper.handle, 0, 0, PM_REMOVE)){ ::TranslateMessage(&msg); ::DispatchMessageW(&msg); }
	
	
	//// init input ////
	//TODO(delle) setup raw input
	
	DeshiStageInitEnd(DS_PLATFORM);
}

b32
platform_update(){DPZoneScoped; DPFrameMark;
	Stopwatch update_stopwatch = start_stopwatch();
	
	//// update time ////
	DeshTime->prevDeltaTime = DeshTime->deltaTime;
	DeshTime->deltaTime     = reset_stopwatch(&DeshTime->stopwatch);
	DeshTime->totalTime    += DeshTime->deltaTime;
	DeshTime->frame        += 1;
	DeshTime->timeTime = reset_stopwatch(&update_stopwatch);
	
	
	//// update window ////
	MSG msg;
	while(::PeekMessageW(&msg, (HWND)window_helper.handle, 0, 0, PM_REMOVE)){
		::TranslateMessage(&msg);
		::DispatchMessageW(&msg);
	}
	
	forI(window_windows.count){
		window_windows[i]->resized = false;
		while(::PeekMessageW(&msg, (HWND)window_windows[i]->handle, 0, 0, PM_REMOVE)){
			::TranslateMessage(&msg);
			::DispatchMessageW(&msg);
			if(window_windows.count == 0) return false;
		}
		//if(window_windows[i]->decorations != Decoration_SystemDecorations) DrawDecorations(window_windows[i]);
		//window_windows[i].hit_test = HitTestNone;
		if(window_windows[i]->cursor_mode == CursorMode_FirstPerson) window_cursor_position(window_windows[i], window_windows[i]->center);
	}
	DeshTime->windowTime = reset_stopwatch(&update_stopwatch);
	
	
	//// update input ////
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
	
	DeshInput->mouseX        = DeshInput->realMouseX;
	DeshInput->mouseY        = DeshInput->realMouseY;
	DeshInput->screenMouseX  = DeshInput->realScreenMouseX;
	DeshInput->screenMouseY  = DeshInput->realScreenMouseY;
	DeshInput->scrollY       = DeshInput->realScrollY;
	DeshInput->realScrollY   = 0;
	DeshInput->charCount     = DeshInput->realCharCount;
	DeshInput->realCharCount = 0;
	DeshTime->inputTime = peek_stopwatch(update_stopwatch);
	
	
	//// update file ////
	/* //TODO(delle) initted file change tracking thru ReadDirectoryChanges
	File file_file{};
	ULARGE_INTEGER file_size, file_time;
	for(File* file : file_files){
		
	}
	*/
	
	return !platform_exit_application;
}

void
platform_sleep(u32 time){DPZoneScoped;
	::Sleep((DWORD)time);
}

void
platform_cursor_position(s32 x, s32 y){DPZoneScoped;
	::SetCursorPos(x, y);
	DeshInput->mouseX = x - window_active->position.x;
	DeshInput->mouseY = y - window_active->position.y;
	DeshInput->screenMouseX = x;
	DeshInput->screenMouseY = y;
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @win32_stopwatch
Stopwatch
start_stopwatch(){DPZoneScoped;
	LARGE_INTEGER current;
	::QueryPerformanceCounter(&current);
	return current.QuadPart;
}

f64
peek_stopwatch(Stopwatch watch){DPZoneScoped;
	LARGE_INTEGER current;
	::QueryPerformanceCounter(&current);
	return 1000.0 * ((f64)((s64)current.QuadPart - watch) / (f64)win32_perf_count_frequency);
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @win32_file
b32
deshi__file_exists(str8 caller_file, upt caller_line, str8 path){DPZoneScoped;
	if(!path || *path.str == 0){
		LogE("file","file_exists() was passed an empty `path` at ",caller_file,"(",caller_line,")");
		return false;
	}
	
	s32 wpath_len;
	wchar_t* wpath = win32_path_from_str8(path, true, 0, &wpath_len);
	
	//strip trailing slash
	if(wpath[wpath_len-1] == L'\\'){
		wpath[wpath_len-1] = L'\0';
		wpath_len -= 1;
	}
	
	WIN32_FIND_DATAW data;
	HANDLE handle = ::FindFirstFileW(wpath, &data);
	defer{ ::FindClose(handle); };
	return (handle != INVALID_HANDLE_VALUE);
}

void
deshi__file_create(str8 caller_file, upt caller_line, str8 path){DPZoneScoped;
	if(!path || *path.str == 0){
		LogE("file","file_create() was passed an empty `path` at ",caller_file,"(",caller_line,")");
		return;
	}
	
	str8 parts{path.str, 0};
	while(path){
		str8 part = str8_eat_until_one_of(path, 2, '/', '\\');
		parts.count += part.count;
		
		wchar_t* wpath = win32_path_from_str8(parts, true);
		if(part.count < path.count){
			Assert(part.str[part.count] == '\\' || part.str[part.count] == '/');
			if((::CreateDirectoryW(wpath, 0) == 0) && (::GetLastError() != ERROR_ALREADY_EXISTS)){
				win32_log_last_error("CreateDirectoryW", file_crash_on_error, str8_from_wchar(wpath+4,deshi_temp_allocator)); //NOTE(delle) +4 b/c of "\\?\"
				return;
			}
			str8_increment(&path, part.count+1);
			parts.count += 1;
		}else{
			HANDLE handle = ::CreateFileW(wpath, GENERIC_READ|GENERIC_WRITE, 0,0, CREATE_NEW, 0,0);
			if((handle == INVALID_HANDLE_VALUE) && (::GetLastError() != ERROR_FILE_EXISTS)){
				win32_log_last_error("CreateFileW", file_crash_on_error, str8_from_wchar(wpath+4,deshi_temp_allocator)); //NOTE(delle) +4 b/c of "\\?\"
				return;
			}
			::CloseHandle(handle);
			break;
		}
	}
}

void
deshi__file_delete(str8 caller_file, upt caller_line, str8 path){DPZoneScoped;
	if(!path || *path.str == 0){
		LogE("file","file_delete() was passed an empty `path` at ",caller_file,"(",caller_line,")");
		return;
	}
	
	s32 wpath_len;
	wchar_t* wpath = win32_path_from_str8(path, true, 0, &wpath_len);
	if(wpath[wpath_len-1] == L'\\'){
		wpath[wpath_len-1] = L'\0';
		wpath_len -= 1;
	}
	
	if(memcmp(wpath+4, win32_file_data_folder, win32_file_data_folder_len*sizeof(wchar_t)) != 0){ //NOTE(delle) +4 b/c of "\\?\"
		LogE("file","File deletion can only occur within the data folder. Input path: ",path);
		if(file_crash_on_error){
			Assert(!"assert before exit so we can stack trace in debug mode");
			::ExitProcess(1);
		}
		return;
	}
	
	WIN32_FIND_DATAW data;
	HANDLE handle = ::FindFirstFileW(wpath, &data);
	if(handle == INVALID_HANDLE_VALUE){
		win32_log_last_error("FindFirstFileW", file_crash_on_error, path);
		return;
	}
	defer{ ::FindClose(handle); };
	
	//TODO(delle) check if initted in file_files to update
	
	//if directory, recursively delete all files and directories
	if(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
		carray<File> dir_files = file_search_directory(path);
		forE(dir_files) file_delete(it->path);
		BOOL success = ::RemoveDirectoryW(wpath);
		if(!success) win32_log_last_error("RemoveDirectoryW", file_crash_on_error, path);
	}else{
		BOOL success = ::DeleteFileW(wpath);
		if(!success) win32_log_last_error("DeleteFileW", file_crash_on_error, path);
	}
}

void
deshi__file_rename(str8 caller_file, upt caller_line, str8 old_path, str8 new_path){DPZoneScoped;
	if(!old_path || *old_path.str == 0){
		LogE("file","file_rename() was passed an empty `old_path` at ",caller_file,"(",caller_line,")");
		return;
	}
	if(!new_path || *new_path.str == 0){
		LogE("file","file_rename() was passed an empty `new_path` at ",caller_file,"(",caller_line,")");
		return;
	}
	
	wchar_t* old_wpath = win32_path_from_str8(old_path, true);
	if(memcmp(old_wpath+4, win32_file_data_folder, win32_file_data_folder_len*sizeof(wchar_t)) != 0){ //NOTE(delle) +4 b/c of "\\?\"
		LogE("file","File renaming can only occur within the data folder. Input old path: ",old_path);
		if(file_crash_on_error){
			Assert(!"assert before exit so we can stack trace in debug mode");
			::ExitProcess(1);
		}
		return;
	}
	
	wchar_t* new_wpath = win32_path_from_str8(new_path, true);
	if(memcmp(new_wpath+4, win32_file_data_folder, win32_file_data_folder_len*sizeof(wchar_t)) != 0){ //NOTE(delle) +4 b/c of "\\?\"
		LogE("file","File renaming can only occur within the data folder. Input new path: ",new_path);
		if(file_crash_on_error){
			Assert(!"assert before exit so we can stack trace in debug mode");
			::ExitProcess(1);
		}
		return;
	}
	
	//TODO(delle) check if initted in file_files to update
	
	BOOL success = ::MoveFileW(old_wpath, new_wpath);
	if(!success) win32_log_last_error("MoveFileW", file_crash_on_error, str8_concat3(old_path,str8_lit("\n"),new_path, deshi_temp_allocator));
}

File
deshi__file_info(str8 caller_file, upt caller_line, str8 path){DPZoneScoped;
	if(!path || *path.str == 0){
		LogE("file","file_info() was passed an empty `path` at ",caller_file,"(",caller_line,")");
		return File{};
	}
	
	s32 wpath_len;
	wchar_t* wpath = win32_path_from_str8(path, true, 0, &wpath_len);
	if(wpath[wpath_len-1] == L'\\'){
		wpath[wpath_len-1] = L'\0';
		wpath_len -= 1;
	}
	
	WIN32_FIND_DATAW data;
	HANDLE handle = ::FindFirstFileW(wpath, &data);
	if(handle == INVALID_HANDLE_VALUE){
		win32_log_last_error("FindFirstFileW", file_crash_on_error, path);
		return File{};
	}
	defer{ ::FindClose(handle); };
	
	ULARGE_INTEGER size, time;
	File result{};
	time.LowPart  = data.ftCreationTime.dwLowDateTime;
	time.HighPart = data.ftCreationTime.dwHighDateTime;
	result.creation_time    = WindowsTimeToUnixTime(time.QuadPart);
	time.LowPart  = data.ftLastAccessTime.dwLowDateTime;
	time.HighPart = data.ftLastAccessTime.dwHighDateTime;
	result.last_access_time = WindowsTimeToUnixTime(time.QuadPart);
	time.LowPart  = data.ftLastWriteTime.dwLowDateTime;
	time.HighPart = data.ftLastWriteTime.dwHighDateTime;
	result.last_write_time  = WindowsTimeToUnixTime(time.QuadPart);
	size.LowPart  = data.nFileSizeLow;
	size.HighPart = data.nFileSizeHigh;
	result.bytes            = size.QuadPart;
	result.is_directory     = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
	
	//TODO(delle) reuse the temporary wpath memory for the result//!Optimizable
	if(result.is_directory) wpath[wpath_len++] = L'\\';
	result.path = str8_from_wchar(wpath+4, deshi_temp_allocator); //NOTE(delle) +4 b/c of "\\?\"
	forI(result.path.count) if(result.path.str[i] == '\\') result.path.str[i] = '/';
	if(result.path){
		result.name = str8_skip_until_last(result.path, '/');
		if(result.name){
			str8_increment(&result.name, 1);
			result.front = str8_eat_until_last(result.name, '.');
			if(result.front){
				result.ext = str8{result.front.str+result.front.count+1, result.name.count-(result.front.count+1)};
			}
		}
	}
	
	return result;
}

carray<File>
deshi__file_search_directory(str8 caller_file, upt caller_line, str8 directory){DPZoneScoped;
	if(!directory || *directory.str == 0){
		LogE("file","file_search_directory() was passed an empty `directory` at ",caller_file,"(",caller_line,")");
		return carray<File>{};
	}
	
	//reformat the string so parent/folder/ becomes parent/folder*
	s32 wpath_len;
	wchar_t* wpath = win32_path_from_str8(directory, true, 2, &wpath_len);
	b32 ends_with_slash = true;
	if(wpath[wpath_len-1] != L'\\'){
		wpath[wpath_len++] = L'\\';
		ends_with_slash = false;
	}
	wpath[wpath_len++] = L'*';
	wpath[wpath_len  ] = L'\0';
	
	WIN32_FIND_DATAW data;
	HANDLE next = ::FindFirstFileW(wpath, &data);
	if(next == INVALID_HANDLE_VALUE){
		win32_log_last_error("FindFirstFileW", file_crash_on_error, directory);
		return carray<File>{};
	}
	defer{ ::FindClose(next); };
	
	//make a str8 of the full directory path with slash for later path building
	str8 directory_slash;
	if(ends_with_slash){
		directory_slash = directory;
	}else{
		directory_slash.count = directory.count+1;
		directory_slash.str   = (u8*)memory_talloc((directory.count+1)*sizeof(u8));
		CopyMemory(directory_slash.str, directory.str, directory.count*sizeof(u8));
		directory_slash.str[directory_slash.count-1] = '/';
	}
	forI(directory_slash.count) if(directory_slash.str[i] == '\\') directory_slash.str[i] = '/';
	
	ULARGE_INTEGER size, time;
	array<File> result(deshi_temp_allocator);
	while(next != INVALID_HANDLE_VALUE){
		if((wcscmp(data.cFileName, L".") == 0) || (wcscmp(data.cFileName, L"..") == 0)){
			if(::FindNextFileW(next, &data) == 0) break;
			continue;
		}
		
		File file{};
		time.LowPart  = data.ftCreationTime.dwLowDateTime;
		time.HighPart = data.ftCreationTime.dwHighDateTime;
		file.creation_time    = WindowsTimeToUnixTime(time.QuadPart);
		time.LowPart  = data.ftLastAccessTime.dwLowDateTime;
		time.HighPart = data.ftLastAccessTime.dwHighDateTime;
		file.last_access_time = WindowsTimeToUnixTime(time.QuadPart);
		time.LowPart  = data.ftLastWriteTime.dwLowDateTime;
		time.HighPart = data.ftLastWriteTime.dwHighDateTime;
		file.last_write_time  = WindowsTimeToUnixTime(time.QuadPart);
		size.LowPart  = data.nFileSizeLow;
		size.HighPart = data.nFileSizeHigh;
		file.bytes            = size.QuadPart;
		file.is_directory     = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
		
		//TODO(delle) reuse the temporary file_wpath memory for the file //!Optimizable
		file.path = str8_concat(directory_slash, str8_from_wchar(data.cFileName, deshi_temp_allocator), deshi_temp_allocator);
		if(file.path){
			file.name = str8_skip_until_last(file.path, '/');
			if(file.name){
				str8_increment(&file.name, 1);
				file.front = str8_eat_until_last(file.name, '.');
				if(file.front){
					file.ext = str8{file.front.str+file.front.count+1, file.name.count-(file.front.count+1)};
				}
			}
		}
		
		result.add(file);
		if(::FindNextFileW(next, &data) == 0) break;
	}
	DWORD error = ::GetLastError();
	if(error != ERROR_NO_MORE_FILES){
		win32_log_last_error("FindNextFileW", file_crash_on_error, directory);
	}
	
	return carray<File>{result.data, result.count};
}

str8
deshi__file_path_absolute(str8 caller_file, upt caller_line, str8 path){DPZoneScoped;
	if(!path || *path.str == 0){
		LogE("file","file_path_absolute() was passed an empty `path` at ",caller_file,"(",caller_line,")");
		return str8{};
	}
	
	s32 wpath_len;
	wchar_t* wpath = win32_path_from_str8(path, true, 1, &wpath_len);
	if(wpath[wpath_len-1] == L'\\'){
		wpath[wpath_len-1] = L'\0';
		wpath_len -= 1;
	}
	
	WIN32_FIND_DATAW data;
	HANDLE handle = ::FindFirstFileW(wpath, &data);
	if(handle == INVALID_HANDLE_VALUE){
		win32_log_last_error("FindFirstFileW", file_crash_on_error, path);
		return str8{};
	}
	defer{ ::FindClose(handle); };
	
	//TODO(delle) reuse the temporary wpath memory for the result //!Optimizable
	if(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) wpath[wpath_len++] = L'\\';
	str8 result = str8_from_wchar(wpath+4, deshi_temp_allocator); //NOTE(delle) +4 b/c of "\\?\"
	forI(result.count) if(result.str[i] == '\\') result.str[i] = '/';
	return result;
}

b32
deshi__file_path_equal(str8 caller_file, upt caller_line, str8 path1, str8 path2){DPZoneScoped;
	if(!path1 || *path1.str == 0){
		LogE("file","file_path_equal() was passed an empty `path1` at ",caller_file,"(",caller_line,")");
		return false;
	}
	if(!path2 || *path2.str == 0){
		LogE("file","file_path_equal() was passed an empty `path2` at ",caller_file,"(",caller_line,")");
		return false;
	}
	
	s32 wpath1_len;
	wchar_t* wpath1 = win32_path_from_str8(path1, false, 0, &wpath1_len);
	s32 wpath2_len;
	wchar_t* wpath2 = win32_path_from_str8(path2, false, 0, &wpath2_len);
	
	//strip trailing slashes for comparison
	if(wpath1[wpath1_len-1] == L'\\'){
		wpath1[wpath1_len-1] = L'\0';
		wpath1_len -= 1;
	}
	if(wpath2[wpath2_len-1] == L'\\'){
		wpath2[wpath2_len-1] = L'\0';
		wpath2_len -= 1;
	}
	
	if(wpath1_len != wpath2_len) return false;
	if(memcmp(wpath1, wpath2, wpath1_len*sizeof(wchar_t)) != 0) return false;
	return true;
}

File*
deshi__file_init(str8 caller_file, upt caller_line, str8 path, FileAccess flags, b32 ignore_nonexistence){DPZoneScoped;
	if(!path || *path.str == 0){
		LogE("file","file_init() was passed an empty `path` at ",caller_file,"(",caller_line,")");
		return 0;
	}
	
	//change access and return the file if it's already init
	for(File* f : file_files){
		if(file_path_equal(path, f->path)){
			file_change_access(f, flags);
			return f;
		}
	}
	
	File* result = 0;
	s32 wpath_len;
	wchar_t* wpath = win32_path_from_str8(path, true, 1, &wpath_len);
	if(wpath[wpath_len-1] == L'\\'){
		wpath[wpath_len-1] = L'\0';
		wpath_len -= 1;
	}
	
	//fill the File struct, create the file if specified and it doesn't exist
	WIN32_FIND_DATAW data;
	HANDLE handle = ::FindFirstFileW(wpath, &data);
	if(handle != INVALID_HANDLE_VALUE){
		//the file exists, so gather information with WIN32_FIND_DATAW
		defer{ ::FindClose(handle); };
		
		result = (File*)memory_alloc(sizeof(File));
		ULARGE_INTEGER size, time;
		time.LowPart  = data.ftCreationTime.dwLowDateTime;
		time.HighPart = data.ftCreationTime.dwHighDateTime;
		result->creation_time    = WindowsTimeToUnixTime(time.QuadPart);
		time.LowPart  = data.ftLastAccessTime.dwLowDateTime;
		time.HighPart = data.ftLastAccessTime.dwHighDateTime;
		result->last_access_time = WindowsTimeToUnixTime(time.QuadPart);
		time.LowPart  = data.ftLastWriteTime.dwLowDateTime;
		time.HighPart = data.ftLastWriteTime.dwHighDateTime;
		result->last_write_time  = WindowsTimeToUnixTime(time.QuadPart);
		size.LowPart  = data.nFileSizeLow;
		size.HighPart = data.nFileSizeHigh;
		result->bytes            = (HasFlag(flags, FileAccess_Truncate)) ? 0 : size.QuadPart;
		result->is_directory     = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
		
		if(result->is_directory) wpath[wpath_len++] = L'\\';
		result->path = str8_from_wchar(wpath+4, deshi_allocator); //NOTE(delle) +4 b/c of "\\?\"
		forI(result->path.count) if(result->path.str[i] == '\\') result->path.str[i] = '/';
		if(result->path){
			result->name = str8_skip_until_last(result->path, '/');
			if(result->name){
				str8_increment(&result->name, 1);
				result->front = str8_eat_until_last(result->name, '.');
				if(result->front){
					result->ext = str8{result->front.str+result->front.count+1, result->name.count-(result->front.count+1)};
				}
			}
		}
	}else{
		if(HasFlag(flags, FileAccess_Create)){
			//the file doesn't exist, so create it then gather information with LPBY_HANDLE_FILE_INFORMATION
			handle = ::CreateFileW(wpath, 0,0,0, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0);
			if(handle == INVALID_HANDLE_VALUE){
				win32_log_last_error("CreateFileW", file_crash_on_error, path);
				return 0;
			}
			defer{ ::CloseHandle(handle); };
			
			BY_HANDLE_FILE_INFORMATION info;
			if(::GetFileInformationByHandle(handle, &info) == 0){
				win32_log_last_error("GetFileInformationByHandle", file_crash_on_error, path);
				return 0;
			}
			
			result = (File*)memory_alloc(sizeof(File));
			ULARGE_INTEGER size, time;
			time.LowPart  = info.ftCreationTime.dwLowDateTime;
			time.HighPart = info.ftCreationTime.dwHighDateTime;
			result->creation_time    = WindowsTimeToUnixTime(time.QuadPart);
			time.LowPart  = info.ftLastAccessTime.dwLowDateTime;
			time.HighPart = info.ftLastAccessTime.dwHighDateTime;
			result->last_access_time = WindowsTimeToUnixTime(time.QuadPart);
			time.LowPart  = info.ftLastWriteTime.dwLowDateTime;
			time.HighPart = info.ftLastWriteTime.dwHighDateTime;
			result->last_write_time  = WindowsTimeToUnixTime(time.QuadPart);
			size.LowPart  = info.nFileSizeLow;
			size.HighPart = info.nFileSizeHigh;
			result->bytes            = size.QuadPart;
			result->is_directory     = (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
			
			if(result->is_directory) wpath[wpath_len++] = L'\\';
			result->path = str8_from_wchar(wpath+4, deshi_allocator); //NOTE(delle) +4 b/c of "\\?\"
			forI(result->path.count) if(result->path.str[i] == '\\') result->path.str[i] = '/';
			if(result->path){
				result->name = str8_skip_until_last(result->path, '/');
				if(result->name){
					str8_increment(&result->name, 1);
					result->front = str8_eat_until_last(result->name, '.');
					if(result->front){
						result->ext = str8{result->front.str+result->front.count+1, result->name.count-(result->front.count+1)};
					}
				}
			}
		}else{
			//the file doesn't exist, and we don't have a creation flag so error out
			if(!ignore_nonexistence) win32_log_last_error("FindFirstFileW", file_crash_on_error, path);
			return 0;
		}
	}
	
	//determine fopen() mode
	Flags open_flags = flags & ~(FileAccess_Append|FileAccess_Create);
	wchar_t* open_mode;
	switch(open_flags){
		case FileAccess_Read:              open_mode = (wchar_t*)L"rb";  break;
		case FileAccess_WriteTruncate:     open_mode = (wchar_t*)L"wb";  break;
		case FileAccess_Write:
		case FileAccess_ReadWrite:         open_mode = (wchar_t*)L"rb+"; break;
		case FileAccess_ReadWriteTruncate: open_mode = (wchar_t*)L"wb+"; break;
		default:{ open_mode = 0; }break;
	}
	
	if(open_mode){
		//open the file
		result->handle = _wfopen(wpath+4, open_mode); //NOTE(delle) +4 b/c of "\\?\"
		if(result->handle == 0){
			LogE("file", "fopen() failed to open file '",result->path,"' with mode '",str8_from_wchar(open_mode,deshi_temp_allocator),"'");
			win32_log_last_error("fopen",file_crash_on_error);
			memory_zfree(result->path.str);
			memory_zfree(result);
			return 0;
		}
		
		if(HasFlag(flags, FileAccess_Append)){
			fseek(result->handle, result->bytes, SEEK_SET);
			result->cursor = result->bytes;
		}
	}
	
	result->access = flags & ~(FileAccess_Append|FileAccess_Create|FileAccess_Truncate);
	file_files.add(result);
	return result;
}

void
deshi__file_deinit(str8 caller_file, upt caller_line, File* file){DPZoneScoped;
	if(file == 0){
		LogE("file","file_deinit() was passed a null `file` pointer at ",caller_file,"(",caller_line,")");
		return;
	}
	
	fclose(file->handle);
	
	forI(file_files.count){
		if(file_files[i] == file){
			file_files.remove_unordered(i);
			break;
		}
	}
	
	memory_zfree(file->path.str);
	memory_zfree(file);
}

void
deshi__file_change_access(str8 caller_file, upt caller_line, File* file, FileAccess flags){DPZoneScoped;
	if(file == 0){
		LogE("file","file_change_access() was passed a null `file` pointer at ",caller_file,"(",caller_line,")");
		return;
	}
	
	RemoveFlag(flags, FileAccess_Create); //the file already exists
	wchar_t* wpath = win32_path_from_str8(file->path, false);
	
	//open the file if access requests it and it's not already open
	if(file->handle == 0 && HasFlag(flags, FileAccess_ReadWrite)){
		Flags open_flags = flags & ~(FileAccess_Append);
		wchar_t* open_mode;
		switch(open_flags){
			case FileAccess_Read:              open_mode = (wchar_t*)L"rb";  break;
			case FileAccess_WriteTruncate:     open_mode = (wchar_t*)L"wb";  file->bytes = 0; break;
			case FileAccess_Write:
			case FileAccess_ReadWrite:         open_mode = (wchar_t*)L"rb+"; break;
			case FileAccess_ReadWriteTruncate: open_mode = (wchar_t*)L"wb+"; file->bytes = 0; break;
		}
		
		file->handle = _wfopen(wpath, open_mode);
		if(file->handle == 0){
			LogE("file", "fopen() failed to open file '",file->path,"' with mode '",str8_from_wchar(open_mode,deshi_temp_allocator),"'");
			win32_log_last_error("fopen",file_crash_on_error);
			return;
		}
	}else if(file->handle != 0 && !HasFlag(flags, FileAccess_ReadWrite)){
		fclose(file->handle);
		file->handle = 0;
	}
	
	if(file->handle != 0 && HasFlag(flags, FileAccess_Append)){
		fseek(file->handle, file->bytes, SEEK_SET);
		file->cursor = file->bytes;
	}
	
	file->access = flags & ~(FileAccess_Append|FileAccess_Truncate);
}

carray<File*>
file_initted_files(){DPZoneScoped;
	return carray<File*>{file_files.data, file_files.count};
}

str8
deshi__file_read_simple(str8 caller_file, upt caller_line, str8 path, Allocator* allocator){DPZoneScoped;
	if(!path || *path.str == 0){
		LogE("file","file_read_simple() was passed an empty `path` at ",caller_file,"(",caller_line,")");
		return str8{};
	}
	if(allocator == 0){
		LogE("file","file_read_simple() was passed a null `allocator` pointer at ",caller_file,"(",caller_line,")");
		return str8{};
	}
	
	wchar_t* wpath = win32_path_from_str8(path, true);
	HANDLE handle = ::CreateFileW(wpath, GENERIC_READ, 0,0, OPEN_EXISTING, 0,0);
	if(handle == INVALID_HANDLE_VALUE){
		win32_log_last_error("CreateFileW", file_crash_on_error, path);
		return str8{};
	}
	defer{ ::CloseHandle(handle); };
	
	LARGE_INTEGER size;
	if(::GetFileSizeEx(handle, &size) == 0){
		win32_log_last_error("GetFileSizeEx", file_crash_on_error, path);
		return str8{};
	}
	
	str8 result{(u8*)allocator->reserve(size.QuadPart+1), size.QuadPart};
	if(::ReadFile(handle, result.str, size.QuadPart, 0,0) == 0){
		win32_log_last_error("ReadFile", file_crash_on_error, path);
		return str8{};
	}
	
	return result;
}

u64
deshi__file_write_simple(str8 caller_file, upt caller_line, str8 path, void* data, u64 bytes){DPZoneScoped;
	if(!path || *path.str == 0){
		LogE("file","file_write_simple() was passed an empty `path` at ",caller_file,"(",caller_line,")");
		return 0;
	}
	if(data == 0 && bytes > 0){
		LogE("file","file_write_simple() was passed a null `data` pointer at ",caller_file,"(",caller_line,")");
		return 0;
	}
	
	wchar_t* wpath = win32_path_from_str8(path, true);
	HANDLE handle = ::CreateFileW(wpath, GENERIC_WRITE, 0,0, CREATE_ALWAYS, 0,0);
	if(handle == INVALID_HANDLE_VALUE){
		win32_log_last_error("CreateFileW", file_crash_on_error, path);
		return 0;
	}
	defer{ ::CloseHandle(handle); };
	
	DWORD bytes_written = 0;
	if(::WriteFile(handle, data, bytes, &bytes_written, 0) == 0){
		win32_log_last_error("WriteFile", file_crash_on_error, path);
		return (u64)bytes_written;
	}
	return (u64)bytes_written;
}

u64
deshi__file_append_simple(str8 caller_file, upt caller_line, str8 path, void* data, u64 bytes){DPZoneScoped;
	if(!path || *path.str == 0){
		LogE("file","file_append_simple() was passed an empty `path` at ",caller_file,"(",caller_line,")");
		return 0;
	}
	if(data == 0 && bytes > 0){
		LogE("file","file_append_simple() was passed a null `data` pointer at ",caller_file,"(",caller_line,")");
		return 0;
	}
	
	wchar_t* wpath = win32_path_from_str8(path, true);
	HANDLE handle = ::CreateFileW(wpath, GENERIC_WRITE, 0,0, OPEN_ALWAYS, 0,0);
	if(handle == INVALID_HANDLE_VALUE){
		win32_log_last_error("CreateFileW", file_crash_on_error, path);
		return 0;
	}
	defer{ ::CloseHandle(handle); };
	
	if(::SetFilePointerEx(handle, LARGE_INTEGER{0}, 0, SEEK_END) == 0){
		win32_log_last_error("SetFilePointerEx", file_crash_on_error, path);
		return 0;
	}
	
	DWORD bytes_written = 0;
	if(::WriteFile(handle, data, bytes, &bytes_written, 0) == 0){
		win32_log_last_error("WriteFile", file_crash_on_error, path);
		return (u64)bytes_written;
	}
	return (u64)bytes_written;
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @win32_modules
void*
platform_load_module(str8 module_path){DPZoneScoped;
	wchar_t* wmodule_path = wchar_from_str8(module_path, 0, deshi_temp_allocator);
	return ::LoadLibraryW(wmodule_path);
}

void
platform_free_module(void* module){DPZoneScoped;
	::FreeLibrary((HMODULE)module);
}

platform_symbol
platform_get_module_symbol(void* module, const char* symbol_name){DPZoneScoped;
	return (platform_symbol)::GetProcAddress((HMODULE)module, symbol_name);
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @win32_clipboard
//!ref: Dear ImGui imgui.cpp@GetClipboardTextFn_DefaultImpl
str8
platform_get_clipboard(){DPZoneScoped;
	str8 result{};
	
	if(::OpenClipboard(NULL) == NULL){
		win32_log_last_error("OpenClipboard");
		return result;
	}
	
	HANDLE wchar_buffer_handle = ::GetClipboardData(CF_UNICODETEXT);
	if(wchar_buffer_handle == NULL){
		win32_log_last_error("GetClipboardData");
		::CloseClipboard();
		return result;
	}
	
	if(const WCHAR* wchar_buffer = (const WCHAR*)::GlobalLock(wchar_buffer_handle)){
		result.count = (s64)::WideCharToMultiByte(CP_UTF8, 0, wchar_buffer, -1, NULL, 0, NULL, NULL);
		result.str   = (u8*)memory_talloc(result.count);
		::WideCharToMultiByte(CP_UTF8, 0, wchar_buffer, -1, (LPSTR)result.str, (int)result.count, NULL, NULL);
	}else{
		win32_log_last_error("GlobalLock");
	}
	::GlobalUnlock(wchar_buffer_handle);
    ::CloseClipboard();
	return result;
}

//!ref: Dear ImGui imgui.cpp@SetClipboardTextFn_DefaultImpl
void
platform_set_clipboard(str8 text){DPZoneScoped;
	if(::OpenClipboard(NULL) == NULL){
		win32_log_last_error("OpenClipboard");
		return;
	}
	
	int wchar_buffer_length = ::MultiByteToWideChar(CP_UTF8, 0, (const char*)text.str, text.count*sizeof(u8), NULL, 0);
	HGLOBAL wchar_buffer_handle = ::GlobalAlloc(GMEM_MOVEABLE, (SIZE_T)wchar_buffer_length*sizeof(WCHAR));
	if(wchar_buffer_handle == NULL){
		win32_log_last_error("GlobalAlloc");
		::CloseClipboard();
		return;
	}
	
	WCHAR* wchar_buffer = (WCHAR*)::GlobalLock(wchar_buffer_handle);
    ::MultiByteToWideChar(CP_UTF8, 0, (const char*)text.str, text.count*sizeof(u8), wchar_buffer, wchar_buffer_length);
    ::GlobalUnlock(wchar_buffer_handle);
    ::EmptyClipboard();
    if(::SetClipboardData(CF_UNICODETEXT, wchar_buffer_handle) == NULL){
		win32_log_last_error("SetClipboardData");
        ::GlobalFree(wchar_buffer_handle);
	}
    ::CloseClipboard();
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @win32_threading
mutex::
mutex(){DPZoneScoped;
	if(handle = ::CreateMutex(NULL, FALSE, NULL); !handle)
		win32_log_last_error("CreateMutex");
}

mutex::
~mutex(){DPZoneScoped;//NOTE a mutex is not released on scope end, use scopedlock for this
	::CloseHandle(handle);
}

void mutex::
lock(){DPZoneScoped;
	DWORD waitResult = ::WaitForSingleObject(handle, INFINITE);
	switch(waitResult){
		case WAIT_ABANDONED:{
			//TODO maybe have an option somewhere to suppress this error
			LogE("threading-win32", "Attempted to lock an abandoned mutex. This mutex was not released by the thread that owned it before the thread terminated.");
		}break;
		//TODO set up our own error reporting once i figure out what error codes are what for this
		case WAIT_FAILED:{ win32_log_last_error("CreateMutex"); Assert(0); }break;
	}
	is_locked = 1;
}

b32 mutex::
try_lock(){DPZoneScoped;
	DWORD waitResult = ::WaitForSingleObject(handle, 0);
	switch(waitResult){
		case WAIT_ABANDONED:{
			//TODO maybe have an option somewhere to suppress this error
			LogE("threading-win32", "Locking an abandoned mutex. This mutex was not released by the thread that owned it before the thread terminated.");
		}break;
		case WAIT_TIMEOUT:{
			return false;
		}break;
		//TODO set up our own error reporting once i figure out what error codes are what for this
		case WAIT_FAILED:{ win32_log_last_error("CreateMutex"); Assert(0); }break;
	}
	is_locked = 1;
	return true;
}

b32 mutex::
try_lock_for(u64 milliseconds){DPZoneScoped;
	DWORD waitResult = ::WaitForSingleObject(handle, milliseconds);
	switch(waitResult){
		case WAIT_ABANDONED:{
			//TODO maybe have an option somewhere to suppress this error
			LogE("threading-win32", "Locking an abandoned mutex. This mutex was not released by the thread that owned it before the thread terminated.");
		}break;
		case WAIT_TIMEOUT:{
			return false;
		}break;
		//TODO set up our own error reporting once i figure out what error codes are what for this
		case WAIT_FAILED:{ win32_log_last_error("CreateMutex"); Assert(0); }break;
	}
	is_locked = 1;
	return true;
}

void mutex::
unlock(){DPZoneScoped;
	if(!ReleaseMutex(handle)){ win32_log_last_error("ReleaseMutex"); Assert(0); }
}


condition_variable::
condition_variable(){DPZoneScoped;
	//NOTE i have to use std mem here in the case that condvar is used before memory is initialized (eg. a condvar global var)
	cvhandle = malloc(sizeof(CONDITION_VARIABLE));
	cshandle = malloc(sizeof(CRITICAL_SECTION));
	::InitializeCriticalSection((CRITICAL_SECTION*)cshandle);
	::InitializeConditionVariable((CONDITION_VARIABLE*)cvhandle);
}

condition_variable::
~condition_variable(){DPZoneScoped;
	free(cvhandle); free(cshandle);
}

void condition_variable::
notify_one(){DPZoneScoped;
	::WakeConditionVariable((CONDITION_VARIABLE*)cvhandle);
}

void condition_variable::
notify_all(){DPZoneScoped;
	::WakeAllConditionVariable((CONDITION_VARIABLE*)cvhandle);
}

void condition_variable::
wait(){DPZoneScoped;
	::EnterCriticalSection((CRITICAL_SECTION*)cshandle);
	if(!::SleepConditionVariableCS((CONDITION_VARIABLE*)cvhandle, (CRITICAL_SECTION*)cshandle, INFINITE)){
		win32_log_last_error("SleepConditionVariableCS");
	}
	::LeaveCriticalSection((CRITICAL_SECTION*)cshandle);
}

void condition_variable::
wait_for(u64 milliseconds){DPZoneScoped;
	::EnterCriticalSection((CRITICAL_SECTION*)cshandle);
	::SleepConditionVariableCS((CONDITION_VARIABLE*)cvhandle, (CRITICAL_SECTION*)cshandle, milliseconds);
	::LeaveCriticalSection((CRITICAL_SECTION*)cshandle);	
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
void
deshi__thread_worker(Thread* me){DPZoneScoped;
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

DWORD WINAPI
deshi__thread_worker__win32_stub(LPVOID in){DPZoneScoped;
	deshi__thread_worker((Thread*)in);
	return 0;
}

void ThreadManager::
init(u32 max_jobs){
	AssertDS(DS_MEMORY, "Attempt to init ThreadManager without loading Memory first");
	job_ring.init(max_jobs, deshi_allocator);
}

void ThreadManager::
spawn_thread(){DPZoneScoped;
	Thread* t = (Thread*)memalloc(sizeof(Thread));
	threads.add(t);
	::CreateThread(0, 0, deshi__thread_worker__win32_stub, (void*)t, 0, 0);
}

void ThreadManager::
close_all_threads(){
	forI(threads.count) threads[i]->close = true;
	wake_threads(0);
	threads.clear();
}

void ThreadManager::
add_job(ThreadJob job){
	job_ring.add(job);
}

void ThreadManager::
add_jobs(carray<ThreadJob> jobs){
	forI(jobs.count) job_ring.add(jobs[i]);
}

void ThreadManager::
cancel_all_jobs(){
	job_ring.clear();
} 

void ThreadManager::
wake_threads(u32 count){
	if(!threads.count){ LogW("Thread", "Attempt to use wake_threads without spawning any threads first"); }
	else if(!count) idle.notify_all(); 
	else{
		forI(count) idle.notify_one();
	}
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @win32_window
Window*
window_create(str8 title, s32 width, s32 height, s32 x, s32 y, DisplayMode display_mode, Decoration decorations){DPZoneScoped;
	AssertDS(DS_PLATFORM, "Called window_create() before initializing Platform module");
	
	//create the window
	Window* window = (Window*)memory_alloc(sizeof(Window));
	window->handle = ::CreateWindowExW(0, DESHI_RENDER_WNDCLASSNAME, wchar_from_str8(title, 0, deshi_temp_allocator),
									 WS_CLIPCHILDREN|WS_CLIPSIBLINGS, 0,0, 0,0, 0, 0, win32_console_instance, 0);
	if(!window->handle){ win32_log_last_error("CreateWindowExW", true); memory_zfree(window); return 0; }
	//set Win32 window user data to the deshi Window pointer
	::SetWindowLongPtrW((HWND)window->handle, GWLP_USERDATA, (LONG_PTR)window);
	window->dc = ::GetDC((HWND)window->handle);
	
	//set window decorations
	window->display_mode = -1;
	window_display_mode(window, display_mode);
	//window_decorations(decorations);
	if(decorations != Decoration_SystemDecorations){
		//TODO(delle) custom titlebar
	}
	
	//set window title
	window->title = str8_copy(title, deshi_allocator);
	
	//set window position and size
	int work_x = 0, work_y = 0, work_w = 0, work_h = 0;
	win32_monitor_info((HWND)window->handle, 0,0, &work_x,&work_y, &work_w,&work_h);
	if(width  == 0xFFFFFFFF) width  = work_w / 2;
	if(height == 0xFFFFFFFF) height = work_h / 2;
	if(x == 0xFFFFFFFF) x = work_x + (width  / 2);
	if(y == 0xFFFFFFFF) y = work_y + (height / 2);
	::SetWindowPos((HWND)window->handle, 0, x,y, width,height, 0);
	
	RECT rect;
	::GetWindowRect((HWND)window->handle, &rect);
	window->position_decorated.x = rect.left;
	window->position_decorated.y = rect.top;
	window->dimensions_decorated.x = rect.right  - rect.left;
	window->dimensions_decorated.y = rect.bottom - rect.top;
	
	::GetClientRect((HWND)window->handle, &rect);
	::ClientToScreen((HWND)window->handle, (POINT*)&rect.left);
	::ClientToScreen((HWND)window->handle, (POINT*)&rect.right);
	window->position.x = rect.left;
	window->position.y = rect.top;
	window->dimensions.x = rect.right  - rect.left;
	window->dimensions.y = rect.bottom - rect.top;
	
	window->center.x = window->dimensions.x / 2;
	window->center.y = window->dimensions.y / 2;
	
	//if this is the first window created, it's global
	if(window_windows.count == 0){
		DeshWindow = window;
	}
	
	window->index = window_windows.count;
	window_active = window;
	window_windows.add(window);
	return window;
}

void
window_close(Window* window){DPZoneScoped;
	PostMessage((HWND)window->handle, WM_CLOSE, 0, 0);
}

void
window_swap_buffers(Window* window){DPZoneScoped;
	::SwapBuffers((HDC)window->dc);
}

void
window_display_mode(Window* window, DisplayMode mode){DPZoneScoped;
	if(window->display_mode == mode) return;
	
	//if previous mode was Fullscreen, restore the screen settings
	if(window->display_mode == DisplayMode_Fullscreen){
		::ChangeDisplaySettingsW(0, CDS_FULLSCREEN);
	}
	
	HWND hwnd = (HWND)window->handle;
	switch(mode){
		case DisplayMode_Windowed:{
			window->resized = true;
			::SetWindowLongPtrW(hwnd, GWL_STYLE, (LONG_PTR)::GetWindowLongPtrW(hwnd,GWL_STYLE) | WS_OVERLAPPEDWINDOW);
			if(window->display_mode == DisplayMode_Borderless){
				window->restore.position   = window->position_decorated;
				window->restore.dimensions = window->dimensions_decorated;
			}
			::SetWindowPos(hwnd, HWND_TOP, window->restore.x,window->restore.y, window->restore.width,window->restore.height, SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_FRAMECHANGED);
		}break;
		case DisplayMode_Borderless:{
			window->resized = true;
			::SetWindowLongPtrW(hwnd, GWL_STYLE, (LONG_PTR)::GetWindowLongPtrW(hwnd,GWL_STYLE) & (~WS_OVERLAPPEDWINDOW));
			if(window->display_mode == DisplayMode_Windowed){
				RECT rect;
				::GetClientRect(hwnd, &rect);
				::ClientToScreen(hwnd, (POINT*)&rect.left);
				::ClientToScreen(hwnd, (POINT*)&rect.right);
				window->restore.x = rect.left;
				window->restore.y = rect.top;
				window->restore.width  = rect.right  - rect.left;
				window->restore.height = rect.bottom - rect.top;
			}
			::SetWindowPos(hwnd, HWND_TOP, window->restore.x,window->restore.y, window->restore.width,window->restore.height, SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_FRAMECHANGED);
		}break;
		case DisplayMode_BorderlessMaximized:{
			MONITORINFO mi = {sizeof(MONITORINFO)};
			if(::GetMonitorInfoW(::MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &mi)){
				window->resized = true;
				::SetWindowLongPtrW(hwnd, GWL_STYLE, (LONG_PTR)::GetWindowLongPtrW(hwnd,GWL_STYLE) & (~WS_OVERLAPPEDWINDOW));
				window->restore.position   = window->position_decorated;
				window->restore.dimensions = window->dimensions_decorated;
				window->x       = mi.rcMonitor.left;
				window->y       = mi.rcMonitor.top;
				window->width   = mi.rcMonitor.right  - mi.rcMonitor.left;
				window->height  = mi.rcMonitor.bottom - mi.rcMonitor.top;
				::SetWindowPos(hwnd, HWND_TOP, window->x,window->y, window->width,window->height, SWP_NOOWNERZORDER|SWP_FRAMECHANGED);
			}
		}break;
		case DisplayMode_Fullscreen:{
			//get current screen settings
			DEVMODEW settings;
			::EnumDisplaySettingsW(0, ENUM_CURRENT_SETTINGS, &settings);
			
			//set desired screen resolution
			//TODO(delle) screen resolution and refresh rate
			
			//apply changed screen settings
			MONITORINFO mi = {sizeof(MONITORINFO)};
			if(   ::ChangeDisplaySettingsW(&settings, CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL
			   && ::GetMonitorInfoW(::MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &mi)){
				window->resized = true;
				::SetWindowLongPtrW(hwnd, GWL_STYLE, (LONG_PTR)::GetWindowLongPtrW(hwnd,GWL_STYLE) & (~WS_OVERLAPPEDWINDOW));
				window->restore.position   = window->position_decorated;
				window->restore.dimensions = window->dimensions_decorated;
				window->x       = mi.rcMonitor.left;
				window->y       = mi.rcMonitor.top;
				window->width   = mi.rcMonitor.right  - mi.rcMonitor.left;
				window->height  = mi.rcMonitor.bottom - mi.rcMonitor.top;
				::SetWindowPos(hwnd, HWND_TOPMOST, window->x,window->y, window->width,window->height, SWP_NOOWNERZORDER|SWP_FRAMECHANGED);
			}
		}break;
	}
	window->display_mode = mode;
}

//void window_decorations(Decoration decorations){DPZoneScoped;}

void
window_show(Window* window){DPZoneScoped;
	::ShowWindow((HWND)window->handle, SW_SHOW);
}

void
window_hide(Window* window){DPZoneScoped;
	::ShowWindow((HWND)window->handle, SW_HIDE);
}

void
window_title(Window* window, str8 title){DPZoneScoped;
	::SetWindowTextW((HWND)window->handle, wchar_from_str8(title, 0, deshi_temp_allocator));
	memory_zfree(window->title.str);
	window->title = str8_copy(title, deshi_allocator);
}

void
window_cursor_mode(Window* window, CursorMode mode){DPZoneScoped;
	if(window->cursor_mode == mode) return;
	switch(mode){
		case CursorMode_Default: default:{
			::SetCursor(::LoadCursor(0, IDC_ARROW));
			::ClipCursor(0);
		}break;
		case CursorMode_FirstPerson:{
			//set cursor to center of screen
			POINT p{window->center.x, window->center.y};
			::ClientToScreen((HWND)window->handle, &p);
			::SetCursorPos(p.x, p.y);
			
			//hide cursor
			::SetCursor(0);
			
			//restrict cursor to client
			RECT rect;
			::GetClientRect((HWND)window->handle, &rect);
			::ClientToScreen((HWND)window->handle, (POINT*)&rect.left);
			::ClientToScreen((HWND)window->handle, (POINT*)&rect.right);
			::ClipCursor(&rect);
		}break;
	}
	window->cursor_mode = mode;
}

void
window_cursor_type(Window* window, CursorType type){DPZoneScoped;
	switch(type){
		case CursorType_Arrow:{
			::SetCursor(::LoadCursor(0, IDC_ARROW));
		}break;
		case CursorType_HResize:{
			::SetCursor(::LoadCursor(0, IDC_SIZEWE));
		}break;
		case CursorType_VResize:{
			::SetCursor(::LoadCursor(0, IDC_SIZENS));
		}break;
		case CursorType_RightDiagResize:{
			::SetCursor(::LoadCursor(0, IDC_SIZENESW));
		}break;
		case CursorType_LeftDiagResize:{
			::SetCursor(::LoadCursor(0, IDC_SIZENWSE));
		}break;
		case CursorType_Hand:{
			::SetCursor(::LoadCursor(0, IDC_HAND));
		}break;
		case CursorType_IBeam:{
			::SetCursor(::LoadCursor(0, IDC_IBEAM));
		}break;
		case CursorType_Hidden:{
			::SetCursor(0);
		}break;
		default:{
			LogE("window","Unknown cursor type: ", type);
			::SetCursor(::LoadCursor(0, IDC_ARROW));
		}break;
	}
}

void
window_cursor_position(Window* window, s32 x, s32 y){DPZoneScoped;
	POINT p{x, y};
	::ClientToScreen((HWND)window->handle, &p);
	::SetCursorPos(p.x, p.y);
	DeshInput->mouseX = x;
	DeshInput->mouseY = y;
	DeshInput->screenMouseX = p.x;
	DeshInput->screenMouseY = p.y;
}