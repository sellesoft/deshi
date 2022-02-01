#pragma once
#ifndef DESHI_WINDOW_H
#define DESHI_WINDOW_H

#include "../defines.h"
#include "../math/vector.h"
#include "../utils/string.h"

#ifdef DESHI_PROFILE_WINDOW
#define DPWinFrameMark DPFrameMark
#define DPWinZoneScoped DPZoneScoped
#define DPWinZoneScopedN(name) DPZoneScopedN(name)
//TODO continue this idea for this and other modules 
#endif

struct Input;
struct GLFWwindow;
struct GLFWmonitor;
struct GLFWcursor;

enum DisplayMode_{
	DisplayMode_Windowed, 
	DisplayMode_Borderless, 
	DisplayMode_BorderlessFullscreen,
	DisplayMode_Fullscreen
}; typedef u32 DisplayMode;

enum Decoration_ {
	Decoration_Titlebar          = 1 << 0, // full titlebar
	Decoration_TitlebarClose     = 1 << 1, // close button	  
	Decoration_TitlebarMaximize  = 1 << 2, // maxmize button  
	Decoration_TitlebarMinimize  = 1 << 3, // minimize button 
	Decoration_TitlebarTitle     = 1 << 4, // title of window in title bar
	Decoration_TitlebarFull      = Decoration_TitlebarTitle | Decoration_TitlebarMinimize | Decoration_TitlebarMaximize | Decoration_TitlebarClose | Decoration_Titlebar,
	Decoration_MinimalTitlebar   = 1 << 5, // thin titlebar that just moves the window, it cant have listed above
	Decoration_Borders           = 1 << 6,
	Decoration_MouseBorders      = 1 << 7, // only displays borders when the mouse gets close to them for resizing
	Decoration_SystemDecorations = 0xFFFFFFFF
}; typedef u32 Decoration;

enum HitTest : u32 {
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

enum CursorMode_{
	CursorMode_Default, 
	CursorMode_FirstPerson, 
	CursorMode_Hidden,
}; typedef u32 CursorMode;

enum CursorType_ {
	CursorType_Arrow,
	CursorType_HResize,
	CursorType_VResize,
	CursorType_RightDiagResize,
	CursorType_LeftDiagResize,
	CursorType_Hand,
	CursorType_IBeam,
	CursorType_Hidden,
}; typedef u32 CursorType;

constexpr u32 max_child_windows = 2;
struct Window{
	string name;
	GLFWwindow*  window;
	GLFWmonitor* monitor;
	
	void* handle = 0; //win32: HWND; linux/mac not implemented
	void* instance = 0; //win32: HINSTANCE; linux/mac not implemented
	void* dc = 0; //win32: HDC; linux/mac not implemented
	
	Window* children[max_child_windows];
	u32 child_count = 0;
	u32 renderer_surface_index = -1;
	
	//TODO add support for colored decor
	Decoration decorations;
	
	//this is only set in the main window, child windows can not set this, and so it is only accessible on the main window's pointer
	Window* activeWindow = 0;
	
	HitTest hittest = HitTestNone;
	
	s32 x, y;
	s32 width, height;
	s32 cx, cy;          //position of client area in window's coordinates
	s32 cwidth, cheight; //size of client area
	s32 screenWidth, screenHeight;
	s32 restoreX, restoreY;
	s32 restoreW, restoreH;
	s32 centerX, centerY;
	s32 refreshRate, screenRefreshRate; //TODO(delle,Wi) add selecting the refresh rate
	DisplayMode displayMode;
	CursorMode cursorMode;
	
	b32 active;
	b32 resized;
	b32 minimized;
	b32 rawInput;
	b32 resizable;
	b32 closeWindow;
	
	s32 titlebarheight = 0;
	s32 borderthickness = 0;
	
	vec2 dimensions;
	
	void    Init(const char* name, s32 width, s32 height, s32 x = 0xFFFFFFFF, s32 y = 0xFFFFFFFF, DisplayMode displayMode = DisplayMode_Windowed);
	Window* MakeChild(const char* name, s32 width, s32 height, s32 x = 0xFFFFFFFF, s32 y = 0xFFFFFFFF);
	
	void    Update();
	void    Cleanup();
	void    Close();
	b32     ShouldClose();
	
	void    UpdateDisplayMode(DisplayMode mode);
	void    UpdateDecorations(Decoration decorations);
	void    UpdateResizable(b32 resizable);
	void    GetScreenSize(s32& width, s32& height);
	void    GetWindowSize(s32& width, s32& height);
	void    GetClientSize(s32& width, s32& height);
	vec2    GetClientAreaPosition();
	vec2    GetClientAreaDimensions();
	void    ShowWindow(u32 child = -1);
	void    HideWindow(u32 child = -1);
	void    UpdateTitle(const char* title);
	void    CloseConsole();
	void    SwapBuffers();
	
	// cursor and input
	void    UpdateRawInput(b32 rawInput);
	void    UpdateCursorMode(CursorMode mode); 
	void    SetCursor(CursorType type);
	void    SetCursorPos(f64 x, f64 y);
	FORCE_INLINE void SetCursorPos(vec2 pos){ SetCursorPos(pos.x, pos.y); }
	void    SetCursorPosScreen(f64 x, f64 y);
	FORCE_INLINE void SetCursorPosScreen(vec2 pos){ SetCursorPosScreen(pos.x, pos.y); }
	
	//define custom cursors
	//maybe could put this somewhere else bc it will make the window struct very large
	private:
	u32 W = 0xffffffff;
	u32 B = 0xff000000;
	public:
	u32 defaultcur[16 * 16] = {
		B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		B,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,
		B,W,W,B,0,0,0,0,0,0,0,0,0,0,0,0,
		B,W,W,W,B,0,0,0,0,0,0,0,0,0,0,0,
		B,W,W,W,W,B,0,0,0,0,0,0,0,0,0,0,
		B,W,W,W,W,W,B,0,0,0,0,0,0,0,0,0,
		B,W,W,W,W,W,W,B,0,0,0,0,0,0,0,0,
		B,W,W,B,W,B,B,B,B,0,0,0,0,0,0,0,
		B,W,B,B,W,B,0,0,0,0,0,0,0,0,0,0,
		B,B,0,B,W,W,B,0,0,0,0,0,0,0,0,0,
		B,0,0,0,B,W,B,0,0,0,0,0,0,0,0,0,
		0,0,0,0,B,W,W,B,0,0,0,0,0,0,0,0,
		0,0,0,0,0,B,W,B,0,0,0,0,0,0,0,0,
		0,0,0,0,0,B,W,W,B,0,0,0,0,0,0,0,
		0,0,0,0,0,0,B,B,B,0,0,0,0,0,0,0,
		
	};
	
	u32 hresizecur[16 * 16] = {
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,B,0,0,0,0,0,0,B,0,0,0,0,
		0,0,0,B,B,0,0,0,0,0,0,B,B,0,0,0,
		0,0,B,W,B,0,0,0,0,0,0,B,W,B,0,0,
		0,B,W,W,B,B,B,B,B,B,B,B,W,W,B,0,
		B,W,W,W,W,W,W,W,W,W,W,W,W,W,W,B,
		B,W,W,W,W,W,W,W,W,W,W,W,W,W,W,B,
		0,B,W,W,B,B,B,B,B,B,B,B,W,W,B,0,
		0,0,B,W,B,0,0,0,0,0,0,B,W,B,0,0,
		0,0,0,B,B,0,0,0,0,0,0,B,B,0,0,0,
		0,0,0,0,B,0,0,0,0,0,0,B,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	};
	
	u32 vresizecur[16 * 16] = {
		0,0,0,0,0,0,0,B,B,0,0,0,0,0,0,0,
		0,0,0,0,0,0,B,W,W,B,0,0,0,0,0,0,
		0,0,0,0,0,B,W,W,W,W,B,0,0,0,0,0,
		0,0,0,0,B,W,W,W,W,W,W,B,0,0,0,0,
		0,0,0,B,B,B,B,W,W,B,B,B,B,0,0,0,
		0,0,0,0,0,0,B,W,W,B,0,0,0,0,0,0,
		0,0,0,0,0,0,B,W,W,B,0,0,0,0,0,0,
		0,0,0,0,0,0,B,W,W,B,0,0,0,0,0,0,
		0,0,0,0,0,0,B,W,W,B,0,0,0,0,0,0,
		0,0,0,0,0,0,B,W,W,B,0,0,0,0,0,0,
		0,0,0,0,0,0,B,W,W,B,0,0,0,0,0,0,
		0,0,0,B,B,B,B,W,W,B,B,B,B,0,0,0,
		0,0,0,0,B,W,W,W,W,W,W,B,0,0,0,0,
		0,0,0,0,0,B,W,W,W,W,B,0,0,0,0,0,
		0,0,0,0,0,0,B,W,W,B,0,0,0,0,0,0,
		0,0,0,0,0,0,0,B,B,0,0,0,0,0,0,0,
	};
	
	u32 rightdiagresizecur[16 * 16] = {
		0,0,0,0,0,0,0,0,0,0,B,B,B,B,B,B,
		0,0,0,0,0,0,0,0,0,0,B,W,W,W,W,B,
		0,0,0,0,0,0,0,0,0,0,0,B,W,W,W,B,
		0,0,0,0,0,0,0,0,0,0,B,W,W,W,W,B,
		0,0,0,0,0,0,0,0,0,B,W,W,W,B,W,B,
		0,0,0,0,0,0,0,0,B,W,W,W,B,0,B,B,
		0,0,0,0,0,0,0,B,W,W,W,B,0,0,0,0,
		0,0,0,0,0,0,B,W,W,W,B,0,0,0,0,0,
		0,0,0,0,0,B,W,W,W,B,0,0,0,0,0,0,
		0,0,0,0,B,W,W,W,B,0,0,0,0,0,0,0,
		B,B,0,B,W,W,W,B,0,0,0,0,0,0,0,0,
		B,W,B,W,W,W,B,0,0,0,0,0,0,0,0,0,
		B,W,W,W,W,B,0,0,0,0,0,0,0,0,0,0,
		B,W,W,W,B,0,0,0,0,0,0,0,0,0,0,0,
		B,W,W,W,W,B,0,0,0,0,0,0,0,0,0,0,
		B,B,B,B,B,B,0,0,0,0,0,0,0,0,0,0,
	};
	
	u32 leftdiagresizecur[16 * 16] = {
		B,B,B,B,B,B,0,0,0,0,0,0,0,0,0,0,
		B,W,W,W,W,B,0,0,0,0,0,0,0,0,0,0,
		B,W,W,W,B,0,0,0,0,0,0,0,0,0,0,0,
		B,W,W,W,W,B,0,0,0,0,0,0,0,0,0,0,
		B,W,B,W,W,W,B,0,0,0,0,0,0,0,0,0,
		B,B,0,B,W,W,W,B,0,0,0,0,0,0,0,0,
		0,0,0,0,B,W,W,W,B,0,0,0,0,0,0,0,
		0,0,0,0,0,B,W,W,W,B,0,0,0,0,0,0,
		0,0,0,0,0,0,B,W,W,W,B,0,0,0,0,0,
		0,0,0,0,0,0,0,B,W,W,W,B,0,0,0,0,
		0,0,0,0,0,0,0,0,B,W,W,W,B,0,B,B,
		0,0,0,0,0,0,0,0,0,B,W,W,W,B,W,B,
		0,0,0,0,0,0,0,0,0,0,B,W,W,W,W,B,
		0,0,0,0,0,0,0,0,0,0,0,B,W,W,W,B,
		0,0,0,0,0,0,0,0,0,0,B,W,W,W,W,B,
		0,0,0,0,0,0,0,0,0,0,B,B,B,B,B,B,
	};
	
	u32 handcur[16 * 16] = {
		0,0,0,0,0,0,0,0,0,B,B,0,0,0,0,0,
		0,0,0,0,0,0,B,B,B,W,W,B,B,0,0,0,
		0,0,0,0,0,B,W,W,B,W,W,B,W,B,0,0,
		0,0,0,0,0,B,W,W,B,W,W,B,W,B,B,0,
		0,0,0,0,0,B,W,W,B,W,W,B,W,B,W,B,
		0,0,0,0,0,B,W,W,B,W,W,B,W,B,W,B,
		0,B,B,B,0,B,W,W,B,W,W,B,W,B,W,B,
		0,B,W,W,B,B,W,W,B,W,W,B,W,B,W,B,
		0,B,W,W,W,B,W,W,W,W,W,W,W,W,W,B,
		0,0,B,W,W,W,W,W,W,W,W,W,W,W,W,B,
		0,0,B,W,W,W,W,W,W,W,W,W,W,W,B,0,
		0,0,0,B,W,W,W,W,W,W,W,W,W,W,B,0,
		0,0,0,B,W,W,W,W,W,W,W,W,W,B,0,0,
		0,0,0,0,B,W,W,W,W,W,W,W,W,B,0,0,
		0,0,0,0,B,W,W,W,W,W,W,W,B,0,0,0,
		0,0,0,0,0,B,B,B,B,B,B,B,0,0,0,0,
	};
	
	u32 textcur[16 * 16] = {
		0,0,0,B,B,B,B,0,B,B,B,B,0,0,0,0,
		0,0,B,W,W,W,W,B,W,W,W,W,B,0,0,0,
		0,0,0,B,B,B,W,W,W,B,B,B,0,0,0,0,
		0,0,0,0,0,0,B,W,B,0,0,0,0,0,0,0,
		0,0,0,0,0,0,B,W,B,0,0,0,0,0,0,0,
		0,0,0,0,0,0,B,W,B,0,0,0,0,0,0,0,
		0,0,0,0,0,0,B,W,B,0,0,0,0,0,0,0,
		0,0,0,0,0,0,B,W,B,0,0,0,0,0,0,0,
		0,0,0,0,0,0,B,W,B,0,0,0,0,0,0,0,
		0,0,0,0,0,0,B,W,B,0,0,0,0,0,0,0,
		0,0,0,0,0,0,B,W,B,0,0,0,0,0,0,0,
		0,0,0,0,0,0,B,W,B,0,0,0,0,0,0,0,
		0,0,0,0,0,0,B,W,B,0,0,0,0,0,0,0,
		0,0,0,B,B,B,W,W,W,B,B,B,0,0,0,0,
		0,0,B,W,W,W,W,B,W,W,W,W,B,0,0,0,
		0,0,0,B,B,B,B,0,B,B,B,B,0,0,0,0,
	};
	
};

//global_ window pointer
extern Window* g_window;
extern Window* g_window2;
#define DeshWindow g_window
#define DeshWindow2 g_window2 //make better names later
#define DeshWinSize g_window->dimensions
#define DeshWin2Size g_window2->dimensions
#endif //DESHI_WINDOW_H