/*Console Message Formatting:
Syntax:
{{plain_specifier, arg_specifier=arg}text that will be formatted}

Notes:
Messages with a formatting starter {{}... will apply that formatting until the end of the string.

Modifiers:
a     - (alert)        flashes the background of the message with modifier color (default red)
e     - (error)        these messages are red
w     - (warning)      these messages are yellow
s     - (success)      these messages are green
t=... - (tag)          these messages have a tag they can be filtered by
c=... - (color)        sets the color of the wrapped message

Examples:
{{c=red}a red message}
{{a}an alert message}
{{t=vulkan, c=yellow}a message with multiple modifiers}
non-formatted text {{a,c=blue,t=vulkan}blue text with a flashing background and the tag "VULKAN"}

// 
//  
//
//  if a message is formatted and ends without terminating the formatting
//  its still valid, } only ends the formatting so you can have different formatting per message
//  for example
//
//  {{c=yellow} some message
//  will still color the message yellow,
//
//  some text before formatting {{a} some formatted text at the end
//  will not format the first part, and format the rest
*/

#pragma once
#ifndef DESHI_CONSOLE_H
#define DESHI_CONSOLE_H

#include "kigu/common.h"
#include "kigu/string_utils.h"
#include "kigu/ring_array.h"
#include "kigu/unicode.h"
#include "math/vector.h"

typedef Type ConsoleState; enum{
	ConsoleState_Closed,
	ConsoleState_OpenSmall,
	ConsoleState_OpenBig,
	ConsoleState_Popout,
	ConsoleState_Window
};

typedef Type ConsoleChunkType; enum{
	ConsoleChunkType_Normal,
	ConsoleChunkType_Error,
	ConsoleChunkType_Warning,
	ConsoleChunkType_Success,
	
	ConsoleChunkType_Alert = (1 << 10),
};

struct ConsoleChunk{
	ConsoleChunkType type;
	str8 tag;
	color color;
	u64 start;
	u64 size;
	b32 newline;
};

struct Console{
	ring_array<ConsoleChunk> dictionary;
	
#define CONSOLE_INPUT_BUFFER_SIZE 1024
	u8  input_buffer[CONSOLE_INPUT_BUFFER_SIZE]{};
	s64 input_length = 0;
	u8  prev_input[CONSOLE_INPUT_BUFFER_SIZE]{};
	 u32 input_history_index = -1;
	 ring_array<pair<u32,u32>> input_history;
	
	b32 tag_show = true;
	 b32 tag_highlighting = true;
	 b32 tag_outlines = true;
	b32 line_highlighing = true;
	b32 automatic_scroll = true;
	
	ConsoleState state = ConsoleState_Closed;
	f32 open_small_percent = 0.2f; //percentage of the height of the window to open to in small mode
	 f32 open_max_percent = 0.7f;   //percentage of the height of the window to open to
	 f32 open_amount = 0.0f;        //current open amount
	 f32 open_target = 0.0f;        //target open amount
	f32 open_dt = 200.0f;          //speed at which it opens
	
	vec2 console_pos;
	vec2 console_dim;
};

void console_init();
void console_update();

//returns the internal `Console` object
FORCE_INLINE Console* console_expose();

//begins the transition of `Console.state` to `new_state`, if new state is the same as previous, set state to `ConsoleState_Closed`
void console_change_state(ConsoleState new_state);

//parses a message into console chunks then logs that message after stripping it of any console-specific formatting
void console_parse_message(str8 message);

//parses a message into console chunks then logs that message after stripping it of any console-specific formatting
#define console_log(...) \
{ \
string message##__LINE__ = ToString(__VA_ARGS__); \
console_parse_message(str8{(u8*)message##__LINE__.str, (s64)message##__LINE__.count}); \
}(void)0

#endif //DESHI_CONSOLE_H