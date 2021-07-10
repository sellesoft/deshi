//TODO remove all ImGui stuff from here and do it ourselves since this might be in final release
//TODO setup popout and window console states
//TODO add scrolling and scrollbar
//TODO add PAGEUP and PAGEDOWN binds (CTRL for max scroll) 
//TODO wrapped text inside a coloredpstring
//TODO input history from previous inputs on UP and DOWN arrows
//TODO commands

#include "console2.h"
#include "input.h"
#include "time.h"
#include "window.h"
#include "imgui.h"
#include "assets.h"
#include "../utils/utils.h"
#include "../utils/Debug.h"
#include "../utils/RingArray.h"
#include "../utils/Color.h"

////////////////////////////////////
//// internal console variables ////
////////////////////////////////////

static_internal f32 open_max_percent = 0.7f; //percentage of the height of the window to open to
static_internal f32 open_amount      = 0.0f; //current opened amount
static_internal f32 open_target      = 0.0f; //target opened amount
static_internal f32 open_dt          = 2000.0f; //speed at which it opens

static_internal f32 console_x = 0.0f;
static_internal f32 console_y = 0.0f;
static_internal f32 console_w = 0.0f;
static_internal f32 console_h = 0.0f;

static_internal f32 font_width  = 0.0f;
static_internal f32 font_height = 0.0f;

static_internal b32 scroll_to_bottom = false;
static_internal b32 show_autocomplete = false;

static_internal f32 console_scroll_y = 0;
static_internal u32 console_rows_in_buffer = 0; 

static_internal ConsoleState state = ConsoleState_Closed;
static_internal ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

#define input_max_size       256
#define input_history_length 8
static_internal char input_buffer[256] = {0};
static_internal char input_history[input_max_size*input_history_length] = {0};
static_internal u32  input_history_index = 0;
static_internal u32  input_history_select_index = 0;

struct ColoredPstring{
    Color color;
    char* start;
    char* end;
};
static_internal RingArray<char>           history;
static_internal RingArray<ColoredPstring> dictionary;

static_internal std::vector<pair<Color, char>> historyc;
static_internal std::vector<u32> lineindicies;

static_internal std::map<std::string, Color> color_strings{
    {"red", Color::RED},       {"dred", Color::DARK_RED},
    {"blue", Color::BLUE},     {"dblue", Color::DARK_BLUE},
    {"cyan", Color::CYAN},     {"dcyan", Color::DARK_CYAN},
    {"grey", Color::GREY},     {"dgrey", Color::DARK_GREY},
    {"green", Color::GREEN},   {"dgreen", Color::DARK_GREEN},
    {"yellow", Color::YELLOW}, {"dyellow", Color::DARK_YELLOW},
    {"magen", Color::MAGENTA}, {"dmagen", Color::DARK_MAGENTA},
    {"white", Color::WHITE},   {"black", Color::BLACK}
};

////////////////////////////
//// internal functions ////
////////////////////////////

static_internal void FlushBuffer(){
    //@Incomplete
}

static_internal void UpdateOpenness(){
    f32 delta_open = DengTime->deltaTime * open_dt;
    
    if(open_amount < open_target){
        open_amount += delta_open;
        if(open_amount > open_target) open_amount = open_target;
    }else if(open_amount > open_target){
        open_amount -= delta_open;
        if(open_amount < 0) open_amount = 0;
    }
}

static_internal int TextEditCallback(ImGuiInputTextCallbackData* data) {
    return 0;
}

static_internal ImVec4 ColorToVec4(Color p) {
    return ImVec4((f32)p.r / 255.f, (f32)p.g / 255.f, (f32)p.b / 255.f, (f32)p.a / 255.f);
}

/////////////////////////////
//// interface functions ////
/////////////////////////////

bool Console2::IsOpen(){
    return open_target > 0;
}

void Console2::Toggle(ConsoleState new_state){
    window_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
    if(state == new_state) new_state = ConsoleState_Closed;
	intercepting_inputs = true;
    
    switch(new_state){
        case ConsoleState_Closed:{
            state = ConsoleState_Closed;
            open_target = 0;
			intercepting_inputs = false;
        }break;
        case ConsoleState_OpenSmall:{
            state = ConsoleState_OpenSmall;
            open_target = (f32)DengWindow->height * 0.2f;
        }break;
        case ConsoleState_OpenBig:{
            state = ConsoleState_OpenBig;
            open_target = (f32)DengWindow->height * open_max_percent;
        }break;
        case ConsoleState_Popout:{
            state = ConsoleState_Popout;
            //@Incomplete
			//create a moveable and resizable ImGui-style window
        }break;
        case ConsoleState_Window:{
            state = ConsoleState_Window;
            //@Incomplete
			//create a new GLFW window child of the deshi window
        }break;
    }
}

//    ^c=cyan^/^c^^c=dcyan^\\^c^ - reference
//    ^c=red^red^c^white^c=blue^blue^c^
void Console2::Log(std::string message){
    message += "\n";
	
    //// history version ////
	
    int special_start_idx = -1, special_stop_idx = -1;
    int color_start_idx = -1, color_stop_idx = -1;
    Color color_color;
    int chunk_start = 0;
    std::string temp;
    
    for(int i = 0; i < message.size(); ++i){
    	//check for special
    	if(message[i] == '^'){
    		if(special_start_idx != -1){
    			special_stop_idx = i;
    			chunk_start = i+1;
    		}else{
    			special_start_idx = i;
    			
    			if((color_start_idx == -1) && (i != chunk_start)){
                    u32 idx = history.count;
                    history.Add(message.c_str()+chunk_start, i-chunk_start);
                    dictionary.Add(ColoredPstring{Color::WHITE, history.At(idx), history.At(idx)+(i-chunk_start)});
    			}
    		}
    	}
    	
    	//parse special
    	if(special_stop_idx != -1){
    		std::string special_text = message.substr(special_start_idx + 1, special_stop_idx - special_start_idx - 1);
    		if(special_text[0] == 'c'){ //color special
    			if(color_start_idx == -1){
    				color_start_idx = special_stop_idx+1;
    				
    				if(special_text.size() > 2){
    					color_color = color_strings.at(special_text.substr(2));
    				}
    			}else{
    				color_stop_idx = special_start_idx;
    				
                    if(color_stop_idx != color_start_idx){
                        u32 idx = history.count;
                        history.Add(message.c_str()+color_start_idx, color_stop_idx-color_start_idx);
                        dictionary.Add(ColoredPstring{color_color, history.At(idx), history.At(idx)+(color_stop_idx-color_start_idx)});
                    }
    				
    				color_start_idx = -1;
    				color_stop_idx = -1;
    			}
    		}else if(special_text[0] == 'a'){ //alert special
    			
    			
    		}else{ //unhandled special
    			
    		}
    		
    		special_start_idx = -1;
    		special_stop_idx = -1;
    	}
    	
    	if(message[i] == '\n'){
            if((i-chunk_start+1 - chunk_start) != 0){
                u32 idx = history.count;
                history.Add(message.c_str()+chunk_start, (i-chunk_start)+2); //+2 to copy the \0 as well
                dictionary.Add(ColoredPstring{Color::WHITE, history.At(idx), history.At(idx)+((i-chunk_start)+1)});
            }
    	}
    }
	
    //// historyc version ////
    
    //if (lineindicies.size() == 0) {
    //	lineindicies.push_back(0);
    //}
    //else {
    //	lineindicies.push_back(historyc.size());
    //}
    //
    //Color currCol = Color::WHITE;
    //
    //for (int i = 0; i < message.size(); i++) {
    //	char ch = message[i];
    //
    //	//check for color formatting
    //	if (ch == '^') { 
    //		if (currCol == Color::WHITE) {
    //			//char indicating our color formatting was found
    //			//so we expect the next char to be c followed by =, then blah blah
    //			//if any of these fail then we ignore it 
    //			if (message[++i] == 'c') {
    //				if (message[++i] == '=') {
    //					//read name of color
    //					ch = message[++i];
    //					std::string col;
    //					while (ch != '^') {
    //						col += ch;
    //						if (i + 1 != message.size()) ch = message[++i];
    //						else { /*TODO(sushi) implement error checking here */ break; }
    //					}
    //
    //					//set the color if it's known
    //					try {
    //						currCol = color_strings.at(col);
    //					} catch(...) { /*unknown color*/ }
    //
    //					ch = message[++i];
    //				}
    //				else i -= 2;
    //			}
    //			else i--;
    //		}
    //		else {
    //			//it may be the end of color formatting
    //			if (message[++i] == 'c') {
    //				if (message[++i] == '^') {
    //					currCol = Color::WHITE;
    //					ch = -1;
    //				} else i -= 2;
    //			} else i--;
    //			
    //		}
    //	}
    //
    //	if (ch != -1) {
    //		historyc.push_back(pair<Color, char>(currCol, ch));
    //	}
    //}
}

void Console2::Init(){
    history.Init(8192);
    dictionary.Init(512);
}

void Console2::Cleanup(){
    FlushBuffer();
}

void Console2::Update(){
	{//// handle inputs ////
		//open and close console
		if(DengInput->KeyPressedAnyMod(Key::F1)){
			if(DengInput->ShiftDown()){
				Toggle(ConsoleState_OpenBig);
			}else{
				Toggle(ConsoleState_OpenSmall);
			}
		}
		
		if(intercepting_inputs){
			//@Incomplete
			//scrolling
			if(DengInput->KeyDownAnyMod(MouseButton::SCROLLUP)) {
				//console_scroll_y--;
			}
			if(DengInput->KeyDownAnyMod(MouseButton::SCROLLDOWN)) {
				//console_scroll_y++;
			}
			
			//@Incomplete
			//input history
			if(DengInput->KeyDownAnyMod(Key::UP)) {
				input_history_select_index += 1;
				if(input_history_select_index > input_history_length) input_history_select_index = 0;
				if(input_history_select_index) {
					memcpy(input_history, input_history+(input_history_select_index*input_max_size), input_max_size);
				}else{
					memset(input_history, 0, input_max_size);
				}
			}
			if(DengInput->KeyDownAnyMod(Key::DOWN)) {
				input_history_select_index -= 1;
				if(input_history_select_index < 0) input_history_select_index = input_history_length;
				if(input_history_select_index) {
					memcpy(input_history, input_history+(input_history_select_index*input_max_size), input_max_size);
				}else{
					memset(input_history, 0, input_max_size);
				}
			}
		}
	}
	{//// draw console ////
		UpdateOpenness(); 
		console_w = (f32)DengWindow->width;
		console_h = open_amount;
		if(!open_amount) return; //early out if fully closed
		
		ImGuiStyle& style = ImGui::GetStyle();
		style.AntiAliasedFill = false;
		style.AntiAliasedLines = false;
		style.AntiAliasedLinesUseTex = false;
		style.WindowMinSize = ImVec2(1.f,1.f);
		
		ImGuiIO& io = ImGui::GetIO();
		//io.BackendFlags = ImGuiBackendFlags_HasGamepad | ImGuiBackendFlags_HasMouseCursors | ImGuiBackendFlags_HasSetMousePos;
		//io.ConfigWindowsMoveFromTitleBarOnly = true;
		//io.ConfigWindowsResizeFromEdges = true;
		
		font_height = ImGui::GetFontSize();
		font_width = ceil(font_height / 2);
		
		ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 0);
		ImGui::PushStyleColor(ImGuiCol_Border,               ImVec4(  0.f,   0.f,   0.f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_TitleBg,              ImVec4(  0.f,   0.f,   0.f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_WindowBg,             ImVec4(  0.f,   0.f,   0.f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_TitleBgActive,        ImVec4(  0.f,   0.f,   0.f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab,        ImVec4(.106f, .141f, .141f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive,  ImVec4(  0.f, .369f, .326f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(.188f, .333f, .353f, 1.f));
		ImGui::SetNextWindowPos(ImVec2(console_x, console_y));
		ImGui::SetNextWindowSize(ImVec2(console_w, console_h));
		
		ImGui::Begin("##console_window", 0, window_flags);{
			//// history report region ////
			f32 footer_height_to_reserve = style.ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(.016f, .067, .082f, 1.f));
			ImGui::BeginChild("##console_report_region", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);{
				
				//// history version ////
				if(dictionary.count){
					f32 x = 0;
					f32 y = ImGui::GetWindowHeight();
					ImGui::PushTextWrapPos(0.0f);
					
					//@Incomplete
					//handle scrolling
					
					u32 str_start = dictionary.end;
					u32 str_end   = dictionary.end;
					while(y > 0){
						//@Incomplete
						//crashes when going past max size
						if((str_start != dictionary.start) && (*(dictionary[str_start-1].end - 1) != '\n')) { //skip until string end
							str_start -= 1;
							if(str_start == -1) str_start = dictionary.capacity-1;
							continue;
						}
						
						//calc text y-draw position
						float string_width = ImGui::CalcTextSize(dictionary[str_start].start, dictionary[str_end].end).x;
						y -= font_height * ceil(string_width / ImGui::GetWindowWidth());
						
						//draw text
						for(int i = str_start; i <= str_end; ++i){
							ImGui::SetCursorPos(ImVec2(x, y));
							ImGui::PushStyleColor(ImGuiCol_Text, ColorToVec4(dictionary[i].color));
							ImGui::TextEx(dictionary[i].start, dictionary[i].end, ImGuiTextFlags_NoWidthForLargeClippedText);
							ImGui::PopStyleColor();
							
							//@Incomplete
							//handle text wrapping
							x += ImGui::CalcTextSize(dictionary[i].start, dictionary[i].end).x;
							if(x > ImGui::GetWindowWidth()){
								y += font_height;
								x = 0;
							}
						}
						x = 0;
						
						if(str_start == dictionary.start) break; //end loop after using last element
						str_start -= 1; //decrement the iterator
						if(str_start == -1) str_start = dictionary.capacity-1; //move the iterator to the other end of the ring
						str_end = str_start;
					}
					ImGui::PopTextWrapPos();
				}
				
				//// historyc version ////
				
				//float winw = ImGui::GetWindowWidth();
				//float winh = ImGui::GetWindowHeight();
				//
				//int chars_can_fit = winw / font_width;
				//int rows_can_fit = winh / font_height;
				//
				////manual scrolling since we implement our own clipper
				//if (DengInput->KeyDownAnyMod(MouseButton::SCROLLUP) && console_scroll_y > 0) {
				//console_scroll_y--;
				//}
				//if (DengInput->KeyDownAnyMod(MouseButton::SCROLLDOWN) && lineindicies.size() > rows_can_fit) {
				//	console_scroll_y++;
				//}
				//ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 0));
				//int rows = 0;
				//for (int i = 0, j = (lineindicies.size() > 0) ? lineindicies[console_scroll_y] : 0; j < historyc.size() && rows < rows_can_fit; i++, j++) {
				//	ImGui::PushStyleColor(ImGuiCol_Text, ColorToVec4(historyc[j].first));
				//	
				//	//we must wrap a command if it reaches the end of the screen or a newline is found
				//	if (i == chars_can_fit || historyc[j].second == '\n') { 
				//		ImGui::TextWrapped("\n");
				//		i = 0;
				//		rows++;
				//	}
				//	else {
				//		ImGui::SameLine(0, 0);
				//		char str[2]{ historyc[j].second, '\0' };
				//		ImGui::Text(str);
				//	}
				//	ImGui::PopStyleColor();
				//}
				//if (console_scroll_y > lineindicies.size() - rows_can_fit) {
				//	console_scroll_y = lineindicies.size() - rows_can_fit;
				//}
				//
				//if (scroll_to_bottom && lineindicies.size() > rows_can_fit) {
				//	console_scroll_y = lineindicies.size() - rows_can_fit;
				//	scroll_to_bottom = false;
				//}
				//ImGui::PopStyleVar();
			}ImGui::EndChild();
			ImGui::PopStyleColor();
			
			//// command text input ////
			ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackAlways;
			if(show_autocomplete) input_text_flags = ImGuiInputTextFlags_None;
			
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ColorToVec4(Color::VERY_DARK_CYAN));
			ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - 16);
			//TODO(delle,OpCl) this can be optimized by reducing the amount of string copies
			if(ImGui::InputText("##console_input_text", input_buffer, input_max_size, input_text_flags, &TextEditCallback, 0)) {
				//add input to history
				std::string input = Utils::eatSpacesLeading(input_buffer);
				Log(TOSTRING("^c=cyan^/^c^^c=dcyan^\\^c^ ", input));
				scroll_to_bottom = true;
				
				//send input to command system
				if(input.size()){
					std::string args;
					size_t t = input.find_first_of(' ');
					if(t != -1){
						args = input.substr(t);
						input.erase(t, input.size()-1);
					}
					
					//@Incomplete
					//add input to input_history if not already in it
					b32 already_in_history = false;
					for(int i = 0; i < input_history_length; ++i){
						if(strcmp(input_buffer, input_history+(input_max_size*i)) == 0){
							already_in_history = true;
							break;
						}
					}
					if(!already_in_history){
						memcpy(input_history+(input_max_size*input_history_index), input_buffer, input_max_size);
						input_history_index = (input_history_index + 1) % (input_history_length - 1);
					}
					
					//@Incomplete
					//RunCommand(input, args);
					memset(input_buffer, 0, input_max_size);
				}
				ImGui::SetKeyboardFocusHere(-1);
			}
			ImGui::SetItemDefaultFocus();
			ImGui::PopStyleColor();
			
		}ImGui::End();
		ImGui::PopStyleVar(1);
		ImGui::PopStyleColor(7);
	}
}