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
	void   GrowArena(Arena* arena);
	void   DeleteArena(Arena* arena);
	
	//general memory functions
	void* Allocate(upt bytes);
	void* TempAllocate(upt bytes);
	void* Reallocate(void* ptr, upt bytes);
	void  ZeroFree(void* ptr);
	
	void Init(upt main_size, upt temp_size);
	void Update();
};

struct DeshiAllocator{
	void* reserve (upt bytes,              void* ctx=0){return Memory::Allocate(bytes);}
	void  commit  (void* ptr, upt bytes,   void* ctx=0){}
	void  decommit(void* ptr, upt bytes,   void* ctx=0){}
	void  release (void* ptr, upt bytes=0, void* ctx=0){Memory::ZeroFree(ptr);};
	void* resize  (void* ptr, upt bytes,   void* ctx=0){return Memory::Reallocate(ptr,bytes);}
};

#endif //DESHI_MEMORY_H
