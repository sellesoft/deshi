#pragma once
#ifndef DESHI_WINDOW_H
#define DESHI_WINDOW_H

#include "../defines.h"
#include "../math/vec.h"

#include <string>

struct Input;
struct GLFWwindow;
struct GLFWmonitor;
struct GLFWcursor;

enum struct DisplayMode{
	WINDOWED, BORDERLESS /*borderless windowed*/, FULLSCREEN
};

enum struct CursorMode{
	DEFAULT, FIRSTPERSON, HIDDEN
};

//TODO(delle,Wi) add window title updating
struct Window{
	GLFWwindow*  window;
	GLFWmonitor* monitor;
	GLFWcursor*  cursor;
	
	s32 x, y;
	s32 width, height;
	s32 screenWidth, screenHeight;
	s32 restoreX, restoreY;
	s32 restoreW, restoreH;
	s32 centerX, centerY;
	s32 refreshRate, screenRefreshRate; //TODO(delle,Wi) add selecting the refresh rate
	DisplayMode displayMode;
	CursorMode cursorMode;
	bool minimized;
	bool rawInput;
	bool resizable;
	bool closeWindow;
	
	bool  resized;
	
	vec2 dimensions;
	
	//NOTE(delle) vsync isnt handled in GLFW when using vulkan
	void Init(s32 width, s32 height, s32 x = 0xFFFFFFFF, s32 y = 0xFFFFFFFF, DisplayMode displayMode = DisplayMode::WINDOWED);
	void Update();
	void Cleanup();
	void UpdateDisplayMode(DisplayMode mode);
	void UpdateCursorMode(CursorMode mode); 
	void SetCursorPos(vec2 pos);
	void UpdateRawInput(bool rawInput);
	void UpdateResizable(bool resizable);
	void Close();
	void UpdateTitle(const char* title);
	
	std::string str();
};

//global_ window pointer
extern Window* g_window;
#define DeshWindow g_window

#endif //DESHI_WINDOW_H