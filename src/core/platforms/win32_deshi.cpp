/* deshi Win32 Platform Backend
Index:
@vars
@helpers
@callback
@platform
@stopwatch
@file
@modules
@clipboard
@threading
@window
@networking
*/


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @vars
local s64 win32_perf_count_frequency;
local wchar_t* win32_file_data_folder;
local s32 win32_file_data_folder_len;
local HINSTANCE win32_console_instance;
local b32 win32_delayed_activate;


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @helpers
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
		win32_log_last_error("GetFullPathNameW", file_shared.crash_on_error, path);
		if(file_shared.crash_on_error){
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

//center, hide, and capture cursor
//!ref: https://github.com/glfw/glfw/blob/master/src/win32_window.c@disableCursor()
local void
win32_disable_cursor(Window* window){
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
}

//show cursor and remove restriction
//!ref: https://github.com/glfw/glfw/blob/master/src/win32_window.c@enableCursor()
local void
win32_enable_cursor(Window* window){
	//show cursor
	::SetCursor(::LoadCursorW(0, IDC_ARROW));
	
	//unrestrict cursor
	::ClipCursor(0);
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @callback
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
				dstr8 s; dstr8_init(&s, KeyCodeStrings[key], deshi_temp_allocator);
				dstr8_append(&s, (upFlag) ? str8l(" released") : str8l(" pressed"));
				if(DeshInput->realKeyState[Key_LSHIFT]) dstr8_append(&s, str8l(" + LSHIFT"));
				if(DeshInput->realKeyState[Key_RSHIFT]) dstr8_append(&s, str8l(" + RSHIFT"));
				if(DeshInput->realKeyState[Key_LCTRL])  dstr8_append(&s, str8l(" + LCTRL"));
				if(DeshInput->realKeyState[Key_RCTRL])  dstr8_append(&s, str8l(" + RCTRL"));
				if(DeshInput->realKeyState[Key_LALT])   dstr8_append(&s, str8l(" + LALT"));
				if(DeshInput->realKeyState[Key_RALT])   dstr8_append(&s, str8l(" + RALT"));
				Log("input", dstr8_peek(&s)); 
#endif
			}
		}break;
		
		case WM_CHAR:{ ////////////////////////////////////////////////////////////// Char From Key (UTF-16)
#ifndef DESHI_ALLOW_CHARINPUT_CONTROLCHARS
			if(iswcntrl(LOWORD(wParam))) break; //NOTE skip control characters
#endif
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
		}break;
		
		case WM_INPUT:{ ///////////////////////////////////////////////////////////// Raw Input
			//TODO(delle) raw input
		}break;
		
		case WM_ACTIVATE:{ ////////////////////////////////////////////////////////// Activated/Deactivated
			if(window){
				if(wParam == WA_ACTIVE || wParam == WA_CLICKACTIVE){
					window->focused = true;
					
					if(window->cursor_mode == CursorMode_FirstPerson){
						win32_disable_cursor(window);
					}
				}else{
					window->focused = false;
					
					//::SetCursor(::LoadCursorW(0, IDC_ARROW));
					//::ClipCursor(0);
				}
			}else{ Assert(!"WM_ACTIVATE passed to win32 HWND that does not have matching deshi Window"); }
		}break;
		
		case WM_MOUSEACTIVATE:{ ///////////////////////////////////////////////////// Activated Thru Mouse
			if(window){
				//postpone cursor disabling when the window is activated by clicking decorations
				//!ref: https://github.com/glfw/glfw/blob/master/src/win32_window.c
				if(HIWORD(lParam) == WM_LBUTTONDOWN && LOWORD(lParam) != HTCLIENT){
					win32_delayed_activate = true;
				}
			}else{ Assert(!"WM_MOUSEACTIVATE passed to win32 HWND that does not have matching deshi Window"); }
		}break;
		
		case WM_CAPTURECHANGED:{ //////////////////////////////////////////////////// Lost Mouse Capture
			if(window){
				//now we can disable the cursor
				//!ref: https://github.com/glfw/glfw/blob/master/src/win32_window.c
				if(lParam == 0 && win32_delayed_activate){
					if(window->cursor_mode == CursorMode_FirstPerson){
						win32_disable_cursor(window);
					}
					
					win32_delayed_activate = false;
				}
			}else{ Assert(!"WM_MOUSEACTIVATE passed to win32 HWND that does not have matching deshi Window"); }
		}break;
		
		case WM_SETFOCUS:{ ////////////////////////////////////////////////////////// Gained Keyboard Focus
			if(window){
				//postpone cursor disabling when the window is clicking down on decorations
				//!ref: https://github.com/glfw/glfw/blob/master/src/win32_window.c
				window->focused = true;
				
				if(win32_delayed_activate){
					break;
				}
				
				if(window->cursor_mode == CursorMode_FirstPerson){
					win32_disable_cursor(window);
				}
				
				return 0;
			}else{ Assert(!"WM_SETFOCUS passed to win32 HWND that does not have matching deshi Window"); }
		}break;
		
		case WM_KILLFOCUS:{ ////////////////////////////////////////////////////////// Lost Keyboard Focus
			if(window){
				//!ref: https://github.com/glfw/glfw/blob/master/src/win32_window.c
				if(window->cursor_mode == CursorMode_FirstPerson){
					win32_enable_cursor(window);
				}
				
				window->focused = false;
				return 0;
			}else{ Assert(!"WM_SETFOCUS passed to win32 HWND that does not have matching deshi Window"); }
		}break;
		
		case WM_ENTERSIZEMOVE:  ///////////////////////////////////////////////////// Start Moving/Resizing
		case WM_ENTERMENULOOP:{ ///////////////////////////////////////////////////// Popup Modal Tracked
			//!ref: https://github.com/glfw/glfw/blob/master/src/win32_window.c
			if(win32_delayed_activate){
				break;
			}
			
			if(window->cursor_mode == CursorMode_FirstPerson){
				win32_disable_cursor(window);
			}
		}break;
		
		case WM_EXITSIZEMOVE:  ////////////////////////////////////////////////////// Stop Moving/Resizing
		case WM_EXITMENULOOP:{ ////////////////////////////////////////////////////// Popup Modal Untracked
			//!ref: https://github.com/glfw/glfw/blob/master/src/win32_window.c
			if(win32_delayed_activate){
				break;
			}
			
			if(window->cursor_mode == CursorMode_FirstPerson){
				win32_disable_cursor(window);
			}
		}break;
	}
	
	if(window && ::GetForegroundWindow() == (HWND)window->handle){
		window_active = window;
	}
	return ::DefWindowProcW(hwnd, msg, wParam, lParam);
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @platform
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
	
	//increase resolution of Sleep() to minimum value
	TIMECAPS tc;
	timeGetDevCaps(&tc, sizeof(tc));
	timeBeginPeriod(tc.wPeriodMin);
	
	//// init file ////
	wchar_t* wpath = win32_path_from_str8(str8_lit("data/"), false, 0, &win32_file_data_folder_len);
	win32_file_data_folder = (wchar_t*)memory_alloc((win32_file_data_folder_len+1)*sizeof(wchar_t));
	CopyMemory(win32_file_data_folder, wpath, win32_file_data_folder_len*sizeof(wchar_t));
	
	file_shared.files = array_create(File, 32, deshi_allocator);
	
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
	render_window_class.hCursor       = 0;
	render_window_class.lpszClassName = DESHI_RENDER_WNDCLASSNAME;
	if(!::RegisterClassExW(&render_window_class)) win32_log_last_error("RegisterClassExW", true);
	
	//create helper window
	window_helper.handle = ::CreateWindowExW(WS_EX_OVERLAPPEDWINDOW|WS_EX_NOACTIVATE, DESHI_HELPER_WNDCLASSNAME,
											 L"_deshi_helper_window_", WS_CLIPSIBLINGS|WS_CLIPCHILDREN,
											 0,0, 1,1, 0, 0, win32_console_instance, 0);
	if(!window_helper.handle) win32_log_last_error("CreateWindowExW", true);
	window_helper.context = GetDC((HWND)window_helper.handle);
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
	
	b32 any_window_focused = false;
	forI(window_windows.count){
		window_windows[i]->resized = false;
		while(::PeekMessageW(&msg, (HWND)window_windows[i]->handle, 0, 0, PM_REMOVE)){
			::TranslateMessage(&msg);
			::DispatchMessageW(&msg);
			if(window_windows.count == 0) return false;
		}
		//if(window_windows[i]->decorations != Decoration_SystemDecorations) DrawDecorations(window_windows[i]);
		//window_windows[i].hit_test = HitTestNone;
		if(window_windows[i]->focused){
			any_window_focused = true;
			if(window_windows[i]->cursor_mode == CursorMode_FirstPerson){
				window_set_cursor_position(window_windows[i], window_windows[i]->center);
			}
		}
	}
	DeshTime->windowTime = reset_stopwatch(&update_stopwatch);
	
	//// update input ////
	if(any_window_focused){
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
	}
	DeshTime->inputTime = peek_stopwatch(update_stopwatch);
	
	
	//// update file ////
	/* //TODO(delle) initted file change tracking thru ReadDirectoryChanges
	File file_file{};
	ULARGE_INTEGER file_size, file_time;
	for_array(file_shared.files){
		
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

Process
platform_get_process_by_name(str8 name){
	Process p{0};
	
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	
	while(Process32Next(snapshot, &entry)){
		str8 pname = str8_from_wchar(entry.szExeFile);
		
		if(str8_equal(pname, name)){ 
			p.handle = OpenProcess(PROCESS_ALL_ACCESS, 0, entry.th32ProcessID);
			break;
		}
	}
	return p;
}

u64 
platform_process_read(Process p, upt address, void* out, upt size){
	if(ReadProcessMemory(p.handle, (LPCVOID)address, out, size, 0)) return 1;
	win32_log_last_error("ReadProcessMemory");
	return 0;
}

u64 
platform_process_write(Process p, upt address, void* data, upt size){
	if(WriteProcessMemory(p.handle, (LPVOID)address, data, size, 0)) return 1;
	win32_log_last_error("WriteProcessMemory");
	return 0;
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @stopwatch
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
//// @file
b32
deshi__file_exists(str8 caller_file, upt caller_line, str8 path, FileResult* error_result){DPZoneScoped;
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

b32
deshi__file_create(str8 caller_file, upt caller_line, str8 path, FileResult* error_result){DPZoneScoped;
	if(!path || *path.str == 0){
		//TODO(sushi) error handling stuff
		LogE("file","file_create() was passed an empty `path` at ",caller_file,"(",caller_line,")");
		return false;
	}
	
	str8 parts{path.str, 0};
	while(path){
		str8 part = str8_eat_until_one_of(path, 2, '/', '\\');
		parts.count += part.count;
		
		wchar_t* wpath = win32_path_from_str8(parts, true);
		if(part.count < path.count){
			Assert(part.str[part.count] == '\\' || part.str[part.count] == '/');
			if((::CreateDirectoryW(wpath, 0) == 0) && (::GetLastError() != ERROR_ALREADY_EXISTS)){
				//TODO(sushi) error handling stuff
				win32_log_last_error("CreateDirectoryW", file_shared.crash_on_error, str8_from_wchar(wpath+4,deshi_temp_allocator)); //NOTE(delle) +4 b/c of "\\?\"
				return false;
			}
			str8_increment(&path, part.count+1);
			parts.count += 1;
		}else{
			HANDLE handle = ::CreateFileW(wpath, GENERIC_READ|GENERIC_WRITE, 0,0, CREATE_NEW, 0,0);
			if((handle == INVALID_HANDLE_VALUE) && (::GetLastError() != ERROR_FILE_EXISTS)){
				//TODO(sushi) error handling stuff
				win32_log_last_error("CreateFileW", file_shared.crash_on_error, str8_from_wchar(wpath+4,deshi_temp_allocator)); //NOTE(delle) +4 b/c of "\\?\"
				return false;
			}
			::CloseHandle(handle);
			break;
		}
	}
	return true;
}

b32
deshi__file_delete(str8 caller_file, upt caller_line, str8 path, u32 flags, FileResult* error_result){DPZoneScoped;
	if(!path || *path.str == 0){
		//TODO(sushi) error handling stuff
		LogE("file","file_delete() was passed an empty `path` at ",caller_file,"(",caller_line,")");
		return false;
	}
	
	s32 wpath_len;
	wchar_t* wpath = win32_path_from_str8(path, true, 0, &wpath_len);
	if(wpath[wpath_len-1] == L'\\'){
		wpath[wpath_len-1] = L'\0';
		wpath_len -= 1;
	}
	
	//TODO(delle) restrict file stuff to root folder not data folder
	/*if(memcmp(wpath+4, win32_file_data_folder, win32_file_data_folder_len*sizeof(wchar_t)) != 0){ //NOTE(delle) +4 b/c of "\\?\"
		//TODO(sushi) error handling stuff
		LogE("file","File deletion can only occur within the data folder. Input path: ",path);
		if(file_shared.crash_on_error){
			Assert(!"assert before exit so we can stack trace in debug mode");
			::ExitProcess(1);
		}
		return false;
	}*/
	
	WIN32_FIND_DATAW data;
	HANDLE handle = ::FindFirstFileW(wpath, &data);
	if(handle == INVALID_HANDLE_VALUE){
		//TODO(sushi) error handling stuff
		win32_log_last_error("FindFirstFileW", file_shared.crash_on_error, path);
		return false;
	}
	defer{ ::FindClose(handle); };
	
	//TODO(delle) check if initted in file_files to update
	
	//if directory, recursively delete all files and directories
	if(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
		if(HasFlag(flags, FileDeleteFlags_File)){
			//TODO(sushi) error handling stuff
			LogE("file","file_delete() called with FileDeleteFlags_File on the directory `",path,"` at ",caller_file,"(",caller_line,")");
			return false;
		}else if(HasFlag(flags, FileDeleteFlags_Directory)){
			File* dir_files = file_search_directory(path);
			if(array_count(dir_files) != 0){
				if(HasFlag(flags, FileDeleteFlags_Recursive)){
					for_array(dir_files){
						file_delete(it->path, FileDeleteFlags_Recursive);
					}
					
					BOOL success = ::RemoveDirectoryW(wpath);
					if(!success){
						//TODO(sushi) error handling stuff
						win32_log_last_error("RemoveDirectoryW", file_shared.crash_on_error, path);
						return false;
					}
				}else{
					//TODO(sushi) error handling stuff
					LogE("file","file_delete() called without FileDeleteFlags_Recursive on the non-empty directory `",path,"` at ",caller_file,"(",caller_line,")");
					return false;
				}
			}else{
				BOOL success = ::RemoveDirectoryW(wpath);
				if(!success){
					//TODO(sushi) error handling stuff
					win32_log_last_error("RemoveDirectoryW", file_shared.crash_on_error, path);
					return false;
				}
			}
		}
	}else{
		if(HasFlag(flags, FileDeleteFlags_Directory)){
			//TODO(sushi) error handling stuff
			LogE("file","file_delete() called with FileDeleteFlags_Directory on the file `",path,"` at ",caller_file,"(",caller_line,")");
			return false;
		}else if(HasFlag(flags, FileDeleteFlags_File)){
			BOOL success = ::DeleteFileW(wpath);
			if(!success){
				//TODO(sushi) error handling stuff
				win32_log_last_error("DeleteFileW", file_shared.crash_on_error, path);
				return false;
			}
		}
	}
	
	return true;
}

b32
deshi__file_rename(str8 caller_file, upt caller_line, str8 old_path, str8 new_path, FileResult* error_result){DPZoneScoped;
	if(!old_path || *old_path.str == 0){
		//TODO(sushi) error handling stuff
		LogE("file","file_rename() was passed an empty `old_path` at ",caller_file,"(",caller_line,")");
		return false;
	}
	if(!new_path || *new_path.str == 0){
		//TODO(sushi) error handling stuff
		LogE("file","file_rename() was passed an empty `new_path` at ",caller_file,"(",caller_line,")");
		return false;
	}
	
	wchar_t* old_wpath = win32_path_from_str8(old_path, true);
	//TODO(delle) restrict file stuff to root folder not data folder
	/*if(memcmp(old_wpath+4, win32_file_data_folder, win32_file_data_folder_len*sizeof(wchar_t)) != 0){ //NOTE(delle) +4 b/c of "\\?\"
		//TODO(sushi) error handling stuff
		LogE("file","File renaming can only occur within the data folder. Input old path: ",old_path);
		if(file_shared.crash_on_error){
			Assert(!"assert before exit so we can stack trace in debug mode");
			::ExitProcess(1);
		}
		return false;
	}*/
	
	wchar_t* new_wpath = win32_path_from_str8(new_path, true);
	//TODO(delle) restrict file stuff to root folder not data folder
	/*if(memcmp(new_wpath+4, win32_file_data_folder, win32_file_data_folder_len*sizeof(wchar_t)) != 0){ //NOTE(delle) +4 b/c of "\\?\"
		//TODO(sushi) error handling stuff
		LogE("file","File renaming can only occur within the data folder. Input new path: ",new_path);
		if(file_shared.crash_on_error){
			Assert(!"assert before exit so we can stack trace in debug mode");
			::ExitProcess(1);
		}
		return false;
	}*/
	
	//TODO(delle) check if initted in file_files to update
	
	BOOL success = ::MoveFileW(old_wpath, new_wpath);
	if(!success){
		//TODO(sushi) error handling stuff
		win32_log_last_error("MoveFileW", file_shared.crash_on_error, str8_concat3(old_path,str8_lit("\n"),new_path, deshi_temp_allocator));
		return false;
	}
	
	return true;
}

b32
deshi__file_copy(str8 caller_file, upt caller_line, str8 src_path, str8 dst_path, FileResult* error_result){DPZoneScoped;
	if(!src_path || *src_path.str == 0){
		//TODO(sushi) error handling stuff
		LogE("file","file_rename() was passed an empty `src_path` at ",caller_file,"(",caller_line,")");
		return false;
	}
	if(!dst_path || *dst_path.str == 0){
		//TODO(sushi) error handling stuff
		LogE("file","file_rename() was passed an empty `dst_path` at ",caller_file,"(",caller_line,")");
		return false;
	}
	
	wchar_t* src_wpath = win32_path_from_str8(src_path, true, 1); //NOTE(delle) 1 extra byte b/c SHFILEOPSTRUCTW expects double null-termination
	//TODO(delle) restrict file stuff to root folder not data folder
	/*if(memcmp(src_wpath+4, win32_file_data_folder, win32_file_data_folder_len*sizeof(wchar_t)) != 0){ //NOTE(delle) +4 b/c of "\\?\"
		//TODO(sushi) error handling stuff
		LogE("file","File renaming can only occur within the data folder. Input src path: ",src_path);
		if(file_shared.crash_on_error){
			Assert(!"assert before exit so we can stack trace in debug mode");
			::ExitProcess(1);
		}
		return false;
	}*/
	
	wchar_t* dst_wpath = win32_path_from_str8(dst_path, true, 1); //NOTE(delle) 1 extra byte b/c SHFILEOPSTRUCTW expects double null-termination
	//TODO(delle) restrict file stuff to root folder not data folder
	/*if(memcmp(dst_wpath+4, win32_file_data_folder, win32_file_data_folder_len*sizeof(wchar_t)) != 0){ //NOTE(delle) +4 b/c of "\\?\"
		//TODO(sushi) error handling stuff
		LogE("file","File renaming can only occur within the data folder. Input dst path: ",dst_path);
		if(file_shared.crash_on_error){
			Assert(!"assert before exit so we can stack trace in debug mode");
			::ExitProcess(1);
		}
		return false;
	}*/
	
	//TODO(delle) check if initted in file_files to update
	
	WIN32_FIND_DATAW data;
	HANDLE handle = ::FindFirstFileW(src_wpath, &data);
	if(handle == INVALID_HANDLE_VALUE){
		//TODO(sushi) error handling stuff
		win32_log_last_error("FindFirstFileW", file_shared.crash_on_error, src_path);
		return false;
	}
	defer{ ::FindClose(handle); };
	
	//if directory, recursively copy all files and directories
	if(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
		BOOL success = ::CreateDirectoryW(dst_wpath, 0);
		if(!success){
			//TODO(sushi) error handling stuff
			win32_log_last_error("CreateDirectoryW", file_shared.crash_on_error, dst_path);
			return false;
		}
		
		File* dir_files = file_search_directory(src_path);
		for_array(dir_files){
			file_copy(it->path, str8_concat(dst_path, it->name, deshi_temp_allocator));
		}
	}else{
		BOOL success = ::CopyFileW(src_wpath, dst_wpath, true);
		if(!success){
			//TODO(sushi) error handling stuff
			win32_log_last_error("CopyFileW", file_shared.crash_on_error, src_path);
			return false;
		}
	}
	
	//TODO(delle) get the stuff below working
	/*
	SHFILEOPSTRUCTW s = {0};
	s.wFunc  = FO_COPY;
	s.pFrom  = src_wpath;
	s.pTo    = dst_wpath;
	s.fFlags = FOF_SILENT | FOF_NOCONFIRMMKDIR | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NO_UI;
	if(int error = SHFileOperationW(&s)){
		str8 error_str{};
		switch(error){
			case 0x71:    error_str = STR8("The source and destination files are the same file."); break;
			case 0x72:    error_str = STR8("Multiple file paths were specified in the source buffer, but only one destination file path."); break;
			case 0x73:    error_str = STR8("Rename operation was specified but the destination path is a different directory. Use the move operation instead."); break;
			case 0x74:    error_str = STR8("The source is a root directory, which cannot be moved or renamed."); break;
			case 0x75:    error_str = STR8("The operation was canceled by the user, or silently canceled if the appropriate flags were supplied to SHFileOperation."); break;
			case 0x76:    error_str = STR8("The destination is a subtree of the source."); break;
			case 0x78:    error_str = STR8("Security settings denied access to the source."); break;
			case 0x79:    error_str = STR8("The source or destination path exceeded or would exceed MAX_PATH."); break;
			case 0x7A:    error_str = STR8("The operation involved multiple destination paths, which can fail in the case of a move operation."); break;
			case 0x7C:    error_str = STR8("The path in the source or destination or both was invalid."); break;
			case 0x7D:    error_str = STR8("The source and destination have the same parent folder."); break;
			case 0x7E:    error_str = STR8("The destination path is an existing file."); break;
			case 0x80:    error_str = STR8("The destination path is an existing folder."); break;
			case 0x81:    error_str = STR8("The name of the file exceeds MAX_PATH."); break;
			case 0x82:    error_str = STR8("The destination is a read-only CD-ROM, possibly unformatted."); break;
			case 0x83:    error_str = STR8("The destination is a read-only DVD, possibly unformatted."); break;
			case 0x84:    error_str = STR8("The destination is a writable CD-ROM, possibly unformatted."); break;
			case 0x85:    error_str = STR8("The file involved in the operation is too large for the destination media or file system."); break;
			case 0x86:    error_str = STR8("The source is a read-only CD-ROM, possibly unformatted."); break;
			case 0x87:    error_str = STR8("The source is a read-only DVD, possibly unformatted."); break;
			case 0x88:    error_str = STR8("The source is a writable CD-ROM, possibly unformatted."); break;
			case 0xB7:    error_str = STR8("MAX_PATH was exceeded during the operation."); break;
			case 0x402:   error_str = STR8("An unknown error occurred. This is typically due to an invalid path in the source or destination. This error does not occur on Windows Vista and later."); break;
			case 0x10000: error_str = STR8("An unspecified error occurred on the destination."); break;
			case 0x10074: error_str = STR8("Destination is a root directory and cannot be renamed."); break;
		}
		
		LogE("win32","SHFileOperationW failed with error ",error,": ",error_str,
			 "\n Source:      ",str8_from_wchar(src_wpath+4, deshi_temp_allocator),
			 "\n Destination: ",str8_from_wchar(dst_wpath+4, deshi_temp_allocator));
		if(file_shared.crash_on_error){
			Assert(!"assert before exit so we can stack trace in debug mode");
			::ExitProcess(error);
		}
		return;
	}
	*/
	
	return true;
}

File
deshi__file_info(str8 caller_file, upt caller_line, str8 path, FileResult* error_result){DPZoneScoped;
	if(!path || *path.str == 0){
		//TODO(sushi) error handling stuff
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
		//TODO(sushi) error handling stuff
		win32_log_last_error("FindFirstFileW", file_shared.crash_on_error, path);
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
	result.type             = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? FileType_Directory : FileType_File; //TODO(delle) other filetypes
	
	//TODO(delle) reuse the temporary wpath memory for the result //!Optimizable
	if(result.type == FileType_Directory){
		wpath[wpath_len++] = L'\\';
	}
	result.path = str8_from_wchar(wpath+4, deshi_temp_allocator); //NOTE(delle) +4 b/c of "\\?\"
	forI(result.path.count){
		if(result.path.str[i] == '\\'){
			result.path.str[i] = '/';
		}
	}
	
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

File*
deshi__file_search_directory(str8 caller_file, upt caller_line, str8 directory, FileResult* error_result){DPZoneScoped;
	if(!directory || *directory.str == 0){
		//TODO(sushi) error handling stuff
		LogE("file","file_search_directory() was passed an empty `directory` at ",caller_file,"(",caller_line,")");
		return 0;
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
		//TODO(sushi) error handling stuff
		win32_log_last_error("FindFirstFileW", file_shared.crash_on_error, directory);
		return 0;
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
	forI(directory_slash.count){
		if(directory_slash.str[i] == '\\'){
			directory_slash.str[i] = '/';
		}
	}
	
	ULARGE_INTEGER size, time;
	File* result = array_create(File, 8, deshi_temp_allocator);
	while(next != INVALID_HANDLE_VALUE){
		if((wcscmp(data.cFileName, L".") == 0) || (wcscmp(data.cFileName, L"..") == 0)){
			if(::FindNextFileW(next, &data) == 0) break;
			continue;
		}
		
		File* file = array_push(result);
		time.LowPart  = data.ftCreationTime.dwLowDateTime;
		time.HighPart = data.ftCreationTime.dwHighDateTime;
		file->creation_time    = WindowsTimeToUnixTime(time.QuadPart);
		time.LowPart  = data.ftLastAccessTime.dwLowDateTime;
		time.HighPart = data.ftLastAccessTime.dwHighDateTime;
		file->last_access_time = WindowsTimeToUnixTime(time.QuadPart);
		time.LowPart  = data.ftLastWriteTime.dwLowDateTime;
		time.HighPart = data.ftLastWriteTime.dwHighDateTime;
		file->last_write_time  = WindowsTimeToUnixTime(time.QuadPart);
		size.LowPart  = data.nFileSizeLow;
		size.HighPart = data.nFileSizeHigh;
		file->bytes            = size.QuadPart;
		file->type             = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? FileType_Directory : FileType_File; //TODO(delle) other filetypes
		
		//TODO(delle) reuse the temporary file_wpath memory for the file //!Optimizable
		file->path = str8_concat(directory_slash, str8_from_wchar(data.cFileName, deshi_temp_allocator), deshi_temp_allocator);
		if(file->path){
			file->name = str8_skip_until_last(file->path, '/');
			if(file->name){
				str8_increment(&file->name, 1);
				file->front = str8_eat_until_last(file->name, '.');
				if(file->front){
					file->ext = str8{file->front.str+file->front.count+1, file->name.count-(file->front.count+1)};
				}
			}
		}
		
		if(::FindNextFileW(next, &data) == 0) break;
	}
	
	DWORD error = ::GetLastError();
	if(error != ERROR_NO_MORE_FILES){
		//TODO(sushi) error handling stuff
		win32_log_last_error("FindNextFileW", file_shared.crash_on_error, directory);
	}
	
	return result;
}

str8
deshi__file_path_absolute(str8 caller_file, upt caller_line, str8 path, FileResult* error_result){DPZoneScoped;
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
		win32_log_last_error("FindFirstFileW", file_shared.crash_on_error, path);
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
deshi__file_path_equal(str8 caller_file, upt caller_line, str8 path1, str8 path2, FileResult* error_result){DPZoneScoped;
	if(!path1 || *path1.str == 0){
		//TODO(sushi) error handling stuff
		LogE("file","file_path_equal() was passed an empty `path1` at ",caller_file,"(",caller_line,")");
		return false;
	}
	if(!path2 || *path2.str == 0){
		//TODO(sushi) error handling stuff
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
deshi__file_init(str8 caller_file, upt caller_line, str8 path, FileAccess flags, b32 ignore_nonexistence, FileResult* error_result){DPZoneScoped;
	if(!path || *path.str == 0){
		//TODO(sushi) error handling stuff
		LogE("file","file_init() was passed an empty `path` at ",caller_file,"(",caller_line,")");
		return 0;
	}
	
	//change access and return the file if it's already init
	for_array(file_shared.files){
		if(file_path_equal(path, it->path)){
			file_change_access(it, flags);
			return it;
		}
	}
	
	File* result = array_push(file_shared.files);
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
		result->type             = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? FileType_Directory : FileType_File; //TODO(delle) other filetypes
		
		if(result->type == FileType_Directory){
			wpath[wpath_len++] = L'\\';
		}
		result->path = str8_from_wchar(wpath+4, deshi_allocator); //NOTE(delle) +4 b/c of "\\?\"
		forI(result->path.count){
			if(result->path.str[i] == '\\'){
				result->path.str[i] = '/';
			}
		}
		
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
				//TODO(sushi) error handling stuff
				win32_log_last_error("CreateFileW", file_shared.crash_on_error, path);
				array_pop(file_shared.files);
				return 0;
			}
			defer{ ::CloseHandle(handle); };
			
			BY_HANDLE_FILE_INFORMATION info;
			if(::GetFileInformationByHandle(handle, &info) == 0){
				//TODO(sushi) error handling stuff
				win32_log_last_error("GetFileInformationByHandle", file_shared.crash_on_error, path);
				array_pop(file_shared.files);
				return 0;
			}
			
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
			result->type             = (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? FileType_Directory : FileType_File; //TODO(delle) other filetypes
			
			if(result->type == FileType_Directory){
				wpath[wpath_len++] = L'\\';
			}
			result->path = str8_from_wchar(wpath+4, deshi_allocator); //NOTE(delle) +4 b/c of "\\?\"
			forI(result->path.count){
				if(result->path.str[i] == '\\'){
					result->path.str[i] = '/';
				}
			}
			
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
			if(!ignore_nonexistence){
				//TODO(sushi) error handling stuff
				win32_log_last_error("FindFirstFileW", file_shared.crash_on_error, path);
			}
			array_pop(file_shared.files);
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
			//TODO(sushi) error handling stuff
			LogE("file", "fopen() failed to open file '",result->path,"' with mode '",str8_from_wchar(open_mode,deshi_temp_allocator),"'");
			win32_log_last_error("fopen",file_shared.crash_on_error);
			memory_zfree(result->path.str);
			memory_zfree(result);
			array_pop(file_shared.files);
			return 0;
		}
		
		if(HasFlag(flags, FileAccess_Append)){
			fseek(result->handle, result->bytes, SEEK_SET);
			result->cursor = result->bytes;
		}
	}
	
	result->access = flags & ~(FileAccess_Append|FileAccess_Create|FileAccess_Truncate);
	return result;
}

b32
deshi__file_deinit(str8 caller_file, upt caller_line, File* file, FileResult* error_result){DPZoneScoped;
	if(file == 0){
		//TODO(sushi) error handling stuff
		LogE("file","file_deinit() was passed a null `file` pointer at ",caller_file,"(",caller_line,")");
		return false;
	}
	
	fclose(file->handle);
	memory_zfree(file->path.str);
	
	forI(array_count(file_shared.files)){
		if(file_shared.files+i == file){
			array_remove_unordered(file_shared.files, i);
			break;
		}
	}
	
	return true;
}

b32
deshi__file_change_access(str8 caller_file, upt caller_line, File* file, FileAccess flags, FileResult* error_result){DPZoneScoped;
	if(file == 0){
		//TODO(sushi) error handling stuff
		LogE("file","file_change_access() was passed a null `file` pointer at ",caller_file,"(",caller_line,")");
		return false;
	}
	
	RemoveFlag(flags, FileAccess_Create); //the file already exists
	wchar_t* wpath = win32_path_from_str8(file->path, false);
	
	//open the file if access requests it and it's not already open
	if(file->handle == 0 && HasFlag(flags, FileAccess_ReadWrite)){
		Flags open_flags = flags & ~(FileAccess_Append);
		wchar_t* open_mode = 0;
		switch(open_flags){
			case FileAccess_Read:              open_mode = (wchar_t*)L"rb";  file->bytes = 0; break;
			case FileAccess_WriteTruncate:     open_mode = (wchar_t*)L"wb";  file->bytes = 0; break;
			case FileAccess_Write:
			case FileAccess_ReadWrite:         open_mode = (wchar_t*)L"rb+"; break;
			case FileAccess_ReadWriteTruncate: open_mode = (wchar_t*)L"wb+"; file->bytes = 0; break;
		}
		
		file->handle = _wfopen(wpath, open_mode);
		if(file->handle == 0){
			//TODO(sushi) error handling stuff
			LogE("file", "fopen() failed to open file '",file->path,"' with mode '",str8_from_wchar(open_mode,deshi_temp_allocator),"'");
			win32_log_last_error("fopen",file_shared.crash_on_error);
			return false;
		}
	}else if(file->handle != 0 && !HasFlag(flags, FileAccess_ReadWrite)){
		fclose(file->handle);
		file->handle = 0;
	}
	
	if(file->handle != 0 && HasFlag(flags, FileAccess_Append)){
		fseek(file->handle, file->bytes, SEEK_SET);
		file->cursor = file->bytes;
	}
	
	if(file->handle && HasFlag(flags, FileAccess_Truncate)){
		fclose(file->handle);
		file->handle = 0;
		file->bytes = 0;
		if     (HasFlag(flags, FileAccess_ReadWrite)){
			file->handle = _wfopen(wpath, (wchar_t*)L"wb+");
		}else if(HasFlag(flags, FileAccess_Write)){
			file->handle = _wfopen(wpath, (wchar_t*)L"wb");
		}else{
			//just truncate the file
			fclose(_wfopen(wpath, (wchar_t*)L"wb"));
		}
	}
	
	file->access = flags & ~(FileAccess_Append|FileAccess_Truncate);
	return true;
}

str8
deshi__file_read_simple(str8 caller_file, upt caller_line, str8 path, Allocator* allocator, FileResult* error_result){DPZoneScoped;
	if(!path || *path.str == 0){
		//TODO(sushi) error handling stuff
		LogE("file","file_read_simple() was passed an empty `path` at ",caller_file,"(",caller_line,")");
		return str8{};
	}
	if(allocator == 0){
		//TODO(sushi) error handling stuff
		LogE("file","file_read_simple() was passed a null `allocator` pointer at ",caller_file,"(",caller_line,")");
		return str8{};
	}
	
	wchar_t* wpath = win32_path_from_str8(path, true);
	HANDLE handle = ::CreateFileW(wpath, GENERIC_READ, 0,0, OPEN_EXISTING, 0,0);
	if(handle == INVALID_HANDLE_VALUE){
		//TODO(sushi) error handling stuff
		win32_log_last_error("CreateFileW", file_shared.crash_on_error, path);
		return str8{};
	}
	defer{ ::CloseHandle(handle); };
	
	LARGE_INTEGER size;
	if(::GetFileSizeEx(handle, &size) == 0){
		//TODO(sushi) error handling stuff
		win32_log_last_error("GetFileSizeEx", file_shared.crash_on_error, path);
		return str8{};
	}
	
	str8 result{(u8*)allocator->reserve(size.QuadPart+1), size.QuadPart};
	if(::ReadFile(handle, result.str, size.QuadPart, 0,0) == 0){
		//TODO(sushi) error handling stuff
		win32_log_last_error("ReadFile", file_shared.crash_on_error, path);
		return str8{};
	}
	
	return result;
}

u64
deshi__file_write_simple(str8 caller_file, upt caller_line, str8 path, void* data, u64 bytes, FileResult* error_result){DPZoneScoped;
	if(!path || *path.str == 0){
		//TODO(sushi) error handling stuff
		LogE("file","file_write_simple() was passed an empty `path` at ",caller_file,"(",caller_line,")");
		return 0;
	}
	if(data == 0 && bytes > 0){
		//TODO(sushi) error handling stuff
		LogE("file","file_write_simple() was passed a null `data` pointer at ",caller_file,"(",caller_line,")");
		return 0;
	}
	
	wchar_t* wpath = win32_path_from_str8(path, true);
	HANDLE handle = ::CreateFileW(wpath, GENERIC_WRITE, 0,0, CREATE_ALWAYS, 0,0);
	if(handle == INVALID_HANDLE_VALUE){
		//TODO(sushi) error handling stuff
		win32_log_last_error("CreateFileW", file_shared.crash_on_error, path);
		return 0;
	}
	defer{ ::CloseHandle(handle); };
	
	DWORD bytes_written = 0;
	if(::WriteFile(handle, data, bytes, &bytes_written, 0) == 0){
		//TODO(sushi) error handling stuff
		win32_log_last_error("WriteFile", file_shared.crash_on_error, path);
		return (u64)bytes_written;
	}
	
	return (u64)bytes_written;
}

u64
deshi__file_append_simple(str8 caller_file, upt caller_line, str8 path, void* data, u64 bytes, FileResult* error_result){DPZoneScoped;
	if(!path || *path.str == 0){
		//TODO(sushi) error handling stuff
		LogE("file","file_append_simple() was passed an empty `path` at ",caller_file,"(",caller_line,")");
		return 0;
	}
	if(data == 0 && bytes > 0){
		//TODO(sushi) error handling stuff
		LogE("file","file_append_simple() was passed a null `data` pointer at ",caller_file,"(",caller_line,")");
		return 0;
	}
	
	wchar_t* wpath = win32_path_from_str8(path, true);
	HANDLE handle = ::CreateFileW(wpath, GENERIC_WRITE, 0,0, OPEN_ALWAYS, 0,0);
	if(handle == INVALID_HANDLE_VALUE){
		//TODO(sushi) error handling stuff
		win32_log_last_error("CreateFileW", file_shared.crash_on_error, path);
		return 0;
	}
	defer{ ::CloseHandle(handle); };
	
	if(::SetFilePointerEx(handle, LARGE_INTEGER{0}, 0, SEEK_END) == 0){
		//TODO(sushi) error handling stuff
		win32_log_last_error("SetFilePointerEx", file_shared.crash_on_error, path);
		return 0;
	}
	
	DWORD bytes_written = 0;
	if(::WriteFile(handle, data, bytes, &bytes_written, 0) == 0){
		//TODO(sushi) error handling stuff
		win32_log_last_error("WriteFile", file_shared.crash_on_error, path);
		return (u64)bytes_written;
	}
	
	return (u64)bytes_written;
}

//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @modules
void*
platform_load_module(str8 module_path){DPZoneScoped;
	wchar_t* wmodule_path = wchar_from_str8(module_path, 0, deshi_temp_allocator);
	return ::LoadLibraryW(wmodule_path);
}

void
platform_free_module(void* module){DPZoneScoped;
	::FreeLibrary((HMODULE)module);
}

#if COMPILER_CLANG
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wmicrosoft-cast"
#endif
void*
platform_get_module_symbol(void* module, const char* symbol_name){DPZoneScoped;
	return (void*)::GetProcAddress((HMODULE)module, symbol_name);
}
#if COMPILER_CLANG
#  pragma clang diagnostic pop
#endif


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @clipboard
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
//// @threading
mutex
mutex_init(){DPZoneScoped;
	mutex m;
	if(m.handle = ::CreateMutex(NULL, FALSE, NULL); !m.handle)
		win32_log_last_error("CreateMutex");
	return m;
}

void 
mutex_deinit(mutex* m){DPZoneScoped;
	::CloseHandle(m->handle);
}

void 
mutex_lock(mutex* m){DPZoneScoped;
	if(!m->handle){
		LogE("threading-win32", "Attempted to use a mutex that hasnt been initialized or has already been deinitializaed.");
		Assert(0);
	} 
	DWORD waitResult = ::WaitForSingleObject(m->handle, INFINITE);
	switch(waitResult){
		case WAIT_ABANDONED:{
			//TODO maybe have an option somewhere to suppress this error
			LogE("threading-win32", "Attempted to lock an abandoned mutex. This mutex was not released by the thread that owned it before the thread terminated.");
		}break;
		//TODO set up our own error reporting once i figure out what error codes are what for this
		case WAIT_FAILED:{ win32_log_last_error("WaitForSingleObject"); Assert(0); }break;
	}
	m->is_locked = 1;
}

b32 
mutex_try_lock(mutex* m){DPZoneScoped;
	if(!m->handle){
		LogE("threading-win32", "Attempted to use a mutex that hasnt been initialized or has already been deinitializaed.");
		Assert(0);
	}
	DWORD waitResult = ::WaitForSingleObject(m->handle, 0);
	switch(waitResult){
		case WAIT_ABANDONED:{
			//TODO maybe have an option somewhere to suppress this error
			LogE("threading-win32", "Locking an abandoned mutex. This mutex was not released by the thread that owned it before the thread terminated.");
		}break;
		case WAIT_TIMEOUT:{
			return false;
		}break;
		//TODO set up our own error reporting once i figure out what error codes are what for this
		case WAIT_FAILED:{ win32_log_last_error("WaitForSingleObject"); Assert(0); }break;
	}
	m->is_locked = 1;
	return true;
}

b32 
mutex_try_lock_for(mutex* m, u64 milliseconds){DPZoneScoped;
	if(!m->handle){
		LogE("threading-win32", "Attempted to use a mutex that hasnt been initialized or has already been deinitializaed.");
		Assert(0);
	}
	DWORD waitResult = ::WaitForSingleObject(m->handle, milliseconds);
	switch(waitResult){
		case WAIT_ABANDONED:{
			//TODO maybe have an option somewhere to suppress this error
			LogE("threading-win32", "Locking an abandoned mutex. This mutex was not released by the thread that owned it before the thread terminated.");
		}break;
		case WAIT_TIMEOUT:{
			return false;
		}break;
		//TODO set up our own error reporting once i figure out what error codes are what for this
		case WAIT_FAILED:{ win32_log_last_error("WaitForSingleObject"); Assert(0); }break;
	}
	m->is_locked = 1;
	return true;
}

void 
mutex_unlock(mutex* m){DPZoneScoped;
	if(!ReleaseMutex(m->handle)){ win32_log_last_error("ReleaseMutex"); Assert(0); }
	m->is_locked = 0;
}


condvar
condition_variable_init(){DPZoneScoped;
	//NOTE i have to use std mem here in the case that condvar is used before memory is initialized (eg. a condvar global var)
	condition_variable cv;
	cv.cvhandle = malloc(sizeof(CONDITION_VARIABLE));
	cv.cshandle = malloc(sizeof(CRITICAL_SECTION));
	::InitializeCriticalSection((CRITICAL_SECTION*)cv.cshandle);
	::InitializeConditionVariable((CONDITION_VARIABLE*)cv.cvhandle);
	return cv;
}

void 
condition_variable_deinit(condvar* cv){DPZoneScoped;
	free(cv->cvhandle);
	free(cv->cshandle);
}

void 
condition_variable_notify_one(condvar* cv){DPZoneScoped;
	::WakeConditionVariable((CONDITION_VARIABLE*)cv->cvhandle);
}

void condition_variable_notify_all(condvar* cv){DPZoneScoped;
	::WakeAllConditionVariable((CONDITION_VARIABLE*)cv->cvhandle);
}

void 
condition_variable_wait(condvar* cv){DPZoneScoped;
	semaphore_leave(&DeshThreadManager->wake_up_barrier);
	::EnterCriticalSection((CRITICAL_SECTION*)cv->cshandle);
	if(!::SleepConditionVariableCS((CONDITION_VARIABLE*)cv->cvhandle, (CRITICAL_SECTION*)cv->cshandle, INFINITE)){
		win32_log_last_error("SleepConditionVariableCS");
	}
	::LeaveCriticalSection((CRITICAL_SECTION*)cv->cshandle);
	semaphore_enter(&DeshThreadManager->wake_up_barrier);
}

void 
condition_variable_wait_for(condvar* cv, u64 milliseconds){DPZoneScoped;
	::EnterCriticalSection((CRITICAL_SECTION*)cv->cshandle);
	::SleepConditionVariableCS((CONDITION_VARIABLE*)cv->cvhandle, (CRITICAL_SECTION*)cv->cshandle, milliseconds);
	::LeaveCriticalSection((CRITICAL_SECTION*)cv->cshandle);	
}


semaphore
semaphore_init(u64 ival, u64 mval){
	semaphore se;
	if(se.handle = CreateSemaphore(0, mval, ival, 0); !se.handle){
		win32_log_last_error("CreateSemaphore");
	}
	return se;
}

void 
semaphore_deinit(semaphore* se){
	CloseHandle(se->handle);
}

void 
semaphore_enter(semaphore* se){
	DWORD waitres = WaitForSingleObject(se->handle, INFINITE);
	switch(waitres){
		case WAIT_ABANDONED:{
			//TODO maybe have an option somewhere to suppress this error
			LogE("threading-win32", "Locking an abandoned mutex. This mutex was not released by the thread that owned it before the thread terminated.");
		}break;
		//TODO set up our own error reporting once i figure out what error codes are what for this
		case WAIT_FAILED:{ win32_log_last_error("WaitForSingleObject"); Assert(0); }break;
	}
	//TODO(sushi) need to see if this should be locked!
	se->count--;
}

void 
semaphore_leave(semaphore* se){
	if(!ReleaseSemaphore(se->handle, 1, 0)){
		win32_log_last_error("ReleaseSemaphore");
	}
	se->count++;
}


#ifdef BUILD_SLOW
#define WorkerLog(message) //DPTracyDynMessage(toStr("worker ", me->idx, " : " message)) //Log("thread", "worker ", me, ": ", message)
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
	semaphore_enter(&man->wake_up_barrier);
	SetThreadDescription(GetCurrentThread(), wchar_from_str8(ToString8(deshi_temp_allocator, "worker ", me->idx), 0, deshi_temp_allocator));
	while(!me->close){
		while(man->job_ring.count){//lock and retrieve a job from ThreadManager
			WorkerLog("looking to take a job from job ring");
			ThreadJob tj;
			//TODO look into how DOOM3 does this w/o locking, I don't understand currently how they atomically do this 
			
			mutex_lock(&man->job_ring_lock);
			
			//check once more that there are jobs since the locking thread could have taken the last one
			//im sure theres a better way to do this
			if(!man->job_ring.count){
				mutex_unlock(&man->job_ring_lock);
				break; 
			} 	
			WorkerLog("locked job ring and taking a job");
			//take the job and remove it from the ring
			tj = man->job_ring[0];
			man->job_ring.remove(1);
			
			mutex_unlock(&man->job_ring_lock);
			
			//run the function
			WorkerLog("running job");
			me->running = true;
			tj.ThreadFunction(tj.data);
			me->running = false;
			WorkerLog("finished running job");
			if(!threader_worker_should_continue()) break;
		}
		//when there are no more jobs go to sleep until notified again by thread manager
		WorkerLog("going to sleep");
		threader_set_thread_name(STR8("sleeping..."));
		condition_variable_wait(&man->idle);
		threader_set_thread_name(STR8("waking up"));
		WorkerLog("woke up");
		
	}
	WorkerLog("closing");
}

DWORD WINAPI
deshi__thread_worker__win32_stub(LPVOID in){DPZoneScoped;
	deshi__thread_worker((Thread*)in);
	return 0;
}

void 
threader_init(u32 max_jobs){DPZoneScoped;
	DeshiStageInitStart(DS_THREAD, DS_MEMORY, "Attempt to init threader loading Memory first");
	g_tmanager->job_ring.init(max_jobs, deshi_allocator);
	DeshThreadManager->wake_up_queue.init(DeshThreadManager->max_awake_threads*4);
	//TODO(sushi) query for max amount of threads 
	DeshThreadManager->wake_up_barrier = semaphore_init(8,8);
	semaphore_enter(&DeshThreadManager->wake_up_barrier);
	DeshThreadManager->idle = condition_variable_init();
	DeshThreadManager->job_ring_lock = mutex_init();
	DeshiStageInitEnd(DS_THREAD);
}

void 
threader_spawn_thread(u32 count){DPZoneScoped;
	if(DeshThreadManager->max_threads && DeshThreadManager->threads.count + count > DeshThreadManager->max_threads){
		LogE("threading-win32", "Attempt to create more threads than the maximum set on manager: ", DeshThreadManager->max_threads);
	}
	forI(count){
		DeshThreadManager->awake_threads++;
		Thread* t = (Thread*)memalloc(sizeof(Thread));
		if(DeshThreadManager->threads.count)
			t->idx = (*DeshThreadManager->threads.last)->idx + 1;
		else
			t->idx = 0;
		DeshThreadManager->threads.add(t);
		::CreateThread(0, 0, deshi__thread_worker__win32_stub, (void*)t, 0, 0);
	}
}

void 
threader_close_all_threads(){DPZoneScoped;
	forI(DeshThreadManager->threads.count) DeshThreadManager->threads[i]->close = true;
	threader_wake_threads(0);
	DeshThreadManager->threads.clear();
}

void 
threader_add_job(ThreadJob job){DPZoneScoped;
	DeshThreadManager->job_ring.add(job);
}

void 
threader_add_jobs(carray<ThreadJob> jobs){DPZoneScoped;
	forI(jobs.count) DeshThreadManager->job_ring.add(jobs[i]);
}

void 
threader_cancel_all_jobs(){DPZoneScoped;
	DeshThreadManager->job_ring.clear();
} 

void 
threader_wake_threads(u32 count){DPZoneScoped;
	if(!DeshThreadManager->threads.count){ LogW("Thread", "Attempt to use wake_threads without spawning any threads first"); }
	else if(!count){
		condition_variable_notify_all(&DeshThreadManager->idle);
	}  
	else{
		forI(count) 
			condition_variable_notify_one(&DeshThreadManager->idle);
	}
}

void 
threader_set_thread_name(str8 name){
	SetThreadDescription(GetCurrentThread(), wchar_from_str8(name, 0, deshi_temp_allocator));
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @window
Window*
window_create(str8 title, s32 width, s32 height, s32 x, s32 y, DisplayMode display_mode, Decoration decorations){DPZoneScoped;
	DeshiStageInitStart(DS_WINDOW, DS_PLATFORM, "Called window_create() before initializing Platform module");
	
	//create the window
	Window* window = (Window*)memory_alloc(sizeof(Window));
	window->handle = ::CreateWindowExW(0, DESHI_RENDER_WNDCLASSNAME, wchar_from_str8(title, 0, deshi_temp_allocator),
									   WS_CLIPCHILDREN|WS_CLIPSIBLINGS, 0,0, 0,0, 0, 0, win32_console_instance, 0);
	if(!window->handle){ win32_log_last_error("CreateWindowExW", true); memory_zfree(window); return 0; }
	//set Win32 window user data to the deshi Window pointer
	::SetWindowLongPtrW((HWND)window->handle, GWLP_USERDATA, (LONG_PTR)window);
	window->context = ::GetDC((HWND)window->handle);
	
	//set window decorations
	window->display_mode = -1;
	window_display_mode(window, display_mode);
	//window_decorations(decorations);
	if(decorations != Decoration_SystemDecorations){
		//TODO(delle) custom titlebar
	}
	
	//set window title
	window->title = str8_copy(title, deshi_allocator);
	
	//set window decorated pos/size
	int work_x = 0, work_y = 0, work_w = 0, work_h = 0;
	win32_monitor_info((HWND)window->handle, 0,0, &work_x,&work_y, &work_w,&work_h);
	if(width  == 0xFFFFFFFF) width  = work_w / 2;
	if(height == 0xFFFFFFFF) height = work_h / 2;
	if(x == 0xFFFFFFFF) x = work_x + (width  / 2);
	if(y == 0xFFFFFFFF) y = work_y + (height / 2);
	::SetWindowPos((HWND)window->handle, 0, x,y, width,height, 0);
	
	//get window decorated pos/size
	RECT rect;
	::GetWindowRect((HWND)window->handle, &rect);
	window->position_decorated.x = rect.left;
	window->position_decorated.y = rect.top;
	window->dimensions_decorated.x = rect.right  - rect.left;
	window->dimensions_decorated.y = rect.bottom - rect.top;
	
	//get window client pos/size 
	::GetClientRect((HWND)window->handle, &rect);
	::ClientToScreen((HWND)window->handle, (POINT*)&rect.left);
	::ClientToScreen((HWND)window->handle, (POINT*)&rect.right);
	window->position.x = rect.left;
	window->position.y = rect.top;
	window->dimensions.x = rect.right  - rect.left;
	window->dimensions.y = rect.bottom - rect.top;
	
	//set window center
	window->center.x = window->dimensions.x / 2;
	window->center.y = window->dimensions.y / 2;
	
	//set window cursor
	::SetCursor(::LoadCursorW(0, IDC_ARROW));
	
	//if this is the first window created, it's global
	if(window_windows.count == 0){
		DeshWindow = window;
	}
	
	window->index = window_windows.count;
	window_active = window;
	window_windows.add(window);
	DeshiStageInitEnd(DS_WINDOW);
	return window;
}

void
window_close(Window* window){DPZoneScoped;
	PostMessage((HWND)window->handle, WM_CLOSE, 0, 0);
}

void
window_swap_buffers(Window* window){DPZoneScoped;
	::SwapBuffers((HDC)window->context);
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
window_set_title(Window* window, str8 title){DPZoneScoped;
	::SetWindowTextW((HWND)window->handle, wchar_from_str8(title, 0, deshi_temp_allocator));
	memory_zfree(window->title.str);
	window->title = str8_copy(title, deshi_allocator);
}

void
window_set_cursor_mode(Window* window, CursorMode mode){DPZoneScoped;
	if(window->cursor_mode == mode) return;
	switch(mode){
		case CursorMode_Default: default:{
			::SetCursor(::LoadCursorW(0, IDC_ARROW));
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
window_set_cursor_type(Window* window, CursorType type){DPZoneScoped;
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
window_set_cursor_position(Window* window, s32 x, s32 y){DPZoneScoped;
	POINT p{x, y};
	::ClientToScreen((HWND)window->handle, &p);
	::SetCursorPos(p.x, p.y);
	DeshInput->mouseX = x;
	DeshInput->mouseY = y;
	DeshInput->screenMouseX = p.x;
	DeshInput->screenMouseY = p.y;
}

//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @networking

//collection of error codes and their messages, so this list isnt typed out in every function
//some of these should just never happen but I'm including them just in case
//all of these are copy paste from win32 docs: https://docs.microsoft.com/en-us/windows/win32/winsock/windows-sockets-error-codes-2
//so they are very general. the function that triggered it is included for context
#define netLogE(func, ...) LogE("networking", func, ": ", __VA_ARGS__)
void WSA_log_error(str8 func){
	s32 err = WSAGetLastError();
	switch(err){
		case WSA_INVALID_HANDLE:         netLogE(func, "Specified event object handle is invalid. An application attempts to use an event object, but the specified handle is not valid.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       break;
		case WSA_NOT_ENOUGH_MEMORY:      netLogE(func, "Insufficient memory available. An application used a Windows Sockets function that directly maps to a Windows function. The Windows function is indicating a lack of required memory resources.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        break;
		case WSA_INVALID_PARAMETER:      netLogE(func, "One or more parameters are invalid. An application used a Windows Sockets function which directly maps to a Windows function. The Windows function is indicating a problem with one or more parameters.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                break;
		case WSA_OPERATION_ABORTED:      netLogE(func, "Overlapped operation aborted. An overlapped operation was canceled due to the closure of the socket, or the execution of the SIO_FLUSH command in WSAIoctl.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            break;
		case WSA_IO_INCOMPLETE:          netLogE(func, "Overlapped I/O event object not in signaled state. The application has tried to determine the status of an overlapped operation which is not yet completed. Applications that use WSAGetOverlappedResult (with the fWait flag set to FALSE) in a polling mode to determine when an overlapped operation has completed, get this error code until the operation is complete.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                            break;
		case WSA_IO_PENDING:             netLogE(func, " will complete later. The application has initiated an overlapped operation that cannot be completed immediately. A completion indication will be given later when the operation has been completed.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   break;
		case WSAEINTR:                   netLogE(func, "Interrupted function call. A blocking operation was interrupted by a call to WSACancelBlockingCall.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    break;
		case WSAEBADF:                   netLogE(func, "File handle is not valid. The file handle supplied is not valid.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       break;
		case WSAEACCES:                  netLogE(func, "Permission denied. An attempt was made to access a socket in a way forbidden by its access permissions. An example is using a broadcast address for sendto without broadcast permission being set using setsockopt(SO_BROADCAST). Another possible reason for the WSAEACCES error is that when the bind function is called (on Windows NT 4.0 with SP4 and later), another application, service, or kernel mode driver is bound to the same address with exclusive access. Such exclusive access is a new feature of Windows NT 4.0 with SP4 and later, and is implemented by using the SO_EXCLUSIVEADDRUSE option.");                                                                                                                                                                                                                                    break;
		case WSAEFAULT:                  netLogE(func, "Bad address. The system detected an invalid pointer address in attempting to use a pointer argument of a call. This error occurs if an application passes an invalid pointer value, or if the length of the buffer is too small. For instance, if the length of an argument, which is a sockaddr structure, is smaller than the sizeof(sockaddr).");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      break;
		case WSAEINVAL:                  netLogE(func, "Invalid argument. Some invalid argument was supplied (for example, specifying an invalid level to the setsockopt function). In some instances, it also refers to the current state of the socket—for instance, calling accept on a socket that is not listening.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       break;
		case WSAEMFILE:                  netLogE(func, "Too many open files. Too many open sockets. Each implementation may have a maximum number of socket handles available, either globally, per process, or per thread.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    break;
		case WSAEWOULDBLOCK:             netLogE(func, "Resource temporarily unavailable. This error is returned from operations on nonblocking sockets that cannot be completed immediately, for example recv when no data is queued to be read from the socket. It is a nonfatal error, and the operation should be retried later. It is normal for WSAEWOULDBLOCK to be reported as the result from calling connect on a nonblocking SOCK_STREAM socket, since some time must elapse for the connection to be established.");                                                                                                                                                                                                                                                                                                                                                                                  break;
		case WSAEINPROGRESS:             netLogE(func, "Operation now in progress. A blocking operation is currently executing. Windows Sockets only allows a single blocking operation—per- task or thread—to be outstanding, and if any other function call is made (whether or not it references that or any other socket) the function fails with the WSAEINPROGRESS error.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                break;
		case WSAEALREADY:                netLogE(func, "Operation already in progress. An operation was attempted on a nonblocking socket with an operation already in progress—that is, calling connect a second time on a nonblocking socket that is already connecting, or canceling an asynchronous request (WSAAsyncGetXbyY) that has already been canceled or completed.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 break;
		case WSAENOTSOCK:                netLogE(func, "Socket operation on nonsocket. An operation was attempted on something that is not a socket. Either the socket handle parameter did not reference a valid socket, or for select, a member of an fd_set was not valid.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  break;
		case WSAEDESTADDRREQ:            netLogE(func, "Destination address required. A required address was omitted from an operation on a socket. For example, this error is returned if sendto is called with the remote address of ADDR_ANY.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               break;
		case WSAEMSGSIZE:                netLogE(func, "Message too long. A message sent on a datagram socket was larger than the internal message buffer or some other network limit, or the buffer used to receive a datagram was smaller than the datagram itself.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          break;
		case WSAEPROTOTYPE:              netLogE(func, "Protocol wrong type for socket. A protocol was specified in the socket function call that does not support the semantics of the socket type requested. For example, the ARPA Internet UDP protocol cannot be specified with a socket type of SOCK_STREAM.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              break;
		case WSAENOPROTOOPT:             netLogE(func, "Bad protocol option. An unknown, invalid or unsupported option or level was specified in a getsockopt or setsockopt call.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              break;
		case WSAEPROTONOSUPPORT:         netLogE(func, "Protocol not supported. The requested protocol has not been configured into the system, or no implementation for it exists. For example, a socket call requests a SOCK_DGRAM socket, but specifies a stream protocol.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  break;
		case WSAESOCKTNOSUPPORT:         netLogE(func, "Socket type not supported. The support for the specified socket type does not exist in this address family. For example, the optional type SOCK_RAW might be selected in a socket call, and the implementation does not support SOCK_RAW sockets at all.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               break;
		case WSAEOPNOTSUPP:              netLogE(func, "Operation not supported. The attempted operation is not supported for the type of object referenced. Usually this occurs when a socket descriptor to a socket that cannot support this operation is trying to accept a connection on a datagram socket.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                break;
		case WSAEPFNOSUPPORT:            netLogE(func, "Protocol family not supported. The protocol family has not been configured into the system or no implementation for it exists. This message has a slightly different meaning from WSAEAFNOSUPPORT. However, it is interchangeable in most cases, and all Windows Sockets functions that return one of these messages also specify WSAEAFNOSUPPORT.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     break;
		case WSAEAFNOSUPPORT:            netLogE(func, "Address family not supported by protocol family. An address incompatible with the requested protocol was used. All sockets are created with an associated address family (that is, AF_INET for Internet Protocols) and a generic protocol type (that is, SOCK_STREAM). This error is returned if an incorrect protocol is explicitly requested in the socket call, or if an address of the wrong family is used for a socket, for example, in sendto.");                                                                                                                                                                                                                                                                                                                                                                                                  break;
		case WSAEADDRINUSE:              netLogE(func, "Address already in use. Typically, only one usage of each socket address (protocol/IP address/port) is permitted. This error occurs if an application attempts to bind a socket to an IP address/port that has already been used for an existing socket, or a socket that was not closed properly, or one that is still in the process of closing. For server applications that need to bind multiple sockets to the same port number, consider using setsockopt (SO_REUSEADDR). Client applications usually need not call bind at all—connect chooses an unused port automatically. When bind is called with a wildcard address (involving ADDR_ANY), a WSAEADDRINUSE error could be delayed until the specific address is committed. This could happen with a call to another function later, including connect, listen, WSAConnect, or WSAJoinLeaf."); break;
		case WSAEADDRNOTAVAIL:           netLogE(func, "Cannot assign requested address. The requested address is not valid in its context. This normally results from an attempt to bind to an address that is not valid for the local computer. This can also result from connect, sendto, WSAConnect, WSAJoinLeaf, or WSASendTo when the remote address or port is not valid for a remote computer (for example, address or port 0).");                                                                                                                                                                                                                                                                                                                                                                                                                                                                        break;
		case WSAENETDOWN:                netLogE(func, "Network is down. A socket operation encountered a dead network. This could indicate a serious failure of the network system (that is, the protocol stack that the Windows Sockets DLL runs over), the network interface, or the local network itself.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  break;
		case WSAENETUNREACH:             netLogE(func, "Network is unreachable. A socket operation was attempted to an unreachable network. This usually means the local software knows no route to reach the remote host.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     break;
		case WSAENETRESET:               netLogE(func, "Network dropped connection on reset. The connection has been broken due to keep-alive activity detecting a failure while the operation was in progress. It can also be returned by setsockopt if an attempt is made to set SO_KEEPALIVE on a connection that has already failed.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       break;
		case WSAECONNABORTED:            netLogE(func, "Software caused connection abort. An established connection was aborted by the software in your host computer, possibly due to a data transmission time-out or protocol error.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         break;
		case WSAECONNRESET:              netLogE(func, "Connection reset by peer. An existing connection was forcibly closed by the remote host. This normally results if the peer application on the remote host is suddenly stopped, the host is rebooted, the host or remote network interface is disabled, or the remote host uses a hard close (see setsockopt for more information on the SO_LINGER option on the remote socket). This error may also result if a connection was broken due to keep-alive activity detecting a failure while one or more operations are in progress. Operations that were in progress fail with WSAENETRESET. Subsequent operations fail with WSAECONNRESET.");                                                                                                                                                                                                             break;
		case WSAENOBUFS:                 netLogE(func, "No buffer space available. An operation on a socket could not be performed because the system lacked sufficient buffer space or because a queue was full.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              break;
		case WSAEISCONN:                 netLogE(func, "Socket is already connected. A connect request was made on an already-connected socket. Some implementations also return this error if sendto is called on a connected SOCK_DGRAM socket (for SOCK_STREAM sockets, the to parameter in sendto is ignored) although other implementations treat this as a legal occurrence.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             break;
		case WSAENOTCONN:                netLogE(func, "Socket is not connected. A request to send or receive data was disallowed because the socket is not connected and (when sending on a datagram socket using sendto) no address was supplied. Any other type of operation might also return this error—for example, setsockopt setting SO_KEEPALIVE if the connection has been reset.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    break;
		case WSAESHUTDOWN:               netLogE(func, "Cannot send after socket shutdown. A request to send or receive data was disallowed because the socket had already been shut down in that direction with a previous shutdown call. By calling shutdown a partial close of a socket is requested, which is a signal that sending or receiving, or both have been discontinued.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          break;
		case WSAETOOMANYREFS:            netLogE(func, "Too many references. Too many references to some kernel object.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        break;
		case WSAETIMEDOUT:               netLogE(func, "Connection timed out. A connection attempt failed because the connected party did not properly respond after a period of time, or the established connection failed because the connected host has failed to respond.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  break;
		case WSAECONNREFUSED:            netLogE(func, "Connection refused. No connection could be made because the target computer actively refused it. This usually results from trying to connect to a service that is inactive on the foreign host—that is, one with no server application running.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        break;
		case WSAELOOP:                   netLogE(func, "Cannot translate name. Cannot translate a name.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        break;
		case WSAENAMETOOLONG:            netLogE(func, "Name too long. A name component or a name was too long.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                break;
		case WSAEHOSTDOWN:               netLogE(func, "Host is down. A socket operation failed because the destination host is down. A socket operation encountered a dead host. Networking activity on the local host has not been initiated. These conditions are more likely to be indicated by the error WSAETIMEDOUT.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    break;
		case WSAEHOSTUNREACH:            netLogE(func, "No route to host. A socket operation was attempted to an unreachable host. See WSAENETUNREACH.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         break;
		case WSAENOTEMPTY:               netLogE(func, "Directory not empty. Cannot remove a directory that is not empty.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      break;
		case WSAEPROCLIM:                netLogE(func, "Too many processes. A Windows Sockets implementation may have a limit on the number of applications that can use it simultaneously. WSAStartup may fail with this error if the limit has been reached.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 break;
		case WSAEUSERS:                  netLogE(func, "User quota exceeded. Ran out of user quota.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            break;
		case WSAEDQUOT:                  netLogE(func, "Disk quota exceeded. Ran out of disk quota.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            break;
		case WSAESTALE:                  netLogE(func, "Stale file handle reference. The file handle reference is no longer available.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         break;
		case WSAEREMOTE:                 netLogE(func, "Item is remote. The item is not available locally.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     break;
		case WSASYSNOTREADY:             netLogE(func, "Network subsystem is unavailable. This error is returned by WSAStartup if the Windows Sockets implementation cannot function at this time because the underlying system it uses to provide network services is currently unavailable. Users should check: That the appropriate Windows Sockets DLL file is in the current path. That they are not trying to use more than one Windows Sockets implementation simultaneously. If there is more than one Winsock DLL on your system, be sure the first one in the path is appropriate for the network subsystem currently loaded. The Windows Sockets implementation documentation to be sure all necessary components are currently installed and configured correctly.");                                                                                                                                 break;
		case WSAVERNOTSUPPORTED:         netLogE(func, "Winsock.dll version out of range. The current Windows Sockets implementation does not support the Windows Sockets specification version requested by the application. Check that no old Windows Sockets DLL files are being accessed.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  break;
		case WSANOTINITIALISED:          netLogE(func, "Successful WSAStartup not yet performed. Either the application has not called WSAStartup or WSAStartup failed. The application may be accessing a socket that the current active task does not own (that is, trying to share a socket between tasks), or WSACleanup has been called too many times.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   break;
		case WSAEDISCON:                 netLogE(func, "Graceful shutdown in progress. Returned by WSARecv and WSARecvFrom to indicate that the remote party has initiated a graceful shutdown sequence.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       break;
		case WSAENOMORE:                 netLogE(func, "No more results. No more results can be returned by the WSALookupServiceNext function.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 break;
		case WSAECANCELLED:              netLogE(func, "Call has been canceled. A call to the WSALookupServiceEnd function was made while this call was still processing. The call has been canceled.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          break;
		case WSAEINVALIDPROCTABLE:       netLogE(func, "Procedure call table is invalid. The service provider procedure call table is invalid. A service provider returned a bogus procedure table to Ws2_32.dll. This is usually caused by one or more of the function pointers being NULL.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   break;
		case WSAEINVALIDPROVIDER:        netLogE(func, "Service provider is invalid. The requested service provider is invalid. This error is returned by the WSCGetProviderInfo and WSCGetProviderInfo32 functions if the protocol entry specified could not be found. This error is also returned if the service provider returned a version number other than 2.0.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          break;
		case WSAEPROVIDERFAILEDINIT:     netLogE(func, "Service provider failed to initialize. The requested service provider could not be loaded or initialized. This error is returned if either a service provider's DLL could not be loaded (LoadLibrary failed) or the provider's WSPStartup or NSPStartup function failed.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               break;
		case WSASYSCALLFAILURE:          netLogE(func, "System call failure. A system call that should never fail has failed. This is a generic error code, returned under various conditions. Returned when a system call that should never fail does fail. For example, if a call to WaitForMultipleEvents fails or one of the registry functions fails trying to manipulate the protocol/namespace catalogs. Returned when a provider does not return SUCCESS and does not provide an extended error code. Can indicate a service provider implementation error.");                                                                                                                                                                                                                                                                                                                                            break;
		case WSASERVICE_NOT_FOUND:       netLogE(func, "Service not found. No such service is known. The service cannot be found in the specified name space.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  break;
		case WSATYPE_NOT_FOUND:          netLogE(func, "Class type not found. The specified class was not found.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               break;
		case WSA_E_NO_MORE:              netLogE(func, "No more results. No more results can be returned by the WSALookupServiceNext function.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 break;
		case WSA_E_CANCELLED:            netLogE(func, "Call was canceled. A call to the WSALookupServiceEnd function was made while this call was still processing. The call has been canceled.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               break;
		case WSAEREFUSED:                netLogE(func, "Database query was refused. A database query failed because it was actively refused.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   break;
		case WSAHOST_NOT_FOUND:          netLogE(func, "Host not found. No such host is known. The name is not an official host name or alias, or it cannot be found in the database(s) being queried. This error may also be returned for protocol and service queries, and means that the specified name could not be found in the relevant database.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        break;
		case WSATRY_AGAIN:               netLogE(func, "Nonauthoritative host not found. This is usually a temporary error during host name resolution and means that the local server did not receive a response from an authoritative server. A retry at some time later may be successful.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  break;
		case WSANO_RECOVERY:             netLogE(func, "This is a nonrecoverable error. This indicates that some sort of nonrecoverable error occurred during a database lookup. This may be because the database files (for example, BSD-compatible HOSTS, SERVICES, or PROTOCOLS files) could not be found, or a DNS request was returned by the server with a severe error.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 break;
		case WSANO_DATA:                 netLogE(func, "Valid name, no data record of requested type. The requested name is valid and was found in the database, but it does not have the correct associated data being resolved for. The usual example for this is a host name-to-address translation attempt (using gethostbyname or WSAAsyncGetHostByName) which uses the DNS (Domain Name Server). An MX record is returned but no A record—indicating the host itself exists, but is not directly reachable.");                                                                                                                                                                                                                                                                                                                                                                                              break;
		case WSA_QOS_RECEIVERS:          netLogE(func, "QoS receivers. At least one QoS reserve has arrived.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   break;
		case WSA_QOS_SENDERS:            netLogE(func, "QoS senders. At least one QoS send path has arrived.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   break;
		case WSA_QOS_NO_SENDERS:         netLogE(func, "No QoS senders. There are no QoS senders.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              break;
		case WSA_QOS_NO_RECEIVERS:       netLogE(func, "QoS no receivers. There are no QoS receivers.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          break;
		case WSA_QOS_REQUEST_CONFIRMED:  netLogE(func, "QoS request confirmed. The QoS reserve request has been confirmed.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     break;
		case WSA_QOS_ADMISSION_FAILURE:  netLogE(func, "QoS admission error. A QoS error occurred due to lack of resources.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    break;
		case WSA_QOS_POLICY_FAILURE:     netLogE(func, "QoS policy failure. The QoS request was rejected because the policy system couldn't allocate the requested resource within the existing policy.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        break;
		case WSA_QOS_BAD_STYLE:          netLogE(func, "QoS bad style. An unknown or conflicting QoS style was encountered.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    break;
		case WSA_QOS_BAD_OBJECT:         netLogE(func, "QoS bad object. A problem was encountered with some part of the filterspec or the provider-specific buffer in general.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 break;
		case WSA_QOS_TRAFFIC_CTRL_ERROR: netLogE(func, "QoS traffic control error. An error with the underlying traffic control (TC) API as the generic QoS request was converted for local enforcement by the TC API. This could be due to an out of memory error or to an internal QoS provider error.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       break;
		case WSA_QOS_GENERIC_ERROR:      netLogE(func, "QoS generic error. A general QoS error.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                break;
		case WSA_QOS_ESERVICETYPE:       netLogE(func, "QoS service type error. An invalid or unrecognized service type was found in the QoS flowspec.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         break;
		case WSA_QOS_EFLOWSPEC:          netLogE(func, "QoS flowspec error. An invalid or inconsistent flowspec was found in the QOS structure.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                break;
		case WSA_QOS_EPROVSPECBUF:       netLogE(func, "Invalid QoS provider buffer. An invalid QoS provider-specific buffer.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  break;
		case WSA_QOS_EFILTERSTYLE:       netLogE(func, "Invalid QoS filter style. An invalid QoS filter style was used.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        break;
		case WSA_QOS_EFILTERTYPE:        netLogE(func, "Invalid QoS filter type. An invalid QoS filter type was used.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          break;
		case WSA_QOS_EFILTERCOUNT:       netLogE(func, "Incorrect QoS filter count. An incorrect number of QoS FILTERSPECs were specified in the FLOWDESCRIPTOR.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               break;
		case WSA_QOS_EOBJLENGTH:         netLogE(func, "Invalid QoS object length. An object with an invalid ObjectLength field was specified in the QoS provider-specific buffer.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             break;
		case WSA_QOS_EFLOWCOUNT:         netLogE(func, "Incorrect QoS flow count. An incorrect number of flow descriptors was specified in the QoS structure.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  break;
		case WSA_QOS_EUNKOWNPSOBJ:       netLogE(func, "Unrecognized QoS object. An unrecognized object was found in the QoS provider-specific buffer.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         break;
		case WSA_QOS_EPOLICYOBJ:         netLogE(func, "Invalid QoS policy object. An invalid policy object was found in the QoS provider-specific buffer.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     break;
		case WSA_QOS_EFLOWDESC:          netLogE(func, "Invalid QoS flow descriptor. An invalid QoS flow descriptor was found in the flow descriptor list.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     break;
		case WSA_QOS_EPSFLOWSPEC:        netLogE(func, "Invalid QoS provider-specific flowspec. An invalid or inconsistent flowspec was found in the QoS provider-specific buffer.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             break;
		case WSA_QOS_EPSFILTERSPEC:      netLogE(func, "Invalid QoS provider-specific filterspec. An invalid FILTERSPEC was found in the QoS provider-specific buffer.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         break;
		case WSA_QOS_ESDMODEOBJ:         netLogE(func, "Invalid QoS shape discard mode object. An invalid shape discard mode object was found in the QoS provider-specific buffer.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             break;
		case WSA_QOS_ESHAPERATEOBJ:      netLogE(func, "Invalid QoS shaping rate object. An invalid shaping rate object was found in the QoS provider-specific buffer.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         break;
		case WSA_QOS_RESERVED_PETYPE:    netLogE(func, "Reserved policy QoS element type. A reserved policy element was found in the QoS provider-specific buffer.");                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             break;
	}
}

void net_init(){
	DeshiStageInitStart(DS_NETWORK, DS_LOGGER | DS_PLATFORM, "Attempt to initialize Networking Module before initializing dependencies");
	
	WSADATA data;
	if(WSAStartup(MAKEWORD(2,2), &data)) return WSA_log_error(STR8("net_init()"));
	
	DeshiStageInitEnd(DS_NETWORK);
}

void net_deinit(){
	if(WSACleanup()) return WSA_log_error(STR8("net_deinit()"));
}

u64 net_socket_open(netSocket* sock, u8 port, b32 non_blocking){
	if(!sock) return netLogE("net_socket_open()", "sock parameter was null"), 1;
	
	//get handle to sock from win32
	sock->handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(sock->handle == INVALID_SOCKET) return WSA_log_error(STR8("net_socket_open()->sock()")), 1;
	
	//bind sock to port
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port); //htons converts the u8 from host byte order to network byte order
	addr.sin_addr.s_addr = INADDR_ANY;
	if(bind(sock->handle, (sockaddr*)&addr, sizeof(sockaddr_in))) return WSA_log_error(STR8("net_socket_open()->bind()")), 1;
	
	//set sock blocking
	if(ioctlsocket(sock->handle, FIONBIO, (u_long*)&non_blocking)) return WSA_log_error(STR8("net_socket_open()->ioctlsocket()")), 1;
	
	sock->blocking = !non_blocking;
	
	return 0;
}

void net_socket_close(netSocket* socket){
	if(!socket) return;
	if(socket->handle) closesocket(socket->handle);
}

u64 net_socket_send(netSocket* socket, netAddress destination, void* data, s32 size){
	if(!socket) return LogE("net_socket_send()","socket parameter was null"), 1;
	
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(destination.port); //htons converts the u8 from host byte order to network byte order
	addr.sin_addr.s_addr = destination.host;
	
	s32 sent_bytes = sendto(socket->handle, (char*)data, size, 0, (sockaddr*)&addr, sizeof(sockaddr_in));
	if(sent_bytes == SOCKET_ERROR) return WSA_log_error(STR8("net_socket_send()")), -1;
	if(sent_bytes != size) netLogE("net_socket_send()", "Failed to fully send data");
	
	return sent_bytes;
}

u64 net_socket_recv(netSocket* socket, netAddress* sender, void* data, s32 size){
	if(!socket) return LogE("net_socket_recv()","socket parameter was null"), 1;
	sockaddr_in	from;
	s32 fromsize = sizeof(sockaddr_in); //so retarded
	
	s32 recieved_bytes = recvfrom(socket->handle, (char*)data, size, 0, (sockaddr*)&from, &fromsize);
	if(recieved_bytes == SOCKET_ERROR){
		//catch WOULDBLOCK error if thats the problem and return 0
		if(WSAGetLastError() == WSAEWOULDBLOCK){
			if(sender){
				sender->host = 0;
				sender->port = 0;
			}
			return 0;
		}
		return WSA_log_error(STR8("net_socket_recv()")), -1;
	} 
	
	if(sender){
		sender->host = from.sin_addr.s_addr;
		sender->port = ntohs(from.sin_port);
	}
	return recieved_bytes;
}

//from https://stackoverflow.com/questions/1824279/how-to-get-ip-address-from-sockaddr
//so from http://beej.us/guide/bgnet/examples/client.c
void* getinaddr(sockaddr* sa){
	if (sa->sa_family == AF_INET) return &(((struct sockaddr_in*)sa)->sin_addr);
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

u64 net_address_init(netAddress* address, str8 host, str8 port){
	if(host.str==0) address->host = INADDR_ANY;
	else{
		addrinfo* result = 0, * ptr = 0, hints = {0};
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_protocol = IPPROTO_UDP;
		if(getaddrinfo((PCSTR)host.str, (PCSTR)port.str, &hints, &result)) return WSA_log_error(STR8("net_address_init()")), 1;
		ptr = result; //NOTE(sushi) not sure yet when this would error or when we would need to go through the linked list
		address->host = ((sockaddr_in*)ptr)->sin_addr.s_addr;
		freeaddrinfo(result);
	}	
	address->port = (u8)stoi((char*)host.str); //probably safe :)
	return 0;
}

str8 net_address_str(netAddress address, b32 incl_port){
	str8 out = {(u8*)memtalloc(21), 21}; 
	inet_ntop(AF_INET, &address.host, (PSTR)out.str, out.count);
	if(incl_port){
		string c = to_string(address.port, deshi_allocator);
		u8* p = out.str;
		while(*p++ != 0);
		memcpy(p, c.str, 5);
	}
	return out;
}