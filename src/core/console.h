#pragma once
#ifndef DESHI_CONSOLE_H
#define DESHI_CONSOLE_H

#include "../defines.h"
#include "../utils/Color.h"
#include "../utils/Debug.h"
#include "../utils/tuple.h"
#include "../utils/map.h"
#include "../utils/array.h"
#include "../utils/string.h"
#include "../utils/ring_array.h"

enum ConsoleState_ {
	ConsoleState_Closed,
	ConsoleState_OpenSmall,
	ConsoleState_OpenBig,
	ConsoleState_Popout,
	ConsoleState_Window
}; typedef u32 ConsoleState;


struct Console{
	void Init();
	void Update();
	void Cleanup();
	
	bool IsOpen();
	void ChangeState(ConsoleState new_state);
	void AddLog(string input);
	void FlushBuffer();
	
	void AddAliases();
};

//global_ console pointer
extern Console* g_console;
#define DeshConsole g_console

#endif //DESHI_CONSOLE_H