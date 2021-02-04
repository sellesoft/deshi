#pragma once
#include "deshi_defines.h"

#include <map>

//constants
#define MAX_KEYBOARD_KEYS 256
#define MAX_MOUSE_BUTTONS 5

enum Key{
	NONE,
	A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
	K0, K1, K2, K3, K4, K5, K6, K7, K8, K9,
	F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
	UP, DOWN, LEFT, RIGHT,
	ESCAPE, TILDE, TAB, CAPSLOCK, LSHIFT, LCONTROL, LALT,
	BACKSPACE, ENTER, RSHIFT, RCONTROL, RALT, MINUS, EQUALS, LBRACKET, RBRACKET, 
	SLASH, SEMICOLON, APOSTROPHE, COMMA, PERIOD, BACKSLASH, SPACE,
	INSERT, DELETE, HOME, END, PAGEUP, PAGEDOWN, PAUSE, SCROLL,
	NUMPAD0, NUMPAD1, NUMPAD2, NUMPAD3, NUMPAD4, NUMPAD5, NUMPAD6, NUMPAD7, NUMPAD8, NUMPAD9,
	NUMPADMULTIPLY, NUMPADDIVIDE, NUMPADPLUS, NUMPADMINUS, NUMPADPERIOD, NUMPADENTER, NUMLOCK
};

enum MouseButton{
	MB_LEFT, MB_RIGHT, MB_MIDDLE, MB_FOUR, MB_FIVE
};

enum InputState{
	RELEASED, PRESSED, HELD
};

enum InputMod{
	NONE_HELD  = 0x0,
	ANY_HELD   = 0x1,
	CTRL_HELD  = 0x2,
	SHIFT_HELD = 0x3,
	ALT_HELD   = 0x4
};

struct Input{
	std::map<size_t, uint8> mapKeys;
	std::map<size_t, uint8> mapMouse;
	
	//TODO(oi,delle) look into storing these as vector<bool> instead
	//TODO(oi,delle) look into storing input modifiers with the keys
	bool oldKeyState[MAX_KEYBOARD_KEYS] = {0};
	bool newKeyState[MAX_KEYBOARD_KEYS] = {0};
	bool oldMouseState[MAX_MOUSE_BUTTONS] = {0};
	bool newMouseState[MAX_MOUSE_BUTTONS] = {0};
	
	int32 mouse_x, mouse_y;
	bool keyFocus, mouseFocus;
	
	void UpdateKeyState(int32 key, bool state){
		newKeyState[key] = state;
	}
	
	void UpdateMouseState(int32 button, bool state){
		newMouseState[button] = state;
	}
	
	//TODO(i,delle) look into safeguarding mouse input, see: PixelGameEngine::olc_UpdateMouse
	void UpdateMousePosition(int32 x, int32 y){
		mouseFocus = true;
		mouse_x = x;
		mouse_y = y;
	}
};
