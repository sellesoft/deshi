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

struct Command;


enum ConsoleState_ {
	ConsoleState_Closed,
	ConsoleState_OpenSmall,
	ConsoleState_OpenBig,
	ConsoleState_Popout,
	ConsoleState_Window
}; typedef u32 ConsoleState;


struct Console  {
	

	
	Console() {}
	
	void Init();
	void Update();
	void AddLog(string input);
	void ChangeState(ConsoleState new_state);
	void FlushBuffer();


	Command* GetCommand(string command);
	string ExecCommand(string command);
	string ExecCommand(string command, string args);
	void AddCommands();
	void AddAliases();
	
	void CleanUp();
};

//global_ console pointer
extern Console* g_console;
#define DeshConsole g_console

#define LOG(...)     g_console->PushConsole(TOSTDSTRING(__VA_ARGS__))
#define ERROR(...)   g_console->PushConsole(TOSTDSTRING("[c:error]", __VA_ARGS__, "[c]"))
#define SUCCESS(...) g_console->PushConsole(TOSTDSTRING("[c:green]", __VA_ARGS__, "[c]"))
#define WARNING(...) g_console->PushConsole(TOSTDSTRING("[c:yellow]", __VA_ARGS__, "[c]"))

//additionally prints where function was called
#define LOG_LOC(...)     g_console->PushConsole(TOSTDSTRING("In ", __FILENAME__, " at ", __LINE__ , ": \n", __VA_ARGS__))
#define ERROR_LOC(...)   g_console->PushConsole(TOSTDSTRING("[c:error]In ", __FILENAME__, " in func ", __FUNCTION__, " at ", __LINE__, ": \n[c]", "[c:error]", __VA_ARGS__, "[c]"))
#define SUCCESS_LOC(...) g_console->PushConsole(TOSTDSTRING("[c:green]In ", __FILENAME__, " in func ", __FUNCTION__, " at ", __LINE__, ": \n[c]", "[c:green]", __VA_ARGS__, "[c]"))
#define WARNING_LOC(...) g_console->PushConsole(TOSTDSTRING("[c:yellow]In ", __FILENAME__, " in func ", __FUNCTION__, " at ", __LINE__, ": \n[c]", "[c:yellow]", __VA_ARGS__, "[c]"))

#define LOGFUNCxx LOG(__FUNCTION__, " called")
#define LOGFUNCMxx(...) LOG(__FUNCTION__, " called ", __VA_ARGS__)

#endif //DESHI_CONSOLE_H