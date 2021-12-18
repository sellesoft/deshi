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
	upt  used;
	upt  size;
	Node order; //overall nodes
	Node empty; //empty nodes
};

namespace Memory{
	//creates an arena AT LEAST 'bytes' in size with all memory zeroed
	Arena* CreateArena(upt bytes);
	//grows the arena to AT LEAST 'new_size' in bytes
	//  returns a different pointer than was passed if the memory was moved
	//  if its memory was moved, pointers to its memory are no longer valid
	Arena* GrowArena(Arena* arena, upt new_size);
	void   DeleteArena(Arena* arena);
	
	void* Allocate(upt bytes);
	void* TempAllocate(upt bytes);
	//grows the allocation to AT LEAST 'new_size' in bytes if greater than previous allocation
	//  returns a different pointer than was passed if the memory was moved
	//  if its memory was moved, pointers to its memory are no longer valid
	void* Reallocate(void* ptr, upt new_size);
	void  ZeroFree(void* ptr);
	
	void Init(upt main_size, upt temp_size);
	void Update();
};

global_ Allocator deshi_allocator_{
	Memory::Allocate,
	Allocator_ChangeMemory_Noop,
	Allocator_ChangeMemory_Noop,
	Memory::ZeroFree,
	Memory::Reallocate
};
global_ Allocator* deshi_allocator = &deshi_allocator_;

function void* TempAllocator_Resize(void* ptr, upt bytes){void* a = Memory::TempAllocate(bytes); memcpy(a, ptr, *((upt*)ptr-1)); return a;}
global_ Allocator deshi_temp_allocator_{
	Memory::TempAllocate,
	Allocator_ChangeMemory_Noop,
	Allocator_ChangeMemory_Noop,
	Allocator_ReleaseMemory_Noop,
	TempAllocator_Resize
};
global_ Allocator* deshi_temp_allocator = &deshi_temp_allocator_;

#endif //DESHI_MEMORY_H
