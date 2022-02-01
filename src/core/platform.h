#pragma once
#ifndef DESHI_PLATFORM_H
#define DESHI_PLATFORM_H

#include "../utils/unicode.h"

//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @Module API

//Loads a module from 'module_path'
//  Windows: calls LoadLibrary()
//  Linux: TODO
//  Mac: TODO
void* platform_load_module(const char* module_path);
void* platform_load_module(str16 module_path);

//Unloads a previously loaded module (load-time or run-time)
//  Windows: calls FreeLibrary()
//  Linux: TODO
//  Mac: TODO
void  platform_free_module(void* module);

//Returns a procedure or variable pointer from the 'module' with 'symbol_name'
//  Windows: calls GetProcAddress()
//  Linux: TODO
//  Mac: TODO
typedef void (*platform_symbol)(void);
platform_symbol platform_get_module_symbol(void* module, const char* symbol_name);
platform_symbol platform_get_module_symbol(void* module, str16 symbol_name);

#endif //DESHI_PLATFORM_H