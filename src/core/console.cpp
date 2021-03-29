#include "../core.h"
#include "../EntityAdmin.h"

#include "Console.h"

#include "time.h"
#include "input.h"

#include <ctime>
#include <time.h>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <filesystem>

#include "../external/imgui/imgui_impl_glfw.h"
#include "../external/imgui/imgui_impl_vulkan.h"

#include "../utils/Command.h"
#include "../utils/defines.h"

#include "../game/Keybinds.h"
#include "../game/systems/WorldSystem.h"
#include "../game/components/Camera.h"
#include "../game/components/Physics.h"
#include "../game/components/Collider.h"
#include "../game/components/Transform.h"
#include "../game/components/AudioSource.h"
#include "../game/components/MeshComp.h"
#include "../scene/Scene.h"
#include "../EntityAdmin.h"

#include "../game/systems/RenderCanvasSystem.h"

#include <functional>



//regex for checking paramaters
#define RegPosParam   std::regex("-pos=\\([0-9|.|-]+,[0-9|.|-]+,[0-9|.|-]+\\)")
#define RegRotParam   std::regex("-rot=\\([0-9|.|-]+,[0-9|.|-]+,[0-9|.|-]+\\)")
#define RegScaleParam std::regex("-scale=\\([0-9|.|-]+,[0-9|.|-]+,[0-9|.|-]+\\)")
#define RegSizeParam  std::regex("-size=\\([0-9|.|-]+,[0-9|.|-]+,[0-9|.|-]+\\)")

//this is repetitive because it has to capture 3 different groups in the same way
#define VecNumMatch std::regex("[,\\(]?([0-9|.|-]+)[,\\)]?[,\\(]?([0-9|.|-]+)[,\\)]?[,\\(]?([0-9|.|-]+)[,\\)]?")

//TODO(delle,Cl) update this to have a try/catch built in
#define NEWCOMMAND(name, desc, func) commands[name] =\
new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {\
try{ func }catch(...){ return desc; }\
}, name, desc);

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

//map for making new components in commands
std::map<std::string, std::function<Component*()>> compstrmap;

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
	if (commands.find(command) != commands.end()) {
		return commands.at(command)->Exec(admin, args);
	}
	else {
		return "[c:red]Command[c] \"" + command + "\" [c:red]not found.[c]";
	}

	//admin->
}

int Console::TextEditCallback(ImGuiInputTextCallbackData* data) {
	switch (data->EventFlag) {
	case ImGuiInputTextFlags_CallbackCompletion: {
		std::string input = data->Buf;

		int fwordl = 0;

		if (std::regex_search(input, std::regex("^.+ +"))) {
			fwordl = input.find_first_of(" ") + 1;
			input.erase(0, input.find_first_of(" ") + 1);

		}

		std::regex e("^" + input + ".*");
		std::vector<std::string> posi;
		for (std::pair<std::string, Command*> c : commands) {
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
	PushStyleColor(ImGuiCol_Border, ColorToVec4(Color(0, 0, 0, 255)));
	PushStyleColor(ImGuiCol_TitleBg, ColorToVec4(Color(0, 0, 0, 255)));
	PushStyleColor(ImGuiCol_WindowBg, ColorToVec4(Color(0, 0, 0, 255)));
	PushStyleColor(ImGuiCol_TitleBgActive, ColorToVec4(Color(0, 0, 0, 255)));
	PushStyleColor(ImGuiCol_ScrollbarGrab, ColorToVec4(Color(37, 36, 36, 255)));
	PushStyleColor(ImGuiCol_ScrollbarGrabActive, ColorToVec4(Color(0, 94, 83, 255)));
	PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ColorToVec4(Color(48, 85, 90, 255)));

	//initialize console window
	SetNextWindowSize(ImVec2(window->width, window->height / 1.5));
	SetNextWindowPos(ImVec2(0, 0));

	ImGui::Begin("Console!", 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove);

	//capture mouse if hovering over this window
	//TODO(sushi, InCon) this is working for some reason pls fix it 
	//if (IsWindowHovered()) admin->canvas->ConsoleHovFlag = true; 
	//else admin->canvas->ConsoleHovFlag = false; 

	bool reclaim_focus = false;

	//display completion table
	//this could probably be done in a better way but idc it works
	static int match_sel = 0;
	bool ok_flag = false;
	if (sel_com) {
		bool selected = false;
		bool escape = false;
		if (input->KeyPressed(Key::DOWN) && match_sel < posis.size() - 1) { match_sel++; }
		if (input->KeyPressed(Key::UP) && match_sel > 0) { match_sel--; }
		if (input->KeyPressed(Key::ENTER)) { selected = true; reclaim_focus = true; }
		if (input->KeyPressed(Key::ESCAPE)) { escape = true; match_sel = 0; reclaim_focus = true; }

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
	if (BeginPopupContextWindow()) {
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

		if (s.size() != 0) history.push_back(s);

		AddLog(TOSTRING("[c:cyan]/[c][c:dcyan]\\[c] ", s)); //print command typed

		//cut off arguments into their own string
		std::string args;
		size_t t = s.find_first_of(" ");
		if (t != std::string::npos) {
			args = s.substr(t);
			s.erase(t, s.size() - 1);
		}

		if (s.size() != 0 ) {
			AddLog(ExecCommand(s, args)); //attempt to execute command and print result
		}

		historyPos = -1; //reset history position

		memset(inputBuf, 0, sizeof(s)); //erase input from text box

		scrollToBottom = true; //scroll to bottom when we press enter
	}


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
	for (std::pair<std::string, Color> a : buffer) {
		output += a.first;
	}

	if (!std::filesystem::is_directory("logs")) {
		std::filesystem::create_directory("logs");
	}


	static std::string filename = TOSTRING("logs/deshiLog_", "-", DengTime->month, "-", DengTime->day, "-", DengTime->year, "_", DengTime->hour, ".", DengTime->minute, ".", DengTime->second, ".txt");
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



///////////////////////////////////////////////////////////////////////
// command creation functions
//////////////////////////////////////////////////////////////////////




inline void AddSpawnCommands(EntityAdmin* admin) {

	//commands["spawn_box"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
	//	//for capturing vector parameters
	//	std::cmatch m;
	//
	//	if (args.size() > 0) {
	//		Vector3 position = Vector3::ZERO;
	//		Vector3 rotation = Vector3::ZERO;
	//		Vector3 scale = Vector3::ONE;
	//		Vector3 size = Vector3::ONE;
	//		float mass = 1;
	//		bool isStatic = false;
	//
	//		for (std::string s : args) { //TODO( sushi,o) see if you can capture the variables when checking for a match
	//			if (std::regex_match(s, RegPosParam)) { // -pos=(1,2,3)
	//				std::regex_search(s.c_str(), m, VecNumMatch);
	//				position = Vector3(std::stof(m[1]), std::stof(m[2]), std::stof(m[3]));
	//			}
	//			else if(std::regex_match(s, RegRotParam)){ //-rot=(1.1,2,3)
	//				std::regex_search(s.c_str(), m, VecNumMatch);
	//				rotation = Vector3(std::stof(m[1]), std::stof(m[2]), std::stof(m[3]));
	//			}
	//			else if (std::regex_match(s, RegScaleParam)) { //-scale=(0,1,0)
	//				std::regex_search(s.c_str(), m, VecNumMatch);
	//				scale = Vector3(std::stof(m[1]), std::stof(m[2]), std::stof(m[3]));
	//			}
	//			else if (std::regex_match(s, RegSizeParam)) { //-size=(3,1,2)
	//				std::regex_search(s.c_str(), m, VecNumMatch);
	//				size = Vector3(std::stof(m[1]), std::stof(m[2]), std::stof(m[3]));
	//			}
	//			else if (std::regex_match(s, std::regex("-mass=[0-9|.]+"))) {
	//				std::regex_search(s.c_str(), m, std::regex("[0-9|.]+"));
	//				mass = std::stof(m[0]);
	//			}
	//			else if (std::regex_match(s, std::regex("-static"))) {
	//				isStatic = true;
	//			}
	//			else {
	//				return "[c:red]Invalid parameter: " + s + "[c]";
	//			}
	//		}
	//		Entity* box = WorldSystem::CreateEntity(admin);
	//		Transform* t = new Transform(position, rotation, scale);
	//		Mesh* m = Mesh::CreateBox(box, size, t->position);
	//		Physics* p = new Physics(t->position, t->rotation, Vector3::ZERO, Vector3::ZERO, Vector3::ZERO, Vector3::ZERO, 0, mass, isStatic);
	//		AudioSource* s = new AudioSource((char*)"sounds/Kick.wav", p);
	//		AABBCollider* c = new AABBCollider(box, size, 1);
	//		WorldSystem::AddComponentsToEntity(box, { t, m, p, s, c });
	//		DengInput->selectedEntity = box;
	//		return TOSTRING("box created at ", position);
	//	}
	//	else {
	//		Entity* box = WorldSystem::CreateEntity(admin);
	//		Transform* t = new Transform(Vector3(0, 0, 3), Vector3::ZERO, Vector3::ONE);
	//		Mesh* m = Mesh::CreateBox(box, Vector3::ONE, t->position);
	//		Physics* p = new Physics(t->position, t->rotation);
	//		AudioSource* s = new AudioSource((char*)"sounds/Kick.wav", p);
	//		AABBCollider* c = new AABBCollider(box, Vector3::ONE, 1);
	//		WorldSystem::AddComponentsToEntity(box, { t, m, p, s, c });
	//		Deng->input->selectedEntity = box;
	//		return TOSTRING("box created at ", Vector3::ZERO);
	//	}
	//
	//	return "Somethings wrong";
	//}, "spawn_box", 
	//	"spawns a box with specified parameters\n"
	//	"avaliable parameters: \n"
	//	"-pos=(x,y,z)\n"
	//	"-rot=(x,y,z)\n"
	//	"-scale=(x,y,z)\n"
	//	"-size=(x,y,z)\n"
	//	"example input:\n"
	//	"spawn_box -pos=(0,1,0) -rot=(45,0,0)"
	//	);
	//
	//commands["spawn_complex"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
	//	Entity* c = WorldSystem::CreateEntity(admin);
	//
	//	Transform* t = new Transform(Vector3(0,0,3), Vector3::ZERO, Vector3::ONE);
	//	Mesh* m = Mesh::CreateComplex(c, "objects/bmonkey.obj", false, t->position);
	//	WorldSystem::AddComponentsToEntity(c, {t, m});
	//	admin->input->selectedEntity = c;
	//	return "";
	//}, "spawn_complex", "spawn_box <filePath: String> <hasTexture: Boolean> <position: Vector3> [rotation: Vector3] [scale: Vector3]");
	//
	//commands["spawn_complex1"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
	//	Entity* c = WorldSystem::CreateEntity(admin);
	//
	//	Transform* t = new Transform(Vector3(0,0,3), Vector3::ZERO, Vector3::ONE);
	//	Mesh* m = Mesh::CreateComplex(c, "objects/whale_ship.obj", false, t->position);
	//	WorldSystem::AddComponentsToEntity(c, {t, m});
	//	admin->input->selectedEntity = c;
	//	return "";
	//}, "spawn_complex1", "spawn_box <filePath: String> <hasTexture: Boolean> <position: Vector3> [rotation: Vector3] [scale: Vector3]");
	//
	//commands["spawn_complex2"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
	//	Entity* c = WorldSystem::CreateEntity(admin);
	//
	//	Transform* t = new Transform(Vector3(0,0,3), Vector3::ZERO, Vector3::ONE);
	//	Mesh* m = Mesh::CreateComplex(c, "objects/24K_Triangles.obj", false, t->position);
	//	WorldSystem::AddComponentsToEntity(c, {t, m});
	//	admin->input->selectedEntity = c;
	//	return "";
	//}, "spawn_complex2", "spawn_box <filePath: String> <hasTexture: Boolean> <position: Vector3> [rotation: Vector3] [scale: Vector3]");
	//
	//commands["spawn_scene"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
	//	Entity* c = WorldSystem::CreateEntity(admin);
	//
	//	Transform* t = new Transform(Vector3(0, 0, 3), Vector3::ZERO, Vector3::ONE);
	//	Mesh* m = Mesh::CreateComplex(c, "scenes/scene_test.obj", true, t->position);
	//	WorldSystem::AddComponentsToEntity(c, { t, m });
	//
	//	Key::Sprite* s = new Key::Sprite(1, 1);
	//	s->SetPixel(Vector2(0, 0), Key::WHITE);
	//
	//	m->texture = s;
	//
	//	admin->input->selectedEntity = c;
	//	return "";
	//	}, "spawn_scene", "spawn_box <filePath: String> <hasTexture: Boolean> <position: Vector3> [rotation: Vector3] [scale: Vector3]");

}
////////////////////////////////////
//// render commands and inputs ////
////////////////////////////////////

void Console::AddRenderCommands() {
	NEWCOMMAND("render_stats", "Lists different rendering stats for the previous frame", {
		//TODO(delle,Cmd) this
		return "";
		});

	NEWCOMMAND("render_options", "render_options <wireframe:Bool>", {
				   if (args.size() > 0) {
					   try {
						   admin->renderer->settings.wireframe = std::stoi(args[0]);
						   return (admin->renderer->settings.wireframe) ? "wireframe=1" : "wireframe=0";
					   }
					   catch (...) {
						   return "render_options <wireframe:Bool>";
					   }
				   }
				   return (admin->renderer->settings.wireframe) ? "wireframe=1" : "wireframe=0";
		});

	//create box 

	//create planarized box

	//mesh_update_matrix, a bit more difficult b/c it should only update the passed arguments

	//create material

	//list materials

	//TODO(delle,Re) fix this, its not working
	NEWCOMMAND("mat_texture", "mat_texture <materialID:Uint> <textureType:Uint> <textureID:Uint>", {
				   if (args.size() != 3) { return "material_texture <materialID:Uint> <textureType:Uint> <textureID:Uint>"; }
				   try {
					   int matID = std::stoi(args[0]);
					   int texType = std::stoi(args[1]);
					   int texID = std::stoi(args[2]);
					   admin->renderer->UpdateMaterialTexture(matID, texType, texID);
					   return TOSTRING("Updated material", matID, "'s texture", texType, " to ", texID);
				   }
				   catch (...) {
					   return "material_texture <materialID:Uint> <textureType:Uint> <textureID:Uint>";
				   }
		});

	NEWCOMMAND("mat_shader", "mat_shader <materialID:Uint> <shaderID:Uint>", {
				   if (args.size() != 2) { return "material_shader <materialID:Uint> <shaderID:Uint>"; }
				   try {
					   int matID = std::stoi(args[0]);
					   int shader = std::stoi(args[1]);
					   admin->renderer->UpdateMaterialShader(matID, shader);
					   return TOSTRING("Updated material", matID, "'s shader to ", shader);
				   }
				   catch (...) {
					   return "material_shader <materialID:Uint> <shaderID:Uint>";
				   }
		});

	NEWCOMMAND("shader_reload", "shader_reload <shaderID:Uint>", {
				   if (args.size() != 1) return "shader_reload <shaderID:Uint>";
				   int id = std::stoi(args[0]);
				   if (id == -1) {
					   admin->renderer->ReloadAllShaders();
					   return "[c:magen]Reloading all shaders[c]";
				   }
else {
   admin->renderer->ReloadShader(id);
   return ""; //printed in renderer
}
		});

	//TODO(delle) update this to be dynamic when shader loading is (if ever,Re)
	NEWCOMMAND("shader_list", "Lists the shaders and their IDs", {
				   return TOSTRING("[c:yellow]ID    SHADER          Description[c]\n",
								   "0    Flat            Vertex color shading without normal/edge smoothing\n",
								   "1    Phong           Vertex color shading with normal smoothing (good with spheres)\n",
								   "2    TwoD            Vertex color shading with 2D position, rotation, and scale\n",
								   "3    PBR             Physically-based rendering; 4 textures per material\n",
								   "4    Wireframe       Vertex color shading with no polygon fill\n",
								   "5    Lavalamp        Sushi's experimental shader\n",
								   "6    Test0           Testing shader 1\n",
								   "7    Test1           Testing shader 2\n",
								   "8    Test2           Testing shader 3\n",
								   "9    Test3           Testing shader 4");
		});

	NEWCOMMAND("mesh_visible", "mesh_visible <meshID:Uint> <visible:Bool>", {
				   if (args.size() == 2) {
					   try {
						   int meshID = std::stoi(args[0]);
						   bool vis = std::stoi(args[1]);
						   admin->renderer->UpdateMeshVisibility(meshID, vis);
						   return TOSTRING("Setting mesh", meshID, "'s visibility to ", vis);
					   }
					   catch (...) {
						   return "mesh_visible <meshID:Uint> <visible:Bool>";
					   }
				   }
				   return "mesh_visible <meshID:Uint> <visible:Bool>";
		});

	NEWCOMMAND("mesh_batch_material", "mesh_batch_material <meshID:Uint> <batchID:Uint> <materialID:Uint>", {
				   if (args.size() != 3) { return "mesh_batch_material <meshID:Uint> <batchID:Uint> <materialID:Uint>"; }
				   try {
					   int mesh = std::stoi(args[0]);
					   int batch = std::stoi(args[1]);
					   int mat = std::stoi(args[2]);
					   admin->renderer->UpdateMeshBatchMaterial(mesh, batch, mat);
					   return TOSTRING("Changed mesh", mesh, "'s batch", batch, "'s material to ", mat);
				   }
				   catch (...) {
					   return "mesh_batch_material <meshID:Uint> <batchID:Uint> <materialID:Uint>";
				   }
		});

	commands["mesh_transform_matrix"] =
		new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (args.size() > 1) {
			Mesh mesh; std::cmatch m;
			Vector3 position{}, rotation{}, scale = { 1.f, 1.f, 1.f };

			//check for optional params after the first arg
			for (auto s = args.begin() + 1; s != args.end(); ++s) {
				if (std::regex_match(*s, RegPosParam)) { // -pos=(1,2,3)
					std::regex_search(s->c_str(), m, VecNumMatch);
					position = Vector3(std::stof(m[1]), std::stof(m[2]), std::stof(m[3]));
				}
				else if (std::regex_match(*s, RegRotParam)) { //-rot=(1.1,2,3)
					std::regex_search(s->c_str(), m, VecNumMatch);
					rotation = Vector3(std::stof(m[1]), std::stof(m[2]), std::stof(m[3]));
				}
				else if (std::regex_match(*s, RegScaleParam)) { //-scale=(0,1,0)
					std::regex_search(s->c_str(), m, VecNumMatch);
					scale = Vector3(std::stof(m[1]), std::stof(m[2]), std::stof(m[3]));
				}
				else {
					return "[c:red]Invalid parameter: " + *s + "[c]";
				}
			}

			//update the mesh's matrix
			try {
				admin->renderer->TransformMeshMatrix(std::stoi(args[0]), Matrix4::TransformationMatrix(position, rotation, scale));
				return TOSTRING("Transforming mesh", args[0], "'s matrix");
			}
			catch (...) {
				return "mesh_transform_matrix <meshID:Uint> -pos=(x,y,z) -rot=(x,y,z) -scale=(x,y,z)";
			}
		}
		else {
			return "mesh_transform_matrix <meshID:Uint> -pos=(x,y,z) -rot=(x,y,z) -scale=(x,y,z)";
		}
			}, "mesh_transform_matrix", "mesh_transform_matrix <meshID:Uint> -pos=(x,y,z) -rot=(x,y,z) -scale=(x,y,z)");

	//TODO(delle,CmdCl) figure out why the macro doesnt work here or on the one above
	commands["load_obj"] =
		new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (args.size() > 0) {
			Mesh mesh;
			std::cmatch m;
			Vector3 position{}, rotation{}, scale = { 1.f, 1.f, 1.f };

			//check for optional params after the first arg
			for (auto s = args.begin() + 1; s != args.end(); ++s) {
				if (std::regex_match(*s, RegPosParam)) { // -pos=(1,2,3)
					std::regex_search(s->c_str(), m, VecNumMatch);
					position = Vector3(std::stof(m[1]), std::stof(m[2]), std::stof(m[3]));
				}
				else if (std::regex_match(*s, RegRotParam)) { //-rot=(1.1,2,3)
					std::regex_search(s->c_str(), m, VecNumMatch);
					rotation = Vector3(std::stof(m[1]), std::stof(m[2]), std::stof(m[3]));
				}
				else if (std::regex_match(*s, RegScaleParam)) { //-scale=(0,1,0)
					std::regex_search(s->c_str(), m, VecNumMatch);
					scale = Vector3(std::stof(m[1]), std::stof(m[2]), std::stof(m[3]));
				}
				else {
					return "[c:red]Invalid parameter: " + *s + "[c]";
				}
			}
			std::string name = args[0].substr(0, args[0].size() - 4);
			//create the mesh and give to the renderer
			mesh = Mesh::CreateMeshFromOBJ(args[0], name,
				Matrix4::TransformationMatrix(position, rotation, scale));

			//Need to make this so that MeshComp has a mesh that isn't deleted 
			Mesh* mes = new Mesh(mesh);

			Entity* e = admin->world->CreateEntity(admin);
			e->name = name;
			e->admin = admin;
			MeshComp* mc = new MeshComp(mes);
			Physics* p = new Physics(Vector3(0,0,0), Vector3(0,0,0));
			AudioSource* s = new AudioSource("data/sounds/Kick.wav", p);
			admin->world->AddComponentsToEntity(admin, e, { mc, p, s });

			u32 id = admin->renderer->LoadMesh(mes);
			Model mod;
			mod.mesh = mesh;
			mc->MeshID = id;
			admin->scene->models.push_back(mod);

			return TOSTRING("Loaded mesh ", args[0], " to ID: ", id);
		}
		return "load_obj <model.obj:String> -pos=(x,y,z) -rot=(x,y,z) -scale=(x,y,z)";
			}, "load_obj", "load_obj <model.obj:String> -pos=(x,y,z) -rot=(x,y,z) -scale=(x,y,z)");

	NEWCOMMAND("texture_load", "texture_load <texture.png:String> [type:Uint]", {
				   if (args.size() > 0) {
					   Texture tex(args[0].c_str());
					   if (args.size() == 2) {
						   try {
							   tex.type = u32(std::stoi(args[1]));
						   }
						   catch (...) {
							   return "texture_load <texture.png:String> [type:Uint]";
						   }
					   }
					   u32 id = admin->renderer->LoadTexture(tex);
					   return TOSTRING("Laoded texture ", args[0], " to ID: ", id);
				   }
				   return "texture_load <texture.png:String> [type:Uint]";
		});

	NEWCOMMAND("texture_list", "Lists the textures and their info", {
				   return admin->renderer->ListTextures();
		});

	NEWCOMMAND("texture_types_list", "Lists the texture types and their IDs", {
				   return TOSTRING("Texture Types: (can be combined)\n",
								   "   0=Albedo, Color, Diffuse\n",
								   "   1=Normal, Bump\n",
								   "   2=Light, Ambient\n",
								   "   4=Specular, Reflective\n",
								   "   8=Cube      (not supported yet)\n",
								   "  16=Sphere    (not supported yet)");
		});
}

////////////////////////////////////
//// camera commands and inputs ////
////////////////////////////////////

void Console::AddCameraCommands() {


	NEWCOMMAND("cam_info", "Prints camera variables", {
				   return "";
		});

	NEWCOMMAND("cam_reset", "Resets camera", {
		admin->mainCamera->position = Vector3(0, 0, -5);
		admin->mainCamera->rotation = Vector3(0, 0, 0);
		return "reset camera";
		});
}

void Console::AddConsoleCommands() {
	commands["listc"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		std::string allcommands = "";

		for (std::pair<std::string, Command*> c : admin->console->commands) {
			allcommands += c.first + ", ";
		}

		return allcommands;
		}, "listc", "lists all avaliable commands");

	commands["help"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (args.size() == 0 || (args.size() == 1 && args[0] == "")) {
			return "help \nprints help about a specified command. \nuse listc to display avaliable commands";
		}
		else if (admin->console->commands.find(args[0]) != admin->console->commands.end()) {
			Command* c = admin->console->commands.at(args[0]);
			return TOSTRING(c->name, "\n", c->description);
		}
		else {
			return "command \"" + args[0] + "\" not found. \n use \"listc\" to list all commands.";
		}
		}, "help", "prints help about a specified command. \nignores any argument after the first.");

}


//TODO(delle,InPh) update entity movement commands to be based on EntityID
void Console::AddSelectedEntityCommands() {
	//// translation ////
	commands["reset_position"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (DengInput->selectedEntity) {
			if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
				p->acceleration = Vector3::ZERO;
				p->velocity = Vector3::ZERO;
				p->position = Vector3::ZERO;
			}
		}
		return "";
		}, "reset_position", "reset_position <EntityID> [String: xyz]");

	commands["reset_position_x"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (DengInput->selectedEntity) {
			if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
				p->acceleration = Vector3(0, p->acceleration.y, p->acceleration.z);
				p->velocity = Vector3(0, p->velocity.y, p->velocity.z);
				p->position = Vector3(0, p->position.y, p->position.z);
			}
		}
		return "";
		}, "reset_position_x", "temp");

	commands["reset_position_y"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (DengInput->selectedEntity) {
			if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
				p->acceleration = Vector3(p->acceleration.x, 0, p->acceleration.z);
				p->velocity = Vector3(p->velocity.x, 0, p->velocity.z);
				p->position = Vector3(p->position.x, 0, p->position.z);
			}
		}
		return "";
		}, "reset_position_y", "temp");

	commands["reset_position_z"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (DengInput->selectedEntity) {
			if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
				p->acceleration = Vector3(p->acceleration.x, p->acceleration.y, 0);
				p->velocity = Vector3(p->velocity.x, p->velocity.y, 0);
				p->position = Vector3(p->position.x, p->position.y, 0);
			}
		}
		return "";
		}, "reset_position_z", "temp");

	commands["reset_velocity"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (DengInput->selectedEntity) {
			if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
				p->acceleration = Vector3::ZERO;
				p->velocity = Vector3::ZERO;
			}
		}
		return "";
		}, "reset_velocity", "reset_position <EntityID> [String: xyz]");

	commands["translate_right"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (DengInput->selectedEntity) {
			if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
				p->AddInput(Vector3::RIGHT);
			}
		}
		return "";
		}, "translate_right", "translate_right <EntityID> <amount> [speed]");

	commands["translate_left"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (DengInput->selectedEntity) {
			if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
				p->AddInput(Vector3::LEFT);
			}
		}
		return "";
		}, "translate_left", "translate_left <EntityID> <amount> [speed]");

	commands["translate_up"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (DengInput->selectedEntity) {
			if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
				p->AddInput(Vector3::UP);
			}
		}
		return "";
		}, "translate_up", "translate_up <EntityID> <amount> [speed]");

	commands["translate_down"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (DengInput->selectedEntity) {
			if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
				p->AddInput(Vector3::DOWN);
			}
		}
		return "";
		}, "translate_down", "translate_down <EntityID> <amount> [speed]");

	commands["translate_forward"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (DengInput->selectedEntity) {
			if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
				p->AddInput(Vector3::FORWARD);
			}
		}
		return "";
		}, "translate_forward", "translate_forward <EntityID> <amount> [speed]");

	commands["translate_backward"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (DengInput->selectedEntity) {
			if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
				p->AddInput(Vector3::BACK);
			}
		}
		return "";
		}, "translate_backward", "translate_backward <EntityID> <amount> [speed]");

	//// rotation ////

	commands["reset_rotation"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (DengInput->selectedEntity) {
			if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
				p->rotAcceleration = Vector3::ZERO;
				p->rotVelocity = Vector3::ZERO;
				p->rotation = Vector3::ZERO;
			}
		}
		return "";
		}, "reset_rotation", "reset_rotation <EntityID> [String: xyz]");

	commands["reset_rotation_x"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (DengInput->selectedEntity) {
			if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
				p->rotAcceleration = Vector3(0, p->rotAcceleration.y, p->rotAcceleration.z);
				p->rotVelocity = Vector3(0, p->rotVelocity.y, p->rotVelocity.z);
				p->rotation = Vector3(0, p->rotation.y, p->rotation.z);
			}
		}
		return "";
		}, "reset_rotation_x", "temp");

	commands["reset_rotation_y"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (DengInput->selectedEntity) {
			if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
				p->rotAcceleration = Vector3(p->rotAcceleration.x, 0, p->rotAcceleration.z);
				p->rotVelocity = Vector3(p->rotVelocity.x, 0, p->rotVelocity.z);
				p->rotation = Vector3(p->rotation.x, 0, p->rotation.z);
			}
		}
		return "";
		}, "reset_rotation_y", "temp");

	commands["reset_rotation_z"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (DengInput->selectedEntity) {
			if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
				p->rotAcceleration = Vector3(p->rotAcceleration.x, p->rotAcceleration.y, 0);
				p->rotVelocity = Vector3(p->rotVelocity.x, p->rotVelocity.y, 0);
				p->rotation = Vector3(p->rotation.x, p->rotation.y, 0);
			}
		}
		return "";
		}, "reset_rotation_z", "temp");

	commands["reset_rotation_velocity"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (DengInput->selectedEntity) {
			if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
				p->rotAcceleration = Vector3::ZERO;
				p->rotVelocity = Vector3::ZERO;
			}
		}
		return "";
		}, "reset_rotation_velocity", "reset_rotation_velocity <EntityID> [String: xyz]");

	commands["rotate_+x"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (DengInput->selectedEntity) {
			if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
				p->rotVelocity += Vector3(5, 0, 0);
			}
		}
		return "";
		}, "rotate_+x", "rotate_+x <EntityID> <amount> [speed]");

	commands["rotate_-x"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (DengInput->selectedEntity) {
			if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
				p->rotVelocity += Vector3(-5, 0, 0);
			}
		}
		return "";
		}, "rotate_-x", "rotate_-x <EntityID> <amount> [speed]");

	commands["rotate_+y"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (DengInput->selectedEntity) {
			if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
				p->rotVelocity += Vector3(0, 5, 0);
			}
		}
		return "";
		}, "rotate_+y", "rotate_+y <EntityID> <amount> [speed]");

	commands["rotate_-y"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (DengInput->selectedEntity) {
			if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
				p->rotVelocity += Vector3(0, -5, 0);
			}
		}
		return "";
		}, "rotate_-y", "rotate_-y <EntityID> <amount> [speed]");

	commands["rotate_+z"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (DengInput->selectedEntity) {
			if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
				p->rotVelocity += Vector3(0, 0, 5);
			}
		}
		return "";
		}, "rotate_+z", "rotate_+z <EntityID> <amount> [speed]");

	commands["rotate_-z"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		if (DengInput->selectedEntity) {
			if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
				p->rotVelocity += Vector3(0, 0, -5);
			}
		}
		return "";
		}, "rotate_-z", "rotate_-z <EntityID> <amount> [speed]");

	//// other ////
	/*
	commands["add_force"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
													   //TODO( sushi,Re) implement ScreenToWorld for ortho projection
													   if (USE_ORTHO) {
													   LOG("\nWarning: ScreenToWorld not yet implemented for orthographic projection. World interaction with mouse will not work.\n");
												   }
												   else {
													   if (DengInput->selectedEntity) {
														   if (Physics* p = DengInput->selectedEntity->GetComponent<Physics>()) {
															   Vector3 pos = Math::ScreenToWorld(DengInput->mousePos, admin->mainCamera->projectionMatrix,
																								 admin->mainCamera->viewMatrix, DengWindow->dimensions);
															   //cant remember what this is doing and will fix later
															   //Vector3 clickPos = Math::ScreenToWorld(DengInput->mouseClickPos, admin->mainCamera->projectionMatrix,
															   //admin->mainCamera->viewMatrix, DengWindow->dimensions);
															   //TODO(delle,PhIn) test that you can add force to a selected entity
															   //Physics::AddForce(nullptr, p, (pos - clickPos).normalized() * 5);
														   }
													   }
												   }
												   return "";
												   }, "add_force", "add_force <EntityID> <force_vector> [constant_force?]");
	*/
}

void Console::AddWindowCommands() {
	NEWCOMMAND("quit", "exits the application", {
		admin->window->Close();
		return("q lol");
		});

	commands["display_mode"] =
		new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		Window* w = admin->window;
		if (args.size() != 1) { return "display_mode <mode: Int>"; }
		try {
			int mode = std::stoi(args[0]);
			switch (mode) {
			case(0): {
				w->UpdateDisplayMode(DisplayMode::WINDOWED);
				return "display_mode=windowed"; }
			case(1): {
				w->UpdateDisplayMode(DisplayMode::BORDERLESS);
				return "display_mode=borderless windowed"; }
			case(2): {
				w->UpdateDisplayMode(DisplayMode::FULLSCREEN);
				return "display_mode=fullscreen"; }
			default: {
				return "display_mode: 0=Windowed, 1=BorderlessWindowed, 2=Fullscreen"; }
			}
		}
		catch (...) {
			return "display_mode: 0=Windowed, 1=BorderlessWindowed, 2=Fullscreen";
		}
			}, "display_mode", "display_mode <mode:Int>");

	commands["cursor_mode"] =
		new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		Window* w = admin->window;
		if (args.size() != 1) { return "cursor_mode <mode:Int>"; }
		try {
			int mode = std::stoi(args[0]);
			switch (mode) {
			case(0): {
				w->UpdateCursorMode(CursorMode::DEFAULT);
				return "cursor_mode=default"; }
			case(1): {
				w->UpdateCursorMode(CursorMode::FIRSTPERSON);
				return "cursor_mode=first person"; }
			case(2): {
				w->UpdateCursorMode(CursorMode::HIDDEN);
				return "cursor_mode=hidden"; }
			default: { return "cursor_mode: 0=Default, 1=FirstPerson, 2=Hidden"; }
			}
		}
		catch (...) {
			return "cursor_mode: 0=Default, 1=FirstPerson, 2=Hidden";
		}
			}, "cursor_mode", "cursor_mode <mode:Int>");

	commands["raw_input"] =
		new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		Window* w = admin->window;
		if (args.size() != 1) { return "raw_input <input:Boolean>"; }
		try {
			int mode = std::stoi(args[0]);
			switch (mode) {
			case(0): { w->UpdateRawInput(false); return "raw_input=false"; }
			case(1): { w->UpdateRawInput(true); return "raw_input=true"; }
			default: { return "raw_input: 0=false, 1=true"; }
			}
		}
		catch (...) {
			return "raw_input: 0=false, 1=true";
		}
			}, "raw_input", "raw_input <input:Boolean>; Only works in firstperson cursor mode");

	NEWCOMMAND("window_resizable", "raw_input <resizable:Boolean>", {
				   Window * w = admin->window;
				   if (args.size() != 1) { return "window_resizable <resizable:Boolean>"; }
				   try {
					   int mode = std::stoi(args[0]);
					   switch (mode) {
						   case(0): { w->UpdateResizable(false); return "window_resizable=false"; }
						   case(1): { w->UpdateResizable(true); return "window_resizable=true"; }
						   default: { return "window_resizable: 0=false, 1=true"; }
					   }
				   }
				   catch (...) {
					   return "window_resizable: 0=false, 1=true";
				   }
		});

	NEWCOMMAND("window_info", "Prints window variables", {
				   Window * w = admin->window;
				   std::string dispMode;
				   switch (w->displayMode) {
					   case(DisplayMode::WINDOWED): { dispMode = "Windowed"; }break;
					   case(DisplayMode::BORDERLESS): { dispMode = "Borderless Windowed"; }break;
					   case(DisplayMode::FULLSCREEN): { dispMode = "Fullscreen"; }break;
				   }
				   std::string cursMode;
				   switch (w->cursorMode) {
					   case(CursorMode::DEFAULT): { cursMode = "Default"; }break;
					   case(CursorMode::FIRSTPERSON): { cursMode = "First Person"; }break;
					   case(CursorMode::HIDDEN): { cursMode = "Hidden"; }break;
				   }
				   return TOSTRING("Window Info"
								   "\n    Window Position: ", w->x, ",", w->y,
								   "\n    Window Dimensions: ", w->width, "x", w->height,
								   "\n    Screen Dimensions: ", w->screenWidth, "x", w->screenHeight,
								   "\n    Refresh Rate: ", w->refreshRate,
								   "\n    Screen Refresh Rate: ", w->screenRefreshRate,
								   "\n    Display Mode: ", dispMode,
								   "\n    Cursor Mode: ", cursMode,
								   "\n    Raw Input: ", w->rawInput,
								   "\n    Resizable: ", w->resizable,
								   "\n    Restores: ", w->restoreX, ",", w->restoreY, " ",
								   w->restoreW, "x", w->restoreH);
		});
}




///////////////////////////////////////////////////////////////////////
// console init and update
//////////////////////////////////////////////////////////////////////




void Console::Init(Time* t, Input* i, Window* w, EntityAdmin* ea) {
	me = this;

	time = t;
	input = i;
	window = w;
	admin = ea;

	compstrmap.emplace("AudioSource", []() { return new AudioSource(); });
	compstrmap.emplace("Collider",    []() { return new Collider(); });
	compstrmap.emplace("MeshComp",    []() { return new MeshComp(); });
	compstrmap.emplace("Transform",   []() { return new Transform(); });
	compstrmap.emplace("Physics",     []() { return new Physics(); });


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

	//TODO(sushi,Cmd) reimplement this at some point

	//commands["debug_global"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
	//	GLOBAL_DEBUG = !GLOBAL_DEBUG;
	//	if (GLOBAL_DEBUG) return "GLOBAL_DEBUG = true";
	//	else return "GLOBAL_DEBUG = false";
	//}, "debug_global", "debug_global");

	commands["debug_command_exec"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		Command::CONSOLE_PRINT_EXEC = !Command::CONSOLE_PRINT_EXEC;
		return ""; //i dont know what this does so im not formatting it 
		}, "debug_command_exec", "if true, prints all command executions to the console");

	commands["engine_pause"] = new Command([](EntityAdmin* admin, std::vector<std::string> args) -> std::string {
		admin->paused = !admin->paused;
		if (admin->paused) return "engine_pause = true";
		else return "engine_pause = false";
		}, "engine_pause", "toggles pausing the engine");

	//AddSpawnCommands();
	AddRenderCommands();
	AddCameraCommands();
	AddConsoleCommands();
	AddSelectedEntityCommands();
	AddWindowCommands();
}

void Console::Update() {
	if (input->KeyPressed(DengKeys->toggleConsole)) dispcon = !dispcon;
		
	if (dispcon) {
		DrawConsole();
		admin->IMGUI_KEY_CAPTURE = true;
	}
	else {
		admin->IMGUI_KEY_CAPTURE = false;
	}

	me = this;

	if (buffersize >= 120000) {
		FlushBuffer();
		buffer.clear();
		buffersize = 0;
	}

	if (dispcon && admin->cons_error_warn) admin->cons_error_warn = false;

	Input* input = admin->input;

}

//Flush the buffer at program close
void Console::CleanUp() {
	FlushBuffer();
	for (auto pair : commands) { delete pair.second; } commands.clear();
}