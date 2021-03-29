#pragma once
#include "System.h"

struct RenderCanvasSystem : public System {

	bool showDebugTools = false;
	bool showDebugBar = true;
	bool showMenuBar = true;
	bool showImGuiDemoWindow = false;

	//this can be done better
	bool ConsoleHovFlag = false;

	void DebugBar();
	void DebugTools();
	void MenuBar();

	void Init() override;
	void Update() override;
	void DrawUI(void);
};