#pragma once
#ifndef DESHI_WINDOW_H
#define DESHI_WINDOW_H

#include "../defines.h"
#include "../math/vector.h"

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
}; typedef u32 CursorType;

constexpr u32 max_child_windows = 2;
struct Window{
	string name;
	GLFWwindow*  window;
	GLFWmonitor* monitor;

	void* handle = 0; //win32: HWND; linux/mac not implemented
	void* instance = 0; //win32: HINSTANCE; linux/mac not implemented
	
	Window* children[max_child_windows];
	u32 child_count = 0;

	s32 x, y;
	s32 width, height;
	s32 screenWidth, screenHeight;
	s32 restoreX, restoreY;
	s32 restoreW, restoreH;
	s32 centerX, centerY;
	s32 refreshRate, screenRefreshRate; //TODO(delle,Wi) add selecting the refresh rate
	DisplayMode displayMode;
	CursorMode cursorMode;
	b32 minimized;
	b32 rawInput;
	b32 resizable;
	b32 closeWindow;

	u32 titlebarheight = 20;
	
	b32  resized;
	
	vec2 dimensions;
	
	//NOTE(delle) vsync isnt handled in GLFW when using vulkan
	void    Init(const char* name, s32 width, s32 height, s32 x = 0xFFFFFFFF, s32 y = 0xFFFFFFFF, DisplayMode displayMode = DisplayMode_Windowed);
	Window* MakeChild(const char* name, s32 width, s32 height, s32 x = 0, s32 y = 0);
	void    Update();
	void    Cleanup();
	void    UpdateDisplayMode(DisplayMode mode);
	void    UpdateCursorMode(CursorMode mode); 
	void    SetCursorPos(vec2 pos);
	void    SetCursor(CursorType curtype);
	void    UpdateRawInput(b32 rawInput);
	void    UpdateResizable(b32 resizable);
	void    GetScreenSize(s32& width, s32& height);
	void    Close();
	void    UpdateTitle(const char* title);
	void    ShowWindow(u32 child = -1);
	void    HideWindow(u32 child = -1);
	void    CloseConsole();
	b32     ShouldClose();

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