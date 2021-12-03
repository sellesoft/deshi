#pragma once
#ifndef DESHI_CONSOLE2_H
#define DESHI_CONSOLE2_H

#include "../defines.h"



namespace Console2{
	local bool intercepting_inputs = false;
	
	bool IsOpen();
	void Toggle(ConsoleState new_state);
	void AddLog(const string& message);
	
	void Init();
	void Cleanup();
	void Update();
};

#endif //DESHI_CONSOLE2_H
