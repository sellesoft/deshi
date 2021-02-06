#pragma once
#include "dsh_System.h"

struct CommandSystem : public System {
	void Init() override;
	void Update() override;
};