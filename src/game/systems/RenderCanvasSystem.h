#pragma once
#include "System.h"

struct RenderCanvasSystem : public System {

	bool showDebugTools = false;
	bool showDebugBar = true;

	void Init() override;
	void Update() override;
	void DrawUI(void);
};