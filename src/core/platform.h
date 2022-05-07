#pragma once
#ifndef DESHI_PLATFORM_H
#define DESHI_PLATFORM_H
#include "kigu/unicode.h"


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @Generic API
//Initializes the platform required for other modules
void platform_init();

//Updates the platform required for other modules
void platform_update();


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @Module API
//Loads a module from `module_path`
//  Windows: calls LoadLibraryW()
//  Linux: TODO
//  Mac: TODO
void* platform_load_module(str8 module_path);

//Unloads a previously loaded `module` (load-time or run-time)
//  Windows: calls FreeLibrary()
//  Linux: TODO
//  Mac: TODO
void  platform_free_module(void* module);

//Returns a procedure or variable pointer from the `module` with `symbol_name`
//  Windows: calls GetProcAddress()
//  Linux: TODO
//  Mac: TODO
typedef void (*platform_symbol)(void);
platform_symbol platform_get_module_symbol(void* module, const char* symbol_name);


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @Clipboard API
//Returns a temporary utf8 string of the clipboard
//  Windows: calls OpenClipboard(), GetClipboardData(), CloseClipboard()
//  Linux: TODO
//  Mac: TODO
 str8 platform_get_clipboard();

//Sets the clipboard to the utf8 string `text`
//  Windows: calls OpenClipboard(), EmptyClipboard(), SetClipboardData(), CloseClipboard()
//  Linux: TODO
//  Mac: TODO
void platform_set_clipboard(str8 text);


#endif //DESHI_PLATFORM_H