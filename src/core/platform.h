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
@platform_monitor
  platform_monitor_infos() -> MonitorInfo*
@platform_processor
  platform_processor_info() -> ProcessorInfo
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

external struct MonitorInfo{
  u8 name[256]; //null terminated
	u8 index;
	u16 refresh_rate;
	u32 resolution_x;
	u32 resolution_y;
};

external struct ProcessorInfo{
	u8 name[65]; //null terminated
	u8 type; //0 = x86
	
	union{
		struct{ //64 bits
			//os enabled
			u32 os_avx : 1;
			u32 os_avx512 : 1;
			
			//vendor
			u32 amd : 1;
			u32 intel : 1;
			
			//128 bit
			u32 sse : 1;
			u32 sse2 : 1;
			u32 sse3 : 1;
			u32 ssse3 : 1;
			u32 sse41 : 1;
			u32 sse42 : 1;
			u32 sse4a : 1;
			u32 aes : 1;
			u32 sha : 1;
			
			//256 bit
			u32 avx : 1;
			u32 xop : 1;
			u32 fma3 : 1;
			u32 fma4 : 1;
			u32 avx2 : 1;
			
			//512 bit
			u32 avx512_f : 1;
			u32 avx512_cd : 1;
			
			//knights landing
			u32 avx512_pf : 1;
			u32 avx512_er : 1;
			
			//skylake
			u32 avx512_vl : 1;
			u32 avx512_bw : 1;
			u32 avx512_dq : 1;
			
			//cannon lake
			u32 avx512_ifma : 1;
			u32 avx512_vbmi : 1;
			
			//knights mill
			u32 avx512_vpopcntdq : 1;
			u32 avx512_4vnniw : 1;
			u32 avx512_4fmaps : 1;
			
			//cascade lake
			u32 avx512_vnni : 1;
			
			//cooper lake
			u32 avx512_bf16 : 1;
			
			//ice lake
			u32 avx512_vmbi2 : 1;
			u32 gfni : 1;
			u32 vaes : 1;
			u32 avx512_vpclmul : 1;
			u32 avx512_bitalg : 1;
			
			u32 padding : 27;
		}x86;
	};
};
static_assert(sizeof(ProcessorInfo) == 76, "ProcessorInfo has been resized; ensure it's padded correctly.");


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
//  Linux: calls usleep()
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
//  Windows: calls LoadLibrary()
//  Linux: calls dlopen()
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
//// @platform_monitor


//Returns a temporary array of MonitorInfo structs
//  Windows: calls EnumDisplayDevices(), EnumDisplaySettings()
//  Linux: TODO
//  Mac: TODO
external MonitorInfo* platform_monitor_infos();


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @platform_processor


//Returns a ProcessorInfo struct
//  Windows: calls __cpuid()
//  Linux: TODO
//  Mac: TODO
external ProcessorInfo platform_processor_info();


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
// Windows: calls CreateToolhelp32Snapshot(), Process32Next(), OpenProcess()
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
//
//  Windows: calls ReadProcessMemory()
//  Linux: TODO
//  Mac: TODO
//TODO(sushi) error codes
external u64 platform_process_read(Process p, upt address, void* out, upt size);

//Writes some data
//       p: a process initialized using platform_get_process_by_name
// address: address in process to write to
//    data: data to be written to the process
//    size: size of data to be written
// returns 0 if write failed, non 0 otherwise
//
//  Windows: calls WriteProcessMemory()
//  Linux: TODO
//  Mac: TODO
//TODO(sushi) error codes
external u64 platform_process_write(Process p, upt address, void* data, upt size);


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @platform_memory


//Reserves and commits memory from the OS; returns zero if it fails
//  Windows: calls VirtualAlloc()
//  Linux: calls mmap()
//  Mac: TODO
external void* platform_allocate_memory(void* address, upt size);

//Releases some memory from the OS; returns zero if it fails
//  Windows: calls VirtualFree()
//  Linux: calls munmap()
//  Mac: TODO
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