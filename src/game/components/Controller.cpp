#include "Camera.h"
#include "MeshComp.h"
#include "Controller.h"
#include "../Transform.h"
#include "../Keybinds.h"
#include "../../core.h"
#include "../../EntityAdmin.h"
#include "../../math/Math.h"
#include "../../scene/Scene.h"
#include "../../geometry/Edge.h"
#include "../../geometry/Triangle.h"

Controller::Controller(EntityAdmin* a, MovementMode m) : Component(a), mode(m) {
	//not sure where i want this yet
	strncpy_s(name, "Controller", 63);
	this->name[63] = '\0';
	layer = NONE;
}

bool CONTROLLER_MOUSE_CAPTURE = false;
bool moveOverride = false; //for moving when using arrow keys (cause i cant use mouse when remoting into my pc so)

inline void CameraMovement(EntityAdmin* admin, MovementMode mode) {
	Camera* camera = admin->mainCamera;
	float deltaTime = admin->time->deltaTime;
	
	//most likely temporary
	if (DengInput->KeyPressed(Key::A | INPUTMOD_CTRL)) moveOverride = !moveOverride;
	
	if (!admin->IMGUI_KEY_CAPTURE && camera->freeCamera) {
		
		if(DengInput->MouseDownAnyMod(MouseButton::MB_RIGHT) || moveOverride){
			Vector3 inputs;
			if (mode == MOVEMENT_MODE_FLYING) {
				//translate up
				if (DengInput->KeyDownAnyMod(DengKeys->movementFlyingUp)) {  inputs.y += 1;  }
				
				//translate down
				if (DengInput->KeyDownAnyMod(DengKeys->movementFlyingDown)) {  inputs.y -= 1; }
			}
			
			//translate forward
			if (DengInput->KeyDownAnyMod(DengKeys->movementFlyingForward)) {  inputs += camera->forward; }
			
			//translate back
			if (DengInput->KeyDownAnyMod(DengKeys->movementFlyingBack)) {  inputs -= camera->forward; }
			
			//translate right
			if (DengInput->KeyDownAnyMod(DengKeys->movementFlyingRight)) {  inputs += camera->right; }
			
			//translate left
			if (DengInput->KeyDownAnyMod(DengKeys->movementFlyingLeft)) { inputs -= camera->right; }
			
			
			if (DengInput->ShiftDown())     { camera->position += inputs.normalized() * 16 * deltaTime; }
			else if (DengInput->CtrlDown()) { camera->position += inputs.normalized() *  4 * deltaTime; }
			else							{ camera->position += inputs.normalized() *  8 * deltaTime; }
		}
	}
}

inline void CameraRotation(EntityAdmin* admin, float sens) {
	Camera* camera = admin->mainCamera;
	Input* input = admin->input;
	Keybinds* binds = admin->keybinds;
	Window* window = admin->window;
	float deltaTime = admin->time->deltaTime;
	
	if (!admin->IMGUI_KEY_CAPTURE && camera->freeCamera) {
		
		//camera rotation up
		if (input->KeyDownAnyMod(binds->cameraRotateUp)) {
			if (input->ModsDown(INPUTMOD_SHIFT))     { camera->rotation.x -= 50 * deltaTime; }
			else if (input->ModsDown(INPUTMOD_CTRL)) { camera->rotation.x -= 5 * deltaTime; }
			else                                     { camera->rotation.x -= 25 * deltaTime; }
		}
		
		//camera rotation down
		if (input->KeyDownAnyMod(binds->cameraRotateDown)) {
			if (input->ModsDown(INPUTMOD_SHIFT))     { camera->rotation.x += 50 * deltaTime; }
			else if (input->ModsDown(INPUTMOD_CTRL)) { camera->rotation.x += 5 * deltaTime; }
			else                                     { camera->rotation.x += 25 * deltaTime; }
		}
		
		//camera rotation right
		if (input->KeyDownAnyMod(binds->cameraRotateRight)) {
			if (input->ModsDown(INPUTMOD_SHIFT))	 { camera->rotation.y += 50 * deltaTime; }
			else if (input->ModsDown(INPUTMOD_CTRL)) { camera->rotation.y += 5 * deltaTime; }
			else								     { camera->rotation.y += 25 * deltaTime; }
		}
		
		//camera rotation left
		if (input->KeyDownAnyMod(binds->cameraRotateLeft)) {
			if (input->ModsDown(INPUTMOD_SHIFT))	 { camera->rotation.y -= 50 * deltaTime; }
			else if (input->ModsDown(INPUTMOD_CTRL)) { camera->rotation.y -= 5 * deltaTime; }
			else								     { camera->rotation.y -= 25 * deltaTime; }
		}
		
	}
	if(!admin->IMGUI_MOUSE_CAPTURE && !CONTROLLER_MOUSE_CAPTURE && camera->freeCamera){
		//TODO(delle,In) change this so its dependent on game state or something (level editor vs gameplay)
		if(input->MousePressed(MouseButton::MB_RIGHT | INPUTMOD_ANY)){
			admin->ExecCommand("cursor_mode", "1");
		}
		if(input->MouseDown(MouseButton::MB_RIGHT | INPUTMOD_ANY)){
			camera->rotation.y += sens * (input->mouseX - window->centerX) * .03f;
			camera->rotation.x += sens * (input->mouseY - window->centerY) * .03f;
		}
		if(input->MouseReleased(MouseButton::MB_RIGHT | INPUTMOD_ANY)){
			admin->ExecCommand("cursor_mode", "0");
		}
	}
}

inline void CameraZoom(EntityAdmin* admin){
	Camera* cam = admin->mainCamera;
	Renderer* renderer = admin->renderer;
	
	if (DengInput->KeyPressed(Key::NUMPADPLUS)) { 
		cam->fov += 5;
		cam->UpdateProjectionMatrix();
	}
	if (DengInput->KeyPressed(Key::NUMPADMINUS)) {
		cam->fov -= 5;
		cam->UpdateProjectionMatrix();
	}
}

void HandleMouseInputs(EntityAdmin* admin) {
	Canvas* canvas = admin->tempCanvas;
	Input* input = admin->input;
	
	//mouse left click pressed
	if (input->MousePressed(MouseButton::MB_LEFT)) {
		
		if (!admin->IMGUI_MOUSE_CAPTURE && !CONTROLLER_MOUSE_CAPTURE) {
			
			//TODO(sushi, Ma) figure out why this sometimes returns true when clicking outside of object
			
			Vector3 pos = Math::ScreenToWorld(admin->input->mousePos, admin->mainCamera->projectionMatrix,
											  admin->mainCamera->viewMatrix, admin->window->dimensions);
			pos *= Math::WorldToLocal(admin->mainCamera->position);
			pos.normalize();
			pos *= 1000;
			pos *= Math::LocalToWorld(admin->mainCamera->position);
			
			RenderedEdge3D* ray = new RenderedEdge3D(pos, admin->mainCamera->position);
			
			admin->input->selectedEntity = nullptr;
			Vector3 p0, p1, p2, norm;
			Matrix4 rot;
			for (auto ep : admin->entities) {
				Entity* e = ep.second;
				if (MeshComp* mc = e->GetComponent<MeshComp>()) {
					Mesh* m = mc->m;
					for (auto& b : m->batchArray) {
						for (int i = 0; i < b.indexArray.size(); i += 3) {
							float t = 0;

							p0 = b.vertexArray[b.indexArray[i]].pos + e->transform.position;
							p1 = b.vertexArray[b.indexArray[i + 1]].pos + e->transform.position;
							p2 = b.vertexArray[b.indexArray[i + 2]].pos + e->transform.position;

							norm = (p1 - p0).cross(p2 - p0);

							Vector3 inter = Math::VectorPlaneIntersect(p0, norm, ray->p[0], ray->p[1], t);

							Vector3 v01 = p1 - p0;
							Vector3 v12 = p2 - p1;
							Vector3 v20 = p0 - p2;

							rot = Matrix4::AxisAngleRotationMatrix(90, Vector4(norm, 0));

							if ((v01 * rot).dot(p0 - inter) < 0 &&
								(v12 * rot).dot(p1 - inter) < 0 &&
								(v20 * rot).dot(p2 - inter) < 0) {
								admin->input->selectedEntity = e;
								goto endloop;
							}
						}
					}
				}
			}
			endloop:
			char* wow = "wow";
		}
	}
	//mouse left click held
	else if (input->MouseDown(MouseButton::MB_LEFT)) {
		//static_internal Vector2 offset;
		//if(input->selectedUI) {
		//	if(!input->ui_drag_latch) {
		//		offset = input->selectedUI->pos - input->mousePos;
		//		input->ui_drag_latch = true;
		//	}
		//	input->selectedUI->pos = input->mousePos + offset;
		//	input->selectedUI->Update();
		//}
	}
	//mouse left click released
	else if (input->MouseReleased(MouseButton::MB_LEFT)) {
		//if(input->selectedUI) {					//deselect UI
		//	input->selectedUI = 0;
		//	input->ui_drag_latch = false;
		//} else if(input->selectedEntity && input->mousePos != input->mouseClickPos) { //add force to selected entity
		//	admin->ExecCommand("add_force");
		//}
		//
		////reset click pos to null
		//input->mouseClickPos = Vector2(admin->p->ScreenWidth(), admin->p->ScreenHeight());
	}
	
	//if(input->KeyPressed(Key::MINUS)) {
	//	admin->p->SetMousePositionLocal(admin->p->GetWindowSize() / 2);
	//}
}

void HandleSelectedEntityInputs(EntityAdmin* admin) {
	Input* input = admin->input;
	Camera* c = admin->mainCamera;
	Entity* sel = input->selectedEntity;

	if (sel) {
		
		static bool grabbingObj = false;
		static bool rotatingObj = false;
		static bool scalingObj = false;
		
		if (!admin->IMGUI_MOUSE_CAPTURE) {
			if (DengInput->KeyPressed(DengKeys->grabSelectedObject) || grabbingObj) {
				//Camera* c = admin->mainCamera;
				grabbingObj = true;
				CONTROLLER_MOUSE_CAPTURE = true;
				
				float t = 0;
				
				//bools for if we're in an axis movement mode
				static bool xaxis = false;
				static bool yaxis = false;
				static bool zaxis = false;
				
				static bool initialgrab = true;
				
				static Vector3 worldpos;
				static Vector3 initialObjPos;
				static float initialdist; 
				
				//different cases for mode chaning
				if (DengInput->KeyPressed(Key::X)) {
					xaxis = true; yaxis = false; zaxis = false; 
					sel->transform.position = initialObjPos;
				}
				if (DengInput->KeyPressed(Key::Y)) {
					xaxis = false; yaxis = true; zaxis = false; 
					sel->transform.position = initialObjPos;
				}
				if (DengInput->KeyPressed(Key::Z)) {
					xaxis = false; yaxis = false; zaxis = true; 
					sel->transform.position = initialObjPos;
				}
				if (!(xaxis || yaxis || zaxis) && DengInput->KeyPressed(Key::ESCAPE)) { //|| DengInput->MousePressed(1)) {
					//stop grabbing entirely if press esc or right click w no grab mode on
					//TODO(sushi, In) figure out why the camera rotates violently when rightlicking to leave grabbing. probably because of the mouse moving to the object?
					xaxis = false; yaxis = false; zaxis = false; 
					sel->transform.position = initialObjPos;
					initialgrab = true; grabbingObj = false;
					CONTROLLER_MOUSE_CAPTURE = false;
					return;
				}
				if ((xaxis || yaxis || zaxis) && DengInput->KeyPressed(Key::ESCAPE)) {
					//leave grab mode if in one when pressing esc
					xaxis = false; yaxis = false; zaxis = false; 
					sel->transform.position = initialObjPos; initialgrab = true;
				}
				if (DengInput->MousePressed(0)) {
					//drop the object if left click
					xaxis = false; yaxis = false; zaxis = false;
					initialgrab = true; grabbingObj = false;  
					CONTROLLER_MOUSE_CAPTURE = false;
					return;
				}

				//if we're initially grabbing the object, set the mouse's position to it
				//and get initial distance from obj
				if (initialgrab) {
					Vector2 screenPos = Math::WorldToScreen2(sel->transform.position, c->projectionMatrix, c->viewMatrix, admin->window->dimensions);
					admin->window->SetCursorPos(screenPos);
					worldpos = Math::ScreenToWorld(DengInput->mousePos, c->projectionMatrix,
												   c->viewMatrix, admin->window->dimensions);
					initialObjPos = sel->transform.position;
					initialdist = (worldpos - sel->transform.position).mag();
					initialgrab = false;
				}
				
				if (!(xaxis || yaxis || zaxis)) {
					
					Vector3 nuworldpos = Math::ScreenToWorld(DengInput->mousePos, c->projectionMatrix,
															 c->viewMatrix, admin->window->dimensions);
					
					
					Vector3 newpos = nuworldpos;
					
					newpos *= Math::WorldToLocal(admin->mainCamera->position);
					newpos.normalize();
					newpos *= initialdist;
					newpos *= Math::LocalToWorld(admin->mainCamera->position);
					
					sel->transform.position = newpos;
					
					worldpos = nuworldpos;
					
				}
				else if (xaxis) {
					ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0,0,0));
					ImGui::Begin("DebugLayer", 0, ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus |  ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);
					
					//project ray out into world from mouse
					Vector3 pos = Math::ScreenToWorld(admin->input->mousePos, admin->mainCamera->projectionMatrix,
													  admin->mainCamera->viewMatrix, admin->window->dimensions);
					pos *= Math::WorldToLocal(admin->mainCamera->position);
					pos.normalize();
					pos *= 1000;
					pos *= Math::LocalToWorld(admin->mainCamera->position);

					
					Vector3 planeinter;

					if (Math::AngBetweenVectors(Vector3(c->forward.x, 0, c->forward.z), c->forward) > 60) {
						planeinter = Math::VectorPlaneIntersect(initialObjPos, Vector3::UP, c->position, pos);
					}
					else {
						planeinter = Math::VectorPlaneIntersect(initialObjPos, Vector3::FORWARD, c->position, pos);
					}

					
					sel->transform.position = Vector3(planeinter.x, initialObjPos.y, initialObjPos.z);

					Vector3 xp1 = initialObjPos - (Vector3::RIGHT * 2000);
					Vector3 xp2 = initialObjPos + (Vector3::RIGHT * 2000);
					
					Vector3 pc1 = Math::WorldToCamera4(xp1, c->viewMatrix).ToVector3();
					Vector3 pc2 = Math::WorldToCamera4(xp2, c->viewMatrix).ToVector3();
					
					if (Math::ClipLineToZPlanes(pc1, pc2, c)) {
						
						Vector3 pn1 = Vector3::FORWARD * Matrix4::AxisAngleRotationMatrix(  70 - (c->fov / 2), Vector4(0, 1, 0, 0));
						Vector3 pn2 = Vector3::FORWARD * Matrix4::AxisAngleRotationMatrix(-(70 - (c->fov / 2)), Vector4(0, 1, 0, 0));

						Vector3 ps1 = Math::VectorPlaneIntersect(Vector3::ZERO, pn1, pc1, pc2, t); 
						Vector3 ps2 = Math::VectorPlaneIntersect(Vector3::ZERO, pn2, pc1, pc2, t); 
						
						//get 2D positions of plane intersections and camera position from top down
						Vector2 p1 = Vector2(ps1.x, ps1.z);
						Vector2 p2 = Vector2(ps2.x, ps2.z);
						
						Vector2 campos = Vector2(c->position.x, c->position.z);
						Vector2 camfor = Vector2(c->forward.x, c->forward.z);

						Vector3 pir1 = Math::CameraToWorld4(ps1, c->viewMatrix).ToVector3();
						Vector3 pir2 = Math::CameraToWorld4(ps2, c->viewMatrix).ToVector3();
						
						Vector3 v1s = Math::CameraToScreen3(ps1, c->projectionMatrix, DengWindow->dimensions);
						Vector3 v2s = Math::CameraToScreen3(ps2, c->projectionMatrix, DengWindow->dimensions);
						
						Math::ClipLineToBorderPlanes(v1s, v2s, DengWindow->dimensions);
						
						Vector2 mouseline = DengInput->mousePos - v1s.ToVector2();
						Vector2 screenline = v2s.ToVector2() - v1s.ToVector2();
						Vector3 worldline = pir2 - pir1;
						
						Vector3 backplaneintersect = Math::VectorPlaneIntersect(c->forward + (c->forward.normalized() * c->farZ), c->forward, pir1, pir2, t);
						
						float dist = mouseline.dot(screenline.normalized());
						float ratio = (screenline.normalized() * dist).mag() / screenline.mag();
						
						//sel->transform->position = pir1 + (worldline.mag() * ratio) * worldline.normalized();
						
						float fontsize = ImGui::GetFontSize();
						std::string str1 = TOSTRING(pir2);
						float strlen1 = (fontsize - (fontsize / 2)) * str1.size();
						
						ImGui::GetBackgroundDrawList()->AddLine(DengInput->mousePos.ToImVec2(), v1s.ToVector2().ToImVec2(), ImGui::GetColorU32(ImVec4(0, 0.6, 0.3, 1)));
						ImGui::GetBackgroundDrawList()->AddLine((v1s.ToVector2() + (screenline.normalized()* dist)).ToImVec2(), v1s.ToVector2().ToImVec2(), ImGui::GetColorU32(ImVec4(0.3, 0.6, 0, 1)));
						ImGui::GetBackgroundDrawList()->AddLine(v1s.ToVector2().ToImVec2(), v2s.ToVector2().ToImVec2(), ImGui::GetColorU32(ImVec4(0, 1, 0.1, 1)));
						
						ImGui::SetCursorPos(v1s.ToVector2().ToImVec2());
						ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8, 0.4, 0, 1));
						ImGui::Text(TOSTRING(pir1).c_str());
						ImGui::PopStyleColor();
						ImGui::SetCursorPos(v2s.ToVector2().ToImVec2());
						ImGui::SetCursorPosX(ImGui::GetCursorPosX() - strlen1);
						ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0.4, 0.8, 1));
						ImGui::Text(TOSTRING(pir2).c_str());
						ImGui::PopStyleColor();
						
						
						
						
					}
					
					//screen axis line
					//Vector2 sal = slpi - srpi;
					
					//displacement of projected point along screen axis line
					//float disp = (DengInput->mousePos - srpi).dot(sal);
					
					//float ratio = (sal.normalized() * disp).mag();
					
					//TODO(sushi, In) implement displacement mod changing when mouse scrolling works
					float dispmod = 0.1;
					
					if (DengInput->ModsDown(INPUTMOD_CTRL)) dispmod = 0.05;
					else dispmod = 0.1;
					
					//sel->transform.position = rpi + ratio * (lpi - rpi);
					
					ImGui::PopStyleColor();
					ImGui::End();
				}
				else if (yaxis) {
					//project ray out into world from mouse
					Vector3 pos = Math::ScreenToWorld(admin->input->mousePos, admin->mainCamera->projectionMatrix,
													  admin->mainCamera->viewMatrix, admin->window->dimensions);
					pos *= Math::WorldToLocal(admin->mainCamera->position);
					pos.normalize();
					pos *= 1000;
					pos *= Math::LocalToWorld(admin->mainCamera->position);


					Vector3 planeinter;

					if (Math::AngBetweenVectors(Vector3(c->forward.x, 0, c->forward.z), c->forward) > 60) {
						planeinter = Math::VectorPlaneIntersect(initialObjPos, Vector3::RIGHT, c->position, pos);
					}
					else {
						planeinter = Math::VectorPlaneIntersect(initialObjPos, Vector3::FORWARD, c->position, pos);
					}
					sel->transform.position = Vector3(initialObjPos.x, planeinter.y, initialObjPos.z);

				}
				else if (zaxis) {
					Vector3 pos = Math::ScreenToWorld(admin->input->mousePos, admin->mainCamera->projectionMatrix,
													  admin->mainCamera->viewMatrix, admin->window->dimensions);
					pos *= Math::WorldToLocal(admin->mainCamera->position);
					pos.normalize();
					pos *= 1000;
					pos *= Math::LocalToWorld(admin->mainCamera->position);


					Vector3 planeinter;

					if (Math::AngBetweenVectors(Vector3(c->forward.x, 0, c->forward.z), c->forward) > 60) {
						planeinter = Math::VectorPlaneIntersect(initialObjPos, Vector3::UP, c->position, pos);
					}
					else {
						planeinter = Math::VectorPlaneIntersect(initialObjPos, Vector3::RIGHT, c->position, pos);
					}
					sel->transform.position = Vector3(initialObjPos.x, initialObjPos.y, planeinter.z);
				}
				
			}
		}
		
		
		if (!admin->IMGUI_KEY_CAPTURE) {
			
			
			if (DengInput->KeyDown(Key::L)) {
				admin->ExecCommand("translate_right");
			}
			
			if (DengInput->KeyDown(Key::J)) {
				admin->ExecCommand("translate_left");
			}
			
			if (DengInput->KeyDown(Key::O)) {
				admin->ExecCommand("translate_up");
			}
			
			if (DengInput->KeyDown(Key::U)) {
				admin->ExecCommand("translate_down");
			}
			
			if (DengInput->KeyDown(Key::I)) {
				admin->ExecCommand("translate_forward");
			}
			
			if (DengInput->KeyDown(Key::K)) {
				admin->ExecCommand("translate_backward");
			}
			
			//rotation
			if (DengInput->KeyDown(Key::L | INPUTMOD_SHIFT)) {
				admin->ExecCommand("rotate_+x");
			}
			
			if (DengInput->KeyDown(Key::J | INPUTMOD_SHIFT)) {
				admin->ExecCommand("rotate_-x");
			}
			
			if (DengInput->KeyDown(Key::O | INPUTMOD_SHIFT)) {
				admin->ExecCommand("rotate_+y");
			}
			
			if (DengInput->KeyDown(Key::U | INPUTMOD_SHIFT)) {
				admin->ExecCommand("rotate_-y");
			}
			
			if (DengInput->KeyDown(Key::I | INPUTMOD_SHIFT)) {
				admin->ExecCommand("rotate_+z");
			}
			
			if (DengInput->KeyDown(Key::K | INPUTMOD_SHIFT)) {
				admin->ExecCommand("rotate_-z");
			}
		}
	}
}

void HandleRenderInputs(EntityAdmin* admin) {
	Renderer* renderer = admin->renderer;
	Input* input = admin->input;
	Keybinds* binds = admin->keybinds;
	
	//reload shaders
	if (input->KeyPressed(Key::F5)) {
		admin->ExecCommand("shader_reload", "-1");
	}
}




void Controller::Update() {
	CameraMovement(admin, mode);
	CameraRotation(admin, mouseSensitivity);
	CameraZoom(admin);
	HandleMouseInputs(admin);
	HandleSelectedEntityInputs(admin);
	HandleRenderInputs(admin);
}
