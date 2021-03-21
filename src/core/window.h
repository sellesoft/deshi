#pragma once
#ifndef DESHI_WINDOW_H
#define DESHI_WINDOW_H

#include "../utils/defines.h"
#include "../math/Vector2.h"

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
	
	i32 x, y;
	i32 width, height;
	i32 screenWidth, screenHeight;
	i32 restoreX, restoreY;
	i32 restoreW, restoreH;
	i32 centerX, centerY;
	i32 refreshRate, screenRefreshRate; //TODO(delle,Wi) add selecting the refresh rate
	DisplayMode displayMode;
	CursorMode cursorMode;
	bool minimized;
	bool rawInput;
	bool resizable;
	
	Vector2 dimensions;
	
	//TODO(delle,Cl) vsync isnt handled in GLFW when using vulkan, handle other renderers when necessary
	//thanks: https://github.com/OneLoneCoder/olcPixelGameEngine/pull/181
	void Init(Input* input, i32 width, i32 height, i32 x = 0, i32 y = 0,  
			  DisplayMode displayMode = DisplayMode::WINDOWED);
	
	void Update();
	
	void Cleanup();
	
	void UpdateDisplayMode(DisplayMode mode);
	
	void UpdateCursorMode(CursorMode mode); 
	
	void UpdateRawInput(bool rawInput);
	
	void UpdateResizable(bool resizable);
};

#endif //DESHI_WINDOW_H