#include "RenderCanvasSystem.h"
#include "../components/Camera.h"
#include "../../core.h"
#include "../../math/Math.h"
#include "../../scene/Scene.h"
#include "../../EntityAdmin.h"
#include "../../game/Keybinds.h"

//for time
#include <iomanip>
#include <sstream>

ImVec4 ColToVec4(Color p) {
	return ImVec4((float)p.r / 255, (float)p.g / 255, (float)p.b / 255, p.a / 255);
}



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

void MakeMenuBar(EntityAdmin* admin) {
	using namespace ImGui;
	static bool show_app_metrics = false;
	static bool show_app_about = false;
	static bool show_app_style_editor = false;
	if (show_app_metrics)       { ImGui::ShowMetricsWindow(&show_app_metrics); }
	if (show_app_about)         { ImGui::ShowAboutWindow(&show_app_about); }
	if (show_app_style_editor)	{ ImGui::Begin("Dear ImGui Style Editor", &show_app_style_editor); ImGui::ShowStyleEditor(); ImGui::End(); }
	
	if(BeginMenuBar()) {
		if(BeginMenu("Debug")) {
			static bool global_debug = true;
			if(MenuItem("global debug", 0, &global_debug)) { global_debug = !global_debug; admin->ExecCommand("debug_global"); }
			static bool print_commands = true;
			if(MenuItem("print commands", 0, &print_commands)) { print_commands = !print_commands; admin->ExecCommand("debug_command_exec"); }
			static bool pause_engine = false;
			if(MenuItem("pause engine", 0, &pause_engine)) { pause_engine = !pause_engine; admin->ExecCommand("time_pause_engine"); }
			if(MenuItem("next frame")) { admin->ExecCommand("time_next_frame"); }
			ImGui::EndMenu();
		}
		if(BeginMenu("Spawn")) {
			if(MenuItem("spawn box")) { admin->ExecCommand("spawn_box"); }
			if(BeginMenu("spawn complex")) {
				if(MenuItem("bmonkey")) { admin->ExecCommand("spawn_complex"); }
				if(MenuItem("whale_ship")) { admin->ExecCommand("spawn_complex1"); }
				if(MenuItem("24k_Triangles")) { admin->ExecCommand("spawn_complex2"); }
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}
		if (BeginMenu("Imgui")){
			MenuItem("Metrics/Debugger", NULL, &show_app_metrics);
			MenuItem("Style Editor", NULL, &show_app_style_editor);
			ImGui::MenuItem("About Dear ImGui", NULL, &show_app_about);
			ImGui::EndMenu();
		}
		EndMenuBar();
	}
}

void MakeGeneralHeader(EntityAdmin* admin) {
	using namespace ImGui;
	Camera* camera = admin->mainCamera;
	if(CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen)) {
		if(TreeNodeEx("Camera", ImGuiTreeNodeFlags_NoTreePushOnOpen)) {
			Text("Address: %#08x", camera); SameLine(); if(ImGui::Button("Copy")) { ImGui::LogToClipboard(); ImGui::LogText("%#08x", camera); ImGui::LogFinish(); }
			Text("Position"); SameLine(); InputVector3("##camPos", &camera->position);
			Text("Rotation"); SameLine(); InputVector3("##camRot", &camera->rotation);
			if(BeginTable("split2", 3)) {
				TableNextColumn(); Text("FOV"); SameLine(); InputFloat("##camFOV", &camera->fieldOfView, 0, 0, "%.2f", ImGuiInputTextFlags_EnterReturnsTrue);
				TableNextColumn(); Text("NearZ"); SameLine(); InputFloat("##camNearZ", &camera->nearZ, 0, 0, "%.2f", ImGuiInputTextFlags_EnterReturnsTrue);
				TableNextColumn(); Text("FarZ"); SameLine(); InputFloat("##camFarZ", &camera->farZ, 0, 0, "%.2f", ImGuiInputTextFlags_EnterReturnsTrue);
				EndTable();
			}
			
			Separator();
		}
	}
}

void MakeEntitiesHeader(EntityAdmin* admin) {
	using namespace ImGui;
	if(CollapsingHeader("Entities")) {
		if(admin->input->selectedEntity) {
			Text("Selected Entity: %d", admin->input->selectedEntity->id);
			if (ImGui::Button("play sound")) {
				admin->ExecCommand("selent_play_sound");
			}
		} else {
			Text("Selected Entity: None");
		}
		
		if(BeginTable("split3", 2, ImGuiTableFlags_BordersOuter | ImGuiTableFlags_Resizable)){
			TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed);
			TableSetupColumn("Components");
			TableHeadersRow();
			int counter = 0;
			for(auto& entity : admin->entities) {
				counter++;
				TableNextRow(); TableNextColumn();
				if(ImGui::Button(std::to_string(entity.first).c_str())) {
					admin->input->selectedEntity = entity.second;
				}
				
				TableNextColumn();
				Text("Address: %#08x", entity.second);
				if(TreeNodeEx((std::string("comps") + std::to_string(entity.first)).c_str(), ImGuiTreeNodeFlags_NoTreePushOnOpen, "Components")) {
					for(Component* comp : entity.second->components) {
						
					}
					Separator();
				}
			}
			EndTable();
		}
	}
}

//void MakeRenderHeader(EntityAdmin* admin) {
//	using namespace ImGui;
//	Scene* scene = admin->;
//	if (CollapsingHeader("Render options")){
//		if (BeginTable("split", 3)) {
//			TableNextColumn(); Checkbox("wireframe", &scene->RENDER_WIREFRAME);
//			TableNextColumn(); Checkbox("textures", &scene->RENDER_TEXTURES);
//			TableNextColumn(); Checkbox("edge numbers", &scene->RENDER_EDGE_NUMBERS);
//			TableNextColumn(); Checkbox("local axis", &scene->RENDER_LOCAL_AXIS);
//			TableNextColumn(); Checkbox("global axis", &scene->RENDER_GLOBAL_AXIS);
//			TableNextColumn(); Checkbox("transforms", &scene->RENDER_TRANSFORMS);
//			TableNextColumn(); Checkbox("physics vectors", &scene->RENDER_PHYSICS);
//			TableNextColumn(); Checkbox("screen aabb", &scene->RENDER_SCREEN_BOUNDING_BOX);
//			TableNextColumn(); Checkbox("mesh vertices", &scene->RENDER_MESH_VERTICES);
//			TableNextColumn(); Checkbox("mesh normals", &scene->RENDER_MESH_NORMALS);
//			TableNextColumn(); Checkbox("grid", &scene->RENDER_GRID);
//			TableNextColumn(); Checkbox("light rays", &scene->RENDER_LIGHT_RAYS);
//			EndTable();
//		}
//	}
//}

void MakeBufferlogHeader(EntityAdmin* admin) {
	using namespace ImGui;
	if(CollapsingHeader("Bufferlog")) {
		for(auto str : g_cBuffer.container) {
			if(str.has_value()) {
				Text(str.get().c_str());
			}
		}
	}
}

//this actually creates the debug tools panel
void MakeP3DPGEDebugTools(EntityAdmin* admin) {
	using namespace ImGui;
	ImGui::Begin("Deshi Debug Tools", 0, ImGuiWindowFlags_MenuBar);
	
	MakeMenuBar(admin);
	MakeGeneralHeader(admin);
	MakeEntitiesHeader(admin);
	//MakeRenderHeader(admin);
	MakeBufferlogHeader(admin);
	
	ImGui::End();
}

void DrawFrameGraph(EntityAdmin* admin) { 
	ImGui::Begin("FPSGraph", 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);
	
	static int prevstoresize = 100;
	static int storesize = 100;
	static int fupdate = 20;
	
	ImGui::SliderInt("amount to store", &storesize, 10, 200);
	ImGui::SliderInt("frames to update", &fupdate, 1, 40);
	
	static std::vector<float> realvalues(storesize);
	static std::vector<float> printvalues(storesize);
	static int max_val = 0;
	static int min_val = INT_MAX;
	
	if (prevstoresize != storesize) {
		//keeps the data in place when resizing
		std::reverse(realvalues.begin(), realvalues.end());    realvalues.resize(storesize);  std::reverse(realvalues.begin(), realvalues.end());
		std::reverse(printvalues.begin(), printvalues.end());  printvalues.resize(storesize); std::reverse(printvalues.begin(), printvalues.end());
		prevstoresize = storesize;
	}
	
	static int frame_count = 0;
	
	std::rotate(realvalues.begin(), realvalues.begin() + 1, realvalues.end()); //rotate vector back one space
	
	int FPS = std::floor(1 / admin->time->deltaTime);
	float avg = Math::average(realvalues.begin(), realvalues.end(), storesize);
	
	if (frame_count < fupdate) {
		realvalues[realvalues.size() - 1] = FPS; //append frame rate
		frame_count++;
	}
	else {
		
		//dynamic max/min_val setting
		if (avg > max_val || avg < max_val - 15) max_val = avg; 
		if (avg < min_val || avg > min_val + 15) min_val = avg;
		
		std::rotate(printvalues.begin(), printvalues.begin() + 1, printvalues.end());
		
		printvalues[printvalues.size() - 1] = std::floorl(avg);
		
		frame_count = 0;
	}
	
	ImGui::Text(TOSTRING(avg, " ", max_val, " ", min_val).c_str());
	
	ImGui::SetNextItemWidth(ImGui::GetWindowWidth());
	ImGui::PlotLines("", &printvalues[0], printvalues.size(), 0, 0, min_val, max_val, ImVec2(300, 200));
	
	ImGui::End();
}

void DebugTools(EntityAdmin* admin) {
	using namespace ImGui;

	SetNextWindowSize(ImVec2(DengWindow->width / 5, DengWindow->height));
	SetNextWindowPos(ImVec2(0, 0));

	PushStyleVar(ImGuiStyleVar_CellPadding,   ImVec2(0, 2));
	PushStyleVar(ImGuiStyleVar_FramePadding,  ImVec2(2, 0));
	PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	PushStyleColor(ImGuiCol_Border,           ColToVec4(Color( 0,  0,  0, 255)));
	PushStyleColor(ImGuiCol_WindowBg,         ColToVec4(Color(20, 20, 20, 255)));
	PushStyleColor(ImGuiCol_TableBorderLight, ColToVec4(Color(45, 45, 45, 255)));
	PushStyleColor(ImGuiCol_TableHeaderBg,    ColToVec4(Color(10, 10, 10, 255)));

	ImGui::Begin("DebugTools", (bool*)1, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
	
	
	if (CollapsingHeader("Entities")) {
		if (admin->input->selectedEntity) {
			Text("Selected Entity: %d", admin->input->selectedEntity->id);
			if (ImGui::Button("play sound")) {
				admin->ExecCommand("selent_play_sound");
			}
		}
		else {
			Text("Selected Entity: None");
		}

		if (BeginTable("split3", 3, ImGuiTableFlags_BordersOuter | ImGuiTableFlags_Resizable)) {
			TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed);
			TableSetupColumn("Name");
			TableSetupColumn("Components");
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
				Text("Address: %#08x", entity.second);
				if (TreeNodeEx((std::string("comps") + std::to_string(entity.first)).c_str(), ImGuiTreeNodeFlags_NoTreePushOnOpen, "Components")) {
					for (Component* comp : entity.second->components) {
						Text(comp->name);
					}
					Separator();
				}
			}
			EndTable();
		}
	}



	ImGui::PopStyleVar();
	ImGui::PopStyleVar();
	ImGui::PopStyleVar();
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
	
	SetNextWindowSize(ImVec2(DengWindow->width, 50));
	SetNextWindowPos(ImVec2(0, DengWindow->height - 20));
	
	//window styling
	PushStyleVar(ImGuiStyleVar_CellPadding,   ImVec2(0, 2));
	PushStyleVar(ImGuiStyleVar_FramePadding,  ImVec2(2, 0));
	PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	PushStyleColor(ImGuiCol_Border,           ColToVec4(Color(0, 0, 0, 255)));
	PushStyleColor(ImGuiCol_WindowBg,         ColToVec4(Color(20, 20, 20, 255)));
	PushStyleColor(ImGuiCol_TableBorderLight, ColToVec4(Color(45, 45, 45, 255)));
	
	ImGui::Begin("DebugBar", (bool*)1, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
	
	//capture mouse if hovering over this window
	if (IsWindowHovered()) { admin->IMGUI_MOUSE_CAPTURE = true; }
	else { admin->IMGUI_MOUSE_CAPTURE = false; }
	
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

		TableSetupColumn("FPS",            ImGuiTableColumnFlags_WidthFixed, 64);
		TableSetupColumn("FPSGraphInline", ImGuiTableColumnFlags_WidthFixed, 64);
		TableSetupColumn("EntCount",       ImGuiTableColumnFlags_None, strlen1 * 1.3);
		TableSetupColumn("TriCount",       ImGuiTableColumnFlags_None, strlen2 * 1.3);
		TableSetupColumn("VerCount",       ImGuiTableColumnFlags_None, strlen3 * 1.3);
		TableSetupColumn("SelTriCount",    ImGuiTableColumnFlags_None, strlen4 * 1.3);
		TableSetupColumn("SelVerCount",    ImGuiTableColumnFlags_None, strlen5 * 1.3);
		TableSetupColumn("MiddleSep",      ImGuiTableColumnFlags_WidthStretch, 0);
		TableSetupColumn("Time",           ImGuiTableColumnFlags_WidthFixed, 64);
		
		
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
			
			PushStyleColor(ImGuiCol_PlotLines, ColToVec4(Color(0, 255, 200, 255)));
			PushStyleColor(ImGuiCol_FrameBg, ColToVec4(Color(20, 20, 20, 255)));
			
			PlotLines("", &pvalues[0], pvalues.size(), 0, 0, 0, maxval, ImVec2(64, 20));
			
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
				PushStyleColor(ImGuiCol_Text, ColToVec4(Color(255 * -(sin(2 * M_PI * time + cos(2 * M_PI * time)) - 1)/2, 0, 0, 255)));
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
	if (DengInput->KeyPressed(DengKeys->toggleDebugMenu)) DebugTools(admin);
	if (DengInput->KeyPressed(DengKeys->toggleDebugBar)) DebugBar(admin);
}

void RenderCanvasSystem::Init() {
	Canvas* canvas = admin->tempCanvas;
	
}

void RenderCanvasSystem::Update() {
	Canvas* canvas = admin->tempCanvas;
	DrawUI();
}

