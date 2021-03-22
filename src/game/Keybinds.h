#pragma once
#ifndef COMPONENT_KEYBINDS_H
#define COMPONENT_KEYBINDS_H

#include "components/Component.h"
#include "../core/input.h"

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
	
	//mapping enum names to strings
	std::map<std::string, Key::Key> stk;
	
	//storing key vars for ez assigning 
	std::vector<Key::Key*> keys;
	
	Keybinds(EntityAdmin* a);
	
	//Keybinds(file ...) {} //TODO(delle,i) look into saving/loading keybinds with a file
	
};

#endif //COMPONENT_KEYBINDS_H