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
#ifndef DESHI_PLATFORM_H
#define DESHI_PLATFORM_H
#include "kigu/common.h"
#include "kigu/unicode.h"
#include "math/vector.h"


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @platform_types


external struct Process{
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


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @platform_memory


//Reserves and commits memory from the OS; returns zero if it fails
//  Windows: VirtualAlloc(address, size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE)
//    Linux: mmap(address, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0)
//      Mac: TODO
external void* platform_allocate_memory(void* address, upt size);

//Releases some memory from the OS; returns zero if it fails
//  Windows: VirtualFree(address, 0, MEM_RELEASE)
//    Linux: munmap(address, size)
//      Mac: TODO
external b32 platform_deallocate_memory(void* address, upt size);


#endif //DESHI_PLATFORM_H
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#if defined(DESHI_IMPLEMENTATION) && !defined(DESHI_PLATFORM_IMPL)
#define DESHI_PLATFORM_IMPL


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


#endif //DESHI_IMPLEMENTATION