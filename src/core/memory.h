#pragma once
#ifndef DESHI_MEMORY_H
#define DESHI_MEMORY_H

#include "../defines.h"
#include <cstdlib>

struct Arena{
	u8* start;
	u8* cursor;
	upt size;
	upt used;
};

struct Node{
	Node* next;
	Node* prev;
};

struct HeapNode{
	Arena arena;
	Node  order; //overall order
	Node  empty; //empty node order
};

struct Heap{
	u8*  start;
	u8*  cursor;
	upt  size;
	upt  used;
	Node order; //overall nodes
	Node empty; //empty nodes
};

namespace Memory{
	//arena functions
	Arena* CreateArena(upt bytes);
	void   DeleteArena(Arena* arena);
	
	//general memory functions
	void* Allocate(upt bytes);
	void* TempAllocate(upt bytes);
	void  ZeroFree(void* ptr);
	
	void Init(upt main_size, upt temp_size);
	void Update();
};

FORCE_INLINE void* alloc(upt bytes) { return Memory::Allocate(bytes); }
FORCE_INLINE void* talloc(upt bytes){ return Memory::TempAllocate(bytes); }
FORCE_INLINE void  zfree(void* ptr) { return Memory::ZeroFree(ptr); }

#endif //DESHI_MEMORY_H
