#pragma once
#include "Component.h"

#include "../core/deshi_input.h"

struct Keybinds : public Component {
	//flying movement
	Key::Key movementFlyingUp;
	Key::Key movementFlyingDown;
	Key::Key movementFlyingForward;
	Key::Key movementFlyingBack;
	Key::Key movementFlyingRight;
	Key::Key movementFlyingLeft;

	//camera rotation
	Key::Key cameraRotateUp;
	Key::Key cameraRotateDown;
	Key::Key cameraRotateRight;
	Key::Key cameraRotateLeft;

	//render debug
	Key::Key debugRenderEdgesNumbers;
	Key::Key debugRenderWireframe;
	Key::Key debugRenderDisplayAxis;

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