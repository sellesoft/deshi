#include "ConsoleSystem.h"

#include "../components/Console.h"

#include "../core/input.h"

#include "../utils/Command.h"
#include <time.h>
#include <iomanip>
#include <sstream>
#include <ctime>

#include <filesystem>

#include "time.h"

#include "../external/imgui/imgui_impl_vulkan.h"
#include "../external/imgui/imgui_impl_glfw.h"

#include <fstream>

#define RegColorFormat std::regex("(?:\\[c:([^\\]]*)\\]([^\\]]*)\\[c\\]|([^\\[]+))")

using namespace ImGui;

int buffersize = 0;

//this is necessary so the textcallback stub can access a function in the obj
//it has to be static i dont really know why maybe ill fix it when i go to rewrite
//this ugly thing
Console* me; 

bool sel_com = false; //true when selecting an auto complete possibility
bool sel_com_ret = false; //tells the callback function that it is going to replace text
std::string sel_com_str = ""; //the string we're replacing input with
std::vector<std::string> posis;
//int match_sel = 0;

std::map<std::string, Color> colstrmap{
	{"red", Color::RED},
	{"dred", Color::DARK_RED},
	{"blue", Color::BLUE},
	{"dblue", Color::DARK_BLUE},
	{"cyan", Color::CYAN},
	{"dcyan", Color::DARK_CYAN},
	{"grey", Color::GREY},
	{"dgrey", Color::DARK_GREY},
	{"green", Color::GREEN},
	{"dgreen", Color::DARK_GREEN},
	{"yellow", Color::YELLOW},
	{"dyellow", Color::DARK_YELLOW},
	{"magen", Color::MAGENTA},
	{"dmagen", Color::DARK_MAGENTA},
	{"black", Color::BLACK},
	{"error", Color::RED} //special error color for the console to know when to flash the debug bar
};

ImVec4 ColorToVec4(Color p) {
	return ImVec4((float)p.r / 255, (float)p.g / 255, (float)p.b / 255, p.a / 255);
}

void Console::AddLog(std::string input) {
	
	std::smatch m;
	
	while (std::regex_search(input, m, RegColorFormat)) { //parse text for color formatting
		
		//check if were dealing with a formatted part of the string
		if (std::regex_search(m[0].str(), std::regex("\\[c:[^\\]]+\\]"))) {
			//if we are, push the actual text with its color into text vector
			buffer.push_back(std::pair<std::string, Color>(m[2].str(), colstrmap.at(m[1])));
			buffersize += m[2].str().size();
			if (m[1] == "error") {
				admin->cons_error_warn = true;
				admin->last_error = m[2].str();
			}
		}
		else {
			//if we arent then just push the line into text vector
			buffer.push_back(std::pair<std::string, Color>(m[0].str(), Color::BLANK));
			buffersize += m[2].str().size();
		}
		input = m.suffix();
	}
	buffer[buffer.size() - 1].first += "\n";
	
}

std::string Console::ExecCommand(std::string command, std::string args) {
	if (admin->commands.find(command) != admin->commands.end()) {
		return admin->commands.at(command)->Exec(admin, args);
	}
	else {
		return "[c:red]Command[c] \"" + command + "\" [c:red]not found.[c]";
	}
	
	//admin->
}

int Console::TextEditCallback(ImGuiInputTextCallbackData* data) {
	switch (data->EventFlag) {
		case ImGuiInputTextFlags_CallbackCompletion:{
			std::string input = data->Buf;
			
			int fwordl = 0;
			
			if (std::regex_search(input, std::regex("^.+ +"))) {
				fwordl = input.find_first_of(" ") + 1;
				input.erase(0, input.find_first_of(" ") + 1);
				
			}
			
			std::regex e("^" + input + ".*");
			std::vector<std::string> posi;
			for (std::pair<std::string, Command*> c : admin->commands) {
				if (std::regex_search(c.first, e)) {
					posi.push_back(c.first);
				}
			}
			//TODO( sushi,Cmd) implement showing a commands help if tab is pressed when the command is already typed
			
			if (posi.size() == 0) {
				AddLog("no matches found");
			}
			else if (posi.size() == 1) {
				data->DeleteChars(fwordl, data->BufTextLen - fwordl);
				data->InsertChars(data->CursorPos, posi[0].c_str());
			}
			else { //if there are multiple we handle them in a selection table
				posis.clear();
				posis = posi;
				sel_com = true;
				
				
			}
			
			scrollToBottom = true; //scroll to bottom when auto completing 
			
			break;
		}
		case ImGuiInputTextFlags_CallbackHistory: {
			
			const int prev_hist_pos = historyPos;
			if (data->EventKey == ImGuiKey_UpArrow) {
				if (historyPos == -1) {
					historyPos = history.size() - 1;
				}
				else if (historyPos > 0) {
					historyPos--;
				}
			}
			else if (data->EventKey == ImGuiKey_DownArrow) {
				if (historyPos != -1) {
					if (++historyPos >= history.size()) {
						historyPos = -1;
					}
				}
			}
			
			if (prev_hist_pos != historyPos)
			{
				std::string history_str = (historyPos >= 0) ? history[historyPos] : "";
				data->DeleteChars(0, data->BufTextLen);
				data->InsertChars(0, history_str.c_str());
			}
			
			break;
		}
		case ImGuiInputTextFlags_CallbackAlways: {
			if (sel_com_ret) {
				std::string str = data->Buf;
				
				int fwordl = 0; //we need to make sure we don't override valid input
				if (std::regex_search(str, std::regex("^.+ *"))) {
					fwordl = str.find_first_of(" ") + 1;
					if (fwordl == 0) {
						str.erase(0, str.length());
					}
					else {
						str.erase(0, str.find_first_of(" ") + 1);
					}
					
				}
				
				str += sel_com_str;
				
				data->DeleteChars(0, data->BufTextLen - fwordl);
				data->InsertChars(data->BufTextLen, str.c_str());
				sel_com_ret = false;
			}
		}
	}
	
	return 0;
}



int Console::TextEditCallbackStub(ImGuiInputTextCallbackData* data) {
	return me->TextEditCallback(data);
}


void Console::DrawConsole() {
	
	
	ImGuiStyle& style = ImGui::GetStyle();
	ImGuiIO& io = ImGui::GetIO();
	
	style.AntiAliasedFill = false;
	style.AntiAliasedLines = false;
	style.AntiAliasedLinesUseTex = false;
	
	//for some reason these werent set in the actual backend and it was causing issues
	//this could no longer be true since we switch to Vulkan but ima keep it just incase :)
	io.BackendFlags = ImGuiBackendFlags_HasGamepad | ImGuiBackendFlags_HasMouseCursors | ImGuiBackendFlags_HasSetMousePos;
	io.ConfigWindowsMoveFromTitleBarOnly = true;
	io.ConfigWindowsResizeFromEdges = true;
	
	//window styling
	PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 0);
	PushStyleColor(ImGuiCol_Border,               ColorToVec4(Color(0, 0, 0, 255)));
	PushStyleColor(ImGuiCol_TitleBg,              ColorToVec4(Color(0, 0, 0, 255)));
	PushStyleColor(ImGuiCol_WindowBg,             ColorToVec4(Color(0, 0, 0, 255)));
	PushStyleColor(ImGuiCol_TitleBgActive,        ColorToVec4(Color(0, 0, 0, 255)));
	PushStyleColor(ImGuiCol_ScrollbarGrab,        ColorToVec4(Color(37, 36, 36, 255)));
	PushStyleColor(ImGuiCol_ScrollbarGrabActive,  ColorToVec4(Color(0, 94, 83, 255)));
	PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ColorToVec4(Color(48, 85, 90, 255)));
	
	//initialize console window
	SetNextWindowSize(ImVec2(DengWindow->width, DengWindow->height / 1.5));
	SetNextWindowPos(ImVec2(0, 0));
	
	ImGui::Begin("Console!", 0, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove);
	
	
	if (BeginMenuBar()) {
		if (BeginMenu("Console")) {
			if (MenuItem("Clear")) { buffer.clear(); }
			if (MenuItem("Autoscroll", 0, &autoScroll)) { autoScroll = !autoScroll; }
			ImGui::EndMenu();
		}
		EndMenuBar();
	}
	bool reclaim_focus = false;
	
	//display completion table
	//this could probably be done in a better way but idc it works
	static int match_sel = 0;
	bool ok_flag = false;
	if (sel_com) {
		bool selected = false;
		bool escape = false;
		if (DengInput->KeyPressed(Key::DOWN) && match_sel < posis.size() - 1) { match_sel++; }
		if (DengInput->KeyPressed(Key::UP)   && match_sel > 0)                { match_sel--; }
		if (DengInput->KeyPressed(Key::ENTER)) { selected = true; reclaim_focus = true; }
		if (DengInput->KeyPressed(Key::ESCAPE)) { escape = true; match_sel = 0; reclaim_focus = true; }
		
		if (escape) { ok_flag = true; }
		else {
			ImGui::SetNextItemOpen(1);
			if (TreeNode("match table")) {
				if (BeginChild("matchScroll", ImVec2(0, 100), false)) {
					if (BeginTable("match table", 1, ImGuiTableFlags_BordersH)) {//posi.size())) {
						
						int i = 0;
						for (std::string s : posis) {
							TableNextColumn();
							if (i == match_sel) {
								SetScrollHereY(0);
								PushStyleColor(ImGuiCol_Text, ColorToVec4(Color::RED));
								Text(s.c_str());
								ImGui::PopStyleColor();
								if (selected) {
									sel_com_ret = true;
									sel_com_str = s;
									ok_flag = true;
									match_sel = 0;
									break;
								}
							}
							else {
								Text(s.c_str());
							}
							i++;
						}
						EndTable();
					}
					EndChild();
				}
				TreePop();
			}
		}
		
	}
	
	// Reserve enough left-over height for 1 separator + 1 input text
	const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
	PushStyleColor(ImGuiCol_ChildBg, ColorToVec4(Color(4, 17, 21, 255)));
	BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar);
	if (BeginPopupContextWindow()){
		if (ImGui::Selectable("hehe")) AddLog("hoho");
		EndPopup();
	}
	
	
	//print previous text
	for (std::pair<std::string, Color> p : buffer) {
		//color formatting is "[c:red]text[c] text text"
		//TODO( sushi,OpCon) maybe optimize by only drawing what we know will be displayed on screen instead of parsing through all of it
		
		if (p.second == Color::BLANK) {
			SameLine(0, 0);
			TextWrapped(p.first.c_str());
		}
		else {
			PushStyleColor(ImGuiCol_Text, ColorToVec4(p.second));
			SameLine(0, 0);
			TextWrapped(p.first.c_str());
			ImGui::PopStyleColor();
		}
		
		if (p.first[p.first.size() - 1] == '\n') {
			TextWrapped("\n");
		}
	}
	
	//auto scroll window
	if (scrollToBottom || (autoScroll && GetScrollY() >= GetScrollMaxY())) SetScrollHereY(1);
	scrollToBottom = false;
	
	EndChild();
	ImGui::PopStyleColor();
	//get input from text box
	ImGuiInputTextFlags input_text_flags = 0;
	if (!sel_com)  input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackAlways;
	else  input_text_flags = 0;
	
	
	PushStyleColor(ImGuiCol_FrameBg, ColorToVec4(Color::VERY_DARK_CYAN));
	SetNextItemWidth(ImGui::GetWindowWidth() - 15);
	ImGui::SetItemDefaultFocus();
	if (InputText("", inputBuf, sizeof(inputBuf), input_text_flags, &TextEditCallbackStub, (void*)this)) { 
		
		std::string s = inputBuf;
		reclaim_focus = true;
		
		if(s.size() != 0) history.push_back(s);
		
		AddLog(TOSTRING("[c:cyan]/[c][c:dcyan]\\[c] ", s)); //print command typed
		
		//cut off arguments into their own string
		std::string args;
		size_t t = s.find_first_of(" ");
		if (t != std::string::npos) {
			args = s.substr(t);
			s.erase(t, s.size() - 1);
		}
		
		if (s.size() != 0) {
			AddLog(ExecCommand(s, args)); //attempt to execute command and print result
		}
		
		historyPos = -1; //reset history position
		
		memset(inputBuf, 0, sizeof(s)); //erase input from text box
		
		scrollToBottom = true; //scroll to bottom when we press enter
	}
	
	ImGui::SetItemDefaultFocus();
	if (reclaim_focus)
		ImGui::SetKeyboardFocusHere(-1);
	
	reclaim_focus = false;
	
	admin->IMGUI_KEY_CAPTURE = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
	
	        ImGui::PopStyleColor();
	    ImGui::PopStyleColor();	    ImGui::PopStyleColor();    
    ImGui::PopStyleColor();	            ImGui::PopStyleColor();    ImGui::PopStyleVar();
        ImGui::PopStyleColor();     ImGui::PopStyleColor();             
            ImGui::PopStyleColor();
	
	//if we selected something from completion menu
	//we have to do this here to prevent enter from sending a command
	if (ok_flag) { sel_com = false; }
	
	
	ImGui::End();
}

//this must be a separate funciton because TextEditCallback had a fit when I tried
//making this the main AddLog function
void Console::PushConsole(std::string s) {
	AddLog(s);
}

//flushes the buffer to a file once it reaches a certain size
void Console::FlushBuffer() {
	std::string output = "";
	for (auto a : buffer) {
		output += a.first;
	}

	if (!std::filesystem::is_directory("logs")) {
		std::filesystem::create_directory("logs");
	}
	

	static std::string filename = TOSTRING("logs/deshiLog_", "-", DengTime->month, "-", DengTime->day, "-", DengTime->year, "_",  DengTime->hour, ".", DengTime->minute, ".", DengTime->second, ".txt");
	static bool session = false;

	std::ofstream file;

	//if start of session make new file
	if (!session) {
		
		file.open(filename);
		file << TOSTRING("Deshi Console Log ", DengTime->weekday, " ", DengTime->month, "/", DengTime->day, "/", DengTime->year, " ", DengTime->hour, ":", DengTime->minute, ":", DengTime->second) << std::endl;
		file << "\n" << output;
		session = true;
	
	}
	else {
		file.open(filename, std::fstream::app);
		file << output;
	}
	
}

void Console::Init() {
	me = this;

	AddLog("[c:dcyan]Deshi Console ver. 0.5.1[c]");
	AddLog("\"listc\" for a list of commands\n\"help {command}\" to view a commands help page");
	AddLog("see console_release_notes.txt for version information");
	AddLog("\n[c:dyellow]Console TODOS:[c]");
	AddLog(
		   "> implement argument completion for commands\n"
		   "> implement arguments for commands that need them\n"
		   "> add help to commands that don't have a descriptive help yet\n"
		   "> fix tabcompletion when trying to complete the first word\n"
		   "> (maybe) rewrite to use characters in the buffer rather than whole strings\n"
		   "> (maybe) implement showing autocomplete as you type");
}

void Console::Update() {
	if (DengInput->KeyPressed(Key::TILDE)) {
		dispcon = !dispcon;
		admin->IMGUI_KEY_CAPTURE = !admin->IMGUI_KEY_CAPTURE; 
	}
	if (dispcon) DrawConsole();
	me = this;
	
	if (buffersize >= 120000) {
		FlushBuffer();
		buffer.clear();
		buffersize = 0;
	}

	if (dispcon && admin->cons_error_warn) admin->cons_error_warn = false;
}
