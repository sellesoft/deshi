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

//struct that the user keeps track of to interact with an Arena in memory
struct Arena { 
	u8* start;
	u8* cursor;
	upt size;
	upt used;
};

namespace Memory{
	//general memory functions
	void* Allocate(upt bytes);
	void* TempAllocate(upt bytes);
	void  ZeroFree(void* ptr);
	void  ClearAll();

	//arena functions 
	Arena* CreateArena(upt bytes);
	void   DeleteArena(Arena* arena);

	//status functions 
	upt RemainingSpace();

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

//deshi's allocator struct for use with our container types
struct DeshiMemoryAllocator {
	void* allocate(upt bytes) {
		return Memory::Allocate(bytes);
	}

	void* callocate(upt count, upt size) {
		return Memory::Allocate(count * size);
	}

	void deallocate(void* ptr) {
		Memory::ZeroFree(ptr);
	}
};

#endif //DESHI_MEMORY_H
