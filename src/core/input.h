#pragma once
#ifndef DESHI_INPUT_H
#define DESHI_INPUT_H

#include "../utils/defines.h"
#include "../utils/debug.h"
#include "../math/Vector.h"

#include <map>
#include <iostream>

struct Entity;

//constants
#define MAX_KEYBOARD_KEYS 256
#define MAX_MOUSE_BUTTONS 5

namespace Key {
	typedef enum KeyBits {
		NONE,
		A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
		K0, K1, K2, K3, K4, K5, K6, K7, K8, K9,
		F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
		UP, DOWN, LEFT, RIGHT,
		ESCAPE, TILDE, TAB, CAPSLOCK, LSHIFT, LCTRL, LALT,
		BACKSPACE, ENTER, RSHIFT, RCTRL, RALT, MINUS, EQUALS, LBRACKET, RBRACKET,
		SLASH, SEMICOLON, APOSTROPHE, COMMA, PERIOD, BACKSLASH, SPACE,
		INSERT, DELETE, HOME, END, PAGEUP, PAGEDOWN, PAUSE, SCROLL,
		NUMPAD0, NUMPAD1, NUMPAD2, NUMPAD3, NUMPAD4, NUMPAD5, NUMPAD6, NUMPAD7, NUMPAD8, NUMPAD9,
		NUMPADMULTIPLY, NUMPADDIVIDE, NUMPADPLUS, NUMPADMINUS, NUMPADPERIOD, NUMPADENTER, NUMLOCK
	} KeyBits;
	typedef u32 Key;
};

namespace MouseButton{
	typedef enum MouseButtonBits{
		MB_LEFT, MB_RIGHT, MB_MIDDLE, MB_FOUR, MB_FIVE
	} MouseButtonBits;
	typedef u32 MouseButton;
}

//TODO(sushi, In) add right and left differenciation and a middle term for both
typedef enum InputModFlagBits{
	INPUTMOD_NONE          = 0,
	INPUTMOD_ANY           = 256,
	INPUTMOD_CTRL          = 512,
	INPUTMOD_SHIFT         = 1024,
	INPUTMOD_ALT           = 2048,
	INPUTMOD_CTRLSHIFT     = 1536,
	INPUTMOD_CTRLALT       = 2560,
	INPUTMOD_SHIFTALT      = 3072,
	INPUTMOD_CTRLSHIFTALT  = 3584,
} InputModFlagBits;
typedef u32 InputModFlags;

struct Input{
	Entity* selectedEntity = nullptr; //TODO(delle,InCl) move this to level editor
	
	std::map<size_t, u8> mapKeys;
	std::map<size_t, u8> mapMouse;
	
	//TODO(delle,OpIn) look into storing these as vector<bool> instead
	bool oldKeyState[MAX_KEYBOARD_KEYS]   = {0};
	bool newKeyState[MAX_KEYBOARD_KEYS]   = {0};
	bool oldMouseState[MAX_MOUSE_BUTTONS] = {0};
	bool newMouseState[MAX_MOUSE_BUTTONS] = {0};
	double mouseX,       mouseY;
	double screenMouseX, screenMouseY;
	double scrollX,      scrollY;
	Vector2 mousePos;
	
	//real values are updated through GLFW callbacks
	bool realKeyState[MAX_KEYBOARD_KEYS]   = {0};
	bool realMouseState[MAX_MOUSE_BUTTONS] = {0};
	double realMouseX,       realMouseY;
	double realScreenMouseX, realScreenMouseY;
	double realScrollX,      realScrollY;
	bool keyFocus, mouseFocus;
	
	//caches values so they are consistent thru the frame
	void Update(){
		memcpy(&oldKeyState, &newKeyState, sizeof(bool) * MAX_KEYBOARD_KEYS);
		memcpy(&newKeyState, &realKeyState, sizeof(bool) * MAX_KEYBOARD_KEYS);
		memcpy(&oldMouseState, &newMouseState, sizeof(bool) * MAX_MOUSE_BUTTONS);
		memcpy(&newMouseState, &realMouseState, sizeof(bool) * MAX_MOUSE_BUTTONS);
		//mouseX = realMouseX; mouseY = realMouseY; //NOTE this doesnt work, idk why
		mousePos.x = mouseX; mousePos.y = mouseY;
		screenMouseY = realScreenMouseX; screenMouseY = realScreenMouseY;
		scrollX = realScrollX; scrollY = realScrollY;
	}
	
	/////////////////////////////////
	//// input helper functions /////
	/////////////////////////////////
	
	inline bool CtrlDown(){ return newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL]; }
	inline bool ShiftDown(){ return newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT]; }
	inline bool AltDown(){ return newKeyState[Key::LALT] || newKeyState[Key::RALT]; }
	
	bool ModsDown(u32 mods){
		if     (!mods)/*INPUTMOD_NONE*/      { return !CtrlDown() && !ShiftDown() && !AltDown(); }
		else if(mods & INPUTMOD_ANY)         { return true; }
		else if(mods & INPUTMOD_CTRL)        { return  CtrlDown() && !ShiftDown() && !AltDown(); }
		else if(mods & INPUTMOD_SHIFT)       { return !CtrlDown() &&  ShiftDown() && !AltDown(); }
		else if(mods & INPUTMOD_ALT)         { return !CtrlDown() && !ShiftDown() &&  AltDown(); }
		else if(mods & INPUTMOD_CTRLSHIFT)   { return  CtrlDown() &&  ShiftDown() && !AltDown(); }
		else if(mods & INPUTMOD_CTRLALT)     { return  CtrlDown() && !ShiftDown() &&  AltDown(); }
		else if(mods & INPUTMOD_SHIFTALT)    { return !CtrlDown() &&  ShiftDown() &&  AltDown(); }
		else if(mods & INPUTMOD_CTRLSHIFTALT){ return  CtrlDown() &&  ShiftDown() &&  AltDown(); }
		else{ PRINT("[ERROR] ModsDown called with invalid mod: "<<mods<<"; defaulting to any mod"); return true; }
	}
	
	/////////////////////////////
	//// keyboard keys input ////
	/////////////////////////////
	
	//mod_key & 0x000000FF extract the key
	//mod_key & 0xFFFFFF00 extract the mods
	
	inline bool KeyDownAnyMod(u32 mod_key){ return newKeyState[mod_key & 0x000000FF]; }
	
	inline bool KeyDown(u32 mod_key) { 
		u32 key = mod_key & 0x000000FF;
		return newKeyState[key] && ModsDown(mod_key & 0xFFFFFF00); 
	}
	
	inline bool KeyUp(u32 mod_key) {
		u32 key = mod_key & 0x000000FF;
		return newKeyState[key] && ModsDown(mod_key & 0xFFFFFF00);
	}
	
	inline bool KeyPressed(u32 mod_key) {
		u32 key = mod_key & 0x000000FF;
		return newKeyState[key] && !oldKeyState[key] && ModsDown(mod_key & 0xFFFFFF00);
	}
	
	inline bool KeyReleased(u32 mod_key) {
		u32 key = mod_key & 0x000000FF;
		return !newKeyState[key] && oldKeyState[key] && ModsDown(mod_key & 0xFFFFFF00);
	}
	
	/////////////////////////////
	//// mouse buttons input ////
	/////////////////////////////
	
	inline bool MouseDownAnyMod(u32 mod_but){ return newMouseState[mod_but & 0x000000FF]; }
	
	inline bool MouseDown(u32 mod_but) {
		u32 button = mod_but & 0x000000FF;
		return newMouseState[button] && ModsDown(mod_but & 0xFFFFFF00);
	}
	
	inline bool MouseUp(u32 mod_but) {
		u32 button = mod_but & 0x000000FF;
		return newMouseState[button] && ModsDown(mod_but & 0xFFFFFF00);
	}
	
	inline bool MousePressed(u32 mod_but) {
		u32 button = mod_but & 0x000000FF;
		return newMouseState[button] && !oldMouseState[button] && ModsDown(mod_but & 0xFFFFFF00);
	}
	
	inline bool MouseReleased(u32 mod_but) {
		u32 button = mod_but & 0x000000FF;
		return !newMouseState[button] && oldMouseState[button] && ModsDown(mod_but & 0xFFFFFF00);
	}
};

#endif //DESHI_INPUT_H