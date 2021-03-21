#include "RenderCanvasSystem.h"
#include "ConsoleSystem.h"
#include "../utils/GLOBALS.h"

#include "../utils/ContainerManager.h"

#include "../components/Canvas.h"
#include "../components/Camera.h"

#include "../external/imgui/imgui_impl_vulkan.h"
#include "../external/imgui/imgui_impl_glfw.h"

#include "../math/Math.h"

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
						//TODO(delle) implement components list on entities
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
	ImGui::Begin("P3DPGE Debug Tools", 0, ImGuiWindowFlags_MenuBar);
	
	MakeMenuBar(admin);
	MakeGeneralHeader(admin);
	MakeEntitiesHeader(admin);
	//MakeRenderHeader(admin);
	MakeBufferlogHeader(admin);
	
	ImGui::End();
}

void DrawFrameGraph(EntityAdmin* admin) { //TODO(r, sushi) implement styling and more options
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
		std::reverse(realvalues.begin(), realvalues.end());  realvalues.resize(storesize); std::reverse(realvalues.begin(), realvalues.end());
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
	
}

void DebugBar(EntityAdmin* admin) {
	
	ImGuiIO& io = ImGui::GetIO();
	
	ImGui::Begin("DebugBar", 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);
	ImGui::Text(TOSTRING(io.Framerate).c_str());
	ImGui::End();
}

void RenderCanvasSystem::DrawUI(void) {
	using namespace ImGui;
	
	static bool showDebugTools = false;
	static bool showDebugBar = true;
	
	
	if (showDebugTools) DebugTools(admin);
	if (showDebugBar) DebugBar(admin);
	
	if (admin->tempCanvas->SHOW_FPS_GRAPH) DrawFrameGraph(admin);
	
	////////////////////////////////////////////
	
	//This finishes the Dear ImGui and renders it to the screen
	//ImGui::Render();
	//ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
}

void RenderCanvasSystem::Init() {
	Canvas* canvas = admin->tempCanvas;
	
}

void RenderCanvasSystem::Update() {
	Canvas*	canvas = admin->tempCanvas;
	
}

