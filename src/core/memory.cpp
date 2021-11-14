#include "memory.h"

//TODO(delle) use OS allocation funcs after platform layers are setup
//TODO(delle) store and check for free regions between allocations

local u8* main_arena_start = 0;
local u8* main_arena_cursor = 0;
local upt main_arena_size = 0;
local upt main_arena_used = 0;

local u8* temp_arena_start = 0;
local u8* temp_arena_cursor = 0;
local upt temp_arena_size = 0;
local upt temp_arena_used = 0;

#define MEMORY_MAX_ARENAS 500 //arbitrary, this could maybe be an argument on init?
local Arena* arena_dict_start;
local Arena* arena_dict_cursor; //this is always set to the first avaliable spot for an arena to be placed in the dictionary

//maybe wont work idk yet
// 
//internal struct to define where openings are in our main arena, this is used by alloc and making arenas
//it stores a pointer to the next opening
//currently what im going to do is prioritize larger opening over smaller ones
//so, if we delete an arena, and the space it opens up is larger than the current opening,
//we set the new opening's pointer to the current one, and then set the new one as the current one
//im not sure if this is better than trying to get rid of smaller space first
//struct Opening { 
//	u8* start;
//	upt size;
//	Opening* next;
//};
//#define MEMORY_MAX_OPENINGS MEMORY_MAX_ARENAS * 2
//local Opening* next_opening;


void* Memory::
Allocate(upt bytes) { //!Incomplete
	if (bytes == 0) return 0;
	Assert(main_arena_used + bytes <= main_arena_size);

	void* result = main_arena_cursor + sizeof(upt);
	*(upt*)main_arena_cursor = bytes;
	main_arena_cursor += bytes + sizeof(upt);
	main_arena_used += bytes + sizeof(upt);
	return result;
}

void* Memory::
TempAllocate(upt bytes) { //!Incomplete
	if (bytes == 0) return 0;
	Assert(temp_arena_used + bytes <= temp_arena_size);

	void* result = temp_arena_cursor + sizeof(upt);
	*(upt*)temp_arena_cursor = bytes;
	temp_arena_cursor += bytes + sizeof(upt);
	temp_arena_used += bytes + sizeof(upt);
	return result;
}

void Memory::
ZeroFree(void* ptr) { //!Incomplete
	if (ptr == 0) return;
	Assert(ptr >= main_arena_start && ptr <= main_arena_start + main_arena_used);
}

Arena* Memory::CreateArena(upt bytes) {
	
	
	return nullptr;
}

void Memory::DeleteArena(Arena* arena) {

}

void Memory::
Init(upt main_size, upt temp_size) {
	//NOTE I decided to calloc everything together so we know that its actually one giant block 
	// tell me if for some reason we shouldn't do this
	// so we have 
	//    | Main Arena | Temp Arena | Dictionary of Main Arena | Space reserved for Openings
	// 
	// this also means that we use two u8* and one Arena* (sorry)
	//
	main_arena_start = (u8*)calloc(1, main_size + temp_size + MEMORY_MAX_ARENAS * sizeof(Arena));
	main_arena_cursor = main_arena_start;
	main_arena_size = main_size;
	main_arena_used = 0;

	//temp_arena_start = (u8*)calloc(1, temp_size);
	temp_arena_start = main_arena_start + main_size;
	temp_arena_cursor = temp_arena_start;
	temp_arena_size = temp_size;
	temp_arena_used = 0;

	arena_dict_start = (Arena*)(temp_arena_start + temp_size);
}

void Memory::
Update() {
	memset(temp_arena_start, 0, temp_arena_used);
	temp_arena_cursor = temp_arena_start;
	temp_arena_used = 0;
}
