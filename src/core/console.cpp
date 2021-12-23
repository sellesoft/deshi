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
local constexpr UIWindowFlags flags = UIWindowFlags_NoMove | UIWindowFlags_NoResize;


UIWindow* conmain = 0; //the main console window
UIWindow* conterm = 0; //the terminal child window

UIStyle* uistyle = 0; //for quick access to the style of ui, we should not change any styles through this pointer


//this is always from Logger
FILE* buffer;

enum Format_ {
	None    = 0,
	Alert   = 1 << 0, 
	Warning = 1 << 1, //these 2 are mutually exclusive!
	Error   = 1 << 2, //these 2 are mutually exclusive!
	Tagged  = 1 << 3,
	Colored = 1 << 4,
}; typedef u32 Format;

#define HasFlag(var, flag) (var & flag)
#define HasFlags(var, flags) ((var & flags) == flags)

struct Chunk{
	color color = Color_White;
	Format format = None;
	u32 charstart = 0;
	u32 strsize = 0;
	b32 eol = 0; //true if this Chunk is the end of a line
	string tag = ""; //change to a char array eventually
};


struct{
	ring_array<Chunk> dict;

	void add(string to_logger, Chunk chunk) {
		dict.add(chunk);

		Logger::LogFromConsole(to_logger);
	}

	//for chunks that have already been logged
	void add(Chunk chunk) {
		dict.add(chunk);
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
//  {{t=vulkan, c=yellow} a message with multiple modifiers {}}
// 
//	if a message is formatted and ends without terminating the formatting
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
//	.a     - (alert)        flashes the background of the message red
//  .e     - (error)        these messages are red
//  .w     - (warning)      these messages are yellow
//  .t=... - (tag)          these messages have a tag they can be filtered by
//  .c=... - (color)        sets the color of the wrapped message
//
//TODO(sushi) clean this function up please
void ParseMessage(string& input, s32 chstart = -1) {
	//if the input is less than 7 characters its impossible for it to have valid modifier syntax
	if (input.count < 7) {
		Chunk chunk;
		chunk.charstart = (chstart == -1 ? ftell(buffer) : chstart);
		chunk.color = Color_White;
		chunk.eol = 1;
		chunk.format = None;
		if (chstart == -1) {
			dictionary.add(input, chunk);
		}
		else {
			dictionary.add(chunk);
		}
		return;
	}
	
	enum ParseState {
		Init,
		ParsingChunk,
		ParsingFormatting,
		ParsingFormattedChunk
	}; ParseState state = Init;

	//defines to make customizing syntax easier
#define ConFormatOpening    (i + 1 < input.count && input[i] == '{' && input[i + 1] == '{')
#define ConFormatSpecifierClosing   (input[i] == '}')
#define ConFormatSpecifierSeparator (input[i] == ',')
#define ConFormatClosing    (i + 2 < input.count && input[i] == '{' && input[i + 1] == '}' && input[i + 2] == '}')
#define ResetChunk chunk = Chunk()
#define AddChunk if(chstart == -1){ string tl(chunk_start, chunk.strsize); dictionary.add(tl, chunk); } else dictionary.add(chunk);
#define AddChunkWithNewline if(chstart == -1) { string tl(chunk_start, chunk.strsize); dictionary.add(tl + "\n", chunk); } else dictionary.add(chunk);

	char* formatting_start = 0;
	char* specifier_start = 0;
	char* chunk_start = 0;

	Chunk chunk;

	u32 charstart = (chstart == -1 ? ftell(buffer) : chstart);

	for (int i = 0; i < input.count; i++) {
#if 0
		//leaving this here for whenever this function goes wrong again,
		// as its stupidly finicky
		char* curr = &input[i];
		PRINTLN("-----------------------------------------");
		PRINTLN("       charstart: " << charstart);
		PRINTLN("     chunk start: " << (chunk_start ? chunk_start : "0"));
		PRINTLN("formatting start: " << (formatting_start ? formatting_start : "0"));
		PRINTLN(" specifier start: " << (specifier_start ? specifier_start : "0"));
		PRINTLN("    chunk format: " << chunk.format);
		PRINTLN(" chunk charstart: " << chunk.charstart);
		PRINTLN("   chunk strsize: " << chunk.strsize);
		PRINTLN("            curr: " << curr);
#endif
		switch (state) {
			case Init: { //initial state and state after finishing a formatted string
				if (ConFormatOpening) {
					state = ParsingFormatting;
					formatting_start = &input[i];
					specifier_start = formatting_start + 2;
					i++; continue;
				}
				else {
					state = ParsingChunk;
					chunk_start = &input[i];
					chunk.charstart = charstart;
				}
			}break;
			case ParsingChunk: {
				if (ConFormatOpening) {
					state = ParsingFormatting;
					formatting_start = &input[i];
					specifier_start = formatting_start + 2;
					chunk.strsize = &input[i] - chunk_start;
					charstart += chunk.strsize;
					AddChunk;
					//dictionary.add(chunk_start, chunk);
					ResetChunk;
					i++; continue;
				}
				else if (input[i] == '\n') {
					//we end the chunk if we find a newline
					state = Init;
					chunk.strsize = &input[i] - chunk_start;
					chunk.eol = 1;
					AddChunk;
					//dictionary.add(chunk_start, chunk);
					ResetChunk;
				}
				else if (i == input.count - 1) {
					//we've reached the end of the string
					chunk.strsize = (&input[i] - chunk_start) + 1;
					chunk.eol = 1;
					AddChunkWithNewline;
					//dictionary.add(chunk_start, chunk);
				}
			}break;
			case ParsingFormatting: {
				auto parse_specifier = [&](string& specifier) {
					if (!HasFlags(chunk.format, (Warning | Error))) {
						if (specifier == "e") {
							chunk.format |= Error;
							return;
						}
						else if (specifier == "w") {
							chunk.format |= Warning;
							return;
						}
					}
					if (specifier == "a") {
						chunk.format |= Alert;
						return;
					}
					if (specifier[0] == 'c' && specifier[1] == '=') {
						chunk.format |= Colored;
						chunk.color = color_strings[specifier.substr(2).str];
						return;
					}
					if (specifier[0] == 't' && specifier[1] == '=') {
						chunk.format |= Tagged;
						chunk.tag = specifier.substr(2);
						return;
					}
				};
				if (ConFormatSpecifierClosing) {
					state = ParsingFormattedChunk;
					string specifier(specifier_start, (&input[i] - specifier_start));
					parse_specifier(specifier);
					specifier_start = 0;
					chunk_start = &input[++i];
					chunk.charstart = charstart;
					
				}
				else if (ConFormatSpecifierSeparator) {
					i++;
					string specifier(specifier_start, (&input[i] - specifier_start) - 1);
					parse_specifier(specifier);
					specifier_start += specifier.count + 1;
					
				}
				else if (input[i] == '\n') {
					//what we were parsing must not have actually been formatting
					//so just end the chunk and start again
					state = Init;
					ResetChunk; //remove any formatting we may have added
					chunk.strsize = (&input[i] - formatting_start);
					chunk.eol = 1;
					AddChunk;
					//dictionary.add(formatting_start, chunk);
					charstart += chunk.strsize + 1;

				}
				else if (i == input.count - 1) {
					//we've reached the end of the string
					chunk.strsize = (&input[i] - formatting_start);
					chunk.eol = 1;
					AddChunkWithNewline;
					
				}
			}break;
			case ParsingFormattedChunk: {
				if (ConFormatClosing) {
					state = Init;
					chunk.strsize = (&input[i] - chunk_start);
					i += 2;
					if (i == input.count - 1) { 
						chunk.eol = 1; 
						AddChunkWithNewline;
						break;
					}
					else {
						AddChunk;
						charstart += chunk.strsize;
					}
					ResetChunk;
				}
				else if (input[i] == '\n') {
					i++;
					if (ConFormatClosing) {
						//in this case we find a newline right at the end of a formatted chunk
						state = Init;
						i--;
						chunk.strsize = (&input[i] - chunk_start);
						chunk.eol = 1;
						AddChunk;
						charstart += chunk.strsize;
						ResetChunk;
						i += 2;
					}
					else {
						i--;
						chunk.strsize = (&input[i] - chunk_start);
						chunk.eol = 1;
						AddChunkWithNewline;
						charstart += chunk.strsize + 1;
						chunk_start = &input[i+1];
						chunk.charstart = charstart;
					}
				}
				else if (i == input.count - 1) {
					//we've reached the end of the string
					chunk.strsize = (&input[i] - chunk_start) + 1;
					chunk.eol = 1;
					AddChunkWithNewline;
				}
			}break;
		}
	}
}

void Console::AddLog(string input){
	ParseMessage(input);
}

void Console::LoggerMirror(string input, u32 charstart) {
	ParseMessage(input, charstart);
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
	uistyle = &GetStyle(); //TODO(sushi) try to get this only once



	Begin("deshiConsole", vec2::ZERO, vec2(DeshWindow->width, DeshWindow->height * open_max_percent), flags);
	conmain = GetWindow(); //TODO(sushi) try to get this only once

	BeginChild("deshiConsoleTerminal", (conmain->dimensions - 2 * uistyle->windowPadding).yAdd(-(uistyle->fontHeight * 1.3 + uistyle->itemSpacing.y)), flags);
	conterm = GetWindow(); //TODO(sushi) try to get this only once

	PushVar(UIStyleVar_WindowPadding, vec2(5, 0));
	PushVar(UIStyleVar_ItemSpacing, vec2(0, 0));
	char toprint[1024];
	for (int i = 0; i < dictionary.dict.count; i++) {
		Chunk colstr = dictionary.dict[i];
		
		fseek(buffer, colstr.charstart, SEEK_SET);
		fread(toprint, colstr.strsize + 1, 1, buffer);
		toprint[dictionary.dict[i].strsize] = 0;
		
		//adjust what we're going to print based on formatting

		PushColor(UIStyleCol_Text, colstr.color);
		Text(toprint);
		if (!colstr.eol && i != (dictionary.dict.count - 1)) SameLine();
		
		PopColor();
	}
	PopVar(2);

	EndChild();
	PushColor(UIStyleCol_WindowBg, color(0, 25, 18));
	End();
	PopColor();
}

void Console::Cleanup() {
	
}

