#include "MovementState.h"
#include "../EntityAdmin.h"
#include "../core/time.h"
#include "../core/input.h"
#include "../components/Keybinds.h"
#include "../components/Camera.h"
#include "../math/Math.h"

MovementState::MovementState(EntityAdmin* a) : Component(a) {
	movementState = MOVEMENT_NOCLIP | MOVEMENT_FLYING;
	
	//not sure where i want this yet
	layer = CL0_COMMAND;
}

MovementState::MovementState(uint32 state) {
	movementState = state;
	
	layer = CL0_COMMAND;
}

void CameraMovement(EntityAdmin* admin, uint32 moveState) {
	Camera* camera = admin->currentCamera;
	Input* input = admin->input;
	Keybinds* binds = admin->currentKeybinds;
	float deltaTime = admin->time->deltaTime;
	
	if (!admin->IMGUI_KEY_CAPTURE) {
		
		Vector3 inputs;
		
		if (moveState & MOVEMENT_FLYING) {
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

void CameraRotation(EntityAdmin* admin) {
	Camera* camera = admin->currentCamera;
	Input* input = admin->input;
	Keybinds* binds = admin->currentKeybinds;
	float deltaTime = admin->time->deltaTime;
	
	if (!admin->IMGUI_KEY_CAPTURE) {
		
		if (input->KeyPressed(Key::N)) {
			camera->MOUSE_LOOK = !camera->MOUSE_LOOK;
			//admin->p->bMouseVisible = !admin->p->bMouseVisible; //TODO(i,delle) add FPS camera mode
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
			//admin->p->bMouseVisible = false; // i have to do this explicitly so the mouse is visible when ImGUI opens the console
			
			//float x = (admin->input->mousePos.x * 2) - admin->screen->width;
			//float y = (admin->input->mousePos.y * 2) - admin->screen->height;
			
			//camera->target.z = Math::clamp(camera->target.z - 0.09 * y, 1, 179);
			//camera->target.y = camera->target.y - 0.09 * x;
			
			//admin->p->SetMousePositionLocal(admin->screen->dimensions / 2);
		}
	}
	else if(admin->currentCamera->MOUSE_LOOK) {
		//admin->p->bMouseVisible = true;
	}
}

void MovementState::Update() {
	CameraMovement(admin, movementState);
	CameraRotation(admin);
}
