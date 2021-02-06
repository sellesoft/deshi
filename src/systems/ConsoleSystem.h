#pragma once
#include "dsh_System.h"

struct ConsoleSystem : public System {
	void Init() override;
	void Update() override;
	void DrawConsole();
	void PushConsole(std::string s);
};