#pragma once
#ifndef DESHI_MEMORY_H
#define DESHI_MEMORY_H

#include "../defines.h"

struct Arena{
	u8* start;
	u8* cursor;
	upt size;
	upt used;
};

struct MemChunk{
	MemChunk* prev; //pointer to previous order chunk
	upt       size; //size of this chunk (including this var and above vars as overhead)(OR'd with flags)
	Node      node; //user memory starts here when in use; points to free chunks when not
};

struct Heap{
	u8*       start;
	u8*       cursor;
	upt       used;
	upt       size;
	Node      empty_nodes;
	MemChunk* last_chunk;
	b32       initialized;
};

struct AllocInfo{
	//automatic
	void* address;
	CodeLocation trigger;
	u64 creation_frame;
	u64 deletion_frame;
	
	//user defined
	cstring name;
	Type type;
};


////////////////
//// @chunk ////
////////////////
#define MEMORY_CHUNK_MEMORY_OFFSET ((upt)OffsetOfMember(MemChunk, node))
#define ChunkToMemory(chunk)\
((void*)((u8*)(chunk) + MEMORY_CHUNK_MEMORY_OFFSET))
#define ChunkToArena(chunk)\
((Arena*)((u8*)(chunk) + MEMORY_CHUNK_MEMORY_OFFSET))
#define MemoryToChunk(memory)\
((MemChunk*)((u8*)(memory) - MEMORY_CHUNK_MEMORY_OFFSET))
#define ArenaToChunk(arena)\
((MemChunk*)((u8*)(arena) - MEMORY_CHUNK_MEMORY_OFFSET))

//NOTE  Chunk Flags  !ref: https://github.com/lattera/glibc/blob/895ef79e04a953cac1493863bcae29ad85657ee1/malloc/malloc.c#L1224
//  Chunk flags are stored in the lower bits of the chunk's size variable, and this doesn't cause problems b/c the size 
//  is always greater than 8 bytes on 32bit and 16 bytes on on 64bit (void* + upt).
//  To get the size, we mask off the bits holding these flags.
//  None of these flags should be used together.
//
//  EMPTY  (0x1): chunk is empty
//  ARENAD (0x2): the generic chunk was large enough to use an Arena
//  LIBC   (0x4): allocated with libc b/c deshi ran out of memory
#define MEMORY_EMPTY_FLAG 0x1
#define MEMORY_ARENAD_FLAG 0x2
#define MEMORY_LIBC_FLAG 0x4

#define MEMORY_CHUNK_SIZE_BITS (MEMORY_EMPTY_FLAG | MEMORY_ARENAD_FLAG | MEMORY_LIBC_FLAG)
#define MEMORY_EXTRACT_SIZE_BITMASK (~MEMORY_CHUNK_SIZE_BITS)

#define GetChunkSize(chunk_ptr)\
((chunk_ptr)->size & MEMORY_EXTRACT_SIZE_BITMASK)
#define GetNextOrderChunk(chunk_ptr)\
((MemChunk*)((u8*)(chunk_ptr) + GetChunkSize(chunk_ptr)))
#define GetPrevOrderChunk(chunk_ptr)\
((chunk_ptr)->prev)
#define GetChunkAtOffset(chunk_ptr,offset)\
((MemChunk*)((u8*)(chunk_ptr) + (offset)))
#define ChunkIsEmpty(chunk_ptr)\
((chunk_ptr)->size & MEMORY_EMPTY_FLAG)
#define ChunkIsArenad(chunk_ptr)\
((chunk_ptr)->size & MEMORY_ARENAD_FLAG)
#define ChunkIsLibc(chunk_ptr)\
((chunk_ptr)->size & MEMORY_LIBC_FLAG)


////////////////
//// @arena ////
////////////////
//creates an arena AT LEAST 'size' in bytes with all memory zeroed
Arena* deshi__memory_arena_create(upt size, cstring file, upt line);
#define memory_create_arena(size) deshi__memory_arena_create(size, cstr_lit(__FILE__), __LINE__)

//grows the arena by AT LEAST 'size' in bytes
//  returns a different pointer than was passed if the memory was moved
//  if its memory was moved, pointers to its memory are no longer valid
Arena* deshi__memory_arena_grow(Arena* arena, upt size, cstring file, upt line);
#define memory_grow_arena(arena, size) deshi__memory_arena_grow(arena, size, cstr_lit(__FILE__), __LINE__)

//zeros the arena's memory and resets used amount to zero
void deshi__memory_arena_clear(Arena* arena, cstring file, upt line);
#define memory_clear_arena(arena) deshi__memory_arena_clear(arena, cstr_lit(__FILE__), __LINE__)

//deletes the arena and zeros its memory
void deshi__memory_arena_delete(Arena* arena, cstring file, upt line);
#define memory_delete_arena(arena) deshi__memory_arena_delete(arena, cstr_lit(__FILE__), __LINE__)

//exposes the internal arena heap
Heap* deshi__memory_arena_expose();
#define memory_expose_arena_heap() deshi__memory_arena_expose()


//////////////////
//// @generic ////
//////////////////
//allocates AT LEAST 'size' in bytes with all memory zeroed
void* deshi__memory_generic_allocate(upt size, cstring file, upt line);
#define memory_alloc(size) deshi__memory_generic_allocate(size, cstr_lit(__FILE__), __LINE__)
#define memalloc(size) memory_alloc(size)

//resizes the allocation to AT LEAST 'new_size' in bytes
//  returns a different pointer than was passed if the memory was moved
//  if its memory was moved, pointers to its memory are no longer valid
//  if 'new_size' is zero, calls ZeroFree on 'ptr' and returns 0
void* deshi__memory_generic_reallocate(void* ptr, upt new_size, cstring file, upt line);
#define memory_realloc(ptr, new_size) deshi__memory_generic_reallocate(ptr, new_size, cstr_lit(__FILE__), __LINE__)
#define memrealloc(ptr, new_size) memory_realloc(ptr, new_size)

//frees allocated memory and zeros it
void deshi__memory_generic_zero_free(void* ptr, cstring file, upt line);
#define memory_zfree(ptr) deshi__memory_generic_zero_free(ptr, cstr_lit(__FILE__), __LINE__)
#define memzfree(ptr) memory_zfree(ptr)

//exposes the internal generic heap
Heap* deshi__memory_generic_expose();
#define memory_expose_generic_heap() deshi__memory_generic_expose()


////////////////////
//// @temporary ////
////////////////////
//allocates 'size' in bytes with all memory zeroed
void* deshi__memory_temp_allocate(upt size, cstring file, upt line);
#define memory_talloc(size) deshi__memory_temp_allocate(size, cstr_lit(__FILE__), __LINE__)
#define memtalloc(size) memory_talloc(size)

//allocates 'size' in bytes and copies the memory to the new location
void* deshi__memory_temp_reallocate(void* ptr, upt size, cstring file, upt line);
#define memory_trealloc(ptr, size) deshi__memory_temp_reallocate(ptr, size, cstr_lit(__FILE__), __LINE__)
#define memtrealloc(ptr, size) memory_trealloc(ptr, size)

//clears and resets the temp arena
void deshi__memory_temp_clear();
#define memory_clear_temp() deshi__memory_temp_clear()

//exposes the internal temp arena
Arena* deshi__memory_temp_expose();
#define memory_expose_temp_arena() deshi__memory_temp_expose()


////////////////
//// @debug ////
////////////////
//attaches a name and type to an address (previously allocated or not)
void deshi__memory_allocinfo_set(void* address, cstring name, Type type);

//gets the allocation info of an address (previously allocated or not)
AllocInfo deshi__memory_allocinfo_get(void* address);

//exposes the internal allocation info array
carray<AllocInfo> deshi__memory_allocinfo_expose();

void deshi__memory_draw();
void deshi__memory_bytes_draw();


///////////////
//// @init ////
///////////////
//reserves and commits one large chunk of memory from the OS
void deshi__memory_init(upt main_size, upt temp_size);
#define memory_init(main_size, temp_size) deshi__memory_init(main_size, temp_size)


////////////////////
//// @allocator ////
////////////////////
FORCE_INLINE void* deshi__memory_generic_allocate_allocator(upt size){return deshi__memory_generic_allocate(size, cstr_lit("deshi_allocator"), 0);}
FORCE_INLINE void* deshi__memory_generic_reallocate_allocator(void* ptr, upt new_size){return deshi__memory_generic_reallocate(ptr, new_size, cstr_lit("deshi_allocator"), 0);}
FORCE_INLINE void deshi__memory_generic_zero_free_allocator(void* ptr){return deshi__memory_generic_zero_free(ptr, cstr_lit("deshi_allocator"), 0);}
global_ Allocator deshi_allocator_{
	deshi__memory_generic_allocate_allocator,
	Allocator_ChangeMemory_Noop,
	Allocator_ChangeMemory_Noop,
	deshi__memory_generic_zero_free_allocator,
	deshi__memory_generic_reallocate_allocator
};
global_ Allocator* deshi_allocator = &deshi_allocator_;

FORCE_INLINE void* deshi__memory_temp_allocate_allocator(upt size){return deshi__memory_temp_allocate(size, cstr_lit("deshi_allocator"), 0);}
FORCE_INLINE void* deshi__memory_temp_reallocate_allocator(void* ptr, upt size){return deshi__memory_temp_reallocate(ptr, size, cstr_lit("deshi_allocator"), 0);}
global_ Allocator deshi_temp_allocator_{
	deshi__memory_temp_allocate_allocator,
	Allocator_ChangeMemory_Noop,
	Allocator_ChangeMemory_Noop,
	Allocator_ReleaseMemory_Noop,
	deshi__memory_temp_reallocate_allocator
};
global_ Allocator* deshi_temp_allocator = &deshi_temp_allocator_;

#endif //DESHI_MEMORY_H
