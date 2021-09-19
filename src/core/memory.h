#pragma once
#ifndef DESHI_MEMORY_H
#define DESHI_MEMORY_H

#include "../defines.h"
#include <cstdlib>
#include <malloc.h>

////////////////////
//// @interface ////
////////////////////
typedef void* (*Allocator)(upt bytes);

namespace Memory{
	local u8* main_arena_start  = 0;
	local u8* main_arena_cursor = 0;
	local upt main_arena_size   = 0;
	local upt main_arena_used   = 0;
	
	local u8* temp_arena_start  = 0;
	local u8* temp_arena_cursor = 0;
	local upt temp_arena_size   = 0;
	local upt temp_arena_used   = 0;
	
	void* Allocate(upt bytes);
	void* TempAllocate(upt bytes);
	void  ZeroFree(void* ptr);
	
	void Init(upt main_size, upt temp_size);
	void Update();
};

inline void* 
alloc(upt bytes){
	return Memory::Allocate(bytes);
}

inline void* 
talloc(upt bytes){
	return Memory::TempAllocate(bytes);
}

inline void
zfree(void* ptr){
	return Memory::ZeroFree(ptr);
}

//TODO(delle) use OS allocation funcs after platform layers are setup
//TODO(delle) store and check for free regions between allocations
/////////////////////////
//// @implementation ////
/////////////////////////
inline void* Memory::
Allocate(upt bytes){ //!Incomplete
	if(bytes == 0) return 0;
	Assert(main_arena_used+bytes <= main_arena_size);
	
	void* result = main_arena_cursor+sizeof(upt);
	*(upt*)main_arena_cursor = bytes;
	main_arena_cursor += bytes+sizeof(upt);
	main_arena_used   += bytes+sizeof(upt);
	return result;
}

inline void* Memory::
TempAllocate(upt bytes){ //!Incomplete
	if(bytes == 0) return 0;
	Assert(temp_arena_used+bytes <= temp_arena_size);
	
	void* result = temp_arena_cursor+sizeof(upt);
	*(upt*)temp_arena_cursor = bytes;
	temp_arena_cursor += bytes+sizeof(upt);
	temp_arena_used   += bytes+sizeof(upt);
	return result;
}

inline void Memory::
ZeroFree(void* ptr){ //!Incomplete
	if(ptr == 0) return;
	Assert(ptr >= main_arena_start && ptr <= main_arena_start+main_arena_used);
}

inline void Memory::
Init(upt main_size, upt temp_size){
	main_arena_start  = (u8*)calloc(1, main_size);
	main_arena_cursor = main_arena_start;
	main_arena_size   = main_size;
	main_arena_used   = 0;
	
	temp_arena_start  = (u8*)calloc(1, temp_size);
	temp_arena_cursor = temp_arena_start;
	temp_arena_size   = temp_size;
	temp_arena_used   = 0;
}

inline void Memory::
Update(){
	memset(temp_arena_start, 0, temp_arena_used);
	temp_arena_cursor = temp_arena_start;
	temp_arena_used = 0;
}

#endif //DESHI_MEMORY_H
