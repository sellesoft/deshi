#pragma once
#ifndef COMPONENT_KEYBINDS_H
#define COMPONENT_KEYBINDS_H

#include "components/Component.h"
#include "../core/input.h"


struct Keybinds : public Component {

	Keybinds(EntityAdmin* a);

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

	//debug menu stuff
	Key::Key toggleConsole;
	Key::Key toggleDebugMenu;
	Key::Key toggleDebugBar;

private: //so VS doesnt show this in intellisense
	//mapping enum names to strings
	std::map<std::string, Key::Key> stk;
	std::map<std::string, Key::Key&> keys;
	
	
	
	//Keybinds(file ...) {} //TODO(delle,i) look into saving/loading keybinds with a file
	
};

#endif //COMPONENT_KEYBINDS_H