#pragma once
#include "dsh_System.h"

struct RenderSceneSystem : public System {
	void Init() override;
	void Update() override;
};