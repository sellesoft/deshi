enum ConChunkFormat_{
	None    = 0,
	Alert   = 1 << 0, 
	Success = 1 << 1, //these 3 are mutually exclusive!
	Warning = 1 << 2, //these 3 are mutually exclusive!
	Error   = 1 << 3, //these 3 are mutually exclusive!
	Tagged  = 1 << 4,
	Colored = 1 << 5,
}; typedef u32 ConChunkFormat;

struct Chunk{
	color color = Color_White;
	ConChunkFormat format = None;
	u32 charstart = 0;
	u32 strsize = 0;
	b32 eol = 0; //true if this Chunk is the end of a line
	string tag = ""; //change to a char array eventually
	
#ifdef DESHI_INTERNAL
	string message = "";
#endif 
};

struct ConsoleDictionary{
	ring_array<Chunk> dict;
	u32& count = dict.count;
	
	void add(string to_logger, Chunk chunk){
		dict.add(chunk);
		
#ifdef DESHI_INTERNAL
		chunk.message = to_logger;
#endif
		
		Logger::LogFromConsole(to_logger);
	}
	
	//for chunks that have already been logged
	void add(Chunk chunk){
		dict.add(chunk);
	}
	
	Chunk& operator[](u32 idx){ return *dict.at(idx); }
};

//// terminal ////
local FILE* buffer; //this is always from Logger

#define CONSOLE_DICTIONARY_SIZE 512
local ConsoleDictionary dictionary;

#define CONSOLE_INPUT_BUFFER_SIZE 256
local char inputBuf[CONSOLE_INPUT_BUFFER_SIZE]{};
local b32  intercepting_inputs = 0;
local u32  max_input_history = 200;
local u32  input_history_index = -1;
local ring_array<pair<u32,u32>> input_history;

//// visuals ////
local vec2 console_pos;
local vec2 console_dim;
local u32  scroll_y = 0;
local u32  rows_in_buffer = 0;
local ConsoleState state = ConsoleState_Closed;

local b32 show_autocomplete = 0;
local b32 scroll_to_bottom = 0;
local b32 tag_show = 1;
local b32 tag_highlighting = 1;
local b32 tag_outlines = 1;
local b32 line_highlighing = 1;

local UIWindow* conmain = 0; //the main console window
local UIWindow* conterm = 0; //the terminal child window
local UIStyle*  uistyle = 0; //for quick access to the style of ui, we should not change any styles through this pointer
local UIWindowFlags flags = UIWindowFlags_NoMove | UIWindowFlags_NoResize | UIWindowFlags_NoScrollX;

local f32 open_small_percent = 0.2f; //percentage of the height of the window to open to in small mode
local f32 open_max_percent = 0.7f;   //percentage of the height of the window to open to
local f32 open_amount = 0.0f;        //current opened amount
local f32 open_target = 0.0f;        //target opened amount
local f32 open_dt = 200.0f;          //speed at which it opens
local TIMER_START(open_timer);

local map<const char*, color> color_strings{
	{"red",    Color_Red},     {"dred",    Color_DarkRed},
	{"blue",   Color_Blue},    {"dblue",   Color_DarkBlue},
	{"cyan",   Color_Cyan},    {"dcyan",   Color_DarkCyan},
	{"grey",   Color_Grey},    {"dgrey",   Color_DarkGrey},
	{"green",  Color_Green},   {"dgreen",  Color_DarkGreen},
	{"yellow", Color_Yellow},  {"dyellow", Color_DarkYellow},
	{"magen",  Color_Magenta}, {"dmagen",  Color_DarkMagenta},
	{"white",  Color_White},   {"black",   Color_Black}
};

//TODO(sushi) clean this function up please
void ParseMessage(string& input, s32 chstart = -1) {
	//if the input is less than 7 characters its impossible for it to have valid modifier syntax
	if (input.count < 7) {
		Chunk chunk;
		chunk.charstart = (chstart == -1 ? ftell(buffer) : chstart);
		chunk.strsize = input.count;
		chunk.color = Color_White;
		chunk.eol = 1;
		chunk.format = None;
		if (chstart == -1) {
			dictionary.add(input + "\n", chunk);
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
					if (!HasAllFlags(chunk.format, (Warning | Error))) {
						if (specifier == "e") {
							AddFlag(chunk.format, Error);
							return;
						}
						else if (specifier == "w") {
							AddFlag(chunk.format, Warning);
							return;
						}
						else if (specifier == "s") {
							AddFlag(chunk.format, Success);
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
	if(DeshiModuleLoaded(DS_CONSOLE)) 
		ParseMessage(input);
}

void Console::LoggerMirror(string input, u32 charstart) {
	if(DeshiModuleLoaded(DS_CONSOLE)) 
		ParseMessage(input, charstart);
}

void Console::ChangeState(ConsoleState new_state) {
	if (state == new_state) new_state = ConsoleState_Closed;
	
	intercepting_inputs = true;
	
	switch (new_state) {
		case ConsoleState_Closed: {
			if (state == ConsoleState_Popout) {
				console_dim = conmain->dimensions;
				console_pos = conmain->position;
			}
			open_target = 0;
			open_amount = console_dim.y;
			intercepting_inputs = false;
			TIMER_RESET(open_timer);
		}break;
		case ConsoleState_OpenSmall: {
			open_target = (f32)DeshWindow->height * open_small_percent;
			open_amount = console_dim.y;
			console_pos = vec2(0, -1);
			console_dim.x = (f32)DeshWindow->width;
			flags = UIWindowFlags_NoMove | UIWindowFlags_NoResize | UIWindowFlags_NoScrollX;
			TIMER_RESET(open_timer);
		}break;
		case ConsoleState_OpenBig: {
			open_target = (f32)DeshWindow->height * open_max_percent;
			open_amount = console_dim.y;
			console_pos = vec2(0, -1);
			console_dim.x = (f32)DeshWindow->width;
			flags = UIWindowFlags_NoMove | UIWindowFlags_NoResize | UIWindowFlags_NoScrollX;
			TIMER_RESET(open_timer);
		}break;
		case ConsoleState_Popout: {
			open_target = console_dim.y;
			open_amount = 0;
			flags = 0;
			TIMER_RESET(open_timer);
			
		}break;
	}
	state = new_state;
}

void Console::Init() {
	AssertDS(DS_MEMORY, "Attempt to load Console without loading Memory first");
	AssertDS(DS_LOGGER, "Attempt to load Console without loading Logger first");
	deshiStage |= DS_CONSOLE;
	
	TIMER_START(t_s);
	buffer = Logger::GetFilePtr();
	
	dictionary.dict.init(CONSOLE_DICTIONARY_SIZE, deshi_allocator);
	input_history.init(max_input_history, deshi_allocator);
	
	scroll_to_bottom = 1;
	
	console_pos = vec2::ZERO;
	console_dim = vec2(f32(DeshWindow->width), f32(DeshWindow->height) * open_max_percent);
	
	Logger::SetMirrorToConsole(true);
	LogS("deshi", "Finished console initialization in ", TIMER_END(t_s), "ms");
}

void Console::Update() {
	using namespace UI;
	
	//check for console state changing inputs
	if (DeshInput->KeyPressed(Key::TILDE)) {
		if (DeshInput->ShiftDown()) {
			ChangeState(ConsoleState_OpenBig);
		}
		else if (DeshInput->CtrlDown()) {
			ChangeState(ConsoleState_Popout);
		}
		else {
			ChangeState(ConsoleState_OpenSmall);
		}
	}
	
	//check for history inputs
	b32 change_input = false;
	if(DeshInput->KeyPressed(Key::UP)){
		change_input = true;
		input_history_index--;
		if(input_history_index == -2) input_history_index = input_history.count-1;
	}
	if(DeshInput->KeyPressed(Key::DOWN)){
		change_input = true;
		input_history_index++;
		if(input_history_index >= input_history.count) input_history_index = -1;
	}
	if(change_input){
		ZeroMemory(inputBuf, CONSOLE_INPUT_BUFFER_SIZE);
		if(input_history_index != -1){
			u32 cursor = 0;
			u32 chunk_idx = input_history[input_history_index].first;
			Chunk& chunk = dictionary[chunk_idx];
			
			fseek(buffer, chunk.charstart, SEEK_SET);
			for(;;){
				u32 characters = (cursor + chunk.strsize > CONSOLE_INPUT_BUFFER_SIZE) ? CONSOLE_INPUT_BUFFER_SIZE - (cursor + chunk.strsize) : chunk.strsize;
				fread(inputBuf, sizeof(char), characters, buffer);
				cursor += characters;
				
				if(chunk_idx >= input_history[input_history_index].second || cursor >= CONSOLE_INPUT_BUFFER_SIZE){
					break;
				}else{
					chunk = dictionary[++chunk_idx];
				}
			}
		}
	}
	
	//console openness
	if(open_target != open_amount){
		//TODO change this to Nudge
		console_dim.y = Math::lerp(open_amount, open_target, Min((f32)TIMER_END(open_timer) / open_dt, 1.f));
		if (console_dim.y == open_target) open_amount = open_target;
	}
	
	uistyle = &GetStyle(); //TODO(sushi) try to get this only once
	if (console_dim.y > 0) {
		if (open_target != open_amount || state != ConsoleState_Popout) {
			SetNextWindowPos(console_pos);
			SetNextWindowSize(console_dim);
		}
		
		Begin("deshiConsole", vec2::ZERO, vec2::ZERO, flags);
		conmain = GetWindow(); //TODO(sushi) try to get this only once
		
		SetNextWindowSize(vec2(MAX_F32, GetMarginedBottom() - (uistyle->fontHeight * uistyle->inputTextHeightRelToFont + uistyle->itemSpacing.y) * 3));
		BeginChild("deshiConsoleTerminal", (conmain->dimensions - 2 * uistyle->windowPadding).yAdd(-(uistyle->fontHeight * 1.3 + uistyle->itemSpacing.y)), flags);
		conterm = GetWindow(); //TODO(sushi) try to get this only once
		
		//draw text fro the dictionary
		PushVar(UIStyleVar_WindowPadding, vec2(5, 0));
		PushVar(UIStyleVar_ItemSpacing, vec2(0, 0));
		char toprint[1024];
		for (int i = 0; i < dictionary.count; i++) {
			Chunk& colstr = dictionary[i];
			u32 format = colstr.format;
			
			fseek(buffer, colstr.charstart, SEEK_SET);
			fread(toprint, colstr.strsize + 1, 1, buffer);
			toprint[dictionary[i].strsize] = 0;
			
			
			//adjust what we're going to print based on formatting
			if (format) {
				string tp = toprint;
				if (HasFlag(format, Error)) {
					PushColor(UIStyleCol_Text, Color_Red);
				}
				else if (HasFlag(format, Warning)) {
					PushColor(UIStyleCol_Text, Color_Yellow);
				}
				else if (HasFlag(format, Success)) {
					PushColor(UIStyleCol_Text, Color_Green);
				}
				else if (HasFlag(format, Colored)) {
					PushColor(UIStyleCol_Text, colstr.color);
				}
				if (HasFlag(format, Alert)) {
					//TODO handle alert flashing
				}
				
				static UIItem* last_tag = 0;
				if (tag_show && HasFlag(format, Tagged)) {
					//TODO(sushi) make a boolean for turning off fancy tag showing
					if (!i || dictionary[i - 1].tag != colstr.tag) {
						
						f32 lpy = GetPositionForNextItem().y;
						f32 mr = GetMarginedRight();
						vec2 tagsize = CalcTextSize(colstr.tag);
						
						//look ahead for the end of the tag range
						//dont draw if its not visible
						f32 tag_end = lpy;
						for (int o = i; o < dictionary.count; o++) {
							if (dictionary[o].tag == colstr.tag)
								tag_end += tagsize.y;
							else break;
						}
						if (tag_end > GetMarginedTop()) {
							if (tag_end < tagsize.y) {
								lpy = GetMarginedTop() + (tag_end - tagsize.y);
							}
							else {
								lpy = Max(lpy, GetMarginedTop());
							}
							
							PushColor(UIStyleCol_Text, color(150, 150, 150, 150));
							Text(colstr.tag.str, vec2(mr - tagsize.x, lpy));
							PopColor();
							
							if (tag_outlines && tag_end - (lpy + tagsize.y) > 4) {
								//vertline
								Line(vec2(mr - 1, lpy + tagsize.y + 3), vec2(mr - 1, tag_end - 3), 2, color(150, 150, 150, 150));
								//tag underline
								Line(vec2(mr - tagsize.x, lpy + tagsize.y + 2), vec2(mr, lpy + tagsize.y + 2), 2, color(150, 150, 150, 150));
								//tag coloser
								Line(vec2(mr - tagsize.x, tag_end - 2), vec2(mr, tag_end - 2), 2, color(150, 150, 150, 150));
								
							}
							
							if (tag_highlighting) {
								PushLayer(GetCenterLayer() - 1);
								SetNextItemSize(vec2(GetMarginedRight(), tag_end - lpy));
								PushColor(UIStyleCol_SelectableBg, Color_Clear);
								PushColor(UIStyleCol_SelectableBgHovered, color(155, 155, 155, 10));
								PushColor(UIStyleCol_SelectableBgActive, color(155, 155, 155, 10));
								SetNextItemMinSizeIgnored();
								Selectable("", vec2(0, lpy), 0);
								PopLayer();
								PopColor(3);
							}
							
							SetMarginSizeOffset(vec2(-tagsize.x, 0));
						}
						
						
					}
				}
				
				Text(tp.str);
				if (!colstr.eol && i != (dictionary.count - 1)) SameLine();
				
				if (HasFlag(format, Error | Warning | Success | Colored))
					PopColor();
				
				if (line_highlighing) {
					UIItem* last_text = GetLastItem();
					SetNextItemSize(vec2(GetMarginedRight(), last_text->size.y));
					PushLayer(GetCenterLayer() - 1);
					PushColor(UIStyleCol_SelectableBg, Color_Clear);
					PushColor(UIStyleCol_SelectableBgHovered, color(155, 155, 155, 10));
					PushColor(UIStyleCol_SelectableBgActive, color(155, 155, 155, 10));
					SetNextItemMinSizeIgnored();
					Selectable("", vec2(0, last_text->position.y), 0);
					PopLayer();
					PopColor(3);
				}
			}
			else {
				Text(toprint);
			}
		}
		PopVar(2);
		
		if (scroll_to_bottom) { SetScroll(vec2(0, MAX_F32)); scroll_to_bottom = 0; }
		
		//SetScroll(vec2(0, MAX_F32));
		EndChild();
		
		
		//get text input
		f32 inputBoxHeight = uistyle->inputTextHeightRelToFont * uistyle->fontHeight;
		SetNextItemSize(vec2(MAX_F32, inputBoxHeight));
		vec2 itpos(GetMarginedLeft(), GetMarginedBottom() - inputBoxHeight);
		if (InputText("deshiConsoleInput", inputBuf, CONSOLE_INPUT_BUFFER_SIZE, itpos, "enter a command", UIInputTextFlags_EnterReturnsTrue)) {
			if(inputBuf[0] != '\0'){
				scroll_to_bottom = 1;
				string input = inputBuf;
				string modified = toStr("{{c=cyan}/{}}{{c=dcyan}\\ {}}",input);
				
				u32 start_chunk = dictionary.count+2; //NOTE skip /\ chunks
				AddLog(modified);
				
				u32 end_chunk = dictionary.count-1; //TODO prevent adding duplicate input to history
				input_history.add({start_chunk,end_chunk});
				input_history_index = -1;
				
				Cmd::Run(input);
				memset(inputBuf, 0, CONSOLE_INPUT_BUFFER_SIZE);
			}
		}
		
		PushColor(UIStyleCol_WindowBg, color(0, 25, 18));
		End();
		PopColor();
	}
}
