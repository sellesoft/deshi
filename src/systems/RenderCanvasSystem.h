#pragma once
#include "dsh_System.h"

struct RenderCanvasSystem : public System {
	void Init() override;
	void Update() override;
	void DrawUI(void);
};