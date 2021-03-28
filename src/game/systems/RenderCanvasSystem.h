#pragma once
#include "System.h"

struct RenderCanvasSystem : public System {

	bool showDebugTools = false;
	bool showDebugBar = true;
	bool showMenuBar = true;

	//this can be done better
	bool ConsoleHovFlag = false;

	void Init() override;
	void Update() override;
	void DrawUI(void);
};