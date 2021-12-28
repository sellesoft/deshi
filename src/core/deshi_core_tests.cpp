#include <typeinfo>
#include <cstdio>
#include <ctime>
#include "../defines.h"

#define DESHI_TEST_CORE_TODO(name) printf("[DESHI-TEST] TODO:   core/"name"\n")
#define DESHI_TEST_CORE_PASSED(name) printf("[DESHI-TEST] PASSED: core/"name"\n")

#include "armature.h"
function void TEST_deshi_core_armature(){
	DESHI_TEST_CORE_TODO("armature");
}

#include "camera.h"
function void TEST_deshi_core_camera(){
	DESHI_TEST_CORE_TODO("camera");
}

#include "commands.h"
function void TEST_deshi_core_commands(){
	DESHI_TEST_CORE_TODO("commands");
}

#include "console.h"
function void TEST_deshi_core_console(){
	DESHI_TEST_CORE_TODO("console");
}

#include "font.h"
function void TEST_deshi_core_font(){
	DESHI_TEST_CORE_TODO("font");
}

#include "input.h"
function void TEST_deshi_core_input(){
	DESHI_TEST_CORE_TODO("input");
}

#include "io.h"
function void TEST_deshi_core_io(){
	DESHI_TEST_CORE_TODO("io");
}

#include "logger.h"
function void TEST_deshi_core_logging(){
	DESHI_TEST_CORE_TODO("logging");
}

#include "memory.h"
function void TEST_deshi_core_memory(){
	using namespace Memory;
	ArenaHeap*   arena_heap = ExposeArenaHeap();
	GenericHeap* generic_heap = ExposeGenericHeap();
	Arena*       temp_arena = ExposeTempArena();
#if DESHI_INTERNAL
	Arena* naming_arena = DEBUG_ExposeAddressNamingArena();
#endif //DESHI_INTERNAL
	
	//arena creation
	Arena* arena1 = CreateArena(Kilobytes(4));
	AssertAlways(arena1);
	AssertAlways(arena1->start);
	AssertAlways(arena1->cursor);
	AssertAlways(arena1->used == 0);
	AssertAlways(arena1->size >= Kilobytes(4));
	
	//arena deletion
	DeleteArena(arena1);
	
	//deleting an arena in between two arenas
	arena1 = CreateArena(Kilobytes(8));
	Arena* arena2 = CreateArena(Kilobytes(16));
	Arena* arena3 = CreateArena(Kilobytes(8));
	DeleteArena(arena2);
	
	//clearing an arena
	memset(arena1->start, '3', 1024);
	arena1->cursor += 1024;
	arena1->used += 1024;
	ClearArena(arena1);
	AssertAlways(*arena1->start == 0);
	AssertAlways(arena1->cursor == arena1->start);
	AssertAlways(arena1->used == 0);
	
	//filling the gap with arenas
	arena2 = CreateArena(Kilobytes(4));
	Arena* arena4 = CreateArena(Kilobytes(4));
	
	//arenas should merge right then left
	DeleteArena(arena1);
	DeleteArena(arena2);
	DeleteArena(arena3);
	DeleteArena(arena4);
	
	//generic allocation creation and deletion
	char* string1 = (char*)Allocate(64);
	forI(52){ string1[i] = 'a'+char(i); }
	char* string2 = (char*)Allocate(256);
	char* string3 = (char*)Allocate(56);
	memset(string2, '2', 128);
	memset(string3, '3', 50);
	ZeroFree(string2);
	ZeroFree(string1);
	
	//big memory generic allocation creation and deletion (should make its own arena)
	void* big_block = Allocate(Kilobytes(512));
	memset(big_block, 7, Kilobytes(64));
	ZeroFree(string3);
	ZeroFree(big_block);
	
	//arena growing with nothing as last node
	arena1 = CreateArena(Kilobytes(4));
	arena1 = GrowArena(arena1, Kilobytes(4));
	
	//arena growing into a gap
	arena2 = CreateArena(Kilobytes(8));
	arena3 = CreateArena(Kilobytes(8));
	DeleteArena(arena2);
	arena1 = GrowArena(arena1, Kilobytes(4));
	
	//grow arena but it needs to move
	arena1 = GrowArena(arena1, Kilobytes(16));
	
	//delete grown arena
	DeleteArena(arena1);
	DeleteArena(arena3);
	
	//generic reallocation of a big block
	big_block = Allocate(Megabytes(8));
	memset(big_block, 7, Kilobytes(1));
	big_block = Reallocate(big_block, Megabytes(16));
	ZeroFree(big_block);
	
	//generic reallocation of a big block where new size is less than previous
	big_block = Allocate(Megabytes(8));
	memset(big_block, 7, Kilobytes(1));
	big_block = Reallocate(big_block, Megabytes(4));
	ZeroFree(big_block);
	
	//generic reallocation of a big block where new size is the same as previous
	big_block = Allocate(Megabytes(8));
	memset(big_block, 7, Kilobytes(1));
	big_block = Reallocate(big_block, Megabytes(8));
	ZeroFree(big_block);
	
	//generic reallocation of small block into big block
	string1 = (char*)Allocate(64);
	memset(string1, '1', 32);
	string1 = (char*)Reallocate(string1, Kilobytes(512));
	ZeroFree(string1);
	
	//generic reallocation with nothing to the right
	string1 = (char*)Allocate(64);
	memset(string1, '1', 32);
	string1 = (char*)Reallocate(string1, 128);
	ZeroFree(string1);
	
	//generic reallocation with nothing to the right where new size is less than previous
	string1 = (char*)Allocate(64);
	memset(string1, '1', 48);
	string1 = (char*)Reallocate(string1, 32);
	ZeroFree(string1);
	
	//generic rellocation with stuff to the right and needs to move
	string1 = (char*)Allocate(64);
	memset(string1, '1', 32);
	string2 = (char*)Allocate(64);
	memset(string2, '2', 32);
	string1 = (char*)Reallocate(string1, 128);
	ZeroFree(string1);
	ZeroFree(string2);
	
	//generic rellocation with stuff to the right where new size is less than previous
	string1 = (char*)Allocate(64);
	memset(string1, '1', 48);
	string2 = (char*)Allocate(64);
	memset(string2, '2', 48);
	string1 = (char*)Reallocate(string1, 32);
	string3 = (char*)Allocate(16);
	memset(string3, '3', 8);
	ZeroFree(string1);
	ZeroFree(string2);
	ZeroFree(string3);
	
	//generic rellocation with stuff to the right where new size is the same as previous
	string1 = (char*)Allocate(64);
	memset(string1, '1', 32);
	string2 = (char*)Allocate(64);
	memset(string2, '2', 32);
	string1 = (char*)Reallocate(string1, 64);
	ZeroFree(string1);
	ZeroFree(string2);
	
#if DESHI_INTERNAL
	//default names
	AssertAlways(equals(cstr_lit("Arena Heap"), DEBUG_GetAddressName(arena_heap)));
	AssertAlways(equals(cstr_lit("Generic Heap"), DEBUG_GetAddressName(generic_heap)));
	AssertAlways(equals(cstr_lit("Temp Arena"), DEBUG_GetAddressName(temp_arena)));
	AssertAlways(equals(cstr_lit("Naming Arena"), DEBUG_GetAddressName(naming_arena)));
	
	//removing a name
	DEBUG_SetAddressName(naming_arena, {});
	cstring name = DEBUG_GetAddressName(naming_arena);
	AssertAlways(name.str == 0 && name.count == 0);
	
	//setting a name
	DEBUG_SetAddressName(naming_arena, cstr_lit("Naming Arena"));
	AssertAlways(equals(cstr_lit("Naming Arena"), DEBUG_GetAddressName(naming_arena)));
	
	//renaming
	DEBUG_SetAddressName(naming_arena, cstr_lit("Address Naming Arena"));
	AssertAlways(equals(cstr_lit("Address Naming Arena"), DEBUG_GetAddressName(naming_arena)));
	
	DEBUG_SetAddressName(naming_arena, cstr_lit("Naming Arena"));
	AssertAlways(equals(cstr_lit("Naming Arena"), DEBUG_GetAddressName(naming_arena)));
#endif //DESHI_INTERNAL
	
	/*
//run out of generic memory SHOULD CRASH/ERROR
	for(u32 i = 0; i < -1; ++i){
		Allocate(Megabytes(1));
	}
	*/
	
	DESHI_TEST_CORE_TODO("memory");
}

#include "renderer.h"
function void TEST_deshi_core_renderer(){
	DESHI_TEST_CORE_TODO("renderer");
}

#include "storage.h"
function void TEST_deshi_core_storage(){
	DESHI_TEST_CORE_TODO("storage");
}

#include "time.h"
function void TEST_deshi_core_time(){
	DESHI_TEST_CORE_TODO("time");
}

#include "ui.h"
function void TEST_deshi_core_ui(){
	DESHI_TEST_CORE_TODO("ui");
}

#include "window.h"
function void TEST_deshi_core_window(){
	DESHI_TEST_CORE_TODO("window");
}

function void TEST_deshi_core(){
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