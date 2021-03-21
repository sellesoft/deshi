#include "Controller.h"
#include "../EntityAdmin.h"
#include "../core/time.h"
#include "../core/input.h"
#include "../core/window.h"
#include "../components/Keybinds.h"
#include "../components/Camera.h"
#include "../math/Math.h"

Controller::Controller(EntityAdmin* a, MovementMode m) : Component(a), mode(m) {
	//not sure where i want this yet
	layer = CL0_COMMAND;
}

inline void CameraMovement(EntityAdmin* admin, MovementMode mode) {
	Camera* camera = admin->currentCamera;
	Input* input = admin->input;
	Keybinds* binds = admin->currentKeybinds;
	float deltaTime = admin->time->deltaTime;
	
	if (!admin->IMGUI_KEY_CAPTURE) {
		
		if(input->MouseDown(MouseButton::MB_RIGHT)){
			Vector3 inputs;
			if (mode == MOVEMENT_MODE_FLYING) {
				//translate up
				if (input->KeyDown(binds->movementFlyingUp)) {  inputs.y += 1;  }
				
				//translate down
				if (input->KeyDown(binds->movementFlyingDown)) {  inputs.y -= 1; }
			}
			
			//translate forward
			if (input->KeyDown(binds->movementFlyingForward)) {  inputs += camera->forward; }
			
			//translate back
			if (input->KeyDown(binds->movementFlyingBack)) {  inputs -= camera->forward; }
			
			//translate right
			if (input->KeyDown(binds->movementFlyingRight)) {  inputs += camera->right; }
			
			//translate left
			if (input->KeyDown(binds->movementFlyingLeft)) { inputs -= camera->right; }
			
			
			if (input->ModDown(INPUT_SHIFT_HELD))     { camera->position += inputs.normalized() * 16 * deltaTime; }
			else if (input->ModDown(INPUT_CONTROL_HELD)) { camera->position += inputs.normalized() *  4 * deltaTime; }
			else								{ camera->position += inputs.normalized() *  8 * deltaTime; }
		}
	}
}

inline void CameraRotation(EntityAdmin* admin, float sens) {
	Camera* camera = admin->currentCamera;
	Input* input = admin->input;
	Keybinds* binds = admin->currentKeybinds;
	Window* window = admin->window;
	float deltaTime = admin->time->deltaTime;
	
	if (!admin->IMGUI_MOUSE_CAPTURE) {
		
		/*if (input->KeyPressed(Key::N)) {
			camera->MOUSE_LOOK = !camera->MOUSE_LOOK;
			admin->ExecCommand("cursor_mode", camera->MOUSE_LOOK ? "1" : "0");
		}
		if (!admin->currentCamera->MOUSE_LOOK) {
			//camera rotation up
			if (input->KeyDown(binds->cameraRotateUp)) {
				if (input->ModDown(INPUT_SHIFT_HELD))        { camera->rotation.x -= 50 * deltaTime; }
				else if (input->ModDown(INPUT_CONTROL_HELD)) { camera->rotation.x -= 5 * deltaTime; }
				else                                         { camera->rotation.x -= 25 * deltaTime; }
			}
			
			//camera rotation down
			if (input->KeyDown(binds->cameraRotateDown)) {
				if (input->ModDown(INPUT_SHIFT_HELD))        { camera->rotation.x += 50 * deltaTime; }
				else if (input->ModDown(INPUT_CONTROL_HELD)) { camera->rotation.x += 5 * deltaTime; }
				else                                         { camera->rotation.x += 25 * deltaTime; }
			}
			
			//camera rotation right
			if (input->KeyDown(binds->cameraRotateRight)) {
				if (input->ModDown(INPUT_SHIFT_HELD))	    { camera->rotation.y += 50 * deltaTime; }
				else if (input->ModDown(INPUT_CONTROL_HELD)) { camera->rotation.y += 5 * deltaTime; }
				else								         { camera->rotation.y += 25 * deltaTime; }
			}
			
			//camera rotation left
			if (input->KeyDown(binds->cameraRotateLeft)) {
				if (input->ModDown(INPUT_SHIFT_HELD))	    { camera->rotation.y -= 50 * deltaTime; }
				else if (input->ModDown(INPUT_CONTROL_HELD)) { camera->rotation.y -= 5 * deltaTime; }
				else								         { camera->rotation.y -= 25 * deltaTime; }
			}
		}
		else {
			camera->rotation.y += sens * (input->mouseX - window->centerX) * .01f;
			camera->rotation.x += sens * (input->mouseY - window->centerY) * .01f;
		}*/
		
		//TODO(i,delle) change this so its dependent on game state or something (level editor vs gameplay)
		if(input->MousePressed(MouseButton::MB_RIGHT)){
			admin->ExecCommand("cursor_mode", "1");
		}
		if(input->MouseDown(MouseButton::MB_RIGHT)){
			camera->rotation.y += sens * (input->mouseX - window->centerX) * .005f;
			camera->rotation.x += sens * (input->mouseY - window->centerY) * .005f;
		}
		if(input->MouseReleased(MouseButton::MB_RIGHT)){
			admin->ExecCommand("cursor_mode", "0");
		}
	}
}

void Controller::Update() {
	CameraMovement(admin, mode);
	CameraRotation(admin, mouseSensitivity);
}
