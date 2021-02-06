#pragma once


//math constants
#define M_PI			3.14159265359f
#define M_E				2.71828182846f
#define M_TWOTHIRDS		0.66666666666f
#define M_ONETWELFTH	0.08333333333f

//conversions
#define TO_RADIANS (M_PI / 180.f)
#define TO_DEGREES (180.f / M_PI)

//number typedefs
typedef signed char		int8;
typedef signed short	int16;
typedef signed int		int32;
typedef signed long		int64;
typedef unsigned char	uint8;
typedef unsigned short	uint16;
typedef unsigned int	uint32;
typedef unsigned long	uint64;

//use ortho projection
#define USE_ORTHO false

//static defines
#define static_internal	static
#define local_persist	static
#define global_variable static

//input defines
#define INPUT_MOUSE_LEFT		0
#define INPUT_MOUSE_RIGHT		1
#define INPUT_MOUSE_MIDDLE		2
#define INPUT_MOUSE_4			3
#define INPUT_MOUSE_5			4

#ifdef KEYBOARD_LAYOUT_US_UK
#define INPUT_SEMICOLON		olc::OEM_1
#define INPUT_BACKSLASH		olc::OEM_2
#define INPUT_TILDE			olc::OEM_3
#define INPUT_LEFT_BRACKET	olc::OEM_4
#define INPUT_FORWARDSLASH	olc::OEM_5
#define INPUT_RIGHT_BRACKET	olc::OEM_6
#define INPUT_SINGLE_QUOTE	olc::OEM_7
#else //TODO(i,delle) add alternate keyboard formats
#define INPUT_SEMICOLON		olc::OEM_1
#define INPUT_BACKSLASH		olc::OEM_2
#define INPUT_TILDE			olc::OEM_3
#define INPUT_LEFT_BRACKET	olc::OEM_4
#define INPUT_FORWARDSLASH	olc::OEM_5
#define INPUT_RIGHT_BRACKET	olc::OEM_6
#define INPUT_SINGLE_QUOTE	olc::OEM_7
#endif

#define INPUT_NONE_HELD		0
#define INPUT_ANY_HELD		1
#define INPUT_CTRL_HELD		2
#define INPUT_SHIFT_HELD	4
//#define INPUT_ALT_HELD	8 //ALT not supported by PGE v2.09

//regex defines
//used in object spawning 
