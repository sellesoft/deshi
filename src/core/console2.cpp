#include "console2.h"
#include "time.h"           //deltaTime
#include "window.h"         //width, height
#include "deshi_imgui.h"    //ImGui
#include "assets.h"         //eat_spaces_leading
#include "../utils/debug.h" //PRINTLN, ASSERT

#include <string_view>      //std::string_view

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

static_internal ConsoleState state = ConsoleState_Closed;

static_internal char input_buffer[256] = {0};
static_internal ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

//TODO(delle,Op) ideally convert this to something better/contiguous
static_internal std::vector<pair<Color, std::string>> history;

std::map<std::string, Color> color_strings{
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

void FlushBuffer(){
	
}

//TODO(delle,Cl) setup a way to get the actual deltaTime outside of the game loop
void UpdateOpenness(){
	f32 delta_open = DengTime->deltaTime * open_dt;
	
	if(open_amount < open_target){
		open_amount += delta_open;
		if(open_amount > open_target) open_amount = open_target;
	}else if(open_amount > open_target){
		open_amount -= delta_open;
		if(open_amount < 0) open_amount = 0;
	}
}

int TextEditCallback(ImGuiInputTextCallbackData* data) {
	return 0;
}

ImVec4 ColorToVec4_(Color p) {
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
	
	switch(new_state){
		case ConsoleState_Closed:{
			state = ConsoleState_Closed;
			open_target = 0;
		}break;
		case ConsoleState_OpenSmall:{
			state = ConsoleState_OpenSmall;
			open_target = (f32)DengWindow->height * 0.2f;
		}break;
		case ConsoleState_OpenBig:{
			state = ConsoleState_OpenBig;
			open_target = (f32)DengWindow->height * open_max_percent;
		}break;
		case ConsoleState_Popout:{ //TODO(Con,delle) setup popout console
			state = ConsoleState_Popout;
			window_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;
		}break;
	}
}

//@Incomplete
//this whole thing sucks, we should store one line as one thing
void Console2::Log(std::string message){
	message += "\n";
	
	int special_start_idx = -1, special_stop_idx = -1;
	int color_start_idx = -1, color_stop_idx = -1;
	Color color_color;
	int chunk_start = 0;
	std::string temp;
	
	for_n(i, message.size()){
		//check for special
		if(message[i] == '^'){
			if(special_start_idx != -1){
				special_stop_idx = i;
				chunk_start = i+1;
			}else{
				special_start_idx = i;
				
				if(color_start_idx == -1){
					temp = message.substr(chunk_start, i-chunk_start);
					if(temp.size()) history.push_back(pair<Color, std::string>(Color::WHITE, temp));
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
					
					temp = message.substr(color_start_idx, color_stop_idx - color_start_idx);
					if(temp.size()) history.push_back(pair<Color, std::string>(color_color, temp));
					
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
			temp = message.substr(chunk_start, i-chunk_start+1);
			if(temp.size()) history.push_back(pair<Color, std::string>(Color::WHITE, temp));
		}
	}
}

void Console2::Init(){
	
}

void Console2::Cleanup(){
	FlushBuffer();
}

void Console2::Draw(){
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
		ImGui::BeginChild("##console_report_region", ImVec2(0, -footer_height_to_reserve), false);{
			//@Incomplete
			ImGuiListClipper clipper; //this doesnt work b/c the way we store history sucks
			clipper.Begin(history.size());
			while(clipper.Step()) for(int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++){
				ImGui::PushStyleColor(ImGuiCol_Text, ColorToVec4_(history[row_n].first));
				ImGui::SameLine(0,0);
				ImGui::TextWrapped("%s", history[row_n].second.c_str());
				ImGui::PopStyleColor();
				if(history[row_n].second[history[row_n].second.size() - 1] == '\n') ImGui::TextWrapped("\n");
			}
			
			if(scroll_to_bottom) {
				ImGui::SetScrollHereY(1); 
				scroll_to_bottom = false;
			}
		}ImGui::EndChild();
		ImGui::PopStyleColor();
		
		//// command text input ////
		ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackAlways;
		if(show_autocomplete) input_text_flags = ImGuiInputTextFlags_None;
		
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ColorToVec4_(Color::VERY_DARK_CYAN));
		ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - 15);
		ImGui::SetItemDefaultFocus();
		//TODO(delle,OpCl) this can be optimized by reducing the amount of string copies
		if(ImGui::InputText("##console_input_text", input_buffer, sizeof(input_buffer), input_text_flags, &TextEditCallback, 0)) {
			//add input to history
			std::string input = deshi::eat_spaces_leading(input_buffer);
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
				//RunCommand(input, args);
				memset(input_buffer, 0, sizeof(input_buffer));
			}
			ImGui::SetKeyboardFocusHere(-1);
		}
		ImGui::PopStyleColor();
		
	}ImGui::End();
	ImGui::PopStyleVar(1);
	ImGui::PopStyleColor(7);
}