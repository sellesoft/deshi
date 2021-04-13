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
	
	//camera controls
	Key::Key cameraRotateUp = Key::UP;
	Key::Key cameraRotateDown = Key::DOWN;
	Key::Key cameraRotateRight = Key::RIGHT;
	Key::Key cameraRotateLeft = Key::LEFT;
	Key::Key orthoOffset = MouseButton::MIDDLE;
	Key::Key orthoZoomIn = MouseButton::SCROLLUP;
	Key::Key orthoZoomOut = MouseButton::SCROLLDOWN;
	Key::Key orthoResetOffset = Key::NUMPADPERIOD;
	Key::Key orthoRightView = Key::NUMPAD6;
	Key::Key orthoLeftView = Key::NUMPAD4;
	Key::Key orthoFrontView = Key::NUMPAD8;
	Key::Key orthoBackView = Key::NUMPAD2;
	Key::Key orthoTopDownView = Key::NUMPAD1;
	Key::Key orthoBottomUpView = Key::NUMPAD3;
	Key::Key perspectiveToggle = Key::NUMPAD0;


	
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
	

	Key::Key undo = Key::LCTRL | Key::Z;
	Key::Key redo = Key::LCTRL | Key::Y;

	//mapping enum names to strings
	std::map<std::string, Key::Key> stk;
	std::map<std::string, Key::Key&> keys;

	//Reads through a keybinds file and fills keybind struct with provided keys
	void ReloadKeybinds(std::fstream& kf);
	
};

#endif //GAME_KEYBINDS_H