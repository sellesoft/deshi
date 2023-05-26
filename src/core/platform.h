/* deshi Platform Module
Notes:
  These functions simply call into the platform backend to do work.
  The variables at the bottom are shared across all backends.

Index:
@platform_types
  Process
@platform_status
  platform_init() -> void
  platform_update() -> void
  platform_sleep(u32 time) -> void
@platform_cursor
  platform_set_cursor_position(s32 x, s32 y) -> void
  platform_get_cursor_position() -> vec2i
@platform_modules
  platform_load_module(str8 module_path) -> void*
  platform_free_module(void* module) -> void
  platform_get_module_symbol(void* module, const char* symbol_name) -> platform_symbol
@platform_clipboard
  platform_get_clipboard() -> str8
  platform_set_clipboard(str8 text) -> void
@platform_process
  platform_get_process_by_name(str8 name) -> Process
  platform_process_read(Process p, upt address, void* out, upt size) -> u64
  platform_process_write(Process p, upt address, void* data, upt size) -> u64
@platform_shared_variables
@platform_shared_functions
*/


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma once
#ifndef DESHI_PLATFORM_H
#define DESHI_PLATFORM_H
#include "kigu/common.h"
#include "kigu/unicode.h"
#include "math/vector.h"

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @platform_types
struct Process{
	void* handle;
};


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @platform_status
//Initializes the platform required for other modules
external void platform_init();

//Updates the platform required for other modules
external b32 platform_update();

//Signals to exit the application
external void platform_exit();

//Sets whether the application requires 2 exits to be send in order to close the application
//    eg: so you can notify a person to save their work before exiting
external void platform_fast_application_exit(b32 exit_fast);

//Sleep for `time` milliseconds
//  Windows: calls Sleep()
//  Linux: TODO
//  Mac: TODO
external void platform_sleep(u32 time);

//Sets the cursor position to (`x`,`y`) in `Screen Space` of the active monitor
//  Windows: calls SetCursorPos()
//  Linux: TODO
//  Mac: TODO
external void platform_cursor_position(s32 x, s32 y);
FORCE_INLINE void platform_cursor_position(vec2i position){ platform_cursor_position(position.x, position.y); };
FORCE_INLINE void platform_cursor_position(vec2 position){ platform_cursor_position((s32)position.x, (s32)position.y); };


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @platform_modules
//Loads a module from `module_path`
//  Windows: calls LoadLibraryW()
//  Linux: TODO
//  Mac: TODO
external void* platform_load_module(str8 module_path);


//Unloads a previously loaded `module` (load-time or run-time)
//  Windows: calls FreeLibrary()
//  Linux: TODO
//  Mac: TODO
external void platform_free_module(void* module);

//Returns a procedure or variable pointer from the `module` with `symbol_name`
//  Windows: calls GetProcAddress()
//  Linux: TODO
//  Mac: TODO
external void* platform_get_module_symbol(void* module, const char* symbol_name);
#define platform_get_module_function(module,symbol_name,symbol_sig) (GLUE(symbol_sig,__sig)*) platform_get_module_symbol((module),(symbol_name))


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @platform_clipboard
//Returns a temporary utf8 string of the clipboard
//  Windows: calls OpenClipboard(), GetClipboardData(), CloseClipboard()
//  Linux: TODO
//  Mac: TODO
external str8 platform_get_clipboard();

//Sets the clipboard to the utf8 string `text`
//  Windows: calls OpenClipboard(), EmptyClipboard(), SetClipboardData(), CloseClipboard()
//  Linux: TODO
//  Mac: TODO
external void platform_set_clipboard(str8 text);

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @platform_process

//Gets a process by name
// Windows: uses CreateToolhelp32Snapshot and iterates it's list of processes; opens with PROCESS_ALL_ACCESS TODO(sushi) options for this
//   Linux: TODO
//     Mac: TODO
//TODO(sushi) error codes
external Process platform_get_process_by_name(str8 name);

//Reads some data from a process into `out`
//       p: a process initialized using platform_get_process_by_name
// address: address in process to read from
//     out: an allocated buffer to write read data to
//    size: size to read
// returns 0 if read failed, non 0 otherwise
//TODO(sushi) error codes
external u64 platform_process_read(Process p, upt address, void* out, upt size);

//Writes some data
//       p: a process initialized using platform_get_process_by_name
// address: address in process to write to
//    data: data to be written to the process
//    size: size of data to be written
// returns 0 if write failed, non 0 otherwise
//TODO(sushi) error codes
external u64 platform_process_write(Process p, upt address, void* data, upt size);

#endif //DESHI_PLATFORM_H
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#if defined( DESHI_IMPLEMENTATION ) && !defined( DESHI_PLATFORM_IMPL )
#define DESHI_PLATFORM_IMPL
#include "file.h"
#include "window.h"
#include "kigu/arrayT.h"


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @platform_shared_variables
local b32 platform_exit_application = false;
local b32 platform_fast_exit = true;


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @platform_shared_functions
void
platform_exit(){DPZoneScoped;
	platform_exit_application = true;
}

void
platform_fast_application_exit(b32 exit_fast){DPZoneScoped;
	platform_fast_exit = exit_fast;
}

/*
//TODO(sushi) options for decoration colors
void DrawDecorations(Window* win){DPZoneScoped;
	s32 x=win->x, y=win->y;
	s32 width = win->width, height = win->height;
	s32 cwidth = win->cwidth, cheight = win->cheight;
	u32 decor = win->decorations;
	b32 hitset = 0;
	persist Font* decorfont = Storage::CreateFontFromFileBDF(str8_lit("gohufont-11.bdf")).second;
	render_set_active_surface(win);
	render_start_cmd2(render_decoration_layer_index(), 0, vec2::ZERO, vec2(width, height));
	
	if(Math::PointInRectangle(input_mouse_position(), vec2::ZERO, vec2(width, height))) win->hittest = HitTestClient;
	
	//minimal titlebar takes precedence over all other title bar flags
	if(HasFlag(decor, Decoration_MinimalTitlebar)){
		win->titlebarheight = 5;
		render_quad_filled2(vec2::ZERO, vec2(width, win->titlebarheight), Color_White);
		if(Math::PointInRectangle(input_mouse_position(), vec2::ZERO, vec2(width, win->titlebarheight))){ win->hittest = HitTestTitle; hitset = 1; }
	}else{
		if(HasFlag(decor, Decoration_Titlebar)){
			win->titlebarheight = 20;
			render_quad_filled2(vec2::ZERO, vec2(width, win->titlebarheight), color(60,60,60));
		}
		if(HasFlag(decor, Decoration_TitlebarTitle)){
			//!Incomplete
		}
	}
	
	if(HasFlag(decor, Decoration_MouseBorders)){
		vec2 mp = input_mouse_position();
		f32 distToAppear = 7;
		f32 borderSize = 1;
		vec2 
			lbpos = vec2::ZERO,
		rbpos = vec2(width - borderSize, 0),
		bbpos = vec2(0, height - borderSize),
		lbsiz = vec2(borderSize, height),
		rbsiz = vec2(borderSize, height),
		bbsiz = vec2(width, borderSize);
		color //TODO make these not show if the mouse goes beyond the window
			bcol = color(255,255,255, Remap(Clamp(mp.y, f32(cheight - distToAppear), f32(cheight - borderSize)), 0.f, 255.f, f32(cheight - distToAppear), f32(cheight - borderSize))),
		rcol = color(255,255,255, Remap(Clamp(mp.x, f32(cwidth - distToAppear), f32(cwidth - borderSize)), 0.f, 255.f, f32(cwidth - distToAppear), f32(cwidth - borderSize))),
		lcol = color(255,255,255, Remap(Clamp(cwidth - mp.x, f32(cwidth - distToAppear), f32(cwidth - borderSize)), 0.f, 255.f, f32(cwidth - distToAppear), f32(cwidth - borderSize)));
		
		if(!hitset && Math::PointInRectangle(mp, lbpos, lbsiz)){win->hittest = HitTestLeft;   hitset = 1;}
		else if(Math::PointInRectangle(mp, rbpos, rbsiz))      {win->hittest = HitTestRight;  hitset = 1;}
		else if(Math::PointInRectangle(mp, bbpos, bbsiz))      {win->hittest = HitTestBottom; hitset = 1;}
		
		render_quad_filled2(vec2::ZERO, vec2(borderSize, height), lcol);
		render_quad_filled2(vec2(width - borderSize, 0), vec2(borderSize, height), rcol);
		render_quad_filled2(vec2(0, height - borderSize), vec2(width, borderSize), bcol);
	}else if(HasFlag(decor, Decoration_Borders)){
		win->borderthickness = 2;
		f32 borderSize = win->borderthickness;
		vec2 
			tbpos = vec2::ZERO,
		lbpos = vec2::ZERO,
		rbpos = vec2(width - borderSize, 0),
		bbpos = vec2(0, height - borderSize),
		tbsiz = vec2(width, borderSize),
		lbsiz = vec2(borderSize, height),
		rbsiz = vec2(borderSize, height),
		bbsiz = vec2(width, borderSize);
		color 
			tcol = color(133,133,133),
		bcol = color(133,133,133),
		rcol = color(133,133,133),
		lcol = color(133,133,133);
		
		render_quad_filled2(lbpos, lbsiz, lcol);
		render_quad_filled2(rbpos, rbsiz, rcol);
		render_quad_filled2(bbpos, bbsiz, bcol);
		render_quad_filled2(tbpos, tbsiz, tcol);
	}
#if LOG_INPUTS
	Log("", win->hittest);
#endif
}
*/

#endif //DESHI_IMPLEMENTATION