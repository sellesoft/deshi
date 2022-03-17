#pragma once
#ifndef DESHI_INPUT_H
#define DESHI_INPUT_H

#include "time.h"
#include "kigu/common.h"
#include "kigu/pair.h"
#include "math/vector.h"

#include <map>
#include <vector>

//constants
#define MAX_KEYBOARD_KEYS 256
#define MAX_MOUSE_BUTTONS 7

namespace Key {
	enum Key_{
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
		LMETA, RMETA,
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
	"LMETA", "RMETA",
	"MBLEFT","MBRIGHT","MBMIDDLE","MBFOUR","MBFIVE","MBSIX","MBSEVEN","MBEIGHT","MBSCROLLDOWN","MBSCROLLUP"
};

#if DESHI_WINDOWS

#elif DESHI_LINUX
#elif DESHI_MAC
#endif


namespace MouseButton{
	enum MouseButton_{
		LEFT  = Key::MBLEFT, RIGHT = Key::MBRIGHT, MIDDLE = Key::MBMIDDLE, 
		FOUR  = Key::MBFOUR, FIVE  = Key::MBFIVE, 
		SIX   = Key::MBSIX,  SEVEN = Key::MBSEVEN, 
		EIGHT = Key::MBEIGHT,
		SCROLLDOWN = Key::MBSCROLLDOWN, SCROLLUP = Key::MBSCROLLUP
	}; typedef u32 MouseButton;
}

//NOTE the first 8bits of a keymod are reserved for the Key enum
enum InputMod_{
	InputMod_Any    = 0,
	InputMod_None   = 1 << 8,
	InputMod_Lctrl  = 1 << 9,
	InputMod_Rctrl  = 1 << 10,
	InputMod_Lshift = 1 << 11,
	InputMod_Rshift = 1 << 12,
	InputMod_Lalt   = 1 << 13,
	InputMod_Ralt   = 1 << 14,
	InputMod_AnyShift        = InputMod_Rshift | InputMod_Lshift,
	InputMod_AnyAlt          = InputMod_Ralt   | InputMod_Lalt,
	InputMod_AnyCtrl         = InputMod_Rctrl  | InputMod_Lctrl,
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
	
	b32 oldKeyState[MAX_KEYBOARD_KEYS] = {0};
	b32 newKeyState[MAX_KEYBOARD_KEYS] = {0};
	
	f64 mouseX,       mouseY; //window space
	f64 screenMouseX, screenMouseY;
	f64 scrollX,      scrollY;
	vec2 mousePos;
	u32 charIn[127] = { 0 };
	u32 charCount = 0;
	
	b32 zero[MAX_KEYBOARD_KEYS] = {0};
	
	b32 anyKeyDown = 0;
	
	b32 capsLock = false;
	b32 numLock = false;
	b32 scrollLock = false;
	
	//NOTE sushi: I was going to put this on keybinds, but I wanted it to only check binds if some input occured, and it seems easiest to do that here
	//for console command binding
	std::vector<pair<std::string, Key::Key>> binds; //TODO remove/move this and make it array
	b32 checkbinds = false; //needed bc glfw callbacks would call the function too early
	
	//real values updated through OS input callbacks
	b32 realKeyState[MAX_KEYBOARD_KEYS]   = {0};
	f64 realMouseX,       realMouseY;
	f64 realScreenMouseX, realScreenMouseY;
	f64 realScrollX,      realScrollY;
	u32 realCharCount = 0;
	b32 keyFocus, mouseFocus;
	
	b32 logInput = false;
	
	f64 time_key_held = 0;
	f64 time_char_held = 0;
	
	TIMER_START(input__time_since_key_hold);
	TIMER_START(input__time_since_char_hold);
	
	//caches values so they are consistent thru the frame
	void Update(){
		TIMER_START(t_d);
		memcpy(&oldKeyState, &newKeyState,  sizeof(b32) * MAX_KEYBOARD_KEYS);
		memcpy(&newKeyState, &realKeyState, sizeof(b32) * MAX_KEYBOARD_KEYS);
		
		if(!memcmp(newKeyState, zero, MAX_KEYBOARD_KEYS)){
			TIMER_RESET(input__time_since_key_hold);
			newKeyState[0] = 1;
			anyKeyDown = 0;
		}else{
			time_key_held = TIMER_END(input__time_since_key_hold);
			anyKeyDown = 1;
		}
		
		if(!realCharCount){
			TIMER_RESET(input__time_since_char_hold);
		}else{
			time_char_held = TIMER_END(input__time_since_char_hold);
		}
		
		if(realScrollY > 0){
			newKeyState[Key::MBSCROLLUP]   = true;
			newKeyState[Key::MBSCROLLDOWN] = false;
		}else if(realScrollY < 0){
			newKeyState[Key::MBSCROLLUP]   = false;
			newKeyState[Key::MBSCROLLDOWN] = true;
		}else{
			newKeyState[Key::MBSCROLLUP]   = false;
			newKeyState[Key::MBSCROLLDOWN] = false;
		}
		
		mousePos.x = mouseX = realMouseX; mousePos.y = mouseY = realMouseY;
		screenMouseX = realScreenMouseX; screenMouseY = realScreenMouseY;
		scrollY = realScrollY;
		realScrollY = 0;
		charCount = realCharCount;
		realCharCount = 0;
		
		DeshTime->inputTime = TIMER_END(t_d);
	}
	
	/////////////////////////////////
	//// input helper functions /////
	/////////////////////////////////
	
	inline b32 LCtrlDown() { return newKeyState[Key::LCTRL]; }
	inline b32 RCtrlDown() { return newKeyState[Key::RCTRL]; }
	inline b32 LShiftDown(){ return newKeyState[Key::LSHIFT]; }
	inline b32 RShiftDown(){ return newKeyState[Key::RSHIFT]; }
	inline b32 LAltDown()  { return newKeyState[Key::LALT]; }
	inline b32 RAltDown()  { return newKeyState[Key::RALT]; }
	inline b32 CtrlDown()  { return newKeyState[Key::RCTRL] || newKeyState[Key::LCTRL]; }
	inline b32 ShiftDown() { return newKeyState[Key::RSHIFT] || newKeyState[Key::LSHIFT]; }
	inline b32 AltDown()   { return newKeyState[Key::RALT] || newKeyState[Key::LALT]; }
	
	//make options for, or just make it so these dont take into account mods
	inline b32 AnyKeyPressed()   { return KeyReleased(Key::Key_NONE); }
	inline b32 AnyKeyDown()      { return !KeyDown(Key::Key_NONE); }
	inline b32 AllKeysReleased() { return KeyPressed(Key::Key_NONE); }
	
	inline b32 LMouseDown()     { return  newKeyState[MouseButton::LEFT]; }
	inline b32 RMouseDown()     { return  newKeyState[MouseButton::RIGHT]; }
	inline b32 LMousePressed()  { return  newKeyState[MouseButton::LEFT]       && !oldKeyState[MouseButton::LEFT]; }
	inline b32 RMousePressed()  { return  newKeyState[MouseButton::RIGHT]      && !oldKeyState[MouseButton::RIGHT]; }
	inline b32 LMouseReleased() { return !newKeyState[MouseButton::LEFT]       &&  oldKeyState[MouseButton::LEFT]; }
	inline b32 RMouseReleased() { return !newKeyState[MouseButton::RIGHT]      &&  oldKeyState[MouseButton::RIGHT]; }
	
	b32 ModsDown(u32 mods){
		switch(mods){
			case(InputMod_Any):            return true;
			case(InputMod_None):           return !LCtrlDown() && !RCtrlDown() && !LShiftDown() && !RShiftDown() && !LAltDown() && !RAltDown();
			case(InputMod_Lctrl):          return  LCtrlDown() && !RCtrlDown() && !LShiftDown() && !RShiftDown() && !LAltDown() && !RAltDown();
			case(InputMod_Rctrl):          return !LCtrlDown() &&  RCtrlDown() && !LShiftDown() && !RShiftDown() && !LAltDown() && !RAltDown();
			case(InputMod_Lshift):         return !LCtrlDown() && !RCtrlDown() &&  LShiftDown() && !RShiftDown() && !LAltDown() && !RAltDown();
			case(InputMod_Rshift):         return !LCtrlDown() && !RCtrlDown() && !LShiftDown() &&  RShiftDown() && !LAltDown() && !RAltDown();
			case(InputMod_Lalt):           return !LCtrlDown() && !RCtrlDown() && !LShiftDown() && !RShiftDown() &&  LAltDown() && !RAltDown();
			case(InputMod_Ralt):           return !LCtrlDown() && !RCtrlDown() && !LShiftDown() && !RShiftDown() && !LAltDown() &&  RAltDown();
			case(InputMod_AnyShift):       return !LCtrlDown() && !RCtrlDown() &&  LShiftDown() ||  RShiftDown() && !LAltDown() && !RAltDown();
			case(InputMod_AnyAlt):		   return !LCtrlDown() && !RCtrlDown() && !LShiftDown() && !RShiftDown() &&  LAltDown() ||  RAltDown();
			case(InputMod_AnyCtrl):		   return  LCtrlDown() ||  RCtrlDown() && !LShiftDown() && !RShiftDown() && !LAltDown() && !RAltDown();
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
	
	inline b32 KeyDown(u32 mod_key = InputMod_Any){ 
		return  newKeyState[mod_key & 0x000000FF] && ModsDown(mod_key & 0xFFFFFF00); 
	}
	
	inline b32 KeyUp(u32 mod_key = InputMod_Any){
		return !newKeyState[mod_key & 0x000000FF] && ModsDown(mod_key & 0xFFFFFF00);
	}
	
	inline b32 KeyPressed(u32 mod_key = InputMod_Any){
		return  newKeyState[mod_key & 0x000000FF] && !oldKeyState[mod_key & 0x000000FF] && ModsDown(mod_key & 0xFFFFFF00);
	}
	
	inline b32 KeyReleased(u32 mod_key = InputMod_Any){
		return !newKeyState[mod_key & 0x000000FF] && oldKeyState[mod_key & 0x000000FF] && ModsDown(mod_key & 0xFFFFFF00);
	}
	
	inline void SimulateKeyPress(u32 key){
		newKeyState[key] = true;
	}
};

//global input pointer
extern Input* g_input;
#define DeshInput g_input

#endif //DESHI_INPUT_H