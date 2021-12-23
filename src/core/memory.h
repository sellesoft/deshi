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

namespace Memory{
	struct ArenaHeapNode{
		Arena arena;
		Node  order; //overall order
		Node  empty; //empty node order
	};
	
	struct ArenaHeap{
		u8*  start;
		u8*  cursor;
		upt  used;
		upt  size;
		Node order; //overall nodes
		Node empty; //empty nodes
		b32  initialized;
	};
	
	struct Chunk{ //generic heap node
		Chunk* prev; //pointer to previous order chunk
		upt    size; //size of this chunk (including this var and above vars as overhead)
		Node   node; //user memory starts here when in use; points to free chunks when not
	};
	
	struct GenericHeap{
		u8*    start;
		u8*    cursor;
		upt    used;
		upt    size;
		Node   empty_nodes;
		Chunk* last_chunk;
		b32    initialized;
	};
	
	//creates an arena AT LEAST 'bytes' in size with all memory zeroed
	Arena* CreateArena(upt bytes);
	//grows the arena by AT LEAST 'bytes' in size
	//  returns a different pointer than was passed if the memory was moved
	//  if its memory was moved, pointers to its memory are no longer valid
	Arena* GrowArena(Arena* arena, upt bytes);
	void   ClearArena(Arena* arena);
	void   DeleteArena(Arena* arena);
	
	void* Allocate(upt bytes);
	//grows the allocation to AT LEAST 'new_size' in bytes if greater than previous allocation
	//  returns a different pointer than was passed if the memory was moved
	//  if its memory was moved, pointers to its memory are no longer valid
	//  if 'new_size' is zero, calls ZeroFree on 'ptr' and returns 0
	void* Reallocate(void* ptr, upt new_size);
	void  ZeroFree(void* ptr);
	
	void* TempAllocate(upt bytes);
	
	ArenaHeap*   ExposeArenaHeap();
	GenericHeap* ExposeGenericHeap();
	Arena*       ExposeTempArena();
	
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

void* TempAllocator_Resize(void* ptr, upt bytes);
global_ Allocator deshi_temp_allocator_{
	Memory::TempAllocate,
	Allocator_ChangeMemory_Noop,
	Allocator_ChangeMemory_Noop,
	Allocator_ReleaseMemory_Noop,
	TempAllocator_Resize
};
global_ Allocator* deshi_temp_allocator = &deshi_temp_allocator_;

#endif //DESHI_MEMORY_H
