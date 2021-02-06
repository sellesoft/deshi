#pragma once
#include "Component.h"

#include "Input.h"

struct Keybinds : public Component {
	//flying movement
	Key movementFlyingUp;
	Key movementFlyingDown;
	Key movementFlyingForward;
	Key movementFlyingBack;
	Key movementFlyingRight;
	Key movementFlyingLeft;

	//camera rotation
	Key cameraRotateUp;
	Key cameraRotateDown;
	Key cameraRotateRight;
	Key cameraRotateLeft;

	//render debug
	Key debugRenderWireframe;
	Key debugRenderEdgesNumbers;
	Key debugRenderDisplayAxis;

	Keybinds() {
		movementFlyingUp =		Key::E;
		movementFlyingDown =	Key::Q;
		movementFlyingForward =	Key::W;
		movementFlyingBack =	Key::S;
		movementFlyingRight =	Key::D;
		movementFlyingLeft =	Key::A;
		
		cameraRotateUp =	Key::UP;
		cameraRotateDown =	Key::DOWN;
		cameraRotateRight =	Key::RIGHT;
		cameraRotateLeft =	Key::LEFT;

		debugRenderWireframe =	  Key::COMMA;
		debugRenderEdgesNumbers = Key::PERIOD;
		debugRenderDisplayAxis =  Key::SLASH;
	}

	//Keybinds(file ...) {} //TODO(i,delle) look into saving/loading keybinds with a file

};