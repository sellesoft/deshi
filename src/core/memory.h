#pragma once
#ifndef DESHI_MEMORY_H
#define DESHI_MEMORY_H

#include "../defines.h"

////////////////
//// @arena ////
////////////////
struct Arena{
	u8* start;
	u8* cursor;
	upt size;
	upt used;
};

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

//creates an arena AT LEAST 'size' in bytes with all memory zeroed
Arena* deshi__memory_arena_create(upt size, char* file, upt line);
#define memory_create_arena(size) deshi__memory_arena_create(size, __FILE__, __LINE__)

//grows the arena by AT LEAST 'size' in bytes
//  returns a different pointer than was passed if the memory was moved
//  if its memory was moved, pointers to its memory are no longer valid
Arena* deshi__memory_arena_grow(Arena* arena, upt size, char* file, upt line);
#define memory_grow_arena(arena, size) deshi__memory_arena_grow(arena, size, __FILE__, __LINE__)

//zeros the arena's memory and resets used amount to zero
void deshi__memory_arena_clear(Arena* arena, char* file, upt line);
#define memory_clear_arena(arena) deshi__memory_arena_clear(arena, __FILE__, __LINE__)

//deletes the arena and zeros its memory
void deshi__memory_arena_delete(Arena* arena, char* file, upt line);
#define memory_delete_arena(arena) deshi__memory_arena_delete(arena, __FILE__, __LINE__)

//exposes the internal arena heap
ArenaHeap* deshi__memory_arena_expose();
#define memory_expose_arena_heap() deshi__memory_arena_expose()


//////////////////
//// @generic ////
//////////////////
struct GenericHeapNode{
	GenericHeapNode* prev; //pointer to previous order chunk
	upt              size; //size of this chunk (including this var and above vars as overhead)
	Node             node; //user memory starts here when in use; points to free chunks when not
};

struct GenericHeap{
	u8*              start;
	u8*              cursor;
	upt              used;
	upt              size;
	Node             empty_nodes;
	GenericHeapNode* last_chunk;
	b32              initialized;
};

//allocates AT LEAST 'size' in bytes with all memory zeroed
void* deshi__memory_generic_allocate(upt size, char* file, upt line);
FORCE_INLINE void* deshi__memory_generic_allocate(upt size){return deshi__memory_generic_allocate(size, "", 0);}
#define memory_alloc(size) deshi__memory_generic_allocate(size, __FILE__, __LINE__)
#define memalloc(size) memory_alloc(size)

//resizes the allocation to AT LEAST 'new_size' in bytes
//  returns a different pointer than was passed if the memory was moved
//  if its memory was moved, pointers to its memory are no longer valid
//  if 'new_size' is zero, calls ZeroFree on 'ptr' and returns 0
void* deshi__memory_generic_reallocate(void* ptr, upt new_size, char* file, upt line);
FORCE_INLINE void* deshi__memory_generic_reallocate(void* ptr, upt new_size){return deshi__memory_generic_reallocate(ptr, new_size, "", 0);}
#define memory_realloc(ptr, new_size) deshi__memory_generic_reallocate(ptr, new_size, __FILE__, __LINE__)
#define memrealloc(ptr, new_size) memory_realloc(ptr, new_size)

//frees allocated memory and zeros it
void deshi__memory_generic_zero_free(void* ptr, char* file, upt line);
FORCE_INLINE void deshi__memory_generic_zero_free(void* ptr){return deshi__memory_generic_zero_free(ptr, "", 0);}
#define memory_zfree(ptr) deshi__memory_generic_zero_free(ptr, __FILE__, __LINE__)
#define memzfree(ptr) memory_zfree(ptr)

//exposes the internal generic heap
GenericHeap* deshi__memory_generic_expose();
#define memory_expose_generic_heap() deshi__memory_generic_expose()


////////////////////
//// @temporary ////
////////////////////
//allocates 'size' in bytes with all memory zeroed
void* deshi__memory_temp_allocate(upt size, char* file, upt line);
FORCE_INLINE void* deshi__memory_temp_allocate(upt size){return deshi__memory_temp_allocate(size, "", 0);}
#define memory_talloc(size) deshi__memory_temp_allocate(size, __FILE__, __LINE__)
#define memtalloc(size) memory_talloc(size)

//allocates 'size' in bytes and copies the memory to the new location
void* deshi__memory_temp_reallocate(void* ptr, upt size, char* file, upt line);
FORCE_INLINE void* deshi__memory_temp_reallocate(void* ptr, upt size){return deshi__memory_temp_reallocate(ptr, size, "", 0);}
#define memory_trealloc(ptr, size) deshi__memory_temp_reallocate(ptr, size, __FILE__, __LINE__)
#define memtrealloc(ptr, size) memory_trealloc(ptr, size)

//clears and resets the temp arena
void deshi__memory_temp_clear();
#define memory_clear_temp() deshi__memory_temp_clear()

//exposes the internal temp arena
Arena* deshi__memory_temp_expose();
#define memory_expose_temp_arena() deshi__memory_temp_expose()


/////////////////
//// @naming ////
/////////////////
#if DESHI_INTERNAL
struct AddressNameInfo{
	void*   address;
	cstring name;
	Type    type;
	u32     reserved_; //padding
};

//attaches a name and type to an address
void deshi__memory_naming_set(void* address, cstring name, Type type);
#define memory_set_address_name(address, name, type) deshi__memory_naming_set(address, name, type)

//gets the name of a previously named address
//  returns an empty cstring if not previously named
cstring deshi__memory_naming_get(void* address);
#define memory_get_address_name(address) deshi__memory_naming_get(address)

//exposes the internal naming arena
Arena* deshi__memory_naming_expose();
#define memory_expose_naming_arena() deshi__memory_naming_expose()
#else
#  define memory_set_address_name(...)
#  define memory_get_address_name(...)
#  define memory_expose_naming_arena(...)
#endif //DESHI_INTERNAL


///////////////
//// @init ////
///////////////
//reserves and commits one large chunk of memory from the OS
void deshi__memory_init(upt main_size, upt temp_size);
#define memory_init(main_size, temp_size) deshi__memory_init(main_size, temp_size)


////////////////////
//// @allocator ////
////////////////////
global_ Allocator deshi_allocator_{
	deshi__memory_generic_allocate,
	Allocator_ChangeMemory_Noop,
	Allocator_ChangeMemory_Noop,
	deshi__memory_generic_zero_free,
	deshi__memory_generic_reallocate
};
global_ Allocator* deshi_allocator = &deshi_allocator_;


global_ Allocator deshi_temp_allocator_{
	deshi__memory_temp_allocate,
	Allocator_ChangeMemory_Noop,
	Allocator_ChangeMemory_Noop,
	Allocator_ReleaseMemory_Noop,
	deshi__memory_temp_reallocate
};
global_ Allocator* deshi_temp_allocator = &deshi_temp_allocator_;

#endif //DESHI_MEMORY_H
