#include "RenderCanvasSystem.h"
#include "../components/Camera.h"
#include "../../core.h"
#include "../../math/Math.h"
#include "../../scene/Scene.h"
#include "../../EntityAdmin.h"
#include "../../game/Keybinds.h"
#include "../../game/systems/WorldSystem.h"

//for time
#include <iomanip>
#include <sstream>

ImVec4 ColToVec4(Color p) {
	return ImVec4((float)p.r / 255, (float)p.g / 255, (float)p.b / 255, p.a / 255);
}

bool WinHovFlag = false;
float menubarheight = 0;
bool menubar = true;

//// utility ui elements ///

void CopyButton(const char* text) {
	if(ImGui::Button("Copy")){ ImGui::LogToClipboard(); ImGui::LogText(text); ImGui::LogFinish(); }
}

void InputVector3(const char* id, Vector3* vecPtr, bool inputUpdate = false) {
	ImGui::SetNextItemWidth(-FLT_MIN);
	if(inputUpdate) { //pointer voodoo to treat Vector3 as float vector
		ImGui::InputFloat3(id, (float*)vecPtr); 
	} else {
		ImGui::InputFloat3(id, (float*)vecPtr, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue); 
	}
}

//// major ui elements ////

void MenuBar(EntityAdmin* admin) {
	using namespace ImGui;

	ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
	ImGui::PushStyleColor(ImGuiCol_PopupBg,   ColToVec4(Color(20, 20, 20, 255)));
	ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ColToVec4(Color(20, 20, 20, 255)));

	if(BeginMainMenuBar()) {
		menubarheight = GetWindowHeight();
		if(BeginMenu("File")) {
			if (MenuItem("placeholder")) {

			}
			ImGui::EndMenu();
		}
		if(BeginMenu("Edit")) {
			if (MenuItem("placeholder")) {

			}
			ImGui::EndMenu();
		}
		if(BeginMenu("Spawn")) {
			std::vector<std::string> files = deshi::iterateDirectory(deshi::getModelsPath());
			for (int i = 0; i < files.size(); i++) {
				if(MenuItem(files[i].c_str())) { admin->console->ExecCommand("load_obj", files[i] + ".obj"); }
			}
			EndMenu();
		}
		
		EndMainMenuBar();
	}

	PopStyleColor();
	PopStyleColor();
	PopStyleVar();
	PopStyleVar();

}

void DebugTools(EntityAdmin* admin) {
	using namespace ImGui;

	if (menubar) {
		ImGui::SetNextWindowSize(ImVec2(DengWindow->width / 5, DengWindow->height - menubarheight));
		ImGui::SetNextWindowPos(ImVec2(0, menubarheight));
	}
	else {
		ImGui::SetNextWindowSize(ImVec2(DengWindow->width / 5, DengWindow->height));
		ImGui::SetNextWindowPos(ImVec2(0, 0));
	}
	
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding,   ImVec2(0, 2));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,  ImVec2(2, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleColor(ImGuiCol_Border,           ColToVec4(Color( 0,  0,  0)));
	ImGui::PushStyleColor(ImGuiCol_Button,           ColToVec4(Color(30, 30, 30)));
	ImGui::PushStyleColor(ImGuiCol_WindowBg,         ColToVec4(Color(20, 20, 20)));
	ImGui::PushStyleColor(ImGuiCol_PopupBg,          ColToVec4(Color(20, 20, 20)));
	ImGui::PushStyleColor(ImGuiCol_TableBorderLight, ColToVec4(Color(45, 45, 45)));
	ImGui::PushStyleColor(ImGuiCol_TableHeaderBg,    ColToVec4(Color(10, 10, 10)));

	ImGui::Begin("DebugTools", (bool*)1, ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus |  ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

	//capture mouse if hovering over this window
	if (IsWindowHovered()) WinHovFlag = true; 

	if (BeginTable("split3", 3, ImGuiTableFlags_BordersOuter | ImGuiTableFlags_Resizable)) {
		ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn("Name");
		ImGui::TableSetupColumn("Components");
		TableHeadersRow();
		int counter = 0;
		for (auto& entity : admin->entities) {
			counter++;
			TableNextRow(); TableNextColumn();
			if (ImGui::Button(std::to_string(entity.first).c_str())) {
				admin->input->selectedEntity = entity.second;
			}

			TableNextColumn();
			Text(entity.second->name.c_str());

			TableNextColumn();
			//Text("Address: %#08x", entity.second);
			if (TreeNodeEx((std::string("comps") + std::to_string(entity.first)).c_str(), ImGuiTreeNodeFlags_NoTreePushOnOpen, TOSTRING(entity.second->components.size()).c_str())) {
				for (Component* comp : entity.second->components) {
					Text(comp->name);
					SameLine(CalcItemWidth());
					if (Button("Del")) {
						admin->world->RemoveAComponentFromEntity(admin, entity.second, comp);
					}
				}
				Separator();
			}
		}
		EndTable();
	}


	

	Separator();

	if (admin->input->selectedEntity) {
		Entity* sel = admin->input->selectedEntity;
		Text(TOSTRING("Selected Entity: ", sel->name).c_str());
		if (ImGui::Button("play sound")) {
			admin->ExecCommand("selent_play_sound");
		}
		if (ImGui::BeginTable("SelectedComponents", 1)) {
			ImGui::EndTable();
		}
	}
	else {
		Text("Selected Entity: None");
	}
	

	ImGui::PopStyleVar();
	ImGui::PopStyleVar();
	ImGui::PopStyleVar();
	ImGui::PopStyleVar();
	ImGui::PopStyleColor();
	ImGui::PopStyleColor();
	ImGui::PopStyleColor();
	ImGui::PopStyleColor();
	ImGui::PopStyleColor();
	ImGui::PopStyleColor();

	ImGui::End();
	
}

void DebugBar(EntityAdmin* admin) {
	using namespace ImGui;
	
	//for getting fps
	ImGuiIO& io = ImGui::GetIO();
	
	int FPS = floor(io.Framerate);
	
	//num of active columns
	int activecols = 7;
	
	//font size for centering text
	float fontsize = ImGui::GetFontSize();
	
	//flags for showing different things
	static bool show_fps = true;
	static bool show_fps_graph = true;
	static bool show_world_stats = true;
	static bool show_selected_stats = true;
	static bool show_floating_fps_graph = false;
	static bool show_time = true;
	
	ImGui::SetNextWindowSize(ImVec2(DengWindow->width, 50));
	ImGui::SetNextWindowPos(ImVec2(0, DengWindow->height - 20));
	
	//window styling
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding,   ImVec2(0, 2));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,  ImVec2(2, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleColor(ImGuiCol_Border,           ColToVec4(Color(0, 0, 0, 255)));
	ImGui::PushStyleColor(ImGuiCol_WindowBg,         ColToVec4(Color(20, 20, 20, 255)));
	ImGui::PushStyleColor(ImGuiCol_TableBorderLight, ColToVec4(Color(45, 45, 45, 255)));
	
	ImGui::Begin("DebugBar", (bool*)1, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
	
	//capture mouse if hovering over this window
	if (IsWindowHovered()) WinHovFlag = true; 
	
	activecols = show_fps + show_fps_graph + 3 * show_world_stats + 2 * show_selected_stats + show_time + 1;
	if (BeginTable("DebugBarTable", activecols, ImGuiTableFlags_BordersV | ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_ContextMenuInBody | ImGuiTableFlags_SizingFixedFit)) {

		//precalc strings and stuff so we can set column widths appropriately
		std::string str1 = TOSTRING("wents: ", admin->entities.size());
		float strlen1 = (fontsize - (fontsize / 2)) * str1.size();
		std::string str2 = TOSTRING("wtris: ", admin->renderer->stats.totalTriangles);
		float strlen2 = (fontsize - (fontsize / 2)) * str2.size();
		std::string str3 = TOSTRING("wverts: ", admin->renderer->stats.totalVertices);
		float strlen3 = (fontsize - (fontsize / 2)) * str3.size();
		std::string str4 = TOSTRING("stris: ", "0");
		float strlen4 = (fontsize - (fontsize / 2)) * str4.size();
		std::string str5 = TOSTRING("sverts: ", "0");
		float strlen5 = (fontsize - (fontsize / 2)) * str5.size();

		ImGui::TableSetupColumn("FPS",            ImGuiTableColumnFlags_WidthFixed, 64);
		ImGui::TableSetupColumn("FPSGraphInline", ImGuiTableColumnFlags_WidthFixed, 64);
		ImGui::TableSetupColumn("EntCount",       ImGuiTableColumnFlags_None, strlen1 * 1.3);
		ImGui::TableSetupColumn("TriCount",       ImGuiTableColumnFlags_None, strlen2 * 1.3);
		ImGui::TableSetupColumn("VerCount",       ImGuiTableColumnFlags_None, strlen3 * 1.3);
		ImGui::TableSetupColumn("SelTriCount",    ImGuiTableColumnFlags_None, strlen4 * 1.3);
		ImGui::TableSetupColumn("SelVerCount",    ImGuiTableColumnFlags_None, strlen5 * 1.3);
		ImGui::TableSetupColumn("MiddleSep",      ImGuiTableColumnFlags_WidthStretch, 0);
		ImGui::TableSetupColumn("Time",           ImGuiTableColumnFlags_WidthFixed, 64);
		
		
		//FPS
		
		if (TableNextColumn() && show_fps) {
			//trying to keep it from changing width of column
			//actually not necessary anymore but im going to keep it cause 
			//it keeps the numbers right aligned
			if (FPS % 1000 == FPS) {
				Text(TOSTRING("FPS:  ", FPS).c_str());
			}
			else if (FPS % 100 == FPS) {
				Text(TOSTRING("FPS:   ", FPS).c_str());
			}
			else {
				Text(TOSTRING("FPS: ", FPS).c_str());
			}
			
		}
		
		//FPS graph inline
		if (TableNextColumn() && show_fps_graph) {
			//how much data we store
			static int prevstoresize = 100;
			static int storesize = 100;
			
			//how often we update
			static int fupdate = 20;
			static int frame_count = 0;
			
			//maximum FPS
			static int maxval = 0;
			
			//real values and printed values
			static std::vector<float> values(storesize);
			static std::vector<float> pvalues(storesize);
			
			//dynamic resizing that may get removed later if it sucks
			//if FPS finds itself as less than half of what the max used to be we lower the max
			if (FPS > maxval || FPS < maxval / 2) {
				maxval = FPS;
			}
			
			//if changing the amount of data we're storing we have to reverse
			//each data set twice to ensure the data stays in the right place when we move it
			if (prevstoresize != storesize) {
				std::reverse(values.begin(), values.end());    values.resize(storesize);  std::reverse(values.begin(), values.end());
				std::reverse(pvalues.begin(), pvalues.end());  pvalues.resize(storesize); std::reverse(pvalues.begin(), pvalues.end());
				prevstoresize = storesize;
			}
			
			std::rotate(values.begin(), values.begin() + 1, values.end());
			
			//update real set if we're not updating yet or update the graph if we are
			if (frame_count < fupdate) {
				values[values.size() - 1] = FPS;
				frame_count++;
			}
			else {
				float avg = Math::average(values.begin(), values.end(), storesize);
				std::rotate(pvalues.begin(), pvalues.begin() + 1, pvalues.end());
				pvalues[pvalues.size() - 1] = std::floorf(avg);
				
				frame_count = 0;
			}
			
			ImGui::PushStyleColor(ImGuiCol_PlotLines, ColToVec4(Color(0, 255, 200, 255)));
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ColToVec4(Color(20, 20, 20, 255)));
			
			ImGui::PlotLines("", &pvalues[0], pvalues.size(), 0, 0, 0, maxval, ImVec2(64, 20));
			
			ImGui::PopStyleColor();
			ImGui::PopStyleColor();
		}
		
		
		//World stats
		
		//Entity Count
		if (TableNextColumn() && show_world_stats) {
			ImGui::SameLine((GetColumnWidth() - strlen1) / 2);
			Text(str1.c_str());
		}
		
		//Triangle Count
		if (TableNextColumn() && show_world_stats) {
			//TODO( sushi,Ui) implement triangle count when its avaliable
			ImGui::SameLine((GetColumnWidth() - strlen2) / 2);
			Text(str2.c_str());
		}

		//Vertice Count
		if (TableNextColumn() && show_world_stats) {
			//TODO( sushi,Ui) implement vertice count when its avaliable
			ImGui::SameLine((GetColumnWidth() - strlen3) / 2);
			Text(str3.c_str());
		}



		// Selected Stats



		//Triangle Count
		if (TableNextColumn() && show_selected_stats) {
			//TODO( sushi,Ui) implement triangle count when its avaliable
			Entity* e = DengInput->selectedEntity;
			ImGui::SameLine((GetColumnWidth() - strlen4) / 2);
			Text(str4.c_str());
		}

		//Vertice Count
		if (TableNextColumn() && show_selected_stats) {
			//TODO( sushi,Ui) implement vertice count when its avaliable
			Entity* e = DengInput->selectedEntity;
			ImGui::SameLine((GetColumnWidth() - strlen5) / 2);
			Text(str5.c_str());
		}
		
		//Middle Empty Separator
		if (TableNextColumn()) {
			static float time = DengTime->totalTime;
			if (admin->cons_error_warn) {
				time = DengTime->totalTime;
				ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, GetColorU32(ColToVec4(Color(255 * (sin(2 * M_PI * time + cos(2 * M_PI * time)) + 1)/2, 0, 0, 255))));
				
				PushItemWidth(-1);
				std::string str6 = admin->last_error;
				float strlen6 = (fontsize - (fontsize / 2)) * str6.size();
				ImGui::SameLine((GetColumnWidth() - strlen6) / 2);
				ImGui::PushStyleColor(ImGuiCol_Text, ColToVec4(Color(255 * -(sin(2 * M_PI * time + cos(2 * M_PI * time)) - 1)/2, 0, 0, 255)));
				Text(str6.c_str());
				PopStyleColor();
			}
			
			
		}
		
		//Show Time
		if (TableNextColumn()) {
			//https://stackoverflow.com/questions/24686846/get-current-time-in-milliseconds-or-hhmmssmmm-format
			using namespace std::chrono;
			
			//get current time
			auto now = system_clock::now();
			
			//convert to std::time_t so we can convert to std::tm
			auto timer = system_clock::to_time_t(now);
			
			//convert to broken time
			std::tm bt = *std::localtime(&timer);
			
			std::ostringstream oss;
			
			oss << std::put_time(&bt, "%H:%M:%S");
			
			std::string str7 = oss.str();
			float strlen7 = (fontsize - (fontsize / 2)) * str7.size();
			ImGui::SameLine(32 - (strlen7 / 2));
			
			Text(str7.c_str());
			
		}
		
		
		//Context menu for toggling parts of the bar
		if (ImGui::IsMouseReleased(1) && IsWindowHovered()) OpenPopup("Context");
		if (BeginPopup("Context")) {
			admin->IMGUI_MOUSE_CAPTURE = true;
			ImGui::Separator();
			if (Button("Open Debug Menu")) {
				//showDebugTools = true;
				CloseCurrentPopup();
			}
			
			EndPopup();
		}
		
		
		
		EndTable();
	}
	
	ImGui::PopStyleVar();
	ImGui::PopStyleVar();
	ImGui::PopStyleVar();
	ImGui::PopStyleColor();
	ImGui::PopStyleColor();
	ImGui::PopStyleColor();
	ImGui::End();
}
void RenderCanvasSystem::DrawUI(void) {
	if (DengInput->KeyPressed(DengKeys->toggleDebugMenu)) showDebugTools = !showDebugTools;
	if (DengInput->KeyPressed(DengKeys->toggleDebugBar)) showDebugBar = !showDebugBar;
	if (DengInput->KeyPressed(DengKeys->toggleMenuBar)) showMenuBar = !showMenuBar;
	
	if (showDebugBar) DebugBar(admin);
	if (showDebugTools) DebugTools(admin);
	if (showMenuBar) MenuBar(admin);

	if (showMenuBar) menubar = true;
	else menubar = false;
}

void RenderCanvasSystem::Init() {
	Canvas* canvas = admin->tempCanvas;
}

void RenderCanvasSystem::Update() {
	WinHovFlag = 0;
	Canvas* canvas = admin->tempCanvas;
	DrawUI();
	if (ConsoleHovFlag || WinHovFlag) admin->IMGUI_MOUSE_CAPTURE = true;
	else                              admin->IMGUI_MOUSE_CAPTURE = false;
}

