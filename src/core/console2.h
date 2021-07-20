#pragma once
#ifndef DESHI_CONSOLE2_H
#define DESHI_CONSOLE2_H

#include "../defines.h"
#include <string>

enum ConsoleStateBits : u32{
	ConsoleState_Closed, ConsoleState_OpenSmall, ConsoleState_OpenBig, ConsoleState_Popout, ConsoleState_Window
};typedef u32 ConsoleState;

namespace Console2{
	local bool intercepting_inputs = false;
	
	bool IsOpen();
	void Toggle(ConsoleState new_state);
	void Log(std::string message);
	
	void Init();
	void Cleanup();
	void Update();
};

#endif //DESHI_CONSOLE2_H
