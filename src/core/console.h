/*Console Message Formatting:
	The console's formatting follows closely to virtual terminal sequences, but contains some custom sequences as well
	Most VTSs arent supported because they arent really useful in console. All color VTSs are though.

	See this link for information on VTSs and the codes:
		https://docs.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences

	NOTE in C, ESC is typed out as \x1b

	While we follow the pattern of "ESC [ <n> m" for color sequences, we dont for custom ones. This is to 
	preserve compatibility with sending console stuff to an actual terminal that uses them, but to make it easier to
	parse custom terminal sequences will be of the form "ESC <type> [ <argument> ]".

	An example of how to write these out in C would be "\x1b[38m", for colors and for our custom messages
	"\x1bt[console]", which would tag the message with "console"

	List of custom terminal sequences:
	
	Tag:     ESC t [ <tag> {, <tag> } ]  -- NOTE this is only considered once, as a message may only define tags once 
	Alert:   ESC a []
	Warning: ESC w []
	Error:   ESC e []
	Success: ESC s []

TODOs:
	implement alert, error, warning, and success sequences
		consider not implementing these as well, because they are generally just set by logger anyways 

*/

#pragma once
#ifndef DESHI_CONSOLE_H
#define DESHI_CONSOLE_H

#include "kigu/common.h"
#include "kigu/pair.h"
#include "kigu/string_utils.h"
#include "kigu/ring_array.h"
#include "kigu/unicode.h"
#include "math/vector.h"
#include "time.h"

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
	color fg;
	color bg;
	u64 start;
	u64 size;
	b32 newline;
};

struct Logger;
struct uiItem;
struct Arena;
struct Console{
	b32 initialized;
	
	ring_array<ConsoleChunk> dictionary;

	Logger* logger;
	
#define CONSOLE_INPUT_BUFFER_SIZE 1024
	u8  prev_input[CONSOLE_INPUT_BUFFER_SIZE]{};
	u32 input_history_index = -1;
	ring_array<pair<u32,u32>> input_history;

	Arena* chunk_render_arena;
	
	b32 tag_show         = true;
	b32 tag_highlighting = true;
	b32 tag_outlines     = true;
	b32 line_highlighing = true;
	b32 automatic_scroll = true;

	ConsoleState state = ConsoleState_Closed;
	f32 open_small_percent = 0.2f;   //percentage of the height of the window to open to in small mode
	f32 open_max_percent   = 0.7f;   //percentage of the height of the window to open to
	f32 open_amount        = 0.0f;   //current open amount
	f32 open_target        = 0.0f;   //target open amount
	f32 open_dt            = 200.0f; //time it takes to open in ms
	Stopwatch open_timer;
	
	vec2 console_pos;
	vec2 console_dim;
	b32 open_pressed     = false;
	b32 scroll_to_bottom = false;

	s64 scroll = 0;

	struct{
		uiItem* main;
		uiItem* buffer;
		uiItem* inputbox; //represents the input decoration box
		uiItem* inputtext; //represents the actual input text
	}ui;
};

void console_init();
void console_update();

//returns the internal `Console` object
FORCE_INLINE Console* console_expose();

//begins the transition of `Console.state` to `new_state`, if new state is the same as previous, set state to `ConsoleState_Closed`
void console_change_state(ConsoleState new_state);

//parses a message into console chunks then logs that message after stripping it of any console-specific formatting
void console_parse_message(str8 message, str8 tag = STR8(""), Type type = ConsoleChunkType_Normal, b32 from_logger = 0, u32 logger_offset = 0);

//parses a message into console chunks then logs that message after stripping it of any console-specific formatting
#define console_log(...)                                                                   \
  {                                                                                        \
    dstr8 message##__LINE__ = to_dstr8v(deshi_temp_allocator, __VA_ARGS__);                \
    console_parse_message(str8{(u8*)message##__LINE__.str, (s64)message##__LINE__.count}); \
  }(void)0

#endif //DESHI_CONSOLE_H