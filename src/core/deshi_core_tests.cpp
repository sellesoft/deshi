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
	DebugBreakpoint;
	
	//arena creation
	Arena* arena1 = Memory::CreateArena(Kilobytes(4));
	Assert(arena1);
	Assert(arena1->start);
	Assert(arena1->cursor);
	Assert(arena1->used == 0);
	Assert(arena1->size >= Kilobytes(4));
	
	//arena deletion
	Memory::DeleteArena(arena1);
	
	//deleting an arena in between two arenas
	arena1 = Memory::CreateArena(Kilobytes(8));
	Arena* arena2 = Memory::CreateArena(Kilobytes(16));
	Arena* arena3 = Memory::CreateArena(Kilobytes(8));
	Memory::DeleteArena(arena2);
	
	//filling the gap with arenas
	arena2 = Memory::CreateArena(Kilobytes(4));
	Arena* arena4 = Memory::CreateArena(Kilobytes(4));
	
	//arenas should merge right then left
	Memory::DeleteArena(arena1);
	Memory::DeleteArena(arena2);
	Memory::DeleteArena(arena3);
	Memory::DeleteArena(arena4);
	
	//generic allocation creation and deletion
	char* string1 = (char*)Memory::Allocate(64);
	forI(52){ string1[i] = 'a'+char(i); }
	char* string2 = (char*)Memory::Allocate(256);
	char* string3 = (char*)Memory::Allocate(56);
	memset(string2, '2', 128);
	memset(string3, '3', 50);
	Memory::ZeroFree(string2);
	Memory::ZeroFree(string1);
	
	//big memory generic allocation creation and deletion (should make its own arena)
	void* big_block = Memory::Allocate(Kilobytes(512));
	memset(big_block, 7, Kilobytes(64));
	Memory::ZeroFree(string3);
	Memory::ZeroFree(big_block);
	
	//arena growing with nothing as last node
	arena1 = Memory::CreateArena(Kilobytes(4));
	arena1 = Memory::GrowArena(arena1, Kilobytes(4));
	
	//arena growing into a gap
	arena2 = Memory::CreateArena(Kilobytes(8));
	arena3 = Memory::CreateArena(Kilobytes(8));
	Memory::DeleteArena(arena2);
	arena1 = Memory::GrowArena(arena1, Kilobytes(4));
	
	//grow arena but it needs to move
	arena1 = Memory::GrowArena(arena1, Kilobytes(16));
	
	//delete grown arena
	Memory::DeleteArena(arena1);
	Memory::DeleteArena(arena3);
	
	//generic reallocation of a big block
	big_block = Memory::Allocate(Megabytes(8));
	big_block = Memory::Reallocate(big_block, Megabytes(16));
	Memory::ZeroFree(big_block);
	
	//generic reallocation of a big block where new size is less than previous
	big_block = Memory::Allocate(Megabytes(8));
	big_block = Memory::Reallocate(big_block, Megabytes(4));
	Memory::ZeroFree(big_block);
	
	//generic reallocation of small block into big block
	string1 = (char*)Memory::Allocate(64);
	string1 = (char*)Memory::Reallocate(string1, Kilobytes(512));
	Memory::ZeroFree(string1);
	
	//generic reallocation with nothing to the right
	string1 = (char*)Memory::Allocate(64);
	string1 = (char*)Memory::Reallocate(string1, 128);
	Memory::ZeroFree(string1);
	
	//generic reallocation with nothing to the right where new size is less than previous
	string1 = (char*)Memory::Allocate(64);
	string1 = (char*)Memory::Reallocate(string1, 32);
	Memory::ZeroFree(string1);
	
	//generic rellocation with stuff to the right and needs to move
	string1 = (char*)Memory::Allocate(64);
	string2 = (char*)Memory::Allocate(64);
	string1 = (char*)Memory::Reallocate(string1, 128);
	Memory::ZeroFree(string1);
	Memory::ZeroFree(string2);
	
	//generic rellocation with stuff to the right where new size is less than previous
	string1 = (char*)Memory::Allocate(64);
	string2 = (char*)Memory::Allocate(64);
	string1 = (char*)Memory::Reallocate(string1, 32);
	Memory::ZeroFree(string1);
	Memory::ZeroFree(string2);
	
	/*
//run out of generic memory SHOULD CRASH/ERROR
	for(u32 i = 0; i < -1; ++i){
		Memory::Allocate(Megabytes(1));
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