#include "Controller.h"
#include "Camera.h"
#include "../Keybinds.h"
#include "../../EntityAdmin.h"
#include "../../core.h"
#include "../../math/Math.h"

Controller::Controller(EntityAdmin* a, MovementMode m) : Component(a), mode(m) {
	//not sure where i want this yet
	name = "Controller";
	layer = NONE;
}

inline void CameraMovement(EntityAdmin* admin, MovementMode mode) {
	Camera* camera = admin->mainCamera;
	float deltaTime = admin->time->deltaTime;
	
	if (!admin->IMGUI_KEY_CAPTURE) {
		
		if(DengInput->MouseDown(MouseButton::MB_RIGHT)){
			Vector3 inputs;
			if (mode == MOVEMENT_MODE_FLYING) {
				//translate up
				if (DengInput->KeyDown(DengKeys->movementFlyingUp)) {  inputs.y += 1;  }
				
				//translate down
				if (DengInput->KeyDown(DengKeys->movementFlyingDown)) {  inputs.y -= 1; }
			}
			
			//translate forward
			if (DengInput->KeyDown(DengKeys->movementFlyingForward)) {  inputs += camera->forward; }
			
			//translate back
			if (DengInput->KeyDown(DengKeys->movementFlyingBack)) {  inputs -= camera->forward; }
			
			//translate right
			if (DengInput->KeyDown(DengKeys->movementFlyingRight)) {  inputs += camera->right; }
			
			//translate left
			if (DengInput->KeyDown(DengKeys->movementFlyingLeft)) { inputs -= camera->right; }
			
			
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
	
	if (!admin->IMGUI_KEY_CAPTURE) {
		/*
					//camera rotation up
					if (input->KeyDown(binds->cameraRotateUp)) {
						if (input->ModDown(INPUT_SHIFT_HELD))        { camera->rotation.x -= 50 * deltaTime; }
						else if (input->ModDown(INPUT_CTRL_HELD)) { camera->rotation.x -= 5 * deltaTime; }
						else                                         { camera->rotation.x -= 25 * deltaTime; }
					}
					
					//camera rotation down
					if (input->KeyDown(binds->cameraRotateDown)) {
						if (input->ModDown(INPUT_SHIFT_HELD))        { camera->rotation.x += 50 * deltaTime; }
						else if (input->ModDown(INPUT_CTRL_HELD)) { camera->rotation.x += 5 * deltaTime; }
						else                                         { camera->rotation.x += 25 * deltaTime; }
					}
					
					//camera rotation right
					if (input->KeyDown(binds->cameraRotateRight)) {
						if (input->ModDown(INPUT_SHIFT_HELD))	    { camera->rotation.y += 50 * deltaTime; }
						else if (input->ModDown(INPUT_CTRL_HELD)) { camera->rotation.y += 5 * deltaTime; }
						else								         { camera->rotation.y += 25 * deltaTime; }
					}
					
					//camera rotation left
					if (input->KeyDown(binds->cameraRotateLeft)) {
						if (input->ModDown(INPUT_SHIFT_HELD))	    { camera->rotation.y -= 50 * deltaTime; }
						else if (input->ModDown(INPUT_CTRL_HELD)) { camera->rotation.y -= 5 * deltaTime; }
						else								         { camera->rotation.y -= 25 * deltaTime; }
					}
		*/
	}
	if(!admin->IMGUI_MOUSE_CAPTURE){
		//TODO(delle,In) change this so its dependent on game state or something (level editor vs gameplay)
		if(input->MousePressed(MouseButton::MB_RIGHT)){
			admin->ExecCommand("cursor_mode", "1");
		}
		if(input->MouseDown(MouseButton::MB_RIGHT)){
			camera->rotation.y += sens * (input->mouseX - window->centerX) * .03f;
			camera->rotation.x += sens * (input->mouseY - window->centerY) * .03f;
		}
		if(input->MouseReleased(MouseButton::MB_RIGHT)){
			admin->ExecCommand("cursor_mode", "0");
		}
	}
}

inline void CameraZoom(EntityAdmin* admin){
	Camera* cam = admin->mainCamera;
	Renderer* renderer = admin->renderer;
	
	if (DengInput->KeyPressed(Key::NUMPADPLUS)) { 
		cam->fieldOfView += 5;
		cam->UsePerspectiveProjection(cam->fieldOfView, admin->window->width, admin->window->height, cam->nearZ, cam->farZ);
	}
	if (DengInput->KeyPressed(Key::NUMPADMINUS)) {
		cam->fieldOfView -= 5;
		cam->UsePerspectiveProjection(cam->fieldOfView, admin->window->width, admin->window->height, cam->nearZ, cam->farZ);
	}
}

void HandleMouseInputs(EntityAdmin* admin) {
	Canvas* canvas = admin->tempCanvas;
	Input* input = admin->input;
	
	//mouse left click pressed
	if (input->MousePressed(MouseButton::MB_LEFT)) {
		bool ui_clicked = false;
		//check if mouse clicked on a UI element
		//if(!canvas->hideAll) {
		//	for(UIContainer* con : canvas->containers) {
		//		for(UI* ui : con->container) {
		//			if(ui->Clicked(input->mousePos)) {
		//				ui_clicked = true;
		//				goto stop;
		//			}//TODO(,delle) re-add menu dragging
		//		}
		//	}
		//}
		//stop: are we even using this anymore?
		
		//if the click wasnt on a UI element, trigger select_entity command
		//admin->ExecCommand("select_entity"); //TODO(delle,i) re-enable clicking entities
		
		//set click pos to mouse pos
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
	
	//translation
	
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
