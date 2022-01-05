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

struct Window{
	const char* name;
	GLFWwindow*  window;
	GLFWmonitor* monitor;
	
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
	
	b32  resized;
	
	vec2 dimensions;
	
	//NOTE(delle) vsync isnt handled in GLFW when using vulkan
	void Init(const char* name, s32 width, s32 height, s32 x = 0xFFFFFFFF, s32 y = 0xFFFFFFFF, DisplayMode displayMode = DisplayMode_Windowed);
	void Update();
	void Cleanup();
	void UpdateDisplayMode(DisplayMode mode);
	void UpdateCursorMode(CursorMode mode); 
	void SetCursorPos(vec2 pos);
	void SetCursor(CursorType curtype);
	void UpdateRawInput(b32 rawInput);
	void UpdateResizable(b32 resizable);
	void Close();
	void UpdateTitle(const char* title);
	void ShowWindow();
	void HideWindow();
	b32  ShouldClose();
};

//global_ window pointer
extern Window* g_window;
#define DeshWindow g_window
#define DeshWinSize g_window->dimensions

#endif //DESHI_WINDOW_H