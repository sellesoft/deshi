#pragma once
#ifndef DESHI_INPUT_H
#define DESHI_INPUT_H

#include "time.h"
#include "kigu/common.h"
#include "math/vector.h"

#define LOG_INPUTS false
#define MAX_KEYBOARD_KEYS 256
#define MAX_MOUSE_BUTTONS 7

typedef Type KeyCode; enum{
	Key_NONE,
	Key_A, Key_B, Key_C, Key_D, Key_E, Key_F, Key_G, Key_H, Key_I, Key_J, Key_K, Key_L, Key_M,
	Key_N, Key_O, Key_P, Key_Q, Key_R, Key_S, Key_T, Key_U, Key_V, Key_W, Key_X, Key_Y, Key_Z,
	Key_0, Key_1, Key_2, Key_3, Key_4, Key_5, Key_6, Key_7, Key_8, Key_9,
	Key_F1, Key_F2, Key_F3, Key_F4, Key_F5, Key_F6, Key_F7, Key_F8, Key_F9, Key_F10, Key_F11, Key_F12,
	Key_UP, Key_DOWN, Key_LEFT, Key_RIGHT,
	Key_ESCAPE, Key_TILDE, Key_TAB, Key_CAPSLOCK, Key_MINUS, Key_EQUALS, Key_BACKSPACE, Key_LBRACKET, Key_RBRACKET,
	Key_BACKSLASH, Key_SEMICOLON, Key_APOSTROPHE, Key_ENTER, Key_COMMA, Key_PERIOD, Key_FORWARDSLASH, Key_SPACE,
	Key_LSHIFT, Key_RSHIFT, Key_LCTRL, Key_RCTRL, Key_LMETA, Key_RMETA, Key_LALT, Key_RALT, Key_APPS,
	Key_INSERT, Key_DELETE, Key_HOME, Key_END, Key_PAGEUP, Key_PAGEDOWN, Key_PRINTSCREEN, Key_SCROLLLOCK, Key_PAUSEBREAK,
	Key_NP0, Key_NP1, Key_NP2, Key_NP3, Key_NP4, Key_NP5, Key_NP6, Key_NP7, Key_NP8, Key_NP9,
	Key_NPMULTIPLY, Key_NPDIVIDE, Key_NPPLUS, Key_NPMINUS, Key_NPPERIOD, Key_NUMLOCK,
	Mouse_LEFT, Mouse_RIGHT, Mouse_MIDDLE, Mouse_4, Mouse_5, Mouse_6, Mouse_7, Mouse_8,
};
global_ str8 KeyCodeStrings[] = { //NOTE(delle) gotta love uncounted string literals :)
	STR8("NONE"),
	STR8("A"),STR8("B"),STR8("C"),STR8("D"),STR8("E"),STR8("F"),STR8("G"),STR8("H"),STR8("I"),STR8("J"),STR8("K"),STR8("L"),STR8("M"),
	STR8("N"),STR8("O"),STR8("P"),STR8("Q"),STR8("R"),STR8("S"),STR8("T"),STR8("U"),STR8("V"),STR8("W"),STR8("X"),STR8("Y"),STR8("Z"),
	STR8("K0"),STR8("K1"),STR8("K2"),STR8("K3"),STR8("K4"),STR8("K5"),STR8("K6"),STR8("K7"),STR8("K8"),STR8("K9"),
	STR8("F1"),STR8("F2"),STR8("F3"),STR8("F4"),STR8("F5"),STR8("F6"),STR8("F7"),STR8("F8"),STR8("F9"),STR8("F10"),STR8("F11"),STR8("F12"),
	STR8("Up Arrow"),STR8("Down Arrow"),STR8("Left Arrow"),STR8("Right Arrow"),
	STR8("Escape"),STR8("Tilde"),STR8("Tab"),STR8("Caps Lock"),STR8("Minus"),STR8("Equals"),STR8("Backspace"),STR8("Left Bracket"),STR8("Right Bracket"),
	STR8("Backslash"),STR8("Semicolon"),STR8("Apostrophe"),STR8("Enter"),STR8("Comma"),STR8("Period"),STR8("Forward Slash"),STR8("Space"),
	STR8("Left Shift"),STR8("Right Shift"),STR8("Left Control"),STR8("Right Control"),STR8("Left Windows"),STR8("Right Windows"),STR8("Left Alt"),STR8("Right Alt"),STR8("Apps"),
	STR8("Insert"),STR8("Delete"),STR8("Home"),STR8("End"),STR8("Page Up"),STR8("Page Down"),STR8("Print Screen"),STR8("Scroll Lock"),STR8("Pause Break"),
	STR8("Numpad 0"),STR8("Numpad 1"),STR8("Numpad 2"),STR8("Numpad 3"),STR8("Numpad 4"),STR8("Numpad 5"),STR8("Numpad 6"),STR8("Numpad 7"),STR8("Numpad 8"),STR8("Numpad 9"),
	STR8("Numpad Multiply"),STR8("Numpad Divide"),STR8("Numpad Plus"),STR8("Numpad Minus"),STR8("Numpad Period"),STR8("Num Lock"),
	STR8("Mouse Left"),STR8("Mouse Right"),STR8("Mouse Middle"),STR8("Mouse 4"),STR8("Mouse 5"),STR8("Mouse 6"),STR8("Mouse 7"),STR8("Mouse 8"),
};

//NOTE(delle) the first 8bits of a keymod are reserved for the Key enum
typedef Type InputMod; enum{
	InputMod_Any    = 0,
	InputMod_None   = 1 << 8,
	InputMod_Lctrl  = 1 << 9,
	InputMod_Rctrl  = 1 << 10,
	InputMod_Lshift = 1 << 11,
	InputMod_Rshift = 1 << 12,
	InputMod_Lalt   = 1 << 13,
	InputMod_Ralt   = 1 << 14,
	InputMod_AnyShift              = InputMod_Rshift | InputMod_Lshift,
	InputMod_AnyAlt                = InputMod_Ralt   | InputMod_Lalt,
	InputMod_AnyCtrl               = InputMod_Rctrl  | InputMod_Lctrl,
	InputMod_AnyShiftAnyAlt        = InputMod_AnyShift | InputMod_AnyAlt,
	InputMod_AnyAltAnyCtrl         = InputMod_AnyAlt | InputMod_AnyCtrl,
	InputMod_AnyCtrlAnyShift       = InputMod_AnyCtrl | InputMod_AnyShift,
	InputMod_AnyShiftAnyCtrlAnyAlt = InputMod_AnyShift | InputMod_AnyCtrl | InputMod_AnyAlt,
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
};

struct Input{
	b32 oldKeyState[MAX_KEYBOARD_KEYS];
	b32 newKeyState[MAX_KEYBOARD_KEYS];
	b32 zero[MAX_KEYBOARD_KEYS];
	b32 anyKeyDown;
	
	f64 mouseX,       mouseY; //window space
	f64 screenMouseX, screenMouseY;
	f64 scrollX,      scrollY;
	
	u8 charIn[256];
	u8 charCount;
	
	b32 capsLock;
	b32 numLock;
	b32 scrollLock;
	
	//real values updated through OS input callbacks
	b32 realKeyState[MAX_KEYBOARD_KEYS];
	f64 realMouseX,       realMouseY;
	f64 realScreenMouseX, realScreenMouseY;
	f64 realScrollX,      realScrollY;
	u32 realCharCount;
	
	f64 time_key_held;
	f64 time_char_held;
	Stopwatch time_since_key_hold;
	Stopwatch time_since_char_hold;
};

//global input pointer
extern Input* g_input;
#define DeshInput g_input


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @input_mouse_states
FORCE_INLINE b32 input_lmouse_down()    { return  DeshInput->newKeyState[Mouse_LEFT]; }
FORCE_INLINE b32 input_rmouse_down()    { return  DeshInput->newKeyState[Mouse_RIGHT]; }
FORCE_INLINE b32 input_lmouse_pressed() { return  DeshInput->newKeyState[Mouse_LEFT]  && !DeshInput->oldKeyState[Mouse_LEFT]; }
FORCE_INLINE b32 input_rmouse_pressed() { return  DeshInput->newKeyState[Mouse_RIGHT] && !DeshInput->oldKeyState[Mouse_RIGHT]; }
FORCE_INLINE b32 input_lmouse_released(){ return !DeshInput->newKeyState[Mouse_LEFT]  &&  DeshInput->oldKeyState[Mouse_LEFT]; }
FORCE_INLINE b32 input_rmouse_released(){ return !DeshInput->newKeyState[Mouse_RIGHT] &&  DeshInput->oldKeyState[Mouse_RIGHT]; }

FORCE_INLINE vec2 input_mouse_position(){ return vec2{(f32)DeshInput->mouseX, (f32)DeshInput->mouseY}; }

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @input_modifier_states
FORCE_INLINE b32 input_lctrl_down() { return DeshInput->newKeyState[Key_LCTRL]; }
FORCE_INLINE b32 input_rctrl_down() { return DeshInput->newKeyState[Key_RCTRL]; }
FORCE_INLINE b32 input_lshift_down(){ return DeshInput->newKeyState[Key_LSHIFT]; }
FORCE_INLINE b32 input_rshift_down(){ return DeshInput->newKeyState[Key_RSHIFT]; }
FORCE_INLINE b32 input_lalt_down()  { return DeshInput->newKeyState[Key_LALT]; }
FORCE_INLINE b32 input_ralt_down()  { return DeshInput->newKeyState[Key_RALT]; }
FORCE_INLINE b32 input_ctrl_down()  { return DeshInput->newKeyState[Key_RCTRL]  || DeshInput->newKeyState[Key_LCTRL]; }
FORCE_INLINE b32 input_shift_down() { return DeshInput->newKeyState[Key_RSHIFT] || DeshInput->newKeyState[Key_LSHIFT]; }
FORCE_INLINE b32 input_alt_down()   { return DeshInput->newKeyState[Key_RALT]   || DeshInput->newKeyState[Key_LALT]; }

global_ b32
input_mods_down(u32 mods){
	switch(mods){
		case(InputMod_Any):            return true;
		case(InputMod_None):           return !input_lctrl_down() && !input_rctrl_down() && !input_lshift_down() && !input_rshift_down() && !input_lalt_down() && !input_ralt_down();
		case(InputMod_Lctrl):          return  input_lctrl_down() && !input_rctrl_down() && !input_lshift_down() && !input_rshift_down() && !input_lalt_down() && !input_ralt_down();
		case(InputMod_Rctrl):          return !input_lctrl_down() &&  input_rctrl_down() && !input_lshift_down() && !input_rshift_down() && !input_lalt_down() && !input_ralt_down();
		case(InputMod_Lshift):         return !input_lctrl_down() && !input_rctrl_down() &&  input_lshift_down() && !input_rshift_down() && !input_lalt_down() && !input_ralt_down();
		case(InputMod_Rshift):         return !input_lctrl_down() && !input_rctrl_down() && !input_lshift_down() &&  input_rshift_down() && !input_lalt_down() && !input_ralt_down();
		case(InputMod_Lalt):           return !input_lctrl_down() && !input_rctrl_down() && !input_lshift_down() && !input_rshift_down() &&  input_lalt_down() && !input_ralt_down();
		case(InputMod_Ralt):           return !input_lctrl_down() && !input_rctrl_down() && !input_lshift_down() && !input_rshift_down() && !input_lalt_down() &&  input_ralt_down();
		case(InputMod_AnyShift):       return !input_lctrl_down() && !input_rctrl_down() && (input_lshift_down() ||  input_rshift_down())&& !input_lalt_down() && !input_ralt_down();
		case(InputMod_AnyAlt):         return !input_lctrl_down() && !input_rctrl_down() && !input_lshift_down() && !input_rshift_down() && (input_lalt_down() ||  input_ralt_down());
		case(InputMod_AnyCtrl):        return (input_lctrl_down() ||  input_rctrl_down())&& !input_lshift_down() && !input_rshift_down() && !input_lalt_down() && !input_ralt_down();
		case(InputMod_LctrlLshift):    return  input_lctrl_down() && !input_rctrl_down() &&  input_lshift_down() && !input_rshift_down() && !input_lalt_down() && !input_ralt_down();
		case(InputMod_LctrlRshift):    return  input_lctrl_down() && !input_rctrl_down() && !input_lshift_down() &&  input_rshift_down() && !input_lalt_down() && !input_ralt_down();
		case(InputMod_RctrlLshift):    return !input_lctrl_down() &&  input_rctrl_down() &&  input_lshift_down() && !input_rshift_down() && !input_lalt_down() && !input_ralt_down();
		case(InputMod_RctrlRshift):    return !input_lctrl_down() &&  input_rctrl_down() && !input_lshift_down() &&  input_rshift_down() && !input_lalt_down() && !input_ralt_down();
		case(InputMod_LctrlLalt):      return  input_lctrl_down() && !input_rctrl_down() && !input_lshift_down() && !input_rshift_down() &&  input_lalt_down() && !input_ralt_down();
		case(InputMod_LctrlRalt):      return  input_lctrl_down() && !input_rctrl_down() && !input_lshift_down() && !input_rshift_down() && !input_lalt_down() &&  input_ralt_down();
		case(InputMod_RctrlLalt):      return !input_lctrl_down() &&  input_rctrl_down() && !input_lshift_down() && !input_rshift_down() &&  input_lalt_down() && !input_ralt_down();
		case(InputMod_RctrlRalt):      return !input_lctrl_down() &&  input_rctrl_down() && !input_lshift_down() && !input_rshift_down() && !input_lalt_down() &&  input_ralt_down();
		case(InputMod_LshiftLalt):     return !input_lctrl_down() && !input_rctrl_down() &&  input_lshift_down() && !input_rshift_down() &&  input_lalt_down() && !input_ralt_down();
		case(InputMod_LshiftRalt):     return !input_lctrl_down() && !input_rctrl_down() &&  input_lshift_down() && !input_rshift_down() && !input_lalt_down() &&  input_ralt_down();
		case(InputMod_RshiftLalt):     return !input_lctrl_down() && !input_rctrl_down() && !input_lshift_down() &&  input_rshift_down() &&  input_lalt_down() && !input_ralt_down();
		case(InputMod_RshiftRalt):     return !input_lctrl_down() && !input_rctrl_down() && !input_lshift_down() &&  input_rshift_down() && !input_lalt_down() &&  input_ralt_down();
		case(InputMod_LctrlLshiftLalt):return  input_lctrl_down() && !input_rctrl_down() &&  input_lshift_down() && !input_rshift_down() &&  input_lalt_down() && !input_ralt_down();
		case(InputMod_LctrlLshiftRalt):return  input_lctrl_down() && !input_rctrl_down() &&  input_lshift_down() && !input_rshift_down() && !input_lalt_down() &&  input_ralt_down();
		case(InputMod_LctrlRshiftLalt):return  input_lctrl_down() && !input_rctrl_down() && !input_lshift_down() &&  input_rshift_down() &&  input_lalt_down() && !input_ralt_down();
		case(InputMod_LctrlRshiftRalt):return  input_lctrl_down() && !input_rctrl_down() && !input_lshift_down() &&  input_rshift_down() && !input_lalt_down() &&  input_ralt_down();
		case(InputMod_RctrlLshiftLalt):return !input_lctrl_down() &&  input_rctrl_down() &&  input_lshift_down() && !input_rshift_down() &&  input_lalt_down() && !input_ralt_down();
		case(InputMod_RctrlLshiftRalt):return !input_lctrl_down() &&  input_rctrl_down() &&  input_lshift_down() && !input_rshift_down() && !input_lalt_down() &&  input_ralt_down();
		case(InputMod_RctrlRshiftLalt):return !input_lctrl_down() &&  input_rctrl_down() && !input_lshift_down() &&  input_rshift_down() &&  input_lalt_down() && !input_ralt_down();
		case(InputMod_RctrlRshiftRalt):return !input_lctrl_down() &&  input_rctrl_down() && !input_lshift_down() &&  input_rshift_down() && !input_lalt_down() &&  input_ralt_down();
		default: return false;
	} 
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @input_key_states
#define INPUT_KEY_MASK 0x000000FF //mod_key & 0x000000FF extract the key
#define INPUT_MOD_MASK 0xFFFFFF00 //mod_key & 0xFFFFFF00 extract the mods
FORCE_INLINE b32
key_down(u32 mod_key){ 
	return  DeshInput->newKeyState[mod_key & INPUT_KEY_MASK]&& input_mods_down(mod_key & INPUT_MOD_MASK);
}

FORCE_INLINE b32
key_up(u32 mod_key){
	return !DeshInput->newKeyState[mod_key & INPUT_KEY_MASK] && input_mods_down(mod_key & INPUT_MOD_MASK);
}

FORCE_INLINE b32
key_pressed(u32 mod_key){
	return  DeshInput->newKeyState[mod_key & INPUT_KEY_MASK] && !DeshInput->oldKeyState[mod_key & INPUT_KEY_MASK] && input_mods_down(mod_key & INPUT_MOD_MASK);
}

FORCE_INLINE b32
key_released(u32 mod_key){
	return !DeshInput->newKeyState[mod_key & INPUT_KEY_MASK] &&  DeshInput->oldKeyState[mod_key & INPUT_KEY_MASK] && input_mods_down(mod_key & INPUT_MOD_MASK);
}

FORCE_INLINE void
simulate_key_press(u32 key){
	DeshInput->newKeyState[key] = true;
}

FORCE_INLINE b32 any_key_pressed() { return  key_released(Key_NONE); }
FORCE_INLINE b32 any_key_down()    { return !key_down(Key_NONE); }
FORCE_INLINE b32 any_key_released(){ return  key_pressed(Key_NONE); }

#endif //DESHI_INPUT_H