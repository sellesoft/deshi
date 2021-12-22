local vec2 console_pos;
local vec2 console_dim;

//fancy console opening parameters
local f32 open_max_percent = 0.7f; //percentage of the height of the window to open to
local f32 open_amount = 0.0f;      //current opened amount
local f32 open_target = 0.0f;      //target opened amount
local f32 open_dt = 2000.0f;       //speed at which it opens

//console 
local char inputBuf[256]{};

local constexpr u32 DICT_SIZE = 250;

//this is always from Logger
FILE* buffer;

enum Format {
	Alert,
	Warning,
	Error,
	Tagged,
	Colored,
};

struct ColoredStr{
	color color;
	Format format;
	u32 charstart;
	u32 strsize;
	b32 EndOfLine = 0; //true if this ColoredStr is the end of a line
	char tag[255];
};


struct{
	ring_array<ColoredStr> dict;
	
	void add(const char* str, u32 strsize, color color, b32 newLine = 0, Format format = Colored) {
		dict.add(ColoredStr{ color,  format, (u32)ftell(buffer), strsize, newLine });
		string to(str, strsize);
		Logger::LogFromConsole(to);
	}

	void add(string& str, color color, b32 newLine = 0, Format format = Colored) {
		dict.add(ColoredStr{ color, format, (u32)ftell(buffer), str.count, newLine });
		Logger::LogFromConsole(str);
	}
	
	void add(u32 charstart, u32 strsize, color color, b32 newLine = 0, Format format = Colored) {
		dict.add(ColoredStr{ color, format, charstart, strsize, newLine });
	}
	
} dictionary;

//state of console
local b32 show_autocomplete = 0;
local u32 scroll_y = 0;
local u32 rows_in_buffer = 0;
local b32 scroll_to_bottom = 0;
local ConsoleState state;

local map<const char*, color> color_strings{
	{"red",    Color_Red},     {"dred",    Color_DarkRed},
	{"blue",   Color_Blue},    {"dblue",   Color_DarkBlue},
	{"cyan",   Color_Cyan},    {"dcyan",   Color_DarkRed},
	{"grey",   Color_Grey},    {"dgrey",   Color_DarkGrey},
	{"green",  Color_Green},   {"dgreen",  Color_DarkGreen},
	{"yellow", Color_Yellow},  {"dyellow", Color_DarkYellow},
	{"magen",  Color_Magenta}, {"dmagen",  Color_DarkMagenta},
	{"white",  Color_White},   {"black",   Color_Black}
};

//message modifier syntax:
//	
//  {{c=red} some kind of message {}}
//
//	{{a} some kind of alert!! {}}
// 
//  {{t=vulkan,c=yellow} a message with multiple modifiers {}}
// 
//	if a message is formatted and ends without terminating the formatting
//  its still valid, {}} only ends the formatting so you can have different formatting per message
//  for example
// 
//  {{c=yellow} some message
//	
//  will still color the message yellow also,
// 
//  some text before formatting {{a} some formatted text at the end
//  
//  will not format the first part, and format the rest
// 
//modifiers
// 
//	a     - flashes the background of the message red
//  e     - these messages are red
//  w     - these messages are yellow
//  t=... - these messages have a tag they can be filtered by
//  c=... - sets the color of the wrapped message
//
void ParseMessage(string& input) {
	enum ParseState {
		None,
		ParsingChunk,
		ParsingFormatting,
		ParsingFormattedChunk
	}; ParseState state = None;

	//defines to make customizing syntax easier
#define ConFormatOpening    i + 1 < input.count && input[i] == '{' && input[i + 1] == '{'
#define ConFormatSpecifierClosing   input[i] == '}'
#define ConFormatSpecifierSeparator input[i] == ','
#define ConFormatClosing    i + 2 < input.count && input[i] == '{' && input[i + 1] == '}' && input[i + 2] == '}'

	char* formatting_start = 0;
	char* chunk_start = 0;

	ColoredStr working;

	color chunk_color = Color_White;
	Format chunk_format = Colored;

	//if the input is less than 7 characters its impossible for it to have valid modifier syntax
	if (input.count < 7) {
		dictionary.add(input.str, input.count, Color_White);
		return;
	}

	for (int i = 0; i < input.count; i++) {
		char* curr = &input[i];
		switch (state) {
			case None: { //initial state and state after finishing a formatted string
				if (ConFormatOpening) {
					state = ParsingFormatting;
					formatting_start = &input[i];
					i++; continue;
				}
				else {
					state = ParsingChunk;
					chunk_start = &input[i];
				}
			}break;
			case ParsingChunk: {
				if (ConFormatOpening) {
					state = ParsingFormatting;
					formatting_start = &input[i];
					dictionary.add(chunk_start, &input[i] - chunk_start, Color_White);
					i++; continue;
				}
				else if (input[i] == '\n') {
					//we end the chunk if we find a newline
					state = None;
					dictionary.add(chunk_start, (&input[i] - chunk_start) + 1, Color_White, 1);
				}
				else if (i == input.count - 1) {
					//we've reached the end of the string
					dictionary.add(chunk_start, (&input[i] - chunk_start) + 1, Color_White, 1);
				}
			}break;
			case ParsingFormatting: {
				if (ConFormatSpecifierClosing) {
					state = ParsingFormattedChunk;
					string format(formatting_start + 2, &input[i] - (formatting_start + 2));
					if (format.count == 1) {
						if (format == "a") {
							chunk_format = Alert;
						}
						else if (format == "w") {
							chunk_format = Warning;
						}
						else if (format == "e") {
							chunk_format = Error;
						}
					}
					else {
						if (format[0] == 'c' && format[1] == '=') {
							chunk_format = Colored;
							chunk_color = color_strings[format.substr(2).str];
						}
						else if (format[0] == 't' && format[1] == '=') {
							chunk_format = Tagged;
						}
					}
					chunk_start = &input[i + 1];
				}
				else if (input[i] == '\n') {
					//what we were parsing must not have actually been formatting
					//so just end the chunk and start again
					state = None;
					dictionary.add(formatting_start, (&input[i] - formatting_start) + 1, Color_White, 1);
				}
				else if (i == input.count - 1) {
					//we've reached the end of the string
					dictionary.add(formatting_start, (&input[i] - formatting_start) + 1, Color_White, 1);
				}
			}break;
			case ParsingFormattedChunk: {
				if (ConFormatClosing) {
					state = None;
					dictionary.add(chunk_start, (&input[i] - chunk_start), chunk_color, 0, chunk_format);
					chunk_color = Color_White;
					chunk_format = Colored;
					chunk_start = 0;
					i += 2;
				}
				else if (input[i] == '\n') {
					i++;
					if (ConFormatClosing) {
						//in this case we find a newline right at the end of a formatted chunk
						state = None;
						dictionary.add(chunk_start, (&input[i - 1] - chunk_start), chunk_color, 1, chunk_format);
						chunk_color = Color_White;
						chunk_format = Colored;
						chunk_start = 0;
						i += 2;
					}
					else {
						dictionary.add(chunk_start, (&input[i - 1] - chunk_start), chunk_color, 1, chunk_format);
						chunk_start = &input[i];
					}
				}
				else if (i == input.count - 1) {
					//we've reached the end of the string
					dictionary.add(chunk_start, (&input[i] - chunk_start) + 1, chunk_color, 1, chunk_format);
				}
			}break;
		}
	}
}

void Console::AddLog(string input){
	ParseMessage(input);
}

void Console::LoggerMirror(string input, u32 charstart) {
	ParseMessage(input);
}

void Console::ChangeState(ConsoleState new_state) {
	
}

void Console::Init() {
	AssertDS(DS_MEMORY, "Attempt to load Console without loading Memory first");
	AssertDS(DS_LOGGER, "Attempt to load Console without loading Logger first");
	deshiStage |= DS_CONSOLE;

	TIMER_START(t_s);
	buffer = Logger::GetFilePtr();

	dictionary.dict.init(DICT_SIZE, deshi_allocator);

	Log("deshi", "Finished console initialization in ", TIMER_END(t_s), "ms");
}

void Console::Update() {
	using namespace UI;

	char toprint[1024];

	Begin("deshiConsole", vec2::ZERO, vec2(DeshWindow->width, DeshWindow->height * open_max_percent));

	for (int i = 0; i < dictionary.dict.count; i++) {
		ColoredStr colstr = dictionary.dict[i];
		
		fseek(buffer, colstr.charstart, SEEK_SET);
		fread(toprint, colstr.strsize + 1, 1, buffer);
		toprint[dictionary.dict[i].strsize] = 0;
		
		PushColor(UIStyleCol_Text, colstr.color);
		if (!colstr.EndOfLine) SameLine();
		Text(toprint);
		
		PopColor();
	}

	End();
}

void Console::Cleanup() {
	
}

