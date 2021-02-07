#pragma once
#include "System.h"

struct RenderSceneSystem : public System {
	void Init() override;
	void Update() override;
};