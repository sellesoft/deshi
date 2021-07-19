#pragma once
#ifndef GAME_KEYBINDS_H
#define GAME_KEYBINDS_H

#include "../core/input.h"
#include "../core/assets.h"

struct Keybinds{
	//flying movement
	Key::Key movementFlyingUp      = Key::E;
	Key::Key movementFlyingDown    = Key::Q;
	Key::Key movementFlyingForward = Key::W;
	Key::Key movementFlyingBack    = Key::S;
	Key::Key movementFlyingRight   = Key::D;
	Key::Key movementFlyingLeft    = Key::A;
	
	//walking movement
	Key::Key movementWalkingForward  = Key::W;
	Key::Key movementWalkingBackward = Key::S;
	Key::Key movementWalkingRight    = Key::D;
	Key::Key movementWalkingLeft     = Key::A;
	Key::Key movementJump            = Key::SPACE;
	Key::Key movementCrouch          = Key::LCTRL;
	Key::Key movementRun             = Key::LSHIFT;
	
	//player controls
	Key::Key use = Key::E;
	
	//camera controls
	Key::Key cameraRotateUp    = Key::UP;
	Key::Key cameraRotateDown  = Key::DOWN;
	Key::Key cameraRotateRight = Key::RIGHT;
	Key::Key cameraRotateLeft  = Key::LEFT;
	Key::Key orthoOffset       = MouseButton::MIDDLE;
	Key::Key orthoZoomIn       = MouseButton::SCROLLUP;
	Key::Key orthoZoomOut      = MouseButton::SCROLLDOWN;
	Key::Key orthoResetOffset  = Key::NUMPADPERIOD;
	Key::Key orthoRightView    = Key::NUMPAD6;
	Key::Key orthoLeftView     = Key::NUMPAD4;
	Key::Key orthoFrontView    = Key::NUMPAD8;
	Key::Key orthoBackView     = Key::NUMPAD2;
	Key::Key orthoTopDownView  = Key::NUMPAD1;
	Key::Key orthoBottomUpView = Key::NUMPAD3;
	Key::Key perspectiveToggle = Key::NUMPAD0;
	Key::Key gotoSelected      = Key::NUMPADENTER;
	
	//debug menu stuff
	Key::Key toggleConsole   = Key::TILDE;
	Key::Key toggleDebugMenu = Key::TILDE | InputMod_Lctrl;
	Key::Key toggleDebugBar  = Key::TILDE | InputMod_Lshift;
	
	//main menu bar
	Key::Key toggleMenuBar = Key::TILDE | Key::LALT;
	
	//selected object manipulation modes
	Key::Key grabSelectedObject   = Key::G;
	Key::Key rotateSelectedObject = Key::R;
	Key::Key scaleSelectedObject  = Key::S;
	
	Key::Key undo  = Key::Z | InputMod_Lctrl;
	Key::Key redo  = Key::Y | InputMod_Lctrl;
	Key::Key cut   = Key::X | InputMod_Lctrl;
	Key::Key copy  = Key::C | InputMod_Lctrl;
	Key::Key paste = Key::V | InputMod_Lctrl;
	
	//mapping enum names to strings
	std::map<std::string, Key::Key>  stk;
	std::map<std::string, Key::Key&> keys;
	
	ConfigMap keybindMap;
	
	void init();
	void save();
	void load();
};

inline void Keybinds::init(){
	keybindMap = {
		{"#keybinds config file",0,0},
		{"\n#movement",ConfigValueType_PADSECTION,(void*)24},
		{"movement_flying_up",      ConfigValueType_Key, &movementFlyingUp},
		{"movement_flying_down",    ConfigValueType_Key, &movementFlyingDown},
		{"movement_flying_forward", ConfigValueType_Key, &movementFlyingForward},
		{"movement_flying_back",    ConfigValueType_Key, &movementFlyingBack},
		{"movement_flying_left",    ConfigValueType_Key, &movementFlyingRight},
		{"movement_flying_right",   ConfigValueType_Key, &movementFlyingLeft},
		{"\n#camera",ConfigValueType_PADSECTION,(void*)27},
		{"camera_rotate_up",           ConfigValueType_Key, &cameraRotateUp},
		{"camera_rotate_down",         ConfigValueType_Key, &cameraRotateDown},
		{"camera_rotate_left",         ConfigValueType_Key, &cameraRotateLeft},
		{"camera_rotate_right",        ConfigValueType_Key, &cameraRotateRight},
		{"camera_perspective_toggle",  ConfigValueType_Key, &perspectiveToggle},
		{"camera_goto_selected",       ConfigValueType_Key, &gotoSelected},
		{"camera_ortho_offset",        ConfigValueType_Key, &orthoOffset},
		{"camera_ortho_reset_offset",  ConfigValueType_Key, &orthoResetOffset},
		{"camera_ortho_zoom_in",       ConfigValueType_Key, &orthoZoomIn},
		{"camera_ortho_zoom_out",      ConfigValueType_Key, &orthoZoomOut},
		{"camera_ortho_right_view",    ConfigValueType_Key, &orthoRightView},
		{"camera_ortho_left_view",     ConfigValueType_Key, &orthoLeftView},
		{"camera_ortho_front_view",    ConfigValueType_Key, &orthoFrontView},
		{"camera_ortho_back_view",     ConfigValueType_Key, &orthoBackView},
		{"camera_ortho_topdown_view",  ConfigValueType_Key, &orthoTopDownView},
		{"camera_ortho_bottomup_view", ConfigValueType_Key, &orthoBottomUpView},
		{"\n#editor",ConfigValueType_PADSECTION,(void*)26},
		{"editor_undo",               ConfigValueType_Key, &undo},
		{"editor_redo",               ConfigValueType_Key, &redo},
		{"editor_translate_selected", ConfigValueType_Key, &grabSelectedObject},
		{"editor_rotate_selected",    ConfigValueType_Key, &rotateSelectedObject},
		{"editor_scale_selected",     ConfigValueType_Key, &scaleSelectedObject},
		{"\n#gui",ConfigValueType_PADSECTION,(void*)21},
		{"gui_console_toggle",   ConfigValueType_Key, &toggleConsole},
		{"gui_debugmenu_toggle", ConfigValueType_Key, &toggleDebugMenu},
		{"gui_debugbar_toggle",  ConfigValueType_Key, &toggleDebugBar},
		{"gui_menubar_toggle",   ConfigValueType_Key, &toggleMenuBar},
	};
	
	load();
}

inline void Keybinds::save(){
	Assets::saveConfig("keybinds.cfg", keybindMap);
}

inline void Keybinds::load(){
	Assets::loadConfig("keybinds.cfg", keybindMap);
}

#endif //GAME_KEYBINDS_H