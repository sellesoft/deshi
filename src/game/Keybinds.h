#pragma once
#ifndef GAME_KEYBINDS_H
#define GAME_KEYBINDS_H

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
	
	//main menu bar
	Key::Key toggleMenuBar = Key::LALT | Key::TILDE;
	
	//selected object manipulation modes
	Key::Key grabSelectedObject = Key::G;
	Key::Key rotateSelectedObject = Key::R;
	Key::Key scaleSelectedObject = Key::S;
	
	private: //so VS doesnt show this in intellisense
	//mapping enum names to strings
	std::map<std::string, Key::Key> stk;
	std::map<std::string, Key::Key&> keys;
	
	
	
	//Keybinds(file ...) {} //TODO(delle,i) look into saving/loading keybinds with a file
	
	//Reads through a keybinds file and fills keybind struct with provided keys
	void ReloadKeybinds(std::fstream& kf);
	
};

#endif //GAME_KEYBINDS_H