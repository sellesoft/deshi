local vec2 console_pos;
local vec2 console_dim;

//fancy console opening parameters
local f32 open_max_percent = 0.7f; //percentage of the height of the window to open to
local f32 open_amount = 0.0f;      //current opened amount
local f32 open_target = 0.0f;      //target opened amount
local f32 open_dt = 2000.0f;       //speed at which it opens

//console 
local char inputBuf[256]{};

local constexpr u32 BUFF_SIZE = 0xFFFF; //size very large to try and ensure the dictionary runs out before the buffer does
local constexpr u32 DICT_SIZE = 5;

struct ColoredStr{
	color color;
	char* strptr;
	u32 strsize;
};


//buffer struct that holds all text that has been logged in the console
struct{
	//TODO change this buffer to just be the file that logger uses
	char                   buff[BUFF_SIZE]; //i decided not to use ring_array on char, i will probably write more about why later
	ring_array<ColoredStr> dictionary;
	u32 buff_count = 0;
	
	void add(char* str, u32 strsize, color color) {
		
		
		//check if the dictionary has been filled and is about to wrap around again
		//if it is, we can safely flush the previous DICT_SIZE dictionary entries
		if (dictionary.start == dictionary.capacity - 1) {
			//TODO flush console stuff to the log file
			u32 sztoflsh = dictionary[DICT_SIZE - 1].strptr - buff + dictionary[DICT_SIZE - 1].strsize;
			buff_count -= sztoflsh;
			memset(buff, 0, sztoflsh);
			//this may could be buff_count - sztoflsh
			u32 sztomove = buff_count;
			memmove(buff, buff + sztoflsh, sztomove);
			memset(buff + sztomove, 0, sztomove);
			
			//move all previous things back
			forI(dictionary.count) 	dictionary[i].strptr -= sztoflsh;
		}
		
		memcpy(buff + buff_count, str, strsize);
		dictionary.add(ColoredStr{ color, buff + buff_count, strsize });
		buff_count += strsize;
	}
	
	
} buffer;
local ColoredStr dictionary;   //dictionary that indexes the buffer

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

void Console::Init(){
	buffer.dictionary.init(DICT_SIZE);
}

void Console::Update(){
	
}

void Console::AddLog(string input){
	buffer.add(input.str, input.count, Color_White);
	Log("------------------------------", "--------------");
	Log("buffer", buffer.buff);
	Log("dict start, end", buffer.dictionary.start, " ", buffer.dictionary.end);
	forI(buffer.dictionary.count) {
		Log(TOSTRING("dict", i).str, string(buffer.dictionary.at(i)->strptr, buffer.dictionary.at(i)->strsize));
	}
}

void Console::ChangeState(ConsoleState new_state) {
	
}

void Console::FlushBuffer() {
	
}

void Console::Cleanup() {
	
}

