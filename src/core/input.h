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
	str8_lit("NONE"),
	str8_lit("A"),str8_lit("B"),str8_lit("C"),str8_lit("D"),str8_lit("E"),str8_lit("F"),str8_lit("G"),str8_lit("H"),str8_lit("I"),str8_lit("J"),str8_lit("K"),str8_lit("L"),str8_lit("M"),
	str8_lit("N"),str8_lit("O"),str8_lit("P"),str8_lit("Q"),str8_lit("R"),str8_lit("S"),str8_lit("T"),str8_lit("U"),str8_lit("V"),str8_lit("W"),str8_lit("X"),str8_lit("Y"),str8_lit("Z"),
	str8_lit("K0"),str8_lit("K1"),str8_lit("K2"),str8_lit("K3"),str8_lit("K4"),str8_lit("K5"),str8_lit("K6"),str8_lit("K7"),str8_lit("K8"),str8_lit("K9"),
	str8_lit("F1"),str8_lit("F2"),str8_lit("F3"),str8_lit("F4"),str8_lit("F5"),str8_lit("F6"),str8_lit("F7"),str8_lit("F8"),str8_lit("F9"),str8_lit("F10"),str8_lit("F11"),str8_lit("F12"),
	str8_lit("Up Arrow"),str8_lit("Down Arrow"),str8_lit("Left Arrow"),str8_lit("Right Arrow"),
	str8_lit("Escape"),str8_lit("Tilde"),str8_lit("Tab"),str8_lit("Caps Lock"),str8_lit("Minus"),str8_lit("Equals"),str8_lit("Backspace"),str8_lit("Left Bracket"),str8_lit("Right Bracket"),
	str8_lit("Backslash"),str8_lit("Semicolon"),str8_lit("Apostrophe"),str8_lit("Enter"),str8_lit("Comma"),str8_lit("Period"),str8_lit("Forward Slash"),str8_lit("Space"),
	str8_lit("Left Shift"),str8_lit("Right Shift"),str8_lit("Left Control"),str8_lit("Right Control"),str8_lit("Left Windows"),str8_lit("Right Windows"),str8_lit("Left Alt"),str8_lit("Right Alt"),str8_lit("Apps"),
	str8_lit("Insert"),str8_lit("Delete"),str8_lit("Home"),str8_lit("End"),str8_lit("Page Up"),str8_lit("Page Down"),str8_lit("Print Screen"),str8_lit("Scroll Lock"),str8_lit("Pause Break"),
	str8_lit("Numpad 0"),str8_lit("Numpad 1"),str8_lit("Numpad 2"),str8_lit("Numpad 3"),str8_lit("Numpad 4"),str8_lit("Numpad 5"),str8_lit("Numpad 6"),str8_lit("Numpad 7"),str8_lit("Numpad 8"),str8_lit("Numpad 9"),
	str8_lit("Numpad Multiply"),str8_lit("Numpad Divide"),str8_lit("Numpad Plus"),str8_lit("Numpad Minus"),str8_lit("Numpad Period"),str8_lit("Num Lock"),
	str8_lit("Mouse Left"),str8_lit("Mouse Right"),str8_lit("Mouse Middle"),str8_lit("Mouse 4"),str8_lit("Mouse 5"),str8_lit("Mouse 6"),str8_lit("Mouse 7"),str8_lit("Mouse 8"),
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