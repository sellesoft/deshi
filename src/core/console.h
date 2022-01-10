#pragma once
#ifndef DESHI_CONSOLE_H
#define DESHI_CONSOLE_H
//message modifier syntax:
//
//  {{c=red} some kind of message {}}
//
//  {{a} some kind of alert!! {}}
// 
//  {{t=vulkan, c=yellow} a message with multiple modifiers {}}
//
//  if a message is formatted and ends without terminating the formatting
//  its still valid, {}} only ends the formatting so you can have different formatting per message
//  for example
//
//  {{c=yellow} some message
//
//  will still color the message yellow also,
//
//  some text before formatting {{.a} some formatted text at the end
//
//  will not format the first part, and format the rest
//
//modifiers
// 
//  .a     - (alert)        flashes the background of the message with modifier color (default red)
//  .e     - (error)        these messages are red
//  .w     - (warning)      these messages are yellow
//  .t=... - (tag)          these messages have a tag they can be filtered by
//  .c=... - (color)        sets the color of the wrapped message

#include "../defines.h"

enum ConsoleState_ {
	ConsoleState_Closed,
	ConsoleState_OpenSmall,
	ConsoleState_OpenBig,
	ConsoleState_Popout,
	ConsoleState_Window
}; typedef u32 ConsoleState;

struct Console{
	b32  IsOpen();
	void ChangeState(ConsoleState new_state);
	void AddLog(string input);
	void LoggerMirror(string in, u32 charstart);
	
	void Init();
	void Update();
};

//global_ console pointer
extern Console* g_console;
#define DeshConsole g_console

FORCE_INLINE b32  console_is_open(){ return DeshConsole->IsOpen(); }
FORCE_INLINE void console_change_state(ConsoleState new_state){ return DeshConsole->ChangeState(new_state); }
#define console_log(...) DeshConsole->AddLog(toStr(__VA_ARGS__))
FORCE_INLINE void console_init(){ return DeshConsole->Init(); }
FORCE_INLINE void console_update(){ return DeshConsole->Update(); }

#endif //DESHI_CONSOLE_H