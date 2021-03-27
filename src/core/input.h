#pragma once
#ifndef DESHI_INPUT_H
#define DESHI_INPUT_H

#include "../utils/defines.h"
#include "../math/Vector2.h"
#include "../utils/Debug.h"

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


enum MouseButton{
	MB_LEFT, MB_RIGHT, MB_MIDDLE, MB_FOUR, MB_FIVE
};

//TODO(sushi, In) add right and left differenciation and a middle term for both
typedef enum InputModFlagBits{
	INPUT_ANY_HELD     = 512,
	INPUT_NONE_HELD    = 1024,
	INPUT_CTRL_HELD = 2048,
	INPUT_SHIFT_HELD   = 4096,
	INPUT_ALT_HELD     = 8192
} InputModFlagBits;
typedef u32 InputModFlags;

struct Input{
	Entity* selectedEntity = nullptr;
	
	std::map<size_t, u8> mapKeys;
	std::map<size_t, u8> mapMouse;
	
	//TODO(delle,OpIn) look into storing these as vector<bool> instead
	//TODO(delle,OpIn) look into storing input modifiers with the keys as a bit combined int
	bool oldKeyState[MAX_KEYBOARD_KEYS]   = {0};
	bool newKeyState[MAX_KEYBOARD_KEYS]   = {0};
	bool oldMouseState[MAX_MOUSE_BUTTONS] = {0};
	bool newMouseState[MAX_MOUSE_BUTTONS] = {0};
	double mouseX, mouseY;
	double screenMouseX, screenMouseY;
	Vector2 mousePos;
	double scrollX, scrollY;
	
	//real values are updated through GLFW callbacks
	bool realKeyState[MAX_KEYBOARD_KEYS]   = {0};
	bool realMouseState[MAX_MOUSE_BUTTONS] = {0};
	double realMouseX, realMouseY;
	double realScreenMouseX, realScreenMouseY;
	double realScrollX, realScrollY;
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
	
	/////////////////////////////
	//// keyboard keys input ////
	/////////////////////////////
	
	inline bool KeyDown(Key::Key key) {

		//so readable
		//TODO(sushi, Cl) redo all of this as a single function when not feelin' lazy
		if (key > 100) {
			if (key & INPUT_NONE_HELD) {
				key = key ^ INPUT_NONE_HELD;
				return KeyDown(key, INPUT_NONE_HELD);
			}
			else if (key & INPUT_ANY_HELD) {
				key = key ^ INPUT_ANY_HELD;
				return KeyDown(key, INPUT_ANY_HELD);
			}
			else if (key & INPUT_CTRL_HELD) {
				key = key ^ INPUT_CTRL_HELD;
				return KeyDown(key, INPUT_CTRL_HELD);
			}
			else if (key & INPUT_SHIFT_HELD) {
				key = key ^ INPUT_SHIFT_HELD;
				return KeyDown(key, INPUT_SHIFT_HELD);
			}
			else if (key & INPUT_ALT_HELD) {
				key = key ^ INPUT_ALT_HELD;
				return KeyDown(key, INPUT_ALT_HELD);
			}
			else if (key & (INPUT_CTRL_HELD | INPUT_SHIFT_HELD)) {
				key = key ^ (INPUT_CTRL_HELD | INPUT_SHIFT_HELD);
				return KeyDown(key, INPUT_CTRL_HELD | INPUT_SHIFT_HELD);
			}
			else if (key & (INPUT_CTRL_HELD | INPUT_ALT_HELD)) {
				key = key ^ (INPUT_CTRL_HELD | INPUT_ALT_HELD);
				return KeyDown(key, INPUT_CTRL_HELD | INPUT_ALT_HELD);
			}
			else if (key & (INPUT_ALT_HELD | INPUT_SHIFT_HELD)) {
				key = key ^ (INPUT_SHIFT_HELD | INPUT_ALT_HELD);
				return KeyDown(key, INPUT_ALT_HELD | INPUT_SHIFT_HELD);
			}
			else if (key & (INPUT_CTRL_HELD | INPUT_SHIFT_HELD | INPUT_ALT_HELD)) {
				key = key ^ (INPUT_CTRL_HELD | INPUT_ALT_HELD | INPUT_SHIFT_HELD);
				return KeyDown(key, INPUT_CTRL_HELD | INPUT_SHIFT_HELD | INPUT_ALT_HELD);
			}
			else {
				//uh
			}
		}

		return newKeyState[key];
	}
	
	bool KeyDown(Key::Key key, InputModFlags mod) {
		switch (mod) {
			case(INPUT_ANY_HELD): {
				return newKeyState[key];
			}
			case(INPUT_CTRL_HELD): {
				return newKeyState[key]
					&& (newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL])
					&& !(newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT]
						 || newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_SHIFT_HELD): {
				return newKeyState[key]
					&& (newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT])
					&& !(newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL]
						 || newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_ALT_HELD): {
				return newKeyState[key]
					&& (newKeyState[Key::LALT] || newKeyState[Key::RALT])
					&& !(newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT]
						 || newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL]);
			}
			case(INPUT_CTRL_HELD | INPUT_SHIFT_HELD): {
				return newKeyState[key]
					&& (newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL])
					&& (newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT])
					&& !(newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case((INPUT_CTRL_HELD | INPUT_ALT_HELD)): {
				return newKeyState[key]
					&& (newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL])
					&& !(newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT])
					&& (newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_ALT_HELD | INPUT_SHIFT_HELD): {
				return newKeyState[key]
					&& !(newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL])
					&& (newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT])
					&& (newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_CTRL_HELD | INPUT_SHIFT_HELD | INPUT_ALT_HELD): {
				return newKeyState[key]
					&& (newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL])
					&& (newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT])
					&& (newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_NONE_HELD):default: {
				
				return newKeyState[key]
					&& !(newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL]
						 || newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT]
						 || newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
		}
	}
	
	inline bool KeyPressed(Key::Key key) {
		if (key > 100) {
			if (key & INPUT_NONE_HELD) {
				key = key ^ INPUT_NONE_HELD;
				return KeyPressed(key, INPUT_NONE_HELD);
			}
			else if (key & INPUT_ANY_HELD) {
				key = key ^ INPUT_ANY_HELD;
				return KeyPressed(key, INPUT_ANY_HELD);
			}
			else if (key & INPUT_CTRL_HELD) {
				key = key ^ INPUT_CTRL_HELD;
				return KeyPressed(key, INPUT_CTRL_HELD);
			}
			else if (key & INPUT_SHIFT_HELD) {
				key = key ^ INPUT_SHIFT_HELD;
				return KeyPressed(key, INPUT_SHIFT_HELD);
			}
			else if (key & INPUT_ALT_HELD) {
				key = key ^ INPUT_ALT_HELD;
				return KeyPressed(key, INPUT_ALT_HELD);
			}
			else if (key & (INPUT_CTRL_HELD | INPUT_SHIFT_HELD)) {
				key = key ^ (INPUT_CTRL_HELD | INPUT_SHIFT_HELD);
				return KeyPressed(key, INPUT_CTRL_HELD | INPUT_SHIFT_HELD);
			}
			else if (key & (INPUT_CTRL_HELD | INPUT_ALT_HELD)) {
				key = key ^ (INPUT_CTRL_HELD | INPUT_ALT_HELD);
				return KeyPressed(key, INPUT_CTRL_HELD | INPUT_ALT_HELD);
			}
			else if (key & (INPUT_ALT_HELD | INPUT_SHIFT_HELD)) {
				key = key ^ (INPUT_SHIFT_HELD | INPUT_ALT_HELD);
				return KeyPressed(key, INPUT_ALT_HELD | INPUT_SHIFT_HELD);
			}
			else if (key & (INPUT_CTRL_HELD | INPUT_SHIFT_HELD | INPUT_ALT_HELD)) {
				key = key ^ (INPUT_CTRL_HELD | INPUT_ALT_HELD | INPUT_SHIFT_HELD);
				return KeyPressed(key, INPUT_CTRL_HELD | INPUT_SHIFT_HELD | INPUT_ALT_HELD);
			}
			else {
				//uh
			}
		}
		return newKeyState[key] && !oldKeyState[key];
	}
	
	bool KeyPressed(Key::Key key, InputModFlags mod) {
		switch (mod) {
			case(INPUT_ANY_HELD): {
				return newKeyState[key] && !oldKeyState[key];
			}
			case(INPUT_CTRL_HELD): {
				return (newKeyState[key] && !oldKeyState[key])
					&& (newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL])
					&& !(newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT]
						 || newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_SHIFT_HELD): {
				return (newKeyState[key] && !oldKeyState[key])
					&& (newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT])
					&& !(newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL]
						 || newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_ALT_HELD): {
				return (newKeyState[key] && !oldKeyState[key])
					&& (newKeyState[Key::LALT] || newKeyState[Key::RALT])
					&& !(newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT]
						 || newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL]);
			}
			case(INPUT_CTRL_HELD | INPUT_SHIFT_HELD): {
				return (newKeyState[key] && !oldKeyState[key])
					&& (newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL])
					&& (newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT])
					&& !(newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_CTRL_HELD | INPUT_ALT_HELD): {
				return (newKeyState[key] && !oldKeyState[key])
					&& (newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL])
					&& !(newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT])
					&& (newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_ALT_HELD | INPUT_SHIFT_HELD): {
				return (newKeyState[key] && !oldKeyState[key])
					&& !(newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL])
					&& (newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT])
					&& (newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_CTRL_HELD | INPUT_SHIFT_HELD | INPUT_ALT_HELD): {
				return (newKeyState[key] && !oldKeyState[key])
					&& (newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL])
					&& (newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT])
					&& (newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_NONE_HELD):default: {
				return (newKeyState[key] && !oldKeyState[key])
					&& !(newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL]
						 || newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT]
						 || newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
		}
	}
	
	inline bool KeyReleased(Key::Key key) {
		if (key > 100) {
			if (key & INPUT_NONE_HELD) {
				key = key ^ INPUT_NONE_HELD;
				return KeyReleased(key, INPUT_NONE_HELD);
			}
			else if (key & INPUT_ANY_HELD) {
				key = key ^ INPUT_ANY_HELD;
				return KeyReleased(key, INPUT_ANY_HELD);
			}
			else if (key & INPUT_CTRL_HELD) {
				key = key ^ INPUT_CTRL_HELD;
				return KeyReleased(key, INPUT_CTRL_HELD);
			}
			else if (key & INPUT_SHIFT_HELD) {
				key = key ^ INPUT_SHIFT_HELD;
				return KeyReleased(key, INPUT_SHIFT_HELD);
			}
			else if (key & INPUT_ALT_HELD) {
				key = key ^ INPUT_ALT_HELD;
				return KeyReleased(key, INPUT_ALT_HELD);
			}
			else if (key & (INPUT_CTRL_HELD | INPUT_SHIFT_HELD)) {
				key = key ^ (INPUT_CTRL_HELD | INPUT_SHIFT_HELD);
				return KeyReleased(key, INPUT_CTRL_HELD | INPUT_SHIFT_HELD);
			}
			else if (key & (INPUT_CTRL_HELD | INPUT_ALT_HELD)) {
				key = key ^ (INPUT_CTRL_HELD | INPUT_ALT_HELD);
				return KeyReleased(key, INPUT_CTRL_HELD | INPUT_ALT_HELD);
			}
			else if (key & (INPUT_ALT_HELD | INPUT_SHIFT_HELD)) {
				key = key ^ (INPUT_SHIFT_HELD | INPUT_ALT_HELD);
				return KeyReleased(key, INPUT_ALT_HELD | INPUT_SHIFT_HELD);
			}
			else if (key & (INPUT_CTRL_HELD | INPUT_SHIFT_HELD | INPUT_ALT_HELD)) {
				key = key ^ (INPUT_CTRL_HELD | INPUT_ALT_HELD | INPUT_SHIFT_HELD);
				return KeyReleased(key, INPUT_CTRL_HELD | INPUT_SHIFT_HELD | INPUT_ALT_HELD);
			}
			else {
				//uh
			}
		}
		return !newKeyState[key] && oldKeyState[key];
	}
	
	bool KeyReleased(Key::Key key, InputModFlags mod) {
		switch (mod) {
			case(INPUT_ANY_HELD): {
				return newKeyState[key] && !oldKeyState[key];

			}
			case(INPUT_CTRL_HELD): {
				return (!newKeyState[key] && oldKeyState[key])
					&& (newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL])
					&& !(newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT]
						 || newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_SHIFT_HELD): {
				return (!newKeyState[key] && oldKeyState[key])
					&& (newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT])
					&& !(newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL]
						 || newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_ALT_HELD): {
				return (!newKeyState[key] && oldKeyState[key])
					&& (newKeyState[Key::LALT] || newKeyState[Key::RALT])
					&& !(newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT]
						 || newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL]);
			}
			case(INPUT_CTRL_HELD | INPUT_SHIFT_HELD): {
				return (!newKeyState[key] && oldKeyState[key])
					&& (newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL])
					&& (newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT])
					&& !(newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_CTRL_HELD | INPUT_ALT_HELD): {
				return (!newKeyState[key] && oldKeyState[key])
					&& (newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL])
					&& !(newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT])
					&& (newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_ALT_HELD | INPUT_SHIFT_HELD): {
				return (!newKeyState[key] && oldKeyState[key])
					&& !(newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL])
					&& (newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT])
					&& (newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_CTRL_HELD | INPUT_SHIFT_HELD | INPUT_ALT_HELD): {
				return (!newKeyState[key] && oldKeyState[key])
					&& (newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL])
					&& (newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT])
					&& (newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_NONE_HELD):default: {
				return (!newKeyState[key] && oldKeyState[key])
					&& !(newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL]
						 || newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT]
						 || newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
		}
	}
	
	
	/////////////////////////////
	//// mouse buttons input ////
	/////////////////////////////
	
	inline bool MouseDown(MouseButton button) {
		return newMouseState[button];
	}
	
	bool MouseDown(MouseButton button, InputModFlags mod) {
		switch (mod) {
			case(INPUT_NONE_HELD): {
				return newMouseState[button]
					&& !(newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL]
						 || newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT]
						 || newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_CTRL_HELD): {
				return newMouseState[button]
					&& (newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL])
					&& !(newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT]
						 || newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_SHIFT_HELD): {
				return newMouseState[button]
					&& (newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT])
					&& !(newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL]
						 || newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_ALT_HELD): {
				return newMouseState[button]
					&& (newKeyState[Key::LALT] || newKeyState[Key::RALT])
					&& !(newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT]
						 || newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL]);
			}
			case(INPUT_CTRL_HELD | INPUT_SHIFT_HELD): {
				return newMouseState[button]
					&& (newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL])
					&& (newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT])
					&& !(newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_CTRL_HELD | INPUT_ALT_HELD): {
				return newMouseState[button]
					&& (newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL])
					&& !(newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT])
					&& (newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_ALT_HELD | INPUT_SHIFT_HELD): {
				return newMouseState[button]
					&& !(newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL])
					&& (newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT])
					&& (newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_CTRL_HELD | INPUT_SHIFT_HELD | INPUT_ALT_HELD): {
				return newMouseState[button]
					&& (newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL])
					&& (newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT])
					&& (newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_ANY_HELD):default: {
				return newMouseState[button];
			}
		}
	}
	
	inline bool MousePressed(MouseButton button) {
		return newMouseState[button] && !oldMouseState[button];
	}
	
	bool MousePressed(MouseButton button, InputModFlags mod) {
		switch (mod) {
			case(INPUT_NONE_HELD): {
				return (newMouseState[button] && !oldMouseState[button])
					&& !(newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL]
						 || newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT]
						 || newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_CTRL_HELD): {
				return (newMouseState[button] && !oldMouseState[button])
					&& (newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL])
					&& !(newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT]
						 || newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_SHIFT_HELD): {
				return (newMouseState[button] && !oldMouseState[button])
					&& (newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT])
					&& !(newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL]
						 || newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_ALT_HELD): {
				return (newMouseState[button] && !oldMouseState[button])
					&& (newKeyState[Key::LALT] || newKeyState[Key::RALT])
					&& !(newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT]
						 || newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL]);
			}
			case(INPUT_CTRL_HELD | INPUT_SHIFT_HELD): {
				return (newMouseState[button] && !oldMouseState[button])
					&& (newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL])
					&& (newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT])
					&& !(newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_CTRL_HELD | INPUT_ALT_HELD): {
				return (newMouseState[button] && !oldMouseState[button])
					&& (newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL])
					&& !(newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT])
					&& (newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_ALT_HELD | INPUT_SHIFT_HELD): {
				return (newMouseState[button] && !oldMouseState[button])
					&& !(newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL])
					&& (newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT])
					&& (newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_CTRL_HELD | INPUT_SHIFT_HELD | INPUT_ALT_HELD): {
				return (newMouseState[button] && !oldMouseState[button])
					&& (newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL])
					&& (newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT])
					&& (newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_ANY_HELD):default: {
				return newMouseState[button] && !oldMouseState[button];
			}
		}
	}
	
	inline bool MouseReleased(MouseButton button) {
		return !newMouseState[button] && oldMouseState[button];
	}
	
	bool MouseReleased(MouseButton button, InputModFlags mod) {
		switch (mod) {
			case(INPUT_NONE_HELD): {
				return (!newMouseState[button] && oldMouseState[button])
					&& !(newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL]
						 || newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT]
						 || newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_CTRL_HELD): {
				return (!newMouseState[button] && oldMouseState[button])
					&& (newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL])
					&& !(newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT]
						 || newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_SHIFT_HELD): {
				return (!newMouseState[button] && oldMouseState[button])
					&& (newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT])
					&& !(newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL]
						 || newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_ALT_HELD): {
				return (!newMouseState[button] && oldMouseState[button])
					&& (newKeyState[Key::LALT] || newKeyState[Key::RALT])
					&& !(newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT]
						 || newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL]);
			}
			case(INPUT_CTRL_HELD | INPUT_SHIFT_HELD): {
				return (!newMouseState[button] && oldMouseState[button])
					&& (newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL])
					&& (newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT])
					&& !(newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_CTRL_HELD | INPUT_ALT_HELD): {
				return (!newMouseState[button] && oldMouseState[button])
					&& (newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL])
					&& !(newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT])
					&& (newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_ALT_HELD | INPUT_SHIFT_HELD): {
				return (!newMouseState[button] && oldMouseState[button])
					&& !(newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL])
					&& (newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT])
					&& (newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_CTRL_HELD | INPUT_SHIFT_HELD | INPUT_ALT_HELD): {
				return (!newMouseState[button] && oldMouseState[button])
					&& (newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL])
					&& (newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT])
					&& (newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_ANY_HELD):default: {
				return newMouseState[button] && !oldMouseState[button];
			}
		}
	}
	
	/////////////////////////////
	//// modifier keys input ////
	/////////////////////////////
	
	bool ModDown(InputModFlags mods){
		switch (mods) {
			case(INPUT_NONE_HELD): {
				return !(newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL]
						 || newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT]
						 || newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_CTRL_HELD): {
				return (newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL])
					&& !(newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT]
						 || newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_SHIFT_HELD): {
				return (newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT])
					&& !(newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL]
						 || newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_ALT_HELD): {
				return (newKeyState[Key::LALT] || newKeyState[Key::RALT])
					&& !(newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT]
						 || newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL]);
			}
			case(INPUT_CTRL_HELD | INPUT_SHIFT_HELD): {
				return (newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL])
					&& (newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT])
					&& !(newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case((INPUT_CTRL_HELD | INPUT_ALT_HELD)): {
				return (newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL])
					&& !(newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT])
					&& (newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_ALT_HELD | INPUT_SHIFT_HELD): {
				return !(newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL])
					&& (newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT])
					&& (newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_CTRL_HELD | INPUT_SHIFT_HELD | INPUT_ALT_HELD): {
				return (newKeyState[Key::LCTRL] || newKeyState[Key::RCTRL])
					&& (newKeyState[Key::LSHIFT] || newKeyState[Key::RSHIFT])
					&& (newKeyState[Key::LALT] || newKeyState[Key::RALT]);
			}
			case(INPUT_ANY_HELD):default: {
				return true;
			}
		}
	}
};

#endif //DESHI_INPUT_H