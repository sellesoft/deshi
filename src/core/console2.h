#pragma once
#ifndef DESHI_CONSOLE2_H
#define DESHI_CONSOLE2_H

#include "../utils/defines.h"
#include "../utils/Color.h"

enum ConsoleStateBits : u32{
	ConsoleState_Closed, ConsoleState_OpenSmall, ConsoleState_OpenBig, ConsoleState_Popout
};typedef u32 ConsoleState;

namespace Console2{
	bool IsOpen();
	void Toggle(ConsoleState new_state);
	void Log(std::string message);
	
	void Init();
	void Cleanup();
	void Draw();
};

#endif //DESHI_CONSOLE2_H
