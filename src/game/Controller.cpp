#include "Controller.h"
#include "Transform.h"
#include "Keybinds.h"
#include "UndoManager.h"
#include "components/Camera.h"
#include "components/MeshComp.h"
#include "components/Physics.h"
#include "../core.h"
#include "../EntityAdmin.h"
#include "../math/Math.h"
#include "../scene/Scene.h"
#include "../geometry/Edge.h"

#include <fstream>

bool CONTROLLER_MOUSE_CAPTURE = false;
bool moveOverride = false; //for moving when using arrow keys (cause i cant use mouse when remoting into my pc so)

inline void AddBindings(EntityAdmin* admin) {
	std::ifstream binds;
	
	if (deshi::getConfig("binds.cfg") != "") {
		binds = std::ifstream(deshi::getConfig("binds.cfg"), std::ios::in);
		while (!binds.eof()) {
			char* c = (char*)malloc(255);
			binds.getline(c, 255);
			std::string s(c);
		
			if (s != "") {
				std::string key = s.substr(0, s.find_first_of(" "));
				std::string command = s.substr(s.find_first_of(" ") + 1, s.length());

				try {
					DengInput->binds.push_back(std::pair<std::string, Key::Key>(command, DengKeys.stk.at(key)));
				}
				catch (...) {
					ERROR("Unknown key \"", key, "\" attempted to bind to command \"", command, "\"");
				}
			}
		}
	}
	else {
		LOG("Creating binds file..");
		deshi::writeFile(deshi::dirConfig() + "binds.cfg", "", 0);
		
		return;
	}
}

inline void CameraMovement(EntityAdmin* admin, MovementMode mode) {
	Camera* camera = admin->mainCamera;
	float deltaTime = admin->time->deltaTime;
	
	//most likely temporary
	if (DengInput->KeyPressed(Key::A | INPUTMOD_CTRL)) moveOverride = !moveOverride;
	
	if(DengInput->KeyDownAnyMod(MouseButton::RIGHT) || moveOverride){
		Vector3 inputs;
		if (mode == MOVEMENT_MODE_FLYING) {
			if (DengInput->KeyDownAnyMod(DengKeys.movementFlyingUp))      {  inputs.y += 1;  }
			if (DengInput->KeyDownAnyMod(DengKeys.movementFlyingDown))    {  inputs.y -= 1; }
			if (DengInput->KeyDownAnyMod(DengKeys.movementFlyingForward)) {  inputs += camera->forward; }
			if (DengInput->KeyDownAnyMod(DengKeys.movementFlyingBack))    {  inputs -= camera->forward; }
			if (DengInput->KeyDownAnyMod(DengKeys.movementFlyingRight))   {  inputs += camera->right; }
			if (DengInput->KeyDownAnyMod(DengKeys.movementFlyingLeft))    {  inputs -= camera->right; }
			
			if     (DengInput->ShiftDown()) { camera->position += inputs.normalized() * 16 * deltaTime; }
			else if(DengInput->CtrlDown())  { camera->position += inputs.normalized() *  4 * deltaTime; }
			else							{ camera->position += inputs.normalized() *  8 * deltaTime; }
		}
	}
}

inline void CameraRotation(EntityAdmin* admin, float sens) {
	Camera* camera = admin->mainCamera;
	Input* input = admin->input;
	Keybinds* binds = &admin->keybinds;
	Window* window = admin->window;
	float deltaTime = admin->time->deltaTime;
	
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
	
	if(!admin->IMGUI_MOUSE_CAPTURE && !CONTROLLER_MOUSE_CAPTURE){
		if(admin->state == GameState::PLAY || admin->state == GameState::PLAY_DEBUG){
			camera->rotation.y += sens * (input->mouseX - window->centerX) * .03f;
			camera->rotation.x += sens * (input->mouseY - window->centerY) * .03f;
		}
		else if(admin->state == GameState::EDITOR){
			if(input->KeyPressed(MouseButton::RIGHT | INPUTMOD_ANY)){
				admin->ExecCommand("window_cursor_mode", "1");
			}
			if(input->KeyDown(MouseButton::RIGHT | INPUTMOD_ANY)){
				camera->rotation.y += sens * (input->mouseX - window->centerX) * .03f;
				camera->rotation.x += sens * (input->mouseY - window->centerY) * .03f;
			}
			if(input->KeyReleased(MouseButton::RIGHT | INPUTMOD_ANY)){
				admin->ExecCommand("window_cursor_mode", "0");
			}
		}
	}
}

inline void CameraZoom(EntityAdmin* admin){
	if (DengInput->KeyPressed(Key::NUMPADPLUS)) { 
		admin->mainCamera->fov += 5;
		admin->mainCamera->UpdateProjectionMatrix();
	}
	if (DengInput->KeyPressed(Key::NUMPADMINUS)) {
		admin->mainCamera->fov -= 5;
		admin->mainCamera->UpdateProjectionMatrix();
	}
}

//TODO(sushi, Ma) figure out why this sometimes returns true when clicking outside of object
inline void HandleSelectEntity(EntityAdmin* admin){
	Vector3 pos = Math::ScreenToWorld(DengInput->mousePos, admin->mainCamera->projectionMatrix,
									  admin->mainCamera->viewMatrix, admin->window->dimensions);
	pos *= Math::WorldToLocal(admin->mainCamera->position);
	pos.normalize();
	pos *= 1000;
	pos *= Math::LocalToWorld(admin->mainCamera->position);
	
	RenderedEdge3D* ray = new RenderedEdge3D(pos, admin->mainCamera->position);
	
	Entity* oldEnt = DengInput->selectedEntity;
	DengInput->selectedEntity = nullptr;
	Vector3 p0, p1, p2, norm;
	Matrix4 rot;
	for (auto& e : admin->entities) {
		if (MeshComp* mc = e.GetComponent<MeshComp>()) {
			if (mc->mesh_visible) {
				Mesh* m = mc->m;
				int te = 0;
				for (auto& b : m->batchArray) {
					te++;
					for (int i = 0; i < b.indexArray.size(); i += 3) {
						float t = 0;
						
						p0 = b.vertexArray[b.indexArray[i]].pos + e.transform.position;
						p1 = b.vertexArray[b.indexArray[i + 1]].pos + e.transform.position;
						p2 = b.vertexArray[b.indexArray[i + 2]].pos + e.transform.position;
						
						norm = (p1 - p0).cross(p2 - p0);
						
						Vector3 inter = Math::VectorPlaneIntersect(p0, norm, ray->p[0], ray->p[1], t);
						
						Vector3 v01 = p1 - p0;
						Vector3 v12 = p2 - p1;
						Vector3 v20 = p0 - p2;
						
						rot = Matrix4::AxisAngleRotationMatrix(90, Vector4(norm, 0));
						
						if ((v01 * rot).dot(p0 - inter) < 0 &&
							(v12 * rot).dot(p1 - inter) < 0 &&
							(v20 * rot).dot(p2 - inter) < 0) {
							
							DengInput->selectedEntity = &e;
							if(oldEnt != &e){
								admin->undoManager.AddUndoSelect((void**)&DengInput->selectedEntity, oldEnt, &e);
							}
							goto endloop;
						}
					}
				}
			}
		}
	}
	endloop:;
}

inline void HandleGrabbing(Entity* sel, Camera* c, EntityAdmin* admin, UndoManager* um) {
	static bool grabbingObj = false;
	
	if (!admin->IMGUI_MOUSE_CAPTURE) { 
		if (DengInput->KeyPressed(DengKeys.grabSelectedObject) || grabbingObj) {
			//Camera* c = admin->mainCamera;
			grabbingObj = true;
			CONTROLLER_MOUSE_CAPTURE = true;
			
			//bools for if we're in an axis movement mode
			static bool xaxis = false;
			static bool yaxis = false;
			static bool zaxis = false;
			
			static bool initialgrab = true;
			
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
			if (DengInput->KeyPressed(MouseButton::LEFT)) {
				//drop the object if left click
				xaxis = false; yaxis = false; zaxis = false;
				initialgrab = true; grabbingObj = false;  
				CONTROLLER_MOUSE_CAPTURE = false;
				if(initialObjPos != sel->transform.position){
					um->AddUndoTranslate(&sel->transform, &initialObjPos, &sel->transform.position);
				}
				return;
			}

			if (DengInput->KeyPressed(MouseButton::SCROLLDOWN)) initialdist -= 1;
			if (DengInput->KeyPressed(MouseButton::SCROLLUP))   initialdist += 1;

			
			//set mouse to obj position on screen and save that position
			if (initialgrab) {
				Vector2 screenPos = Math::WorldToScreen2(sel->transform.position, c->projectionMatrix, c->viewMatrix, admin->window->dimensions);
				admin->window->SetCursorPos(screenPos);
				initialObjPos = sel->transform.position;
				initialdist = (initialObjPos - c->position).mag();
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
				if (Physics* p = sel->GetComponent<Physics>()) {
					p->position = newpos;
				}
				
			}
			else if (xaxis) {
				Vector3 pos = Math::ScreenToWorld(DengInput->mousePos, admin->mainCamera->projectionMatrix,
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
				if (Physics* p = sel->GetComponent<Physics>()) {
					p->position = Vector3(planeinter.x, initialObjPos.y, initialObjPos.z);
				}
			}
			else if (yaxis) {
				Vector3 pos = Math::ScreenToWorld(DengInput->mousePos, admin->mainCamera->projectionMatrix,
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
				if (Physics* p = sel->GetComponent<Physics>()) {
					p->position = Vector3(initialObjPos.x, planeinter.y, initialObjPos.z);
				}	
			
			}
			else if (zaxis) {
				Vector3 pos = Math::ScreenToWorld(DengInput->mousePos, admin->mainCamera->projectionMatrix,
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
				if (Physics* p = sel->GetComponent<Physics>()) {
					p->position = Vector3(initialObjPos.x, initialObjPos.y, planeinter.z);
				}
			
			}
		} //if(DengInput->KeyPressed(DengKeys.grabSelectedObject) || grabbingObj)
	} //if(!admin->IMGUI_MOUSE_CAPTURE)
}

inline void HandleRotating(Entity* sel, Camera* c, EntityAdmin* admin, UndoManager* um) {
	static bool rotatingObj = false;
	
	if (!admin->IMGUI_MOUSE_CAPTURE) { 
		if (DengInput->KeyPressed(DengKeys.rotateSelectedObject) || rotatingObj) {
			rotatingObj = true;
			CONTROLLER_MOUSE_CAPTURE = true;
			
			//bools for if we're in an axis movement mode
			static bool xaxis = false;
			static bool yaxis = false;
			static bool zaxis = false;
			
			static Vector2 origmousepos = DengInput->mousePos;
			
			static bool initialrot = true;
			
			static Vector3 initialObjRot;
			
			//different cases for mode chaning
			if (DengInput->KeyPressed(Key::X)) {
				xaxis = true; yaxis = false; zaxis = false; 
				sel->transform.rotation = initialObjRot;
			}
			if (DengInput->KeyPressed(Key::Y)) {
				xaxis = false; yaxis = true; zaxis = false; 
				sel->transform.rotation = initialObjRot;
			}
			if (DengInput->KeyPressed(Key::Z)) {
				xaxis = false; yaxis = false; zaxis = true; 
				sel->transform.rotation = initialObjRot;
			}
			if (!(xaxis || yaxis || zaxis) && DengInput->KeyPressed(Key::ESCAPE)) {
				//stop rotating entirely if press esc or right click w no rotate mode on
				xaxis = false; yaxis = false; zaxis = false; 
				sel->transform.rotation = initialObjRot;
				initialrot = true; rotatingObj = false;
				CONTROLLER_MOUSE_CAPTURE = false;
				return;
			}
			if ((xaxis || yaxis || zaxis) && DengInput->KeyPressed(Key::ESCAPE)) {
				//leave rotation mode if in one when pressing esc
				xaxis = false; yaxis = false; zaxis = false; 
				sel->transform.rotation = initialObjRot; initialrot = true;
			}
			if (DengInput->KeyPressed(MouseButton::LEFT)) {
				//drop the object if left click
				xaxis = false; yaxis = false; zaxis = false;
				initialrot = true; rotatingObj = false;  
				CONTROLLER_MOUSE_CAPTURE = false;
				if(initialObjRot != sel->transform.rotation){
					um->AddUndoRotate(&sel->transform, &initialObjRot, &sel->transform.rotation);
				}
				return;
			}
			
			if (initialrot) {
				initialObjRot = sel->transform.rotation;
				initialrot = false;
				origmousepos = DengInput->mousePos;
			}
			
			//TODO(sushi, InMa) implement rotating over an arbitrary axis in a nicer way everntually
			//TODO(sushi, In) make rotation controls a little more nice eg. probably just make it how far along the screen the mouse is to determine it.
			if (!(xaxis || yaxis || zaxis)) {
				
				Vector2 center = Vector2(DengWindow->width / 2, DengWindow->height / 2);
				Vector2 mousepos = DengInput->mousePos;
				
				Vector2 ctm = mousepos - center;
				
				float ang = Math::AngBetweenVectors(ctm, origmousepos - center);
				
				//make angle go between 360 instead of -180 and 180
				if (ang < 0) {
					ang = 180 + (180 + ang);
				}
				
				sel->transform.rotation = Matrix4::AxisAngleRotationMatrix(ang, Vector4((sel->transform.position - c->position).normalized(), 0)).Rotation();
				
				sel->transform.rotation.x = DEGREES(sel->transform.rotation.x);
				sel->transform.rotation.y = DEGREES(sel->transform.rotation.y);
				sel->transform.rotation.z = DEGREES(sel->transform.rotation.z);
				
			}
			else if (xaxis) {
				Vector2 center = Vector2(DengWindow->width / 2, DengWindow->height / 2);
				Vector2 mousepos = DengInput->mousePos;
				
				Vector2 ctm = mousepos - center;
				
				float ang = Math::AngBetweenVectors(ctm, origmousepos - center);
				
				if (ang < 0) {
					ang = 180 + (180 + ang);
				}
				
				sel->transform.rotation.z = ang;
			}
			else if (yaxis) {
				Vector2 center = Vector2(DengWindow->width / 2, DengWindow->height / 2);
				Vector2 mousepos = DengInput->mousePos;
				
				Vector2 ctm = mousepos - center;
				
				float ang = Math::AngBetweenVectors(ctm, origmousepos - center);
				
				if (ang < 0) {
					ang = 180 + (180 + ang);
				}
				
				sel->transform.rotation.y = ang;
			}
			else if (zaxis) {
				Vector2 center = Vector2(DengWindow->width / 2, DengWindow->height / 2);
				Vector2 mousepos = DengInput->mousePos;
				
				Vector2 ctm = mousepos - center;
				
				float ang = Math::AngBetweenVectors(ctm, origmousepos - center);
				
				if (ang < 0) {
					ang = 180 + (180 + ang);
				}
				
				sel->transform.rotation.x = ang;
			}
		} //if(DengInput->KeyPressed(DengKeys.grabSelectedObject) || grabbingObj)
	} //if(!admin->IMGUI_MOUSE_CAPTURE)
}

inline void HandleEditorInputs(EntityAdmin* admin){
	{//// selected entity ////
		Entity* sel = DengInput->selectedEntity;
		
		if (!admin->IMGUI_MOUSE_CAPTURE && !CONTROLLER_MOUSE_CAPTURE) {
			if (DengInput->KeyPressed(MouseButton::LEFT)) { HandleSelectEntity(admin); }
		}
		
		if (sel) {
			HandleGrabbing(sel, admin->mainCamera, admin, &admin->undoManager);
			HandleRotating(sel, admin->mainCamera, admin, &admin->undoManager);
			
			//translation
			if (DengInput->KeyDown(Key::L)) { admin->ExecCommand("translate_right"); }
			if (DengInput->KeyDown(Key::J)) { admin->ExecCommand("translate_left"); }
			if (DengInput->KeyDown(Key::O)) { admin->ExecCommand("translate_up"); }
			if (DengInput->KeyDown(Key::U)) { admin->ExecCommand("translate_down"); }
			if (DengInput->KeyDown(Key::I)) { admin->ExecCommand("translate_forward"); }
			if (DengInput->KeyDown(Key::K)) { admin->ExecCommand("translate_backward"); }
			
			//rotation
			if (DengInput->KeyDown(Key::L | INPUTMOD_SHIFT)) { admin->ExecCommand("rotate_+x"); }
			if (DengInput->KeyDown(Key::J | INPUTMOD_SHIFT)) { admin->ExecCommand("rotate_-x"); }
			if (DengInput->KeyDown(Key::O | INPUTMOD_SHIFT)) { admin->ExecCommand("rotate_+y"); }
			if (DengInput->KeyDown(Key::U | INPUTMOD_SHIFT)) { admin->ExecCommand("rotate_-y"); }
			if (DengInput->KeyDown(Key::I | INPUTMOD_SHIFT)) { admin->ExecCommand("rotate_+z"); }
			if (DengInput->KeyDown(Key::K | INPUTMOD_SHIFT)) { admin->ExecCommand("rotate_-z"); }
		}
	}
	{//// render ////
		//reload all shaders
		if (DengInput->KeyPressed(Key::F5)) { admin->ExecCommand("shader_reload", "-1"); }
		
		//fullscreen toggle
		if (DengInput->KeyPressed(Key::F11)) {
			if(admin->window->displayMode == DisplayMode::WINDOWED || admin->window->displayMode == DisplayMode::BORDERLESS){
				admin->window->UpdateDisplayMode(DisplayMode::FULLSCREEN);
			}else{
				admin->window->UpdateDisplayMode(DisplayMode::WINDOWED);
			}
		}
		
		if (DengInput->KeyPressed(Key::P | INPUTMOD_CTRL)) {
			admin->paused = !admin->paused;
		}
		
	}
	{//// camera ////
		Camera* c = admin->mainCamera;
		
		//toggle ortho
		static Vector3 ogpos;
		static Vector3 ogrot;
		if (DengInput->KeyPressed(DengKeys.perspectiveToggle)) {
			switch (c->type) {
				case(CameraType::PERSPECTIVE): {  
					ogpos = c->position;
					ogrot = c->rotation;
					c->type = CameraType::ORTHOGRAPHIC; 
					c->farZ = 1000000; 
				} break;
				case(CameraType::ORTHOGRAPHIC): { 
					c->position = ogpos; 
					c->rotation = ogrot;
					c->type = CameraType::PERSPECTIVE; 
					c->farZ = 1000; 
					c->UpdateProjectionMatrix(); 
				} break;
			}
		}
		
		//ortho views
		if (DengInput->KeyPressed(DengKeys.orthoFrontView))    c->orthoview = FRONT;
		if (DengInput->KeyPressed(DengKeys.orthoBackView))     c->orthoview = BACK;
		if (DengInput->KeyPressed(DengKeys.orthoRightView))    c->orthoview = RIGHT;
		if (DengInput->KeyPressed(DengKeys.orthoLeftView))     c->orthoview = LEFT;
		if (DengInput->KeyPressed(DengKeys.orthoTopDownView))  c->orthoview = TOPDOWN;
		if (DengInput->KeyPressed(DengKeys.orthoBottomUpView)) c->orthoview = BOTTOMUP;
	}
	{//// undo/redo ////
		UndoManager* um = &admin->undoManager;
		
		if (DengInput->KeyPressed(DengKeys.undo)) { um->Undo(); }
		if (DengInput->KeyPressed(DengKeys.redo)) { um->Redo(); }
	}
}

inline void CheckBinds(EntityAdmin* admin) {
	if (DengInput->checkbinds) {
		for (auto b : DengInput->binds) {
			if (DengInput->KeyPressed(b.second)) {
				std::string args = "";
				std::string com = b.first;
				size_t t = com.find_first_of(" ");
				if (t != std::string::npos) {
					args = com.substr(t);
					com.erase(t, com.size() - 1);
				}
				g_console->AddLog(g_console->ExecCommand(com, args));
			}
		}
		DengInput->checkbinds = false;
	}
}

void Controller::Init(EntityAdmin* a, MovementMode m){
	this->admin = a; this->mode = m;
	
	if(admin->state == GameState::PLAY || admin->state == GameState::PLAY_DEBUG){
		admin->ExecCommand("window_cursor_mode", "1");
	}
	
	AddBindings(a);
}

void Controller::Update() {
	if (!admin->IMGUI_KEY_CAPTURE) {
		CameraMovement(admin, mode);
		CameraRotation(admin, mouseSensitivity);
		CameraZoom(admin);
		if(admin->state == GameState::EDITOR){
			HandleEditorInputs(admin);
		}
		CheckBinds(admin);
	}
}
