#include "Controller.h"
#include "admin.h"
#include "components/Camera.h"
#include "components/MeshComp.h"
#include "components/Physics.h"
#include "components/Movement.h"
#include "components/Player.h"
#include "../core/assets.h"
#include "../core/imgui.h"
#include "../core/time.h"
#include "../core/window.h"
#include "../math/Math.h"
#include "../scene/Scene.h"
#include "../geometry/Edge.h"

#include <fstream>

local f32 MOUSE_SENS_FRACTION = .03f;

local bool moveOverride = false; //for moving when using arrow keys (cause i cant use mouse when remoting into my pc so)

local inline void AddBindings(Admin* admin) {
	std::ifstream binds = std::ifstream(Assets::dirConfig()+"binds.cfg", std::ios::in);
	if(binds.is_open()){
		while (!binds.eof()) {
			char* c = (char*)malloc(255);
			binds.getline(c, 255);
			std::string s(c);
			
			if (s != "") {
				std::string key = s.substr(0, s.find_first_of(" "));
				std::string command = s.substr(s.find_first_of(" ") + 1, s.length());
				
				try {
					DengInput->binds.push_back(pair<std::string, Key::Key>(command, admin->keybinds.stk.at(key)));
				}
				catch (...) {
					ERROR("Unknown key '", key, "' attempted to bind to command '", command, "'");
				}
			}
		}
	}else{
		LOG("Creating binds file..");
		Assets::writeFile(Assets::dirConfig() + "binds.cfg", "", 0);
		return;
	}
}

local inline void CameraMovement(Admin* admin, MovementMode mode) {
	Camera* camera = admin->mainCamera;
	float deltaTime = DengTime->deltaTime;
	Vector3 inputs;
	
	//most likely temporary
	if (DengInput->KeyPressed(Key::A | InputMod_Lctrl)) moveOverride = !moveOverride;
	
	//TODO(sushi, Cl) figure out a nicer way to do all these conditions like right click down, game state, and whatever else shows up in here next
	if(DengInput->KeyDownAnyMod(MouseButton::RIGHT) || moveOverride){
		if (DengInput->KeyDownAnyMod(DengKeys.movementFlyingUp))      { inputs.y += 1;  }
		if (DengInput->KeyDownAnyMod(DengKeys.movementFlyingDown))    { inputs.y -= 1; }
		if (DengInput->KeyDownAnyMod(DengKeys.movementFlyingForward)) { inputs += camera->forward; }
		if (DengInput->KeyDownAnyMod(DengKeys.movementFlyingBack))    { inputs -= camera->forward; }
		if (DengInput->KeyDownAnyMod(DengKeys.movementFlyingRight))   { inputs += camera->right; }
		if (DengInput->KeyDownAnyMod(DengKeys.movementFlyingLeft))    { inputs -= camera->right; }
		
		if     (DengInput->LShiftDown()){ camera->position += inputs.normalized() * 16 * deltaTime; }
		else if(DengInput->LCtrlDown()) { camera->position += inputs.normalized() *  4 * deltaTime; }
		else							{ camera->position += inputs.normalized() *  8 * deltaTime; }
	}
}

inline void PlayerMovement(Admin* admin, MovementMode mode, Movement* playermove) {
	Camera* camera = admin->mainCamera;
	float deltaTime = DengTime->deltaTime;
	Vector3 inputs;
	
	//TEMP CROSSHAIR
	ImGui::DebugDrawCircle(DengWindow->dimensions / 2, 2.5, Color::DARK_GREY);
	
	
	if (mode == MOVEMENT_MODE_WALKING) {
		if (DengInput->KeyDownAnyMod(DengKeys.movementWalkingForward))  { inputs += Vector3(camera->forward.x, 0, camera->forward.z); }
		if (DengInput->KeyDownAnyMod(DengKeys.movementWalkingBackward)) { inputs -= Vector3(camera->forward.x, 0, camera->forward.z); }
		if (DengInput->KeyDownAnyMod(DengKeys.movementWalkingRight))    { inputs += Vector3(camera->right.x, 0, camera->right.z); }
		if (DengInput->KeyDownAnyMod(DengKeys.movementWalkingLeft))     { inputs -= Vector3(camera->right.x, 0, camera->right.z); }
		if (DengInput->KeyPressedAnyMod(DengKeys.movementJump)
			&& !playermove->inAir)                                      { playermove->jump = true; }
		
		if (playermove && admin->player) {
			playermove->inputs = inputs.normalized();
			
			
		}else{
			ERROR_LOC("Playermovement/player pointer is null");
		}
	}
}

//NOTE sushi: this can probably be implemented somewhere else, dunno yet
local inline void PlayerGrabbing() {
	
}

local inline void CameraRotation(Admin* admin, float sens) {
	Camera* camera = admin->mainCamera;
	Keybinds* binds = &admin->keybinds;
	float deltaTime = DengTime->deltaTime;
	
	//camera rotation up
	if (DengInput->KeyDownAnyMod(binds->cameraRotateUp)) {
		if (DengInput->LShiftDown())     { camera->rotation.x -= 50 * deltaTime; }
		else if (DengInput->LCtrlDown()) { camera->rotation.x -= 5 * deltaTime; }
		else							 { camera->rotation.x -= 25 * deltaTime; }
	}
	
	//camera rotation down
	if (DengInput->KeyDownAnyMod(binds->cameraRotateDown)) {
		if (DengInput->LShiftDown())     { camera->rotation.x += 50 * deltaTime; }
		else if (DengInput->LCtrlDown()) { camera->rotation.x += 5 * deltaTime; }
		else							 { camera->rotation.x += 25 * deltaTime; }
	}
	
	//camera rotation right
	if (DengInput->KeyDownAnyMod(binds->cameraRotateRight)) {
		if (DengInput->LShiftDown())     { camera->rotation.y += 50 * deltaTime; }
		else if (DengInput->LCtrlDown()) { camera->rotation.y += 5 * deltaTime; }
		else							 { camera->rotation.y += 25 * deltaTime; }
	}
	
	//camera rotation left
	if (DengInput->KeyDownAnyMod(binds->cameraRotateLeft)) {
		if (DengInput->LShiftDown())     { camera->rotation.y -= 50 * deltaTime; }
		else if (DengInput->LCtrlDown()) { camera->rotation.y -= 5 * deltaTime; }
		else							 { camera->rotation.y -= 25 * deltaTime; }
	}
	
	if (!DengConsole->IMGUI_MOUSE_CAPTURE && !admin->controller.cameraLocked){
		if(admin->state == GameState_Play || admin->state == GameState_Debug){
			persist bool debugmouse = false;
			
			if (!debugmouse) {
				camera->rotation.y += (DengInput->mouseX - DengWindow->centerX) * sens * MOUSE_SENS_FRACTION;
				camera->rotation.x += (DengInput->mouseY - DengWindow->centerY) * sens * MOUSE_SENS_FRACTION;
			}
			
			if (DengInput->KeyPressed(Key::F3)) {
				if (debugmouse) {
					DengWindow->UpdateCursorMode(CursorMode::FIRSTPERSON);
					debugmouse = false;
				}
				else {
					DengWindow->UpdateCursorMode(CursorMode::DEFAULT);
					debugmouse = true;
				}
			}
			
		}
		else if(admin->state == GameState_Editor){
			if(DengInput->KeyPressedAnyMod(MouseButton::RIGHT)){
				DengWindow->UpdateCursorMode(CursorMode::FIRSTPERSON);
			}
			if(DengInput->KeyDownAnyMod(MouseButton::RIGHT)){
				camera->rotation.y += (DengInput->mouseX - DengWindow->centerX) * sens*MOUSE_SENS_FRACTION;
				camera->rotation.x += (DengInput->mouseY - DengWindow->centerY) * sens*MOUSE_SENS_FRACTION;
			}
			if(DengInput->KeyReleasedAnyMod(MouseButton::RIGHT)){
				DengWindow->UpdateCursorMode(CursorMode::DEFAULT);
			}
		}
	}
}

local inline void CameraZoom(Admin* admin){
	if (DengInput->KeyPressed(Key::NUMPADPLUS)) { 
		admin->mainCamera->fov += 5;
		admin->mainCamera->UpdateProjectionMatrix();
	}
	if (DengInput->KeyPressed(Key::NUMPADMINUS)) {
		admin->mainCamera->fov -= 5;
		admin->mainCamera->UpdateProjectionMatrix();
	}
}

local inline void CheckBinds(Admin* admin) {
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
				DengConsole->AddLog(DengConsole->ExecCommand(com, args));
			}
		}
		DengInput->checkbinds = false;
	}
}

void Controller::Init(Admin* a, MovementMode m){
	this->admin = a; this->mode = m;
	mouseSensitivity = 2.5f;
	playermove = 0;
	
	if(admin->state == GameState_Play || admin->state == GameState_Debug){
		DengWindow->UpdateCursorMode(CursorMode::FIRSTPERSON);
	}else{
		DengWindow->UpdateCursorMode(CursorMode::DEFAULT);
	}
	
	AddBindings(a);
}

void Controller::Update() {
	if(DengInput->KeyPressed(DengKeys.toggleConsole)) DengConsole->dispcon = !DengConsole->dispcon;
	
#if !defined(DESHI_BUILD_PLAY) && !defined(DESHI_BUILD_DEBUG)
	if (DengInput->KeyPressed(Key::F8)) {
		admin->ChangeState(GameState_Editor);
	}
#endif //if not built for playing, allow for easy exit to editor
	
	if(!ImGui::GetIO().WantCaptureKeyboard){ //&& !DengConsole->CONSOLE_KEY_CAPTURE) {
		
		if (!playermove && admin->player) {
			playermove = admin->player->GetComponent<Movement>();
		}
		
		switch(admin->state){
			case GameState_Play:{
				PlayerMovement(admin, MOVEMENT_MODE_WALKING, playermove);
				CameraRotation(admin, mouseSensitivity);
			}break;
			case GameState_Menu:{
				
			}break;
			case GameState_Debug:{
				PlayerMovement(admin, MOVEMENT_MODE_WALKING, playermove);
				CameraRotation(admin, mouseSensitivity);
			}break;
			case GameState_Editor:{
				CameraMovement(admin, MOVEMENT_MODE_FLYING);
				CameraRotation(admin, mouseSensitivity);
				CameraZoom(admin);
			}break;
		}
		CheckBinds(admin);
	}
	
}
