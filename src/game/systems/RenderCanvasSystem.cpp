#include "RenderCanvasSystem.h"
#include "../components/Camera.h"
#include "../../core.h"
#include "../../utils/defines.h"
#include "../../math/Math.h"
#include "../../scene/Scene.h"
#include "../../EntityAdmin.h"
#include "../../game/Keybinds.h"
#include "../../game/components/MeshComp.h"
#include "../../game/Transform.h"

#include "../../game/systems/WorldSystem.h"

//for time
#include <iomanip>
#include <sstream>

ImVec4 ColToVec4(Color p) {
	return ImVec4((float)p.r / 255, (float)p.g / 255, (float)p.b / 255, p.a / 255);
}

bool WinHovFlag = false;
float menubarheight = 0;
float debugbarheight = 0;
float debugtoolswidth = 0;

float padding = 0.95;

//current palette:
//https://lospec.com/palette-list/slso8
//TODO(sushi, Ui) implement menu style file loading sort of stuff yeah
//TODO(sushi, Ui) standardize what UI element each color belongs to
struct {
	Color c1 = Color(0x0d2b45);
	Color c2 = Color(0x203c56);
	Color c3 = Color(0x544e68);
	Color c4 = Color(0x8d697a);
	Color c5 = Color(0xd08159);
	Color c6 = Color(0xffaa5e);
	Color c7 = Color(0xffd4a3);
	Color c8 = Color(0xffecd6);
	Color c9 = Color(20, 20, 20);
}colors;

std::vector<std::string> files;
std::vector<std::string> textures;


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

void RenderCanvasSystem::MenuBar() {
	using namespace ImGui;
	
	ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
	ImGui::PushStyleColor(ImGuiCol_PopupBg,   ColToVec4(Color(20, 20, 20, 255)));
	ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ColToVec4(Color(20, 20, 20, 255)));
	
	
	if(BeginMainMenuBar()) {
		if (IsWindowHovered()) WinHovFlag = true; 

		menubarheight = GetWindowHeight();
		if(BeginMenu("File")) {
			if (IsWindowHovered()) WinHovFlag = true; 

			if (MenuItem("placeholder")) {

			}
			ImGui::EndMenu();
		}
		if(BeginMenu("Edit")) {
			if (IsWindowHovered()) WinHovFlag = true; 

			if (MenuItem("placeholder")) {

			}
			ImGui::EndMenu();
		}
		if(BeginMenu("Spawn")) {
			if (IsWindowHovered()) WinHovFlag = true; 

			for (int i = 0; i < files.size(); i++) {
				if (files[i].find(".obj") != std::string::npos) {
					if(MenuItem(files[i].c_str())) { admin->console->ExecCommand("load_obj", files[i]); }
				}
			}
			EndMenu();
		}//agh
		if (BeginMenu("Window")) {
			if (IsWindowHovered()) WinHovFlag = true; 

			if (MenuItem("Object Property Menu")) showDebugTools = !showDebugTools;
			if (MenuItem("Debug Bar")) showDebugBar = !showDebugBar;
			if (MenuItem("DebugLayer")) showDebugLayer = !showDebugLayer;
			if (MenuItem("ImGui Demo Window")) showImGuiDemoWindow = !showImGuiDemoWindow;
			EndMenu();
		}
		
		EndMainMenuBar();
	}
	
	PopStyleColor();
	PopStyleColor();
	PopStyleVar();
	PopStyleVar();
	
}

void RenderCanvasSystem::DebugTools() {
	using namespace ImGui;
	
	float fontsize = ImGui::GetFontSize();
	
	//resize tool menu if main menu bar is open
	ImGui::SetNextWindowSize(ImVec2(DengWindow->width / 5, DengWindow->height - (menubarheight + debugbarheight)));
	ImGui::SetNextWindowPos(ImVec2(0, menubarheight));
	
	//window styling
	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 5);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 0);
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding,   ImVec2(0, 2));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,  ImVec2(2, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	
	ImGui::PushStyleColor(ImGuiCol_Border,               ColToVec4(Color( 0,  0,  0)));
	ImGui::PushStyleColor(ImGuiCol_Button,               ColToVec4(Color(30, 30, 30)));
	ImGui::PushStyleColor(ImGuiCol_WindowBg,             ColToVec4(colors.c9));
	ImGui::PushStyleColor(ImGuiCol_PopupBg,              ColToVec4(Color(20, 20, 20)));
	ImGui::PushStyleColor(ImGuiCol_TableBorderLight,     ColToVec4(Color(45, 45, 45)));
	ImGui::PushStyleColor(ImGuiCol_TableHeaderBg,        ColToVec4(Color(10, 10, 10)));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarBg,          ColToVec4(Color(10, 10, 10)));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab,        ColToVec4(Color(55, 55, 55)));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive,  ColToVec4(Color(75, 75, 75)));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ColToVec4(Color(65, 65, 65)));
	
	
	
	ImGui::Begin("DebugTools", (bool*)1, ImGuiWindowFlags_NoFocusOnAppearing |  ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
	
	debugtoolswidth = GetWindowWidth();
	
	//capture mouse if hovering over this window
	if (IsWindowHovered()) WinHovFlag = true; 
	
	SetCursorPosX((GetWindowWidth() - (GetWindowWidth() * padding)) / 2);
	ImGui::Text("Entities");
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ColToVec4(Color(25, 25, 25)));
	SetCursorPosX((GetWindowWidth() - (GetWindowWidth() * padding)) / 2);
	
	if (BeginChild("entityListScroll", ImVec2(GetWindowWidth() * 0.95, 100), false)) {
		if (IsWindowHovered()) WinHovFlag = true; 
		if (admin->entities.size() == 0) {
			float time = DengTime->totalTime;
			std::string str1 = "Nothing yet...";
			float strlen1 = (fontsize - (fontsize / 2)) * str1.size();
			for (int i = 0; i < str1.size(); i++) {
				SetCursorPos(ImVec2((GetWindowSize().x - strlen1) / 2 + i * (fontsize / 2), (GetWindowSize().y - fontsize) / 2 + sin(10 * time + cos(10 * time + (i * M_PI / 2)) + (i * M_PI / 2))));
				Text(str1.substr(i, 1).c_str());
			}
		}
		else {
			if (BeginTable("split3", 4, ImGuiTableFlags_BordersInner)) {
				
				std::string str1 = "ID";
				float strlen1 = (fontsize - (fontsize / 2)) * str1.size();
				
				ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("Vis", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("Name");
				ImGui::TableSetupColumn("Components");
				int counter = 0;
				
				
				for (auto& entity : admin->entities) {
					counter++;
					PushID(counter);
					TableNextRow(); TableNextColumn();
					std::string id = std::to_string(entity.first);
					//SetCursorPosX((GetColumnWidth() - (fontsize - (fontsize / 2)) * id.size()) / 2);
					if (ImGui::Button(id.c_str())) {
						admin->input->selectedEntity = entity.second;
					}
					TableNextColumn();
					
					//TODO(UiEnt, sushi) implement visibility for things other than meshes like lights, etc.
					MeshComp* m = entity.second->GetComponent<MeshComp>();
					if (m) {
						if (m->mesh_visible) {
							if (SmallButton("O")) {
								m->ToggleVisibility();
							}
						}
						else {
							if (SmallButton("X")) {
								m->ToggleVisibility();
							}
						}
					}
					else {
						Text("NM");
					}
					
					TableNextColumn();
					Text(TOSTRING(" ", entity.second->name).c_str());
					
					TableNextColumn();
					//TODO(sushi, Ui) find something better to put here
					Text(TOSTRING(" comps: ", entity.second->components.size()).c_str());
					PopID();
				}
				
				ImGui::EndTable();
			}
		}
		EndChild();
	}
	ImGui::PopStyleColor();
	
	ImGui::Separator();
	
	ImGui::PushStyleColor(ImGuiCol_TabActive, ColToVec4(Color::VERY_DARK_CYAN));
	ImGui::PushStyleColor(ImGuiCol_TabHovered, ColToVec4(Color::DARK_CYAN));
	ImGui::PushStyleColor(ImGuiCol_Tab, ColToVec4(colors.c1));
	ImGui::PushStyleVar(ImGuiStyleVar_TabRounding, 0);

	if (BeginTabBar("SettingsTabs")) {
	//Selected Entity property editing
		if (BeginTabItem("Ent")) {
			if (admin->input->selectedEntity) {
				if (BeginChild("SelectedEntityMenu", ImVec2(GetWindowWidth(), 500), true)) {
					if (IsWindowHovered()) WinHovFlag = true;
					Entity* sel = admin->input->selectedEntity;
					SetCursorPosX((GetWindowWidth() - (GetWindowWidth() * padding)) / 2);
					Text(TOSTRING("Selected Entity: ", sel->name).c_str());
					Text("Components: ");

					if (BeginTabBar("ObjectPropertyMenus")) {


						if (BeginTabItem("Obj")) {
							SetCursorPosX((GetWindowWidth() - (GetWindowWidth() * 0.95)) / 2);
							if (BeginChild("ObjMenu", ImVec2(GetWindowWidth()* 0.95, 100), true)) {
								if (IsWindowHovered()) WinHovFlag = true;

								Text("Transform");
								Separator();

								SetCursorPosX((GetWindowWidth() - (GetWindowWidth() * padding)) / 2);
								Text("Position ");
								SameLine(); InputVector3("position", &sel->transform.position);
								Separator();

								SetCursorPosX((GetWindowWidth() - (GetWindowWidth() * padding)) / 2);
								Text("Rotation ");
								SameLine(); InputVector3("rotation", &sel->transform.rotation);
								Separator();

								SetCursorPosX((GetWindowWidth() - (GetWindowWidth() * padding)) / 2);
								Text("Scale    ");
								SameLine(); InputVector3("scale", &sel->transform.scale);
								Separator();

								EndChild();
							}
							EndTabItem();
						}

						//Components menu
						if (BeginTabItem("Comp")) {
							SetCursorPosX((GetWindowWidth() - (GetWindowWidth() * 0.95)) / 2);
							if (BeginChild("SelectedComponentsWindow", ImVec2(GetWindowWidth() * 0.95, 100), true)) {
								if (IsWindowHovered()) WinHovFlag = true;

								if (ImGui::BeginTable("SelectedComponents", 1)) {

									ImGui::TableSetupColumn("Comp", ImGuiTableColumnFlags_WidthFixed);
									for (Component* c : sel->components) {
										TableNextColumn(); //TableNextRow();
										SetCursorPosX((GetWindowWidth() - (GetWindowWidth() * padding)) / 2);
										Text(c->name);
										SameLine(CalcItemWidth() + 20);
										if (Button("Del")) {
											admin->world->RemoveAComponentFromEntity(admin, sel, c);
										}
									}
									ImGui::EndTable();
								}
								EndChild();
							}
							EndTabItem();
						}

						//Materials menu
						if (BeginTabItem("Mat")) {
							SetCursorPosX((GetWindowWidth() - (GetWindowWidth() * padding)) / 2);
							if (BeginChild("SelectedMaterialsWindow", ImVec2(GetWindowWidth() * 0.95, 200), true)) {
								if (IsWindowHovered()) WinHovFlag = true;
								if (BeginTabBar("MaterialTab")) {
									if (BeginTabItem("Shdr")) {
										MeshComp* m = sel->GetComponent<MeshComp>();
										if (m) {
											//TODO(sushi, Ui) set up showing multiple batches shaders when that becomes relevant
											Text(TOSTRING("Shader: ", shadertostring.at(m->m->batchArray[0].shader)).c_str());
											SetCursorPosX((GetWindowWidth() - (GetWindowWidth() * padding)) / 2);
											if (ImGui::TreeNode("Shader Select")) {
												static int selected = -1;
												for (int i = 0; i < shadertostringint.size(); i++) {
													if (Selectable(shadertostringint[i].c_str(), selected == i)) {
														selected = i;
														for (int iter = 0; iter < sel->components.size(); iter++) {
															if (MeshComp* mc = dynamic_cast<MeshComp*>(sel->components[iter])) {
																mc->ChangeMaterialShader(stringtoshader.at(shadertostringint[i]));
															}
														}
													}
												}
												TreePop();
											}
										}
										EndTabItem();
									}
									if (BeginTabItem("Tex")) {
										
										MeshComp* m = sel->GetComponent<MeshComp>();
										if (m) {
											if (m->m->batchArray.size() > 0) {
												if (m->m->batchArray[0].textureArray.size() > 0) {
													Text(TOSTRING("Texture: ", m->m->batchArray[0].textureArray[0].filename).c_str());
												}
												else {
													Text("Current Texture: None");
												}
												//TODO(sushi, Ui) immplement showing multiple textures when yeah
												Separator();
												SetCursorPosX((GetWindowWidth() - (GetWindowWidth() * padding)) / 2);

												static int selected = -1;
												for (int i = 0; i < textures.size(); i++) {
													SetCursorPosX((GetWindowWidth() - (GetWindowWidth() * padding)) / 2);
													if (Selectable(textures[i].c_str(), selected == i)) {
														selected = i;

														for (int iter = 0; iter < sel->components.size(); iter++) {
															if (MeshComp* mc = dynamic_cast<MeshComp*>(sel->components[iter])) {
																Texture tex(textures[i].c_str(), 0);
																mc->ChangeMaterialTexture(admin->renderer->LoadTexture(tex));
															}
														}
													}

												}

											}
										}
										EndTabItem();
									}
									EndTabBar();
								}
								EndChild();
							}
							EndTabItem();
						}
						EndTabBar();
					}
					EndChild();
				}

			}
			else {
				SetCursorPosX((GetWindowWidth() - (GetWindowWidth() * padding)) / 2);
				Text("Selected Entity: None");
			}
			EndTabItem();
		}
		if (BeginTabItem("Cam")) {
			if (BeginChild("SelectedEntityMenu", ImVec2(GetWindowWidth(), 500), true)) {
				if (IsWindowHovered()) WinHovFlag = true;
				SetCursorPosX((GetWindowWidth() - (GetWindowWidth() * padding)) / 2);
				
				Camera* c = admin->mainCamera;

				Text("Transform");
				Separator();

				SetCursorPosX((GetWindowWidth() - (GetWindowWidth() * padding)) / 2);
				Text("Position ");
				SameLine(); InputVector3("position", &c->position);
				Separator();

				SetCursorPosX((GetWindowWidth() - (GetWindowWidth() * padding)) / 2);
				Text("Rotation ");
				SameLine(); InputVector3("rotation", &c->rotation);
				Separator();

				if (Button("Reset camera")) admin->ExecCommand("cam_reset");
				
				Separator();

				SetCursorPosX((GetWindowWidth() - (GetWindowWidth() * padding)) / 2);
				Text("fov:   ");
				SameLine(); ImGui::SetNextItemWidth(-FLT_MIN); 
				if (InputFloat("fov", &c->fov, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsDecimal)) {
					c->UpdateProjectionMatrix();
				}
				Separator();

				SetCursorPosX((GetWindowWidth() - (GetWindowWidth() * padding)) / 2);
				Text("nearZ: ");
				SameLine(); ImGui::SetNextItemWidth(-FLT_MIN); 
				if (InputFloat("nearz", &c->nearZ, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsDecimal)) {
					c->UpdateProjectionMatrix();
				};
				Separator();

				SetCursorPosX((GetWindowWidth() - (GetWindowWidth() * padding)) / 2);
				Text("farZ:  ");
				SameLine(); ImGui::SetNextItemWidth(-FLT_MIN); 
				if (InputFloat("farz", &c->farZ, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsDecimal)) {
					c->UpdateProjectionMatrix();
				};
				Separator();

				EndChild();
			}
			EndTabItem();
		}
		EndTabBar();
	}
	
                                         ImGui::PopStyleVar();
                                             ImGui::PopStyleVar();
                                         ImGui::PopStyleVar();
                                     ImGui::PopStyleVar();
                                 ImGui::PopStyleVar();
                                     ImGui::PopStyleVar();
                                         ImGui::PopStyleVar();
                                     ImGui::PopStyleColor();
                        ImGui::PopStyleColor();    ImGui::PopStyleColor();
                    ImGui::PopStyleColor();            ImGui::PopStyleColor();
                ImGui::PopStyleColor();                    ImGui::PopStyleColor();
            ImGui::PopStyleColor();        /*  .u.  */          ImGui::PopStyleColor();
        ImGui::PopStyleColor();                                    ImGui::PopStyleColor();
    ImGui::PopStyleColor();                                            ImGui::PopStyleColor();
	
										  ImGui::End();
}

void RenderCanvasSystem::DebugBar() {
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
	
	ImGui::SetNextWindowSize(ImVec2(DengWindow->width, 20));
	ImGui::SetNextWindowPos(ImVec2(0, DengWindow->height - 20));
	
	
	//window styling
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding,   ImVec2(0, 2));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,  ImVec2(2, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleColor(ImGuiCol_Border,           ColToVec4(Color(0, 0, 0, 255)));
	ImGui::PushStyleColor(ImGuiCol_WindowBg,         ColToVec4(Color(20, 20, 20, 255)));
	ImGui::PushStyleColor(ImGuiCol_TableBorderLight, ColToVec4(Color(45, 45, 45, 255)));
	
	ImGui::Begin("DebugBar", (bool*)1, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
	debugbarheight = 20;
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
		ImGui::EndTable();
	}
	
	ImGui::PopStyleVar();
	ImGui::PopStyleVar();
	ImGui::PopStyleVar();
	ImGui::PopStyleColor();
	ImGui::PopStyleColor();
	ImGui::PopStyleColor();
	ImGui::End();
}

//sort of sandbox for drawing ImGui stuff over the entire screen
void RenderCanvasSystem::DebugLayer() {
	
	ImGui::SetNextWindowSize(ImVec2(DengWindow->width, DengWindow->height));
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ColToVec4(Color(0, 0, 0, 0)));
	Camera* c = admin->mainCamera;
	float time = DengTime->totalTime;
	
	static std::vector<std::pair<float, Vector2>> times;
	
	static std::vector<Vector3> spots;
	
	
	ImGui::Begin("DebugLayer", 0, ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus |  ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);
	
	Vector2 mp = DengInput->mousePos;
	
	float fontsize = ImGui::GetFontSize();
	
	
	
	
	//psuedo grid
	int lines = 100;
	for (int i = 0; i < lines * 2; i++) {
		Vector3 cpos = c->position;
		Vector3 v1 = Math::WorldToCamera4(Vector3(floor(cpos.x) + -lines + i, 0, floor(cpos.z) + -lines), c->viewMatrix).ToVector3();
		Vector3 v2 = Math::WorldToCamera4(Vector3(floor(cpos.x) + -lines + i, 0, floor(cpos.z) + lines), c->viewMatrix).ToVector3();
		Vector3 v3 = Math::WorldToCamera4(Vector3(floor(cpos.x) + -lines, 0, floor(cpos.z) + -lines + i), c->viewMatrix).ToVector3();
		Vector3 v4 = Math::WorldToCamera4(Vector3(floor(cpos.x) + lines, 0, floor(cpos.z) + -lines + i), c->viewMatrix).ToVector3();
		
		bool l1flag = false;
		bool l2flag = false;
		
		if (floor(cpos.x) - lines + i == 0) {
			l1flag = true;
		}
		if (floor(cpos.z) - lines + i == 0) {
			l2flag = true;
		}
		//Vector3 v1t = v1.ToVector3();
		//Vector3 v2t = v2.ToVector3();
		//Vector3 v3t = v3.ToVector3();
		//Vector3 v4t = v4.ToVector3();
		
		
		if (Math::ClipLineToZPlanes(v1, v2, c)) {
			Vector3 v1s = Math::CameraToScreen3(v1, c->projectionMatrix, DengWindow->dimensions);
			Vector3 v2s = Math::CameraToScreen3(v2, c->projectionMatrix, DengWindow->dimensions);
			Math::ClipLineToBorderPlanes(v1s, v2s, DengWindow->dimensions);
			if (!l1flag) ImGui::GetBackgroundDrawList()->AddLine(v1s.ToVector2().ToImVec2(), v2s.ToVector2().ToImVec2(), ImGui::GetColorU32(ImVec4(1, 1, 1, 0.3)));
			else        ImGui::GetBackgroundDrawList()->AddLine(v1s.ToVector2().ToImVec2(), v2s.ToVector2().ToImVec2(), ImGui::GetColorU32(ImVec4(1, 0, 0, 1)));
		}
		if (Math::ClipLineToZPlanes(v3, v4, c)) {
			Vector3 v3s = Math::CameraToScreen3(v3, c->projectionMatrix, DengWindow->dimensions);
			Vector3 v4s = Math::CameraToScreen3(v4, c->projectionMatrix, DengWindow->dimensions);
			Math::ClipLineToBorderPlanes(v3s, v4s, DengWindow->dimensions);
			if (!l2flag) ImGui::GetBackgroundDrawList()->AddLine(v3s.ToVector2().ToImVec2(), v4s.ToVector2().ToImVec2(), ImGui::GetColorU32(ImVec4(1, 1, 1, 0.3)));
			else        ImGui::GetBackgroundDrawList()->AddLine(v3s.ToVector2().ToImVec2(), v4s.ToVector2().ToImVec2(), ImGui::GetColorU32(ImVec4(0, 0, 1, 1)));
		}
	}
	
	if (DengInput->MousePressed(MouseButton::MB_LEFT) && rand() % 100 + 1 == 80) {
		times.push_back(std::pair<float, Vector2>(0.f, mp));
	}
	
	int index = 0;
	for (auto& f : times) {
		ImGui::PushStyleColor(ImGuiCol_Text, ColToVec4(Color(255. * fabs(sinf(time)), 255. * fabs(cosf(time)), 255, 255)));
		
		f.first += DengTime->deltaTime;
		
		Vector2 p = f.second;
		
		ImGui::SetCursorPos(ImVec2(p.x + 20 * sin(2 * time), p.y - 200 * (f.first / 5)));
		
		Vector2 curpos = Vector2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY());
		
		std::string str1 = "hehe!!!!";
		float strlen1 = (fontsize - (fontsize / 2)) * str1.size();
		for (int i = 0; i < str1.size(); i++) {
			ImGui::SetCursorPos(ImVec2(
									   curpos.x + i * fontsize / 2,
									   curpos.y + sin(10 * time + cos(10 * time + (i * M_PI / 2)) + (i * M_PI / 2))
									   ));
			ImGui::Text(str1.substr(i, 1).c_str());
		}
		
		if (f.first >= 5) {
			times.erase(times.begin() + index);
			index--;
		}
		
		ImGui::PopStyleColor();
		index++;
	}
	//ImGui::Text("test");
	
	
	ImGui::PopStyleColor();
	ImGui::End();
}

void RenderCanvasSystem::DrawUI(void) {
	if (DengInput->KeyPressed(DengKeys->toggleDebugMenu)) showDebugTools = !showDebugTools;
	if (DengInput->KeyPressed(DengKeys->toggleDebugBar)) showDebugBar = !showDebugBar;
	if (DengInput->KeyPressed(DengKeys->toggleMenuBar)) showMenuBar = !showMenuBar;
	
	if (showDebugLayer) DebugLayer();
	if (showDebugTools) DebugTools();
	if (showDebugBar)   DebugBar();
	if (showMenuBar)    MenuBar();
	if (showImGuiDemoWindow) ImGui::ShowDemoWindow();
	
	
	if (!showMenuBar) {
		menubarheight = 0;
	}
	
	if (!showDebugBar){
		debugbarheight = 0;
	}
	if (!showDebugTools) {
		debugtoolswidth = 0;
	}
}

void RenderCanvasSystem::Init(EntityAdmin* admin) {
	System::Init(admin);
	files = deshi::iterateDirectory(deshi::dirModels());
	textures = deshi::iterateDirectory(deshi::dirTextures());
	Canvas* canvas = admin->tempCanvas;
}

void RenderCanvasSystem::Update() {
	WinHovFlag = 0;
	Canvas* canvas = admin->tempCanvas;
	DrawUI();
	if (ConsoleHovFlag || WinHovFlag) admin->IMGUI_MOUSE_CAPTURE = true;
	else                              admin->IMGUI_MOUSE_CAPTURE = false;
}

