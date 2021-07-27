#pragma once
#ifndef DESHI_INPUT_H
#define DESHI_INPUT_H

#include "../defines.h"
#include "../math/Vector.h"
#include "../utils/tuple.h"

#include <map>
#include <vector>

//constants
#define MAX_KEYBOARD_KEYS 256
#define MAX_MOUSE_BUTTONS 7

namespace Key {
	enum KeyBits{
		Key_NONE,
		A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
		K0, K1, K2, K3, K4, K5, K6, K7, K8, K9,
		F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
		UP, DOWN, LEFT, RIGHT,
		ESCAPE, TILDE, TAB, CAPSLOCK, LSHIFT, LCTRL, LALT,
		BACKSPACE, ENTER, RSHIFT, RCTRL, RALT, MINUS, EQUALS, LBRACKET, RBRACKET,
		SLASH, SEMICOLON, APOSTROPHE, COMMA, PERIOD, BACKSLASH, SPACE,
		INSERT, DELETE, HOME, END, PAGEUP, PAGEDOWN, PAUSE, SCROLL,
		NUMPAD0, NUMPAD1, NUMPAD2, NUMPAD3, NUMPAD4, NUMPAD5, NUMPAD6, NUMPAD7, NUMPAD8, NUMPAD9,
		NUMPADMULTIPLY, NUMPADDIVIDE, NUMPADPLUS, NUMPADMINUS, NUMPADPERIOD, NUMPADENTER, NUMLOCK,
		MBLEFT, MBRIGHT, MBMIDDLE, MBFOUR, MBFIVE, MBSIX, MBSEVEN, MBEIGHT, MBSCROLLDOWN, MBSCROLLUP,
		Key_COUNT
	}; typedef u32 Key;
};
global_ const char* KeyStrings[] = {
	"NONE",
	"A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S","T","U","V","W","X","Y","Z",
	"K0","K1","K2","K3","K4","K5","K6","K7","K8","K9",
	"F1","F2","F3","F4","F5","F6","F7","F8","F9","F10","F11","F12",
	"UP","DOWN","LEFT","RIGHT",
	"ESCAPE","TILDE","TAB","CAPSLOCK","LSHIFT","LCTRL","LALT",
	"BACKSPACE","ENTER","RSHIFT","RCTRL","RALT","MINUS","EQUALS","LBRACKET","RBRACKET",
	"SLASH","SEMICOLON","APOSTROPHE","COMMA","PERIOD","BACKSLASH","SPACE",
	"INSERT","DELETE","HOME","END","PAGEUP","PAGEDOWN","PAUSE","SCROLL",
	"NUMPAD0","NUMPAD1","NUMPAD2","NUMPAD3","NUMPAD4","NUMPAD5","NUMPAD6","NUMPAD7","NUMPAD8","NUMPAD9",
	"NUMPADMULTIPLY","NUMPADDIVIDE","NUMPADPLUS","NUMPADMINUS","NUMPADPERIOD","NUMPADENTER","NUMLOCK",
	"MBLEFT","MBRIGHT","MBMIDDLE","MBFOUR","MBFIVE","MBSIX","MBSEVEN","MBEIGHT","MBSCROLLDOWN","MBSCROLLUP"
};

namespace MouseButton{
	enum MouseButtonBits{
		LEFT  = Key::MBLEFT, RIGHT = Key::MBRIGHT, MIDDLE = Key::MBMIDDLE, 
		FOUR  = Key::MBFOUR, FIVE  = Key::MBFIVE, 
		SIX   = Key::MBSIX,  SEVEN = Key::MBSEVEN, 
		EIGHT = Key::MBEIGHT,
		SCROLLDOWN = Key::MBSCROLLDOWN, SCROLLUP = Key::MBSCROLLUP
	}; typedef u32 MouseButton;
}

//NOTE the first 8bits of a keymod are reserved for the Key enum
enum InputModBits{
	InputMod_NONE   = 0,
	InputMod_Any    = 1 << 8,
	InputMod_Lctrl  = 1 << 9,
	InputMod_Rctrl  = 1 << 10,
	InputMod_Lshift = 1 << 11,
	InputMod_Rshift = 1 << 12,
	InputMod_Lalt   = 1 << 13,
	InputMod_Ralt   = 1 << 14,
	InputMod_LctrlLshift     = InputMod_Lctrl  | InputMod_Lshift,
	InputMod_LctrlRshift     = InputMod_Lctrl  | InputMod_Rshift,
	InputMod_RctrlLshift     = InputMod_Rctrl  | InputMod_Lshift,
	InputMod_RctrlRshift     = InputMod_Rctrl  | InputMod_Rshift,
	InputMod_LctrlLalt       = InputMod_Lctrl  | InputMod_Lalt,
	InputMod_LctrlRalt       = InputMod_Lctrl  | InputMod_Ralt,
	InputMod_RctrlLalt       = InputMod_Rctrl  | InputMod_Lalt,
	InputMod_RctrlRalt       = InputMod_Rctrl  | InputMod_Ralt,
	InputMod_LshiftLalt      = InputMod_Lshift | InputMod_Lalt,
	InputMod_LshiftRalt      = InputMod_Lshift | InputMod_Ralt,
	InputMod_RshiftLalt      = InputMod_Rshift | InputMod_Lalt,
	InputMod_RshiftRalt      = InputMod_Rshift | InputMod_Ralt,
	InputMod_LctrlLshiftLalt = InputMod_Lctrl  | InputMod_Lshift | InputMod_Lalt,
	InputMod_LctrlLshiftRalt = InputMod_Lctrl  | InputMod_Lshift | InputMod_Ralt,
	InputMod_LctrlRshiftLalt = InputMod_Lctrl  | InputMod_Rshift | InputMod_Lalt,
	InputMod_LctrlRshiftRalt = InputMod_Lctrl  | InputMod_Rshift | InputMod_Ralt,
	InputMod_RctrlLshiftLalt = InputMod_Rctrl  | InputMod_Lshift | InputMod_Lalt,
	InputMod_RctrlLshiftRalt = InputMod_Rctrl  | InputMod_Lshift | InputMod_Ralt,
	InputMod_RctrlRshiftLalt = InputMod_Rctrl  | InputMod_Rshift | InputMod_Lalt,
	InputMod_RctrlRshiftRalt = InputMod_Rctrl  | InputMod_Rshift | InputMod_Ralt,
}; typedef u32 InputMod;

struct Input{
	std::map<size_t, u8> mapKeys;
	std::map<size_t, u8> mapMouse;
	
	bool oldKeyState[MAX_KEYBOARD_KEYS] = {0};
	bool newKeyState[MAX_KEYBOARD_KEYS] = {0};
	double mouseX,       mouseY;
	double screenMouseX, screenMouseY;
	double scrollX,      scrollY;
	Vector2 mousePos;
	
	//NOTE sushi: I was going to put this on keybinds, but I wanted it to only check binds if some input occured, and it seems easiest to do that here
	//for console command binding
	std::vector<pair<std::string, Key::Key>> binds;
	bool checkbinds = false; //needed bc glfw callbacks would call the function too early
	
	//real values are updated through GLFW callbacks
	bool realKeyState[MAX_KEYBOARD_KEYS]   = {0};
	double realMouseX,       realMouseY;
	double realScreenMouseX, realScreenMouseY;
	double realScrollX,      realScrollY;
	bool keyFocus, mouseFocus;
	
	bool logInput = false;
	
	//caches values so they are consistent thru the frame
	void Update(){
		memcpy(&oldKeyState, &newKeyState,  sizeof(bool) * MAX_KEYBOARD_KEYS);
		memcpy(&newKeyState, &realKeyState, sizeof(bool) * MAX_KEYBOARD_KEYS);
		//mouseX = realMouseX; mouseY = realMouseY; //NOTE this doesnt work, idk why
		mousePos.x = mouseX; mousePos.y = mouseY;
		screenMouseY = realScreenMouseX; screenMouseY = realScreenMouseY;
		//bit scuffed mouse wheel stuff
		if      (realScrollY < 0) newKeyState[MouseButton::SCROLLDOWN] = 1;
		else if (realScrollY > 0) newKeyState[MouseButton::SCROLLUP] = 1;
		else    { newKeyState[MouseButton::SCROLLDOWN] = 0; newKeyState[MouseButton::SCROLLUP] = 0; }
		realScrollY = 0;
	}
	
	/////////////////////////////////
	//// input helper functions /////
	/////////////////////////////////
	
	inline bool LCtrlDown() { return newKeyState[Key::LCTRL]; }
	inline bool RCtrlDown() { return newKeyState[Key::RCTRL]; }
	inline bool LShiftDown(){ return newKeyState[Key::LSHIFT]; }
	inline bool RShiftDown(){ return newKeyState[Key::RSHIFT]; }
	inline bool LAltDown()  { return newKeyState[Key::LALT]; }
	inline bool RAltDown()  { return newKeyState[Key::RALT]; }
	
	bool ModsDown(u32 mods){
		switch(mods){
			case(InputMod_Any):            return true;
			case(InputMod_NONE):           return !LCtrlDown() && !RCtrlDown() && !LShiftDown() && !RShiftDown() && !LAltDown() && !RAltDown();
			case(InputMod_Lctrl):          return  LCtrlDown() && !RCtrlDown() && !LShiftDown() && !RShiftDown() && !LAltDown() && !RAltDown();
			case(InputMod_Rctrl):          return !LCtrlDown() &&  RCtrlDown() && !LShiftDown() && !RShiftDown() && !LAltDown() && !RAltDown();
			case(InputMod_Lshift):         return !LCtrlDown() && !RCtrlDown() &&  LShiftDown() && !RShiftDown() && !LAltDown() && !RAltDown();
			case(InputMod_Rshift):         return !LCtrlDown() && !RCtrlDown() && !LShiftDown() &&  RShiftDown() && !LAltDown() && !RAltDown();
			case(InputMod_Lalt):           return !LCtrlDown() && !RCtrlDown() && !LShiftDown() && !RShiftDown() &&  LAltDown() && !RAltDown();
			case(InputMod_Ralt):           return !LCtrlDown() && !RCtrlDown() && !LShiftDown() && !RShiftDown() && !LAltDown() &&  RAltDown();
			case(InputMod_LctrlLshift):    return  LCtrlDown() && !RCtrlDown() &&  LShiftDown() && !RShiftDown() && !LAltDown() && !RAltDown();
			case(InputMod_LctrlRshift):    return  LCtrlDown() && !RCtrlDown() && !LShiftDown() &&  RShiftDown() && !LAltDown() && !RAltDown();
			case(InputMod_RctrlLshift):    return !LCtrlDown() &&  RCtrlDown() &&  LShiftDown() && !RShiftDown() && !LAltDown() && !RAltDown();
			case(InputMod_RctrlRshift):    return !LCtrlDown() &&  RCtrlDown() && !LShiftDown() &&  RShiftDown() && !LAltDown() && !RAltDown();
			case(InputMod_LctrlLalt):      return  LCtrlDown() && !RCtrlDown() && !LShiftDown() && !RShiftDown() &&  LAltDown() && !RAltDown();
			case(InputMod_LctrlRalt):      return  LCtrlDown() && !RCtrlDown() && !LShiftDown() && !RShiftDown() && !LAltDown() &&  RAltDown();
			case(InputMod_RctrlLalt):      return !LCtrlDown() &&  RCtrlDown() && !LShiftDown() && !RShiftDown() &&  LAltDown() && !RAltDown();
			case(InputMod_RctrlRalt):      return !LCtrlDown() &&  RCtrlDown() && !LShiftDown() && !RShiftDown() && !LAltDown() &&  RAltDown();
			case(InputMod_LshiftLalt):     return !LCtrlDown() && !RCtrlDown() &&  LShiftDown() && !RShiftDown() &&  LAltDown() && !RAltDown();
			case(InputMod_LshiftRalt):     return !LCtrlDown() && !RCtrlDown() &&  LShiftDown() && !RShiftDown() && !LAltDown() &&  RAltDown();
			case(InputMod_RshiftLalt):     return !LCtrlDown() && !RCtrlDown() && !LShiftDown() &&  RShiftDown() &&  LAltDown() && !RAltDown();
			case(InputMod_RshiftRalt):     return !LCtrlDown() && !RCtrlDown() && !LShiftDown() &&  RShiftDown() && !LAltDown() &&  RAltDown();
			case(InputMod_LctrlLshiftLalt):return  LCtrlDown() && !RCtrlDown() &&  LShiftDown() && !RShiftDown() &&  LAltDown() && !RAltDown();
			case(InputMod_LctrlLshiftRalt):return  LCtrlDown() && !RCtrlDown() &&  LShiftDown() && !RShiftDown() && !LAltDown() &&  RAltDown();
			case(InputMod_LctrlRshiftLalt):return  LCtrlDown() && !RCtrlDown() && !LShiftDown() &&  RShiftDown() &&  LAltDown() && !RAltDown();
			case(InputMod_LctrlRshiftRalt):return  LCtrlDown() && !RCtrlDown() && !LShiftDown() &&  RShiftDown() && !LAltDown() &&  RAltDown();
			case(InputMod_RctrlLshiftLalt):return !LCtrlDown() &&  RCtrlDown() &&  LShiftDown() && !RShiftDown() &&  LAltDown() && !RAltDown();
			case(InputMod_RctrlLshiftRalt):return !LCtrlDown() &&  RCtrlDown() &&  LShiftDown() && !RShiftDown() && !LAltDown() &&  RAltDown();
			case(InputMod_RctrlRshiftLalt):return !LCtrlDown() &&  RCtrlDown() && !LShiftDown() &&  RShiftDown() &&  LAltDown() && !RAltDown();
			case(InputMod_RctrlRshiftRalt):return !LCtrlDown() &&  RCtrlDown() && !LShiftDown() &&  RShiftDown() && !LAltDown() &&  RAltDown();
			default: return false;
		} 
	}
	
	/////////////////////////////
	//// keyboard keys input ////
	/////////////////////////////
	
	//mod_key & 0x000000FF extract the key
	//mod_key & 0xFFFFFF00 extract the mods
	
	inline bool KeyDownAnyMod(u32 mod_key){ 
		u32 key = mod_key & 0x000000FF;
		return newKeyState[key]; 
	}
	
	inline bool KeyDown(u32 mod_key) { 
		u32 key = mod_key & 0x000000FF;
		return newKeyState[key] && ModsDown(mod_key & 0xFFFFFF00); 
	}
	
	inline bool KeyUp(u32 mod_key) {
		u32 key = mod_key & 0x000000FF;
		return !newKeyState[key] && ModsDown(mod_key & 0xFFFFFF00);
	}
	
	inline bool KeyPressedAnyMod(u32 mod_key){ 
		u32 key = mod_key & 0x000000FF;
		return newKeyState[key] && !oldKeyState[key];
	}
	
	inline bool KeyPressed(u32 mod_key) {
		u32 key = mod_key & 0x000000FF;
		return newKeyState[key] && !oldKeyState[key] && ModsDown(mod_key & 0xFFFFFF00);
	}
	
	inline bool KeyReleased(u32 mod_key) {
		u32 key = mod_key & 0x000000FF;
		return !newKeyState[key] && oldKeyState[key] && ModsDown(mod_key & 0xFFFFFF00);
	}
	
	inline bool KeyReleasedAnyMod(u32 mod_key) {
		u32 key = mod_key & 0x000000FF;
		return !newKeyState[key] && oldKeyState[key];
	}
	
	inline void AddBind(std::string command, Key::Key key) {
		
	}
};

//global_ input pointer
extern Input* g_input;
#define DengInput g_input


#endif //DESHI_INPUT_H