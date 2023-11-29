/* deshi Window Module
Notes:

Index:
@window_types
  DisplayMode
  Decoration
  HitTest
  CursorMode
  CursorType
  Window
@window_management
  window_create(str8 title, s32 width, s32 height, s32 x, s32 y, DisplayMode display_mode) -> Window*
  window_update(Window* window) -> void
  window_close(Window* window) -> void
  window_swap_buffers(Window* window) -> void
@window_visuals
  window_display_mode(Window* window, DisplayMode mode) -> void
  window_decorations(Window* window, Decoration decorations) -> void
  window_show(Window* window) -> void
  window_hide(Window* window) -> void
  window_set_title(str8 title) -> void
@window_cursor
  window_set_cursor_mode(Window* window, CursorMode mode) -> void
  window_set_cursor_type(Window* window, CursorType type) -> void
  window_set_cursor_position(Window* window, s32 x, s32 y) -> void
@window_shared_variables

TODOs:
[05/17/22,EASY,Feature] add window icon modification
[05/17/22,EASY,Feature] add cursor icons modification
[05/18/22,MEDI,Feature] add custom titlebar and decorations
[05/18/22,MEDI,Feature] add monitor selection for fullscreen
[05/18/22,MEDI,Feature] add children windows
[05/18/22,MEDI,Feature] add custom resolution and refresh rate
*/

#ifndef DESHI_WINDOW_H
#define DESHI_WINDOW_H
#include "kigu/common.h"
#include "kigu/unicode.h"
#include "math/vector.h"

struct RenderPass;

#ifdef DESHI_PROFILE_WINDOW
#  define DPWinFrameMark DPFrameMark
#  define DPWinZoneScoped DPZoneScoped
#  define DPWinZoneScopedN(name) DPZoneScopedN(name)
//TODO continue this idea for this and other modules 
#endif

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @window_types
typedef Type DisplayMode; enum{
	DisplayMode_Windowed,
	DisplayMode_Borderless,
	DisplayMode_BorderlessMaximized,
	DisplayMode_Fullscreen,
};

typedef Flags Decoration; enum{
	Decoration_SystemDecorations = 0,
	//Decoration_Titlebar          = 1 << 0, // full titlebar
	//Decoration_TitlebarClose     = 1 << 1, // close button
	//Decoration_TitlebarMaximize  = 1 << 2, // maxmize button
	//Decoration_TitlebarMinimize  = 1 << 3, // minimize button
	//Decoration_TitlebarTitle     = 1 << 4, // title of window in title bar
	//Decoration_MinimalTitlebar   = 1 << 5, // thin titlebar that just moves the window, it cant have listed above
	//Decoration_Borders           = 1 << 6,
	//Decoration_MouseBorders      = 1 << 7, // only displays borders when the mouse gets close to them for resizing
	//Decoration_TitlebarFull      = Decoration_TitlebarTitle | Decoration_TitlebarMinimize | Decoration_TitlebarMaximize | Decoration_TitlebarClose | Decoration_Titlebar,
};

typedef Type HitTest; enum{
	HitTestNone,
	HitTestTop,
	HitTestBottom,
	HitTestLeft,
	HitTestRight,
	HitTestTopRight,
	HitTestTopLeft,
	HitTestBottomRight,
	HitTestBottomLeft,
	HitTestTitle,
	HitTestClient
};

typedef Type CursorMode; enum{
	CursorMode_Default, 
	CursorMode_FirstPerson, 
};

typedef Type CursorType; enum{
	CursorType_Arrow,
	CursorType_HResize,
	CursorType_VResize,
	CursorType_RightDiagResize,
	CursorType_LeftDiagResize,
	CursorType_Hand,
	CursorType_IBeam,
	CursorType_Hidden,
};

struct Window{
	u32 index;
	str8 title;
	
	void* handle; //win32: HWND; linux: X11::Window; mac not implemented
	void* context; //win32: HDC; linux: X11::GC; linux/mac not implemented
	
	// opaque handle to the info needed by the current render backend 
	// to properly draw things to this window
	void* render_info;
	
	union{
		vec2i position;
		struct{ s32 x, y; };
	};
	union{
		vec2i dimensions;
		struct{ s32 width, height; };
	};
	
	struct{
		union{
			vec2i position;
			struct{ s32 x, y; };
		};
		union{
			vec2i dimensions;
			struct{ s32 width, height; };
		};
	}restore;
	
	vec2i position_decorated;
	vec2i dimensions_decorated;
	vec2i center;
	
	DisplayMode display_mode;
	CursorMode cursor_mode;
	
	b32 focused;
	b32 resized;
	b32 minimized;
	b32 close_window;
	
	Decoration decorations;
	//HitTest hit_test;
	//s32 titlebar_height;
	//s32 border_thickness;
};

//global window pointer
extern Window* g_window;
#define DeshWindow g_window


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @window_management
//Creates an `OS` window with the specified values
//The first time this function is called, g_window (aka DeshWindow, the global window pointer), is set to the created window
external Window* window_create(str8 title, s32 width = 0xFFFFFFFF, s32 height = 0xFFFFFFFF, s32 x = 0xFFFFFFFF, s32 y = 0xFFFFFFFF, DisplayMode display_mode = DisplayMode_Windowed, Decoration decorations = Decoration_SystemDecorations);
FORCE_INLINE Window* window_create(const char* title, s32 width = 0xFFFFFFFF, s32 height = 0xFFFFFFFF, s32 x = 0xFFFFFFFF, s32 y = 0xFFFFFFFF, DisplayMode display_mode = DisplayMode_Windowed, Decoration decorations = Decoration_SystemDecorations){ return window_create(str8{(u8*)title,(s64)strlen(title)},width,height,x,y,display_mode,decorations); }

//Closes and deletes the `window` (exits the application if this this is the last open window)
external void window_close(Window* window);


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @window_visuals
//Updates the `DisplayMode` of `window` to `mode`
external void window_display_mode(Window* window, DisplayMode mode);

//Updates the `Decoration` of `window` to `decorations`
//external void window_decorations(Window* window, Decoration decorations);

//Shows the `window` and adds it to the `OS` taskbar
external void window_show(Window* window);

//Hides the `window` and removes it from the `OS` taskbar
external void window_hide(Window* window);

//Updates the title of the `window` to `title`
external void window_set_title(Window* window, str8 title);
FORCE_INLINE void window_set_title(Window* window, const char* title){ window_set_title(window, str8{(u8*)title, (s64)strlen(title)}); }


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @window_cursor
//Sets the `CursorMode` of `window` to `mode`
external void window_set_cursor_mode(Window* window, CursorMode mode);

//Sets the `CursorType` of `window` to `type`
external void window_set_cursor_type(Window* window, CursorType type);

//Sets the cursor position in `Client Space` of `window`
external void window_set_cursor_position(Window* window, s32 x, s32 y);
FORCE_INLINE void window_set_cursor_position(Window* window, vec2i position){ window_set_cursor_position(window, position.x, position.y); }
FORCE_INLINE void window_set_cursor_position(Window* window, vec2 position){ window_set_cursor_position(window, (s32)position.x, (s32)position.y); }


#endif //DESHI_WINDOW_H
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#if defined(DESHI_IMPLEMENTATION) && !defined(DESHI_WINDOW_IMPL)
#define DESHI_WINDOW_IMPL
#include "kigu/arrayT.h"

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @window_shared_variables
local Window window_helper{};
local arrayT<Window*> window_windows;
local Window* window_active;

#endif //DESHI_IMPLEMENTATION
