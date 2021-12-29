#include <typeinfo>
#include <cstdio>
#include <ctime>
#include "../defines.h"

#define DESHI_TEST_CORE_TODO(name) printf("[DESHI-TEST] TODO:   core/"name"\n")
#define DESHI_TEST_CORE_PASSED(name) printf("[DESHI-TEST] PASSED: core/"name"\n")

#include "armature.h"
local void TEST_deshi_core_armature(){
	DESHI_TEST_CORE_TODO("armature");
}

#include "camera.h"
local void TEST_deshi_core_camera(){
	DESHI_TEST_CORE_TODO("camera");
}

#include "commands.h"
local void TEST_deshi_core_commands(){
	DESHI_TEST_CORE_TODO("commands");
}

#include "console.h"
local void TEST_deshi_core_console(){
	DESHI_TEST_CORE_TODO("console");
}

#include "font.h"
local void TEST_deshi_core_font(){
	DESHI_TEST_CORE_TODO("font");
}

#include "input.h"
local void TEST_deshi_core_input(){
	DESHI_TEST_CORE_TODO("input");
}

#include "io.h"
local void TEST_deshi_core_io(){
	DESHI_TEST_CORE_TODO("io");
}

#include "logger.h"
local void TEST_deshi_core_logging(){
	DESHI_TEST_CORE_TODO("logging");
}

#include "memory.h"
local void TEST_deshi_core_memory(){
	ArenaHeap*   arena_heap = memory_expose_arena_heap();
	GenericHeap* generic_heap = memory_expose_generic_heap();
	Arena*       temp_arena = memory_expose_temp_arena();
#if DESHI_INTERNAL
	Arena* naming_arena = memory_expose_naming_arena();
#endif //DESHI_INTERNAL
	
	//arena creation
	Arena* arena1 = memory_create_arena(Kilobytes(4));
	AssertAlways(arena1);
	AssertAlways(arena1->start);
	AssertAlways(arena1->cursor);
	AssertAlways(arena1->used == 0);
	AssertAlways(arena1->size >= Kilobytes(4));
	
	//arena deletion
	memory_delete_arena(arena1);
	
	//deleting an arena in between two arenas
	arena1 = memory_create_arena(Kilobytes(8));
	Arena* arena2 = memory_create_arena(Kilobytes(16));
	Arena* arena3 = memory_create_arena(Kilobytes(8));
	memory_delete_arena(arena2);
	
	//clearing an arena
	memset(arena1->start, '3', 1024);
	arena1->cursor += 1024;
	arena1->used += 1024;
	memory_clear_arena(arena1);
	AssertAlways(*arena1->start == 0);
	AssertAlways(arena1->cursor == arena1->start);
	AssertAlways(arena1->used == 0);
	
	//filling the gap with arenas
	arena2 = memory_create_arena(Kilobytes(4));
	Arena* arena4 = memory_create_arena(Kilobytes(4));
	
	//arenas should merge right then left
	memory_delete_arena(arena1);
	memory_delete_arena(arena2);
	memory_delete_arena(arena3);
	memory_delete_arena(arena4);
	
	//generic allocation creation and deletion
	char* string1 = (char*)memory_alloc(64);
	forI(52){ string1[i] = 'a'+char(i); }
	char* string2 = (char*)memory_alloc(256);
	char* string3 = (char*)memory_alloc(56);
	memset(string2, '2', 128);
	memset(string3, '3', 50);
	memory_zfree(string2);
	memory_zfree(string1);
	
	//big memory generic allocation creation and deletion (should make its own arena)
	void* big_block = memory_alloc(Kilobytes(512));
	memset(big_block, 7, Kilobytes(64));
	memory_zfree(string3);
	memory_zfree(big_block);
	
	//arena growing with nothing as last node
	arena1 = memory_create_arena(Kilobytes(4));
	arena1 = memory_grow_arena(arena1, Kilobytes(4));
	
	//arena growing into a gap
	arena2 = memory_create_arena(Kilobytes(8));
	arena3 = memory_create_arena(Kilobytes(8));
	memory_delete_arena(arena2);
	arena1 = memory_grow_arena(arena1, Kilobytes(4));
	
	//grow arena but it needs to move
	arena1 = memory_grow_arena(arena1, Kilobytes(16));
	
	//delete grown arena
	memory_delete_arena(arena1);
	memory_delete_arena(arena3);
	
	//generic reallocation of a big block
	big_block = memory_alloc(Megabytes(8));
	memset(big_block, 7, Kilobytes(1));
	big_block = memory_realloc(big_block, Megabytes(16));
	memory_zfree(big_block);
	
	//generic reallocation of a big block where new size is less than previous
	big_block = memory_alloc(Megabytes(8));
	memset(big_block, 7, Kilobytes(1));
	big_block = memory_realloc(big_block, Megabytes(4));
	memory_zfree(big_block);
	
	//generic reallocation of a big block where new size is the same as previous
	big_block = memory_alloc(Megabytes(8));
	memset(big_block, 7, Kilobytes(1));
	big_block = memory_realloc(big_block, Megabytes(8));
	memory_zfree(big_block);
	
	//generic reallocation of small block into big block
	string1 = (char*)memory_alloc(64);
	memset(string1, '1', 32);
	string1 = (char*)memory_realloc(string1, Kilobytes(512));
	memory_zfree(string1);
	
	//generic reallocation with nothing to the right
	string1 = (char*)memory_alloc(64);
	memset(string1, '1', 32);
	string1 = (char*)memory_realloc(string1, 128);
	memory_zfree(string1);
	
	//generic reallocation with nothing to the right where new size is less than previous
	string1 = (char*)memory_alloc(64);
	memset(string1, '1', 48);
	string1 = (char*)memory_realloc(string1, 32);
	memory_zfree(string1);
	
	//generic rellocation with stuff to the right and needs to move
	string1 = (char*)memory_alloc(64);
	memset(string1, '1', 32);
	string2 = (char*)memory_alloc(64);
	memset(string2, '2', 32);
	string1 = (char*)memory_realloc(string1, 128);
	memory_zfree(string1);
	memory_zfree(string2);
	
	//generic rellocation with stuff to the right where new size is less than previous
	string1 = (char*)memory_alloc(64);
	memset(string1, '1', 48);
	string2 = (char*)memory_alloc(64);
	memset(string2, '2', 48);
	string1 = (char*)memory_realloc(string1, 32);
	string3 = (char*)memory_alloc(16);
	memset(string3, '3', 8);
	memory_zfree(string1);
	memory_zfree(string2);
	memory_zfree(string3);
	
	//generic rellocation with stuff to the right where new size is the same as previous
	string1 = (char*)memory_alloc(64);
	memset(string1, '1', 32);
	string2 = (char*)memory_alloc(64);
	memset(string2, '2', 32);
	string1 = (char*)memory_realloc(string1, 64);
	memory_zfree(string1);
	memory_zfree(string2);
	
#if DESHI_INTERNAL
	//default names
	AssertAlways(equals(cstr_lit("Arena Heap"),   memory_get_address_name(arena_heap)));
	AssertAlways(equals(cstr_lit("Generic Heap"), memory_get_address_name(generic_heap)));
	AssertAlways(equals(cstr_lit("Temp Arena"),   memory_get_address_name(temp_arena)));
	AssertAlways(equals(cstr_lit("Naming Arena"), memory_get_address_name(naming_arena)));
	
	//removing a name
	memory_set_address_name(naming_arena, {}, 0);
	cstring name = memory_get_address_name(naming_arena);
	AssertAlways(name.str == 0 && name.count == 0);
	
	//setting a name
	memory_set_address_name(naming_arena, cstr_lit("Naming Arena"), 0);
	AssertAlways(equals(cstr_lit("Naming Arena"), memory_get_address_name(naming_arena)));
	
	//renaming
	memory_set_address_name(naming_arena, cstr_lit("Address Naming Arena"), 0);
	AssertAlways(equals(cstr_lit("Address Naming Arena"), memory_get_address_name(naming_arena)));
	
	memory_set_address_name(naming_arena, cstr_lit("Naming Arena"), 0);
	AssertAlways(equals(cstr_lit("Naming Arena"), memory_get_address_name(naming_arena)));
#endif //DESHI_INTERNAL
	
	/*
//run out of generic memory SHOULD CRASH/ERROR
	for(u32 i = 0; i < -1; ++i){
		memory_alloc(Megabytes(1));
	}
	*/
	
	DESHI_TEST_CORE_TODO("memory");
}

#include "renderer.h"
local void TEST_deshi_core_renderer(){
	DESHI_TEST_CORE_TODO("renderer");
}

#include "storage.h"
local void TEST_deshi_core_storage(){
	DESHI_TEST_CORE_TODO("storage");
}

#include "time.h"
local void TEST_deshi_core_time(){
	DESHI_TEST_CORE_TODO("time");
}

#include "ui.h"
local void TEST_deshi_core_ui(){
	DESHI_TEST_CORE_TODO("ui");
}

#include "window.h"
local void TEST_deshi_core_window(){
	DESHI_TEST_CORE_TODO("window");
}

local void TEST_deshi_core(){
	TEST_deshi_core_armature();
	TEST_deshi_core_camera();
	TEST_deshi_core_commands();
	TEST_deshi_core_console();
	TEST_deshi_core_font();
	TEST_deshi_core_input();
	TEST_deshi_core_io();
	TEST_deshi_core_logging();
	TEST_deshi_core_memory();
	TEST_deshi_core_renderer();
	TEST_deshi_core_storage();
	TEST_deshi_core_time();
	TEST_deshi_core_ui();
	TEST_deshi_core_window();
}