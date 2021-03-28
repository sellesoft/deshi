#pragma once
#ifndef COMPONENT_KEYBINDS_H
#define COMPONENT_KEYBINDS_H

#include "components/Component.h"
#include "../core/input.h"


struct Keybinds : public Component {

	Keybinds(EntityAdmin* a);

	//flying movement
	Key::Key movementFlyingUp = Key::E;
	Key::Key movementFlyingDown = Key::Q;
	Key::Key movementFlyingForward = Key::W;
	Key::Key movementFlyingBack = Key::S;
	Key::Key movementFlyingRight = Key::D;
	Key::Key movementFlyingLeft = Key::A;
	
	//camera rotation
	Key::Key cameraRotateUp = Key::UP;
	Key::Key cameraRotateDown = Key::DOWN;
	Key::Key cameraRotateRight = Key::RIGHT;
	Key::Key cameraRotateLeft = Key::LEFT;
	
	//render debug
	Key::Key debugRenderEdgesNumbers = Key::COMMA;
	Key::Key debugRenderWireframe = Key::PERIOD;
	Key::Key debugRenderDisplayAxis = Key::SLASH;

	//debug menu stuff
	Key::Key toggleConsole = Key::TILDE;
	Key::Key toggleDebugMenu = Key::LCTRL | Key::TILDE;
	Key::Key toggleDebugBar = Key::LSHIFT | Key::TILDE;

private: //so VS doesnt show this in intellisense
	//mapping enum names to strings
	std::map<std::string, Key::Key> stk;
	std::map<std::string, Key::Key&> keys;
	
	
	
	//Keybinds(file ...) {} //TODO(delle,i) look into saving/loading keybinds with a file
	 
	//Reads through a keybinds file and fills keybind struct with provided keys
	void ReloadKeybinds(std::fstream& kf);
	
};

#endif //COMPONENT_KEYBINDS_H