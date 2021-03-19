#pragma once
#ifndef DESHI_WINDOW_H
#define DESHI_WINDOW_H

#include "../utils/defines.h"
#include "../math/Vector2.h"

struct Input;
struct GLFWwindow;
struct GLFWmonitor;

enum struct DisplayMode{
	WINDOWED, BORDERLESS_WINDOWED, FULLSCREEN
};

//TODO(,delle) add window title updating
struct Window{
	GLFWwindow* window;
	GLFWmonitor* monitor;
	inline static Input* input; //TODO(c,delle) find a way to not do this static
	
	int32 x, y;
	int32 width, height;
	int32 screenWidth, screenHeight;
	//TODO(r,delle) add resolution option
	int32 screenRefreshRate;
	DisplayMode displayMode;
	bool minimized;
	
	Vector2 dimensions;
	
	//TODO(c,delle) vsync isnt handled in GLFW when using vulkan, handle other renderers when necessary
	//thanks: https://github.com/OneLoneCoder/olcPixelGameEngine/pull/181
	void Init(Input* input, int32 width, int32 height, int32 x = 0, int32 y = 0,  
			  DisplayMode displayMode = DisplayMode::WINDOWED);
	
	void Update();
	
	void Cleanup();
	
	void UpdateDisplayMode(DisplayMode displayMode);
};

#endif //DESHI_WINDOW_H