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
	u32 debugRenderEdgesNumbers = Key::COMMA;
	u32 debugRenderWireframe = Key::PERIOD;
	u32 debugRenderDisplayAxis = Key::SLASH;
	
	//debug menu stuff
	u32 toggleConsole = Key::TILDE;
	u32 toggleDebugMenu = Key::LCTRL | Key::TILDE;
	u32 toggleDebugBar = Key::LSHIFT | Key::TILDE;
	
	//main menu bar
	u32 toggleMenuBar = Key::LALT | Key::TILDE;
	
	//selected object manipulation modes
	u32 grabSelectedObject = Key::G;
	u32 rotateSelectedObject = Key::R;
	u32 scaleSelectedObject = Key::S;

	//orthographic view controls
	u32 orthoOffset = MouseButton::MIDDLE;
	u32 orthoZoomIn = MouseButton::SCROLLUP;
	u32 orthoZoomOut = MouseButton::SCROLLDOWN;
	u32 orthoResetOffset = Key::NUMPADPERIOD;

	u32 undo = Key::LCTRL | Key::Z;
	u32 redo = Key::LCTRL | Key::Y;

	//mapping enum names to strings
	std::map<std::string, Key::Key> stk;
	std::map<std::string, Key::Key&> keys;

	//Reads through a keybinds file and fills keybind struct with provided keys
	void ReloadKeybinds(std::fstream& kf);
	
};

#endif //GAME_KEYBINDS_H