#pragma once
#ifndef DESHI_WINDOW_H
#define DESHI_WINDOW_H

#include "../defines.h"
#include "../math/Vector.h"

#include <iosfwd> //forward delcare: <string>

struct Input;
struct GLFWwindow;
struct GLFWmonitor;

enum struct DisplayMode{
	WINDOWED, BORDERLESS /*borderless windowed*/, FULLSCREEN
};

enum struct CursorMode{
	DEFAULT, FIRSTPERSON, HIDDEN
};

//TODO(delle,Wi) add window title updating
struct Window{
	GLFWwindow* window;
	GLFWmonitor* monitor;
	
	inline static Input* input; //TODO(delle,Cl) find a way to not do this static
	
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
	
	b32  resized;
	
	Vector2 dimensions;
	
	//NOTE(delle) vsync isnt handled in GLFW when using vulkan
	void Init(Input* input, s32 width, s32 height, s32 x = 0, s32 y = 0,  
			  DisplayMode displayMode = DisplayMode::WINDOWED);
	void Update();
	void Cleanup();
	void UpdateDisplayMode(DisplayMode mode);
	void UpdateCursorMode(CursorMode mode); 
	void SetCursorPos(Vector2 pos);
	void UpdateRawInput(bool rawInput);
	void UpdateResizable(bool resizable);
	void Close();
	
	std::string str();
};

//global window pointer
extern Window* g_window;
#define DengWindow g_window

#endif //DESHI_WINDOW_H