#include "../game/admin.h"
#include "../game/components/Camera.h"
#include "../game/components/Physics.h"
#include "../game/components/Collider.h"
#include "../game/components/AudioSource.h"
#include "../game/components/MeshComp.h"
#include "../game/components/Player.h"
#include "../game/components/Movement.h"
#include "../game/entities/Trigger.h"
#include "../external/imgui/imgui_impl_glfw.h"
#include "../external/imgui/imgui_impl_vulkan.h"

//regex for checking paramaters
std::regex RegColorFormat("(?:\\[c:([^\\]]*)\\]([^\\]]*)\\[c\\]|([^\\[]+))", std::regex::optimize);
std::regex StringRegex(const char* param){ return std::regex(std::string("-")+ param +"=([A-z]+)", std::regex::optimize|std::regex::icase); }
std::regex IntRegex(const char* param)   { return std::regex(std::string("-")+ param +"=([-]?[0-9]+)", std::regex::optimize); }
std::regex FloatRegex(const char* param){ return std::regex(std::string("-")+ param +"=([-]?[0-9|.]+)", std::regex::optimize); }
std::regex BoolRegex(const char* param)  { return std::regex(std::string("-")+ param +"=(true|1|false|0)", std::regex::optimize|std::regex::icase); }
std::regex Vec3Regex(const char* param)  { return std::regex(std::string("-")+ param +"=\\(([-]?[0-9|.]+),([-]?[0-9|.]+),([-]?[0-9|.]+)\\)", std::regex::optimize); }

using namespace ImGui;

local bool mirror_logging_to_stdout = false;
int buffersize = 0;

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

ImVec4 ColorToVec4(Color p){
	return ImVec4((float)p.r / 255, (float)p.g / 255, (float)p.b / 255, p.a / 255);
}

void Console::AddLog(std::string input){
	
	if(this){
		std::smatch m;
		
		while(std::regex_search(input, m, RegColorFormat)){ //parse text for color formatting
			
			//check if were dealing with a formatted part of the string
			if(std::regex_search(m[0].str(), std::regex("\\[c:[^\\]]+\\]"))){
				//if we are, push the actual text with its color into text vector
				buffer.push_back(pair<std::string, Color>(m[2].str(), colstrmap.at(m[1])));
				buffersize += m[2].str().size();
				if(m[1] == "error"){
					show_alert = true;
					alert_message = m[2].str();
					alert_count++;
				}
				
				
			}
			else {
				//if we arent then just push the line into text vector
				buffer.push_back(pair<std::string, Color>(m[0].str(), Color::BLANK));
				buffersize += m[2].str().size();
			}
			input = m.suffix();
		}
		buffer[buffer.size() - 1].first += "\n";
	}
	
	if(mirror_logging_to_stdout) std::cout << input << std::endl;
}


Command* Console::GetCommand(std::string command){
	try {
		return commands.at(command);
	} catch (std::exception e){
		ERROR("Command, '",command,"' does not exist");
		return 0;
	}
}

std::string Console::ExecCommand(std::string command){
	if(commands.find(command) != commands.end()){
		return commands.at(command)->Exec(g_admin, "");
	}
	else {
		ERROR("Command, '",command,"' does not exist");
		return "";
	}
}

std::string Console::ExecCommand(std::string command, std::string args){
	if(commands.find(command) != commands.end()){
		return commands.at(command)->Exec(g_admin, args);
	}
	else {
		ERROR("Command, '",command,"' does not exist");
		return "";
	}
}

int Console::TextEditCallback(ImGuiInputTextCallbackData* data){
	switch(data->EventFlag){
		case ImGuiInputTextFlags_CallbackCompletion: {
			std::string input = data->Buf;
			
			int fwordl = 0;
			
			if(std::regex_search(input, std::regex("^.+ +"))){
				fwordl = input.find_first_of(" ") + 1;
				input.erase(0, input.find_first_of(" ") + 1);
				
			}
			
			std::regex e("^" + input + ".*");
			std::vector<std::string> posi;
			for (auto& c : commands){
				if(std::regex_search(c.first, e)){
					posi.push_back(c.first);
				}
			}
			//TODO( sushi,Cmd) implement showing a commands help if tab is pressed when the command is already typed
			
			if(posi.size() == 0){
				AddLog("no matches found");
			}
			else if(posi.size() == 1){
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
			if(data->EventKey == ImGuiKey_UpArrow){
				if(historyPos == -1){
					historyPos = history.size() - 1;
				}
				else if(historyPos > 0){
					historyPos--;
				}
			}
			else if(data->EventKey == ImGuiKey_DownArrow){
				if(historyPos != -1){
					if(++historyPos >= history.size()){
						historyPos = -1;
					}
				}
			}
			
			if(prev_hist_pos != historyPos)
			{
				std::string history_str = (historyPos >= 0) ? history[historyPos] : "";
				data->DeleteChars(0, data->BufTextLen);
				data->InsertChars(0, history_str.c_str());
			}
			
			break;
		}
		case ImGuiInputTextFlags_CallbackAlways: {
			if(sel_com_ret){
				std::string str = data->Buf;
				
				int fwordl = 0; //we need to make sure we don't override valid input
				if(std::regex_search(str, std::regex("^.+ *"))){
					fwordl = str.find_first_of(" ") + 1;
					if(fwordl == 0){
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



int Console::TextEditCallbackStub(ImGuiInputTextCallbackData* data){
	return DengConsole->TextEditCallback(data);
}


void Console::DrawConsole(){
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
	PushStyleColor(ImGuiCol_Border, ColorToVec4(Color(0, 0, 0, 255)));
	PushStyleColor(ImGuiCol_TitleBg, ColorToVec4(Color(0, 0, 0, 255)));
	PushStyleColor(ImGuiCol_WindowBg, ColorToVec4(Color(0, 0, 0, 255)));
	PushStyleColor(ImGuiCol_TitleBgActive, ColorToVec4(Color(0, 0, 0, 255)));
	PushStyleColor(ImGuiCol_ScrollbarGrab, ColorToVec4(Color(37, 36, 36, 255)));
	PushStyleColor(ImGuiCol_ScrollbarGrabActive, ColorToVec4(Color(0, 94, 83, 255)));
	PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ColorToVec4(Color(48, 85, 90, 255)));
	
	//initialize console window
	if(!window){
		SetNextWindowSize(ImVec2(DengWindow->width, DengWindow->height / 1.5));
		SetNextWindowPos(ImVec2(0, 0));
	}
	
	ImGui::Begin("Console!", 0);
	
	//capture mouse if hovering over this window
	//TODO(sushi, InCon) this is working for some reason pls fix it 
	//if(IsWindowHovered()) admin->canvas->ConsoleHovFlag = true; 
	//else admin->canvas->ConsoleHovFlag = false; 
	
	bool reclaim_focus = false;
	
	//display completion table
	//this could probably be done in a better way but idc it works
	persist int match_sel = 0;
	bool ok_flag = false;
	if(sel_com){
		bool selected = false;
		bool escape = false;
		if(DengInput->KeyPressed(Key::DOWN) && match_sel < posis.size() - 1){ match_sel++; }
		if(DengInput->KeyPressed(Key::UP) && match_sel > 0){ match_sel--; }
		if(DengInput->KeyPressed(Key::ENTER)){ selected = true; reclaim_focus = true; }
		if(DengInput->KeyPressed(Key::ESCAPE)){ escape = true; match_sel = 0; reclaim_focus = true; }
		
		if(escape){ ok_flag = true; }
		else {
			ImGui::SetNextItemOpen(1);
			if(TreeNode("match table")){
				if(BeginChild("matchScroll", ImVec2(0, 100), false)){
					if(BeginTable("match table", 1, ImGuiTableFlags_BordersH)){//posi.size())){
						
						int i = 0;
						for (std::string s : posis){
							TableNextColumn();
							if(i == match_sel){
								SetScrollHereY(0);
								PushStyleColor(ImGuiCol_Text, ColorToVec4(Color::RED));
								Text(s.c_str());
								ImGui::PopStyleColor();
								if(selected){
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
	//TODO(sushi, Con) figure out why the scroll bar doesnt allow you to drag it
	const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
	PushStyleColor(ImGuiCol_ChildBg, ColorToVec4(Color(4, 17, 21, 255)));
	BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve-20), false);
	if(BeginPopupContextWindow()){
		if(ImGui::Selectable("hehe")) AddLog("hoho");
		EndPopup();
	}
	
	
	//print previous text
	//ImGuiListClipper clipper;
	//clipper.Begin(buffer.size());
	//while(clipper.Step()){
	for (pair<std::string, Color> p : buffer){
		//for(int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++){
		//color formatting is "[c:red]text[c] text text"
		//TODO( sushi,OpCon) maybe optimize by only drawing what we know will be displayed on screen instead of parsing through all of it
		
		if(p.second == Color::BLANK){
			SameLine(0, 0);
			TextWrapped(p.first.c_str());
		}
		else {
			ImGui::PushStyleColor(ImGuiCol_Text, ColorToVec4(p.second));
			SameLine(0, 0);
			TextWrapped(p.first.c_str());
			ImGui::PopStyleColor();
		}
		
		if(p.first[p.first.size() - 1] == '\n'){
			TextWrapped("\n");
		}
	}
	//}
	
	
	//auto scroll window
	if(scrollToBottom || (autoScroll && GetScrollY() >= GetScrollMaxY())) SetScrollHereY(1);
	scrollToBottom = false;
	
	EndChild();
	ImGui::PopStyleColor();
	//get input from text box
	
	ImGuiInputTextFlags input_text_flags = 0;
	if(!sel_com)  input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackAlways;
	else  input_text_flags = 0;
	
	
	PushStyleColor(ImGuiCol_FrameBg, ColorToVec4(Color::VERY_DARK_CYAN));
	SetNextItemWidth(ImGui::GetWindowWidth() - 15);
	ImGui::SetItemDefaultFocus();
	
	if(InputText("", inputBuf, sizeof(inputBuf), input_text_flags, &TextEditCallbackStub, (void*)this)){
		
		std::string s = inputBuf;
		reclaim_focus = true;
		
		if(s.size() != 0) history.push_back(s);
		
		AddLog(TOSTRING("[c:cyan]/[c][c:dcyan]\\[c] ", s)); //print command typed
		
		//cut off arguments into their own string
		std::string args;
		size_t t = s.find_first_of(" ");
		if(t != std::string::npos){
			args = s.substr(t);
			s.erase(t, s.size() - 1);
		}
		
		if(s.size() != 0 ){
			AddLog(ExecCommand(s, args)); //attempt to execute command and print result
		}
		
		historyPos = -1; //reset history position
		
		memset(inputBuf, 0, sizeof(s)); //erase input from text box
		
		scrollToBottom = true; //scroll to bottom when we press enter
		ImGui::SetKeyboardFocusHere(-1);
	}
	
	Checkbox("Window", &window);
	//if(Button("Window")){
	//	window = !window;
	//}
	
	reclaim_focus = false;
	
	//IMGUI_KEY_CAPTURE = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
	
	ImGui::PopStyleColor(8);
	ImGui::PopStyleVar();
	//ImGui::PopStyleColor();	    ImGui::PopStyleColor();
	//ImGui::PopStyleColor();	    ImGui::PopStyleColor();  
	//ImGui::PopStyleColor();     ImGui::PopStyleColor();
	//ImGui::PopStyleColor();
	
	//if we selected something from completion menu
	//we have to do this here to prevent enter from sending a command
	if(ok_flag){ sel_com = false; }
	
	
	ImGui::End();
}

//this must be a separate funciton because TextEditCallback had a fit when I tried
//making this the main AddLog function
void Console::PushConsole(std::string s){
	AddLog(s);
}

//flushes the buffer to a file once it reaches a certain size
void Console::FlushBuffer(){
	std::string output = "";
	for (pair<std::string, Color> a : buffer){
		output += a.first;
	}
	
	persist std::string filename = Assets::dirLogs() + DengTime->FormatDateTime("deshiLog_{M}-{d}-{y}_{h}.{m}.{s}.txt");
	persist bool session = false;
	
	std::ofstream file;
	
	//if start of session make new file
	if(!session){
		
		file.open(filename);
		file << DengTime->FormatDateTime("Deshi Console Log {w} {M}/{d}/{y} {h}:{m}:{s}") << std::endl;
		file << "\n" << output;
		session = true;
		
	}
	else {
		file.open(filename, std::fstream::app);
		file << output;
	}
	
}

void Console::AddAliases(){
	std::string filepath = Assets::dirConfig() + "aliases.cfg";
	char* buffer = Assets::readFileAsciiToArray(filepath);
	if(!buffer){
		LOG("Creating new aliases file");
		Assets::writeFile(filepath, "", 0);
		return;
	}
	defer{ delete[] buffer; };
	
	std::string line;
	char* new_line = buffer-1;
	char* line_start;
	for(u32 line_number = 1; ;line_number++){
		//get the next line
		line_start = new_line+1;
		if((new_line = strchr(line_start, '\n')) == 0) break; //end of file if cant find '\n'
		line = std::string(line_start, new_line-line_start);
		
		//format the line
		line = Utils::eatComments(line, "#");
		line = Utils::eatSpacesLeading(line);
		line = Utils::eatSpacesTrailing(line);
		if(line.empty()) continue;
		
		//parse alias
		size_t first_space  = line.find_first_of(" ");
		std::string alias   = line.substr(0, first_space);
		std::string command = line.substr(first_space + 1, line.length());
		
		try{
			Command* com = 0;
			com = commands.at(command);
			commands.emplace(alias, com);
		}catch(...){
			ERROR("Unknown command \"", command, "\" was attempted to be aliased from aliases.cfg");
		}
	}
}





///////////////////////////////////////////////////////////////////////
// console init and update
//////////////////////////////////////////////////////////////////////

void Console::Init(){
	AddLog("[c:dcyan]Deshi Console ver. 0.5.1[c]");
	AddLog("\"listc\" for a list of commands\n\"help {command}\" to view a commands help page");
	AddLog("see console_release_notes.txt for version information");
	AddLog("\n[c:dyellow]Console TODOS:[c]");
	AddLog("> implement argument completion for commands\n"
		   "> implement arguments for commands that need them\n"
		   "> add help to commands that don't have a descriptive help yet\n"
		   "> fix tabcompletion when trying to complete the first word\n"
		   "> (maybe) rewrite to use characters in the buffer rather than whole strings\n"
		   "> (maybe) implement showing autocomplete as you type");
	
	AddCommands();
	AddAliases();
}

void Console::Update(){
	if(dispcon){
		DrawConsole();
		CONSOLE_KEY_CAPTURE = true;
	}else{
		CONSOLE_KEY_CAPTURE = false;
	}
	
	if(buffersize >= 120000){
		FlushBuffer();
		buffer.clear();
		buffersize = 0;
	}
	
	if(dispcon && show_alert){
		show_alert = false; alert_count = 0;
	}
}

//Flush the buffer at program close and clean up commands
//TODO(sushi, Con) fix this once we have new command system
void Console::CleanUp(){
	FlushBuffer();
	//for (auto pair : commands){ 
	//	if(pair.second){
	//		try {
	//			delete pair.second; 
	//			pair.second = nullptr; 
	//		}
	//		catch (...){
	//			pair.second = nullptr;
	//		}
	//	}
	//} commands.clear();
}


///////////////////////////////////////////////////////////////////////
// command creation functions
//////////////////////////////////////////////////////////////////////

#define CMDFUNC(name) std::string command_##name##_back(Admin* admin, std::vector<std::string> args)

#define CMDERROR args.at(-1) = ""
#define CMDSTART(name) std::string command_##name##_back(Admin* admin, std::vector<std::string> args){ try{std::cmatch m;
#define CMDSTARTA(name,assert) CMDSTART(name) if(!(assert)){CMDERROR;}
#define CMDEND(error) CMDERROR; return ""; }catch(...){ ERROR(error); return ""; }}

#define CMDADD(name, desc) commands[#name] = new Command(command_##name##_back, #name, desc)

////////////////////////////////////////
//// various uncategorized commands ////
////////////////////////////////////////

CMDFUNC(engine_pause){
	DengAdmin->paused = !DengAdmin->paused;
	if(DengAdmin->paused) return "engine_pause = true";
	else return "engine_pause = false";
}

CMDFUNC(save){
	std::string path = (args.size() > 0) ? args[0] : "save.desh";
	DengAdmin->SaveDESH(path.c_str());
	return "";
}

CMDFUNC(daytime){
	return DengTime->FormatDateTime("{w} {M}/{d}/{y} {h}:{m}:{s}");
}

CMDFUNC(time_engine){
	return DengTime->FormatTickTime("Time:   {t}ms Window:{w}ms Input:{i}ms Admin:{a}ms\n"
									"Console:{c}ms Render:{r}ms Frame:{f}ms Delta:{d}ms");
}

CMDFUNC(time_game){
	return DengAdmin->FormatAdminTime("Layers:  Physics:{P}ms Canvas:{C}ms World:{W}ms Send:{S}ms Last:{L}ms\n"
									  "Systems: Physics:{p}ms Canvas:{c}ms World:{w}ms Send:{s}ms");
}

CMDFUNC(undo){
	DengAdmin->editor.undo_manager.Undo(); return "";
}

CMDFUNC(redo){
	DengAdmin->editor.undo_manager.Redo(); return "";
}

CMDFUNC(flush){
	DengConsole->FlushBuffer(); return "";
}

CMDFUNC(level){
	DengAdmin->LoadTEXT((args.size() > 0) ? args[0].c_str() : 0); return "";
}

CMDSTARTA(state, args.size() > 0){
	if(args[0] == "play"){
		DengAdmin->ChangeState(GameState_Play);
	}else if(args[0] == "menu"){
		DengAdmin->ChangeState(GameState_Menu);
	}else if(args[0] == "debug"){
		DengAdmin->ChangeState(GameState_Debug);
	}else if(args[0] == "editor"){
		DengAdmin->ChangeState(GameState_Editor);
	}
}CMDEND("state <new_state:String>{play|menu|debug|editor}");

CMDSTARTA(load_obj, args.size() > 0){
	Vector3 pos{}, rot{}, scale = Vector3::ONE;
	f32 mass = 1.f, elasticity = .5f; bool staticPosition = 1, twoDphys = false;
	ColliderShape ctype = ColliderShape_NONE;
	Event event = 0;
	//check for optional params after the first arg
	for (auto s = args.begin() + 1; s != args.end(); ++s){
		if(std::regex_search(s->c_str(), m, Vec3Regex("pos"))){
			pos = Vector3(std::stof(m[1]), std::stof(m[2]), std::stof(m[3]));
		} else if(std::regex_search(s->c_str(), m, Vec3Regex("rot"))){
			rot = Vector3(std::stof(m[1]), std::stof(m[2]), std::stof(m[3]));
		} else if(std::regex_search(s->c_str(), m, Vec3Regex("scale"))){
			scale = Vector3(std::stof(m[1]), std::stof(m[2]), std::stof(m[3]));
		} else if(std::regex_search(s->c_str(), m, StringRegex("collider"))){
			if     (m[1] == "aabb")      ctype = ColliderShape_AABB;
			else if(m[1] == "sphere")    ctype = ColliderShape_Sphere;
			else if(m[1] == "landscape") ctype = ColliderShape_Landscape;
			else if(m[1] == "box")       ctype = ColliderShape_Box;
			else if(m[1] == "complex")   ctype = ColliderShape_Complex;
		} else if(std::regex_search(s->c_str(), m, FloatRegex("mass"))){
			if(std::stof(m[1]) <= 0) return "[c:red]Mass can't be zero or less than zero[c]";
			mass = std::stof(m[1]);
		} else if(std::regex_search(s->c_str(), m, FloatRegex("elasticity"))){
			elasticity = std::stof(m[1]);
		} else if(std::regex_search(s->c_str(), m, BoolRegex("static"))){
			if(m[1] == "0" || m[1] == "false"){
				staticPosition = false;
				//mass = INFINITY;
			}
		} else if(std::regex_search(s->c_str(), m, StringRegex("twoDphys"))){
			if(m[1] == "1" || m[1] == "true") twoDphys = true;
		} else if(std::regex_search(s->c_str(), m , StringRegex("event"))){
			if(ctype == ColliderShape_NONE) return "[c:red]Attempt to attach event with no collider[c]";
			bool found = false;
			forI(sizeof(EventStrings)) 
				if(m[1] == EventStrings[i]){ 
				found = true; event = (u32)i;  break;
			}
			if(!found) return TOSTRING("[c:red]Unknown event '", m[1], "'");
		} else {
			return "[c:red]Invalid parameter: " + *s + "[c]";
		}
	}
	
	//cut off the .obj extension for entity name
	char name[DESHI_NAME_SIZE];
	cpystr(name, args[0].substr(0, args[0].size() - 4).c_str(), DESHI_NAME_SIZE);
	
	//create the model
	Model* model = DengScene->CreateModelFromOBJ(args[0].c_str()).second;
	
	//collider
	Collider* col = nullptr;
	switch(ctype){
		case ColliderShape_AABB:      col = new AABBCollider(model->mesh, 1, 0U, event); break;
		case ColliderShape_Sphere:    col = new SphereCollider(1, 1, 0U, event); break;
		case ColliderShape_Landscape: col = new LandscapeCollider(model->mesh, 0U, event); break;
		case ColliderShape_Box:       col = new BoxCollider(Vector3(1, 1, 1), 1, 0U, event); break;
		case ColliderShape_Complex:   col = new ComplexCollider(model->mesh, 0, event); break;
	}
	
	ModelInstance* mc = new ModelInstance(model);
	Physics* p = new Physics(pos, rot, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, elasticity, mass, staticPosition);
	if(twoDphys) p->twoDphys = true;
	AudioSource* s = new AudioSource("data/sounds/Kick.wav", p);
	DengAdmin->CreateEntity({ mc, p, s, col }, name, Transform(pos, rot, scale));
	
	return TOSTRING("Loaded model ", args[0]);
}CMDEND("load_obj <model.obj:String> -pos=(x,y,z) -rot=(x,y,z) -scale=(x,y,z) -collider=String{aabb|sphere} -mass=Float -static=Bool");

CMDSTARTA(cam_vars, args.size() != 0){
	Camera* c = DengAdmin->mainCamera;
	for (auto s = args.begin(); s != args.end(); ++s){
		if(std::regex_search(s->c_str(), m, Vec3Regex("pos"))){
			c->position = Vector3(std::stof(m[1]), std::stof(m[2]), std::stof(m[3]));
		}else if(std::regex_search(s->c_str(), m, Vec3Regex("rot"))){
			c->rotation = Vector3(std::stof(m[1]), std::stof(m[2]), std::stof(m[3]));
		}else if(std::regex_search(s->c_str(), m, FloatRegex("nearZ"))){
			c->nearZ = std::stof(m[1]);
		}else if(std::regex_search(s->c_str(), m, FloatRegex("farZ"))){
			c->farZ = std::stof(m[1]);
		}else if(std::regex_search(s->c_str(), m, FloatRegex("fov"))){
			c->fov = std::stof(m[1]);
		}else{
			return "[c:red]Invalid parameter: " + *s + "[c]";
		}
	}
	
	c->UpdateProjectionMatrix();
	return c->str();
}CMDEND("cam_vars -pos=(x,y,z) -rot=(x,y,z) -static=(Bool) -nearZ=(Float) -farZ=(Float) -fov=(Float)");

CMDSTARTA(add_player, args.size() > 0){
	std::cmatch m;
	Vector3 position{}, rotation{}, scale = { 1.f, 1.f, 1.f };
	float mass = 1.f;
	float elasticity = 0;
	bool staticc = true;
	ColliderShape ctype;
	
	//check for optional params after the first arg
	for (auto s = args.begin() + 1; s != args.end(); ++s){
		if(std::regex_search(s->c_str(), m, Vec3Regex("pos"))){
			position = Vector3(std::stof(m[1]), std::stof(m[2]), std::stof(m[3]));
		}
		else if(std::regex_search(s->c_str(), m, Vec3Regex("rot"))){
			rotation = Vector3(std::stof(m[1]), std::stof(m[2]), std::stof(m[3]));
		}
		else if(std::regex_search(s->c_str(), m, Vec3Regex("scale"))){
			scale = Vector3(std::stof(m[1]), std::stof(m[2]), std::stof(m[3]));
		}
		else if(std::regex_search(s->c_str(), m, StringRegex("collider"))){
			if(m[1] == "aabb") ctype = ColliderShape_AABB;
			else if(m[1] == "sphere") ctype = ColliderShape_Sphere;
			else if(m[1] == "landscape") ctype = ColliderShape_Landscape;
			else if(m[1] == "box") ctype = ColliderShape_Box;
		}
		else if(std::regex_search(s->c_str(), m, FloatRegex("mass"))){
			if(std::stof(m[1]) < 0) return "[c:red]Mass must be greater than 0[c]";
			mass = std::stof(m[1]);
		}
		else if(std::regex_search(s->c_str(), m, BoolRegex("static"))){
			if(m[1] == "0" || m[1] == "false") staticc = false;
		}
		else if(std::regex_search(s->c_str(), m, FloatRegex("elasticity"))){
			elasticity = std::stof(m[1]);
		}
		else {
			return "[c:red]Invalid parameter: " + *s + "[c]";
		}
	}
	
	//cut off the .obj extension for entity name
	char name[DESHI_NAME_SIZE];
	cpystr(name, args[0].substr(0, args[0].size() - 4).c_str(), DESHI_NAME_SIZE);
	
	//create the model
	Model* model = DengScene->CreateModelFromOBJ(args[0].c_str()).second;
	
	//collider
	Collider* col = nullptr;
	switch(ctype){
		case ColliderShape_AABB: col = new AABBCollider(Vector3(0.5, 1, 0.5), 2); break;
		case ColliderShape_Sphere: col = new SphereCollider(1, 1); break;
		case ColliderShape_Landscape: col = new LandscapeCollider(model->mesh); break;
		case ColliderShape_Box: col = new BoxCollider(Vector3(1, 1, 1), 1); break;
	}
	
	ModelInstance* mc = new ModelInstance(model);
	Physics* p = new Physics(position, rotation, Vector3::ZERO, Vector3::ZERO,
							 Vector3::ZERO, Vector3::ZERO, elasticity, mass, staticc);
	AudioSource* s = new AudioSource("data/sounds/Kick.wav", p);
	Movement* mov = new Movement(p);
	mov->camera = DengAdmin->mainCamera;
	Player* pl = new Player(mov);
	DengAdmin->player = DengAdmin->CreateEntityNow({ mc, p, s, col, mov, pl },
												   name, Transform(position, rotation, scale));
	DengAdmin->controller.playermove = mov;
	return TOSTRING("Added player.");
}CMDEND("load_obj <model.obj:String> -pos=(x,y,z) -rot=(x,y,z) -scale=(x,y,z)");


CMDSTARTA(add_force, args.size() > 0){
	std::cmatch m;
	for (std::string s : args){
		if(std::regex_search(s.c_str(), m, Vec3Regex("force"))){
			if(Physics* p = DengAdmin->editor.selected[0]->GetComponent<Physics>()){
				p->AddForce(nullptr, Vector3(std::stof(m[1]), std::stof(m[2]), std::stof(m[3])));
				return "";
			}else{
				ERROR("Selectesd object doesn't have a physics component");
				return "";
			}
		}
	}
}CMDEND("add_force -force=(x,y,z)");

CMDFUNC(cam_info){
	return DengAdmin->mainCamera->str();
}

CMDFUNC(cam_matrix_projection){
	return DengAdmin->mainCamera->projMat.str2f();
}

CMDFUNC(cam_matrix_view){
	return DengAdmin->mainCamera->viewMat.str2f();
}

CMDFUNC(cam_reset){
	DengAdmin->mainCamera->position = Vector3(4.f, 3.f, -4.f);
	DengAdmin->mainCamera->rotation = Vector3(28.f, -45.f, 0.f);
	return "reset camera";
}

CMDFUNC(listc){
	std::string allcommands = "";
	
	for (auto& c : DengConsole->commands){
		allcommands += c.first + "\n";
	}
	
	return allcommands;
}

CMDSTARTA(help, args.size() != 0 && !(args.size() == 1 && args[0] == "")){
	if(DengConsole->commands.find(args[0]) != DengConsole->commands.end()){
		Command* c = DengConsole->commands.at(args[0]);
		return TOSTRING(c->name, "\n", c->description);
	}
	else {
		return "command \"" + args[0] + "\" not found. \n use \"listc\" to list all commands.";
	}
}CMDEND("help \nprints help about a specified command. \nuse listc to display avaliable commands");

CMDFUNC(alias){
	if(args.size() == 0){
		return "alias \nassign an alias to another command to call it with a different name\n alias (alias name) (command name)";
	}
	else if(args.size() == 1){
		return "you must specify a command to assign to this alias.";
	}
	else if(args.size() == 2){
		Command* com;
		try {
			com = DengConsole->commands.at(args[1]);
			DengConsole->commands.emplace(args[0], com);
			
			std::string data = args[0] + " " + args[1] + "\n";
			std::vector<char> datav;
			
			for (auto c : data){
				datav.push_back(c);
			}
			
			Assets::appendFile(Assets::dirConfig()+"aliases.cfg", datav, datav.size());
			
			return "[c:green]alias \"" + args[0] + "\" successfully assigned to command \"" + args[1] + "\"[c]";
		}
		catch (...){
			return "[c:red]command \"" + args[1] + "\" not found in the commands list[c]";
		}
	}
	else {
		return "too many arguments specified.";
	}
	
}

CMDFUNC(bind){
	if(args.size() == 0){
		return "bind \nassign a command to a key\n bind (key) (command name)";
	}else if(args.size() == 1){
		return "you must specify a command to assign to this bind.";
	}else{
		std::string s = "";
		for (int i = 1; i < args.size(); i++){
			s += args[i] + " ";
		}
		Key::Key key;
		
		try{
			key = DengKeys.stk.at(args[0]);
			DengInput->binds.push_back(pair<std::string, Key::Key>(s, key));
			std::vector<char> datav;
			for (auto c : args[0] + " " + s){
				datav.push_back(c);
			}
			datav.push_back('\n');
			Assets::appendFile(Assets::dirConfig()+"binds.cfg", datav, datav.size());
			return "[c:green]key \"" + args[0] + "\" successfully bound to \n" + s + "[c]";
		}catch(...){
			return "[c:red]key \"" + args[0] + "\" not found in the key list.[c]";
		}
	}
}

CMDSTARTA(window_display_mode, args.size() == 1){
	try{
		int mode = std::stoi(args[0]);
		switch(mode){
			case(0): {
				DengWindow->UpdateDisplayMode(DisplayMode::WINDOWED);
				return "display_mode=windowed"; }
			case(1): {
				DengWindow->UpdateDisplayMode(DisplayMode::BORDERLESS);
				return "display_mode=borderless windowed"; }
			case(2): {
				DengWindow->UpdateDisplayMode(DisplayMode::FULLSCREEN);
				return "display_mode=fullscreen"; }
			default: {
				return "display_mode: 0=Windowed, 1=BorderlessWindowed, 2=Fullscreen"; }
		}
	}catch(...){
		return "display_mode: 0=Windowed, 1=BorderlessWindowed, 2=Fullscreen";
	}
}CMDEND("display_mode <mode: Int>");

CMDSTARTA(window_cursor_mode, args.size() == 1){
	try{
		int mode = std::stoi(args[0]);
		switch(mode){
			case(0): {
				DengWindow->UpdateCursorMode(CursorMode::DEFAULT);
				return "cursor_mode=default"; }
			case(1): {
				DengWindow->UpdateCursorMode(CursorMode::FIRSTPERSON);
				return "cursor_mode=first person"; }
			case(2): {
				DengWindow->UpdateCursorMode(CursorMode::HIDDEN);
				return "cursor_mode=hidden"; }
			default: { return "cursor_mode: 0=Default, 1=FirstPerson, 2=Hidden"; }
		}
	}catch(...){
		return "cursor_mode: 0=Default, 1=FirstPerson, 2=Hidden";
	}
} CMDEND("cursor_mode <mode:Int>");

CMDSTARTA(window_raw_input, args.size() == 1){
	try{
		int mode = std::stoi(args[0]);
		switch(mode){
			case(0): { DengWindow->UpdateRawInput(false); return "raw_input=false"; }
			case(1): { DengWindow->UpdateRawInput(true); return "raw_input=true"; }
			default: { return "raw_input: 0=false, 1=true"; }
		}
	}catch(...){
		return "raw_input: 0=false, 1=true";
	}
}CMDEND("raw_input <input:Boolean>");

CMDSTARTA(window_resizable, args.size() == 1){
	try{
		int mode = std::stoi(args[0]);
		switch(mode){
			case(0): { DengWindow->UpdateResizable(false); return "window_resizable=false"; }
			case(1): { DengWindow->UpdateResizable(true); return "window_resizable=true"; }
			default: { return "window_resizable: 0=false, 1=true"; }
		}
	}catch(...){
		return "window_resizable: 0=false, 1=true";
	}
}CMDEND("window_resizable <resizable:Boolean>");

CMDFUNC(window_info){
	return DengWindow->str();
}

CMDSTARTA(mat_texture, args.size() == 3){
	int matID = std::stoi(args[0]);
	int texSlot = std::stoi(args[1]);
	int texID = std::stoi(args[2]);
	DengScene->materials[matID]->textureArray[texSlot] = texID;
	LOG("Updated material ",DengScene->materials[matID]->name,"'s texture",texSlot," to ",DengScene->textures[texID]->name);
	return "";
}CMDEND("mat_texture <materialID:Uint> <textureSlot:Uint> <textureID:Uint>");

CMDSTARTA(mat_shader, args.size() == 2){
	int matID = std::stoi(args[0]);
	int shader = std::stoi(args[1]);
	DengScene->materials[matID]->shader = (Shader)shader;
	LOG("Updated material ",DengScene->materials[matID]->name,"'s shader to ", ShaderStrings[shader]);
	return "";
} CMDEND("mat_shader <materialID:Uint> <shaderID:Uint>");

CMDFUNC(mat_list){
	LOG("Material List:"
		"\nName\tShader\tTextures");
	for(Material* mat : DengScene->materials){
		std::string text = TOSTRING(mat->name,'\t',ShaderStrings[mat->shader],'\t');
		forI(mat->textureCount){
			text += ' ';
			text += DengScene->TextureName(mat->textureArray[i]);
		}
		LOG(text);
	}
	return "";
}

CMDSTARTA(shader_reload, args.size() == 1){
	int id = std::stoi(args[0]);
	if(id == -1){
		Render::ReloadAllShaders();
		LOG("[c:magen]Reloading all shaders[c]");
	}else{
		Render::ReloadShader(id);
	}
	return "";
}CMDEND("shader_reload <shaderID:Uint>");

CMDFUNC(shader_list){
	LOG("Shader List:"
		"\nID\tName");
	forI(ArrayCount(ShaderStrings)){
		LOG(i,'\t',ShaderStrings[i]);
	}
	return "";
}

CMDSTARTA(texture_load, args.size() > 0){
	TIMER_START(t_l);
	TextureType type = TextureType_2D;
	if(args.size() == 2) type = (TextureType)std::stoi(args[1]);
	DengScene->CreateTextureFromFile(args[0].c_str(), ImageFormat_RGBA, type);
	SUCCESS("Loaded texture '",args[0],"' in ",TIMER_END(t_l),"ms");
	return "";
}CMDEND("texture_load <texture.png:String> [type:Uint]");

CMDFUNC(texture_type_list){
	LOG("Texture Type List:"
		"\nID\tName");
	forI(TextureType_COUNT){
		LOG(i,'\t',TextureTypeStrings[i]);
	}
	return "";
}

CMDFUNC(texture_list){
	LOG("Texture List:"
		"\nName\tWidth\tHeight\tDepth\tMipmaps\tType");
	for(Texture* tex : DengScene->textures){
		LOG('\n',tex->name,'\t',tex->width,'\t',tex->height,'\t',tex->depth,'\t',tex->mipmaps,'\t',TextureTypeStrings[tex->type]);
	}
	return "";
}

CMDFUNC(quit){
	DengWindow->Close();
	return("");
}

CMDSTARTA(add_trigger, args.size() > 0){
	Vector3 pos{}, rot{}, scale = vec3::ONE;
	ColliderShape shape = ColliderShape_NONE;
	Event event = 0;
	for (auto s = args.begin(); s != args.end(); ++s){
		if(std::regex_search(s->c_str(), m, Vec3Regex("pos"))){
			pos = Vector3(std::stof(m[1]), std::stof(m[2]), std::stof(m[3]));
		}
		else if(std::regex_search(s->c_str(), m, Vec3Regex("rot"))){
			rot = Vector3(std::stof(m[1]), std::stof(m[2]), std::stof(m[3]));
		}
		else if(std::regex_search(s->c_str(), m, Vec3Regex("scale"))){
			scale = Vector3(std::stof(m[1]), std::stof(m[2]), std::stof(m[3]));
		}
		else if(std::regex_search(s->c_str(), m, StringRegex("shape"))){
			if      (m[1] == "aabb")      shape = ColliderShape_AABB;
			else if(m[1] == "sphere")    shape = ColliderShape_Sphere;
			else if(m[1] == "landscape") shape = ColliderShape_Landscape;
			else if(m[1] == "box")       shape = ColliderShape_Box;
		}
		else if(std::regex_search(s->c_str(), m, StringRegex("event"))){
			if(shape == ColliderShape_NONE) return "[c:red]Attempt to attach event with no collider[c]";
			bool found = false;
			forI(sizeof(EventStrings))
				if(m[1] == EventStrings[i]){
				found = true; event = (u32)i;  break;
			}
			if(!found) return TOSTRING("[c:red]Unknown event '", m[1], "'");
		}
		else {
			return "[c:red]Invalid parameter: " + *s + "[c]";
		}
	}
	
	Collider* col = nullptr;
	switch(shape){
		case ColliderShape_AABB:   col = new AABBCollider(Vector3::ONE / 2, 1, 0U, event); break;
		case ColliderShape_Sphere: col = new SphereCollider(1, 1, 0U, event); break;
		case ColliderShape_Box:    col = new BoxCollider(Vector3(1, 1, 1), 1, 0U, event); break;
	}
	
	Trigger* te = new Trigger(Transform(pos, rot, scale), col);
	
	DengAdmin->CreateEntity(te);
	
	return TOSTRING("Created trigger");
}CMDEND("add_trigger -shape=[ColliderType] -pos=(x,y,z) -rot=(x,y,z) -scale=(x,y,z)")

void Console::AddCommands(){
	CMDADD(add_trigger, "adds a trigger collider entity");
	CMDADD(level, "Loads a level from the levels directory");
	CMDADD(engine_pause, "Toggles pausing the engine");
	CMDADD(save, "Saves the state of the editor");
	CMDADD(daytime, "Logs the time in day-time format");
	CMDADD(time_engine, "Logs the engine times");
	CMDADD(time_game, "Logs the game times");
	CMDADD(undo, "Undos previous level editor action");
	CMDADD(redo, "Redos last undone level editor action");
	CMDADD(add_player, "Adds a player to the world.");
	CMDADD(load_obj, "Loads a .obj file from the models folder with desired options");
	CMDADD(cam_vars, "Allows editing to the camera variables");
	CMDADD(state, "Changes the admin's gamestate");
	CMDADD(flush, "Flushes the console's buffer to the log file.");
	CMDADD(add_force, "Adds a force to the selected object.");
	CMDADD(cam_info, "Prints camera variables");
	CMDADD(cam_matrix_projection, "Prints camera's projection matrix");
	CMDADD(cam_matrix_view, "Prints camera's view matrix");
	CMDADD(cam_reset, "Resets camera");
	CMDADD(listc, "Lists all avaliable commands");
	CMDADD(help, "Prints help about a specified command. \nignores any argument after the first");
	CMDADD(alias, "Assign an alias to another command to call it with a different name");
	CMDADD(bind, "Bind a command to a key");
	CMDADD(window_display_mode, "Sets the window format to windowed, borderless, or fullscreen");
	CMDADD(window_cursor_mode, "Switches between default, first person, or hidden cursor mode");
	CMDADD(window_raw_input, "Only works in first person mode");
	CMDADD(window_resizable, "Sets if the window is resizable");
	CMDADD(window_info, "Prints window variables");
	//CMDADD(render_stats, "Lists different rendering stats for the previous frame"); //TODO(delle,Re) this
	//CMDADD(render_options, "Set wireframe mode to true or false"); //TODO(delle,Re) this
	CMDADD(mat_texture, "Set material texture");
	CMDADD(mat_shader, "Give a material a specific shader");
	CMDADD(mat_list, "Shows the material list");
	CMDADD(shader_reload, "Reloads selected shader");
	CMDADD(shader_list, "Lists the shaders and their IDs");
	CMDADD(texture_load, "Loads a specific texture");
	CMDADD(texture_list, "Lists the textures and their info");
	CMDADD(texture_type_list, "Lists the texture types");
	CMDADD(quit, "Exits the application");
}
