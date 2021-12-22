local vec2 console_pos;
local vec2 console_dim;

//fancy console opening parameters
local f32 open_max_percent = 0.7f; //percentage of the height of the window to open to
local f32 open_amount = 0.0f;      //current opened amount
local f32 open_target = 0.0f;      //target opened amount
local f32 open_dt = 2000.0f;       //speed at which it opens

//console 
local char inputBuf[256]{};

local constexpr u32 DICT_SIZE = 25;

//this is always from Logger
FILE* buffer;

enum Format {
	Alert,
	Colored,
};

struct ColoredStr{
	color color;
	Format format;
	u32 charstart;
	u32 strsize;
};


struct{
	ring_array<ColoredStr> dict;
	
	void add(const char* str, u32 strsize, color color, Format format = Colored) {
		dict.add(ColoredStr{ color,  format, (u32)ftell(buffer), strsize });
		string to(str, strsize);
		Logging::LogFromConsole(to);
	}

	void add(string& str, color color, Format format = Colored) {
		dict.add(ColoredStr{ color, format, (u32)ftell(buffer), str.count });
		Logging::LogFromConsole(str);
	}
	
	void add(u32 charstart, u32 strsize, color color, Format format = Colored) {
		dict.add(ColoredStr{ color, format, charstart, strsize });
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
//modifiers
// 
//	a     - flashes the background of the message red
//  c=... - sets the color of the wrapped message
//
void ParseMessage(string input) {
	enum ParseState {
		None,
		ParsingChunk,
		ParsingFormatting,
		ParsingFormattedChunk
	}; ParseState state = None;

#define ConFormatOpening i + 1 < input.count && input[i] == '{' && input[i + 1] == '{'
#define ConFormatClosing i + 2 < input.count && input[i] == '{' && input[i + 1] == '}' && input[i + 2] == '}'

	
	char* formatting_start = 0;
	char* chunk_start = 0;

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
				else if (i == input.count - 1) {
					//we've reached the end of the string
					dictionary.add(chunk_start, (&input[i] - chunk_start) + 1, Color_White);
				}
			}break;
			case ParsingFormatting: {
				if (input[i] == '}') {
					state = ParsingFormattedChunk;
					string format(formatting_start+2, &input[i] - (formatting_start+2));
					if (format == "a") {
						chunk_format = Alert;
					}
					else if (format[0] == 'c' && format[1] == '=') {
						chunk_format = Colored;
						chunk_color = color_strings[format.substr(2).str];
					}
					chunk_start = &input[i + 1];
				}
				else if (i == input.count - 1) {
					//we've reached the end of the string
					dictionary.add(formatting_start, (&input[i] - formatting_start) + 1, Color_White);
				}
			}break;
			case ParsingFormattedChunk: {
				if (ConFormatClosing) {
					state = None;
					dictionary.add(chunk_start, (&input[i] - chunk_start), chunk_color, chunk_format);
					chunk_color = Color_White;
					chunk_format = Colored;
					chunk_start = 0;
					i+=2;
				}
				else if (i == input.count - 1) {
					//we've reached the end of the string
					dictionary.add(formatting_start, (&input[i] - formatting_start) + 1, Color_White);
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
	TIMER_START(t_s);
	buffer = Logging::GetFilePtr();

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
		Text(toprint);
		PopColor();
	}

	End();
}

void Console::Cleanup() {
	
}

