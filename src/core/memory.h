/* deshi Memory Module
Notes:
  The memory module is mostly meant as a replacement to the memory functions of libc, but I don't guarantee it's better.
  All memory allocated from this module is guarenteed to be zeroed already, because we push the zeroing cost onto freeing the memory.
  Generic allocation and Heaps use 16 bytes per allocation/item because of MemChunk, so keep that in mind with small objects or allocations.
  Arena and Heap both allocate memory in a contiguous manner.

Memory Layout:  (not to scale)
|                      Total Size                         |
|                Main Heap                  |  Temp Arena |
| Generic Heap  |  Heap Arena |  Heap Arena | Item | Item |
| Chunk | Chunk | Item | Item | Item | Item |      |      |

Index:
@memory_types
  Arena
  MemChunk
  Heap
  AllocInfo
@memory_chunk
  ChunkToMemory(MemChunk* chunk) -> void*
  ChunkToArena(MemChunk* chunk) -> Arena*
  MemoryToChunk(void* memory) -> MemChunk*
  ArenaToChunk(Arena* arena) -> MemChunk*
  MEMORY_EMPTY_FLAG  :: 0x1
  MEMORY_ARENAD_FLAG :: 0x2
  MEMORY_LIBC_FLAG   :: 0x4
  GetChunkSize(Chunk* chunk) -> upt
  GetNextOrderChunk(Chunk* chunk) -> MemChunk*
  GetPrevOrderChunk(Chunk* chunk) -> MemChunk*
  GetChunkAtOffset(Chunk* chunk, upt offset) -> MemChunk*
  ChunkIsEmpty(Chunk* chunk) -> b32
  ChunkIsArenad(Chunk* chunk) -> b32
  ChunkIsLibc(Chunk* chunk) -> b32
@memory_heap
  memory_heap_init_bytes(upt bytes) -> Heap*
  memory_heap_init(type, count) -> Heap*
  memory_heap_deinit(Heap* heap) -> void
  memory_heap_add_bytes(Heap* heap, void* data, upt bytes) -> void*
  memory_heap_add(heap, item) -> void*
  memory_heap_remove(Heap* heap, void* ptr) -> void
  memory_heap_remove_idx(heap, index) -> void
  memory_heap_clear(Heap* heap) -> void
  memory_heap_index(heap, type, index) -> upt
  memory_heap_last(heap, type) -> type*
  memory_heap_count(heap, type) -> upt
  memory_heap_space(heap, type) -> upt
@memory_arena
  memory_create_arena(upt size) -> Arena*
  memory_grow_arena(Arena* arena, upt size) -> Arena*
  memory_clear_arena(Arena* arena) -> void
  memory_delete_arena(Arena* arena) -> void
  memory_expose_arena_heap() -> Heap*
@memory_generic
  memory_alloc(upt size) -> void*
  memory_realloc(void* ptr, upt new_size) -> void*
  memory_zfree(void* ptr) -> void
  memory_expose_generic_heap() -> Heap*
@memory_temp
  memory_talloc(upt size) -> void
  memory_retalloc(void* ptr, upt new_size) -> void
  memory_clear() -> void
  memory_expose_temp_arena() -> Arena*
@memory_debug
  deshi__memory_allocinfo_set(void* address, str8 name, Type type) -> void
  deshi__memory_allocinfo_get(void* address) -> AllocInfo
  deshi__memory_allocinfo_active_expose() -> carray<AllocInfo>
  deshi__memory_allocinfo_inactive_expose() -> carray<AllocInfo>
  deshi__memory_draw() -> void
  deshi__memory_bytes_draw() -> void
@memory_state
  memory_init(upt main_size, upt temp_size) -> void
  memory_cleanup() -> void
@memory_allocators
  deshi_allocator
  deshi_temp_allocator

References:
https://github.com/lattera/glibc/blob/895ef79e04a953cac1493863bcae29ad85657ee1/malloc/malloc.c
https://github.com/Dion-Systems/metadesk/blob/master/source/md.h
https://github.com/Dion-Systems/metadesk/blob/master/source/md.c
https://github.com/nothings/stb/blob/master/stb_ds.h
*/


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma once
#ifndef DESHI_MEMORY_H
#define DESHI_MEMORY_H

#include "kigu/common.h"
#include "kigu/node.h"
#include "kigu/unicode.h"

#define MEMORY_CHECK_HEAPS true
#define MEMORY_TRACK_ALLOCS false
#define MEMORY_PRINT_ARENA_CHUNKS false
#define MEMORY_PRINT_ARENA_ACTIONS false
#define MEMORY_PRINT_HEAP_ACTIONS false
#define MEMORY_PRINT_GENERIC_CHUNKS false
#define MEMORY_PRINT_GENERIC_ACTIONS false
#define MEMORY_PRINT_TEMP_ACTIONS false


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @memory_types
external struct Arena{
	u8* start;
	u8* cursor;
	upt size;
	upt used;
};

external struct MemChunk{
	MemChunk* prev; //pointer to previous order chunk
	upt       size; //size of this chunk (including this var and above vars as overhead)(OR'd with flags)
	Node      node; //user memory starts here when in use; points to free chunks when not
};

external struct Heap{
	u8*       start;
	u8*       cursor;
	upt       used;
	upt       size;
	Node      empty_nodes;
	MemChunk* last_chunk;
	b32       initialized;
};

external struct AllocInfo{
	//automatic
	void* address;
	CodeLocation trigger;
	u64 creation_frame;
	u64 deletion_frame;
	
	//user defined
	str8 name;
	Type type;
};

external struct MemoryContext{
	b32    cleanup_happened;
	Heap   arena_heap;
	Arena* generic_arena; //generic_heap is stored here; not used otherwise
	Heap*  generic_heap;
	Arena  temp_arena;
};
//global memory pointer
extern MemoryContext* g_memory;


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @memory_chunk
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


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @memory_heap
//Initializes a heap with an initial `bytes` of memory
external Heap* deshi__memory_heap_init_bytes(upt bytes, str8 file, upt line);
#define memory_heap_init_bytes(bytes) deshi__memory_heap_init_bytes(bytes, str8_lit(__FILE__), __LINE__)

//Initializes a heap with an initial `count` slots of `type`
#define memory_heap_init(type,count) memory_heap_init_bytes((count)*sizeof(type))

//Deinitializes a `heap` and frees its memory
external void deshi__memory_heap_deinit(Heap* heap, str8 file, upt line);
#define memory_heap_deinit(heap) deshi__memory_heap_deinit(heap, str8_lit(__FILE__), __LINE__)

//Finds an empty chunk in `heap` of at least `bytes` in size and copies `bytes` from `data` to that chunk
//    if the heap is full, it will grow its internal space by a factor of 2
external void* deshi__memory_heap_add_bytes(Heap* heap, void* data, upt bytes, str8 file, upt line);
#define memory_heap_add_bytes(heap,data,bytes) deshi__memory_heap_add_bytes(heap, data, bytes, str8_lit(__FILE__), __LINE__)

//Adds an `item` to an empty chunk in `heap`
//    if the heap is full, it will grow its internal space by a factor of 2
#define memory_heap_add(heap,item) memory_heap_add_bytes(heap, &item, sizeof(item))

//Removes the item at `ptr` from `heap` by turning it into an empty chunk
external void deshi__memory_heap_remove(Heap* heap, void* ptr, str8 file, upt line);
#define memory_heap_remove(heap,ptr) deshi__memory_heap_remove(heap, ptr, str8_lit(__FILE__), __LINE__)

//Removes the item at `index` in `heap` by turning it into an empty chunk
#define memory_heap_remove_idx(heap,index) memory_heap_remove((heap), ((heap)->start + (index)))

//Resets a `heap` and zeroes its memory
external void deshi__memory_heap_clear(Heap* heap, str8 file, upt line);
#define memory_heap_clear(heap) deshi__memory_heap_clear(heap, str8_lit(__FILE__), __LINE__)

//Returns a pointer to the item at `index` in `heap`
//    if the chunk at `index` is empty, returns 0
#define memory_heap_index(heap,type,index) ((!ChunkIsEmpty(MemoryToChunk((type*)((heap)->start) + (index)))) ? (type*)((heap)->start) + (index) : 0)

//Returns the a pointer to the last item in `heap`
//    if `heap` is empty, returns 0
#define memory_heap_last(heap,type) (type*)(((heap)->last_chunk) ? ChunkToMemory((heap)->last_chunk) : 0)

//Returns the number of items of `type` that `heap` can hold before growing
#define memory_heap_space(heap,type) ((heap)->size / sizeof(type))


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @memory_arena
//Creates an arena AT LEAST `size` in bytes with all memory zeroed
external Arena* deshi__memory_arena_create(upt size, str8 file, upt line);
#define memory_create_arena(size) deshi__memory_arena_create(size, str8_lit(__FILE__), __LINE__)

//Grows the `arena` by AT LEAST `size` bytes
//  returns a different pointer than was passed if the memory was moved
//  if its memory was moved, pointers to its memory are no longer valid
external Arena* deshi__memory_arena_grow(Arena* arena, upt size, str8 file, upt line);
#define memory_grow_arena(arena, size) deshi__memory_arena_grow(arena, size, str8_lit(__FILE__), __LINE__)

//Zeros the used amount of `arena` memory and resets used amount to zero
external void deshi__memory_arena_clear(Arena* arena, str8 file, upt line);
#define memory_clear_arena(arena) deshi__memory_arena_clear(arena, str8_lit(__FILE__), __LINE__)

//Deletes the `arena` and zeros its memory
external void deshi__memory_arena_delete(Arena* arena, str8 file, upt line);
#define memory_delete_arena(arena) deshi__memory_arena_delete(arena, str8_lit(__FILE__), __LINE__)

//Exposes the internal `Arena Heap`
external Heap* deshi__memory_arena_expose();
#define memory_expose_arena_heap() deshi__memory_arena_expose()

//TODO maybe implement file/line number stuff for these
template<typename T>
T* memory_arena_add(Arena* arena, const T& item);

template<typename T>
T* memory_arena_add_new(Arena* arena);


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @memory_generic
//Allocates AT LEAST `size` bytes with all memory zeroed
external void* deshi__memory_generic_allocate(upt size, str8 file, upt line);
#define memory_alloc(size) deshi__memory_generic_allocate(size, str8_lit(__FILE__), __LINE__)
#define memalloc(size) memory_alloc(size)

//Resizes the allocation at `ptr` to AT LEAST `new_size` bytes
//  returns a different pointer than was passed if the memory was moved
//  if its memory was moved, pointers to its memory are no longer valid
//  if `new_size` is zero, calls ZeroFree on `ptr` and returns 0
external void* deshi__memory_generic_reallocate(void* ptr, upt new_size, str8 file, upt line);
#define memory_realloc(ptr, new_size) deshi__memory_generic_reallocate(ptr, new_size, str8_lit(__FILE__), __LINE__)
#define memrealloc(ptr, new_size) memory_realloc(ptr, new_size)

//Frees allocated memory at `ptr` and zeros it
external void deshi__memory_generic_zero_free(void* ptr, str8 file, upt line);
#define memory_zfree(ptr) deshi__memory_generic_zero_free(ptr, str8_lit(__FILE__), __LINE__)
#define memzfree(ptr) memory_zfree(ptr)

//Exposes the internal generic heap
external Heap* deshi__memory_generic_expose();
#define memory_expose_generic_heap() deshi__memory_generic_expose()


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @memory_temp
//Allocates 'size' bytes with all memory zeroed from the `Temp Arena`
external void* deshi__memory_temp_allocate(upt size, str8 file, upt line);
#define memory_talloc(size) deshi__memory_temp_allocate(size, str8_lit(__FILE__), __LINE__)
#define memtalloc(size) memory_talloc(size)

//Allocates `size` bytes in the `Temp Arena` and copies the previous memory to the new location
external void* deshi__memory_temp_reallocate(void* ptr, upt size, str8 file, upt line);
#define memory_trealloc(ptr, size) deshi__memory_temp_reallocate(ptr, size, str8_lit(__FILE__), __LINE__)
#define memtrealloc(ptr, size) memory_trealloc(ptr, size)

//Clears and resets the `Temp Arena`
external void deshi__memory_temp_clear();
#define memory_clear_temp() deshi__memory_temp_clear()

//Exposes the internal `Temp Arena`
external Arena* deshi__memory_temp_expose();
#define memory_expose_temp_arena() deshi__memory_temp_expose()


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @memory_debug
//Attaches a `name` and `type` to an `address` (previously allocated or not)
external void deshi__memory_allocinfo_set(void* address, str8 name, Type type);

//Gets the allocation info of an `address` (previously allocated or not)
external AllocInfo deshi__memory_allocinfo_get(void* address);

//Exposes the internal allocation info array
carray<AllocInfo> deshi__memory_allocinfo_active_expose();
carray<AllocInfo> deshi__memory_allocinfo_inactive_expose();

void deshi__memory_draw();
void deshi__memory_bytes_draw();


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @memory_state
//Reserves and commits `main_size`+`temp_size` bytes of memory from the OS
external void deshi__memory_init(upt main_size, upt temp_size);
#define memory_init(main_size, temp_size) deshi__memory_init(main_size, temp_size)

//Prevents memory operations on program close, since OS will cleanup anyways
external void deshi__memory_cleanup();
#define memory_cleanup() deshi__memory_cleanup()


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @memory_allocators
FORCE_INLINE void* deshi__memory_generic_allocate_allocator(upt size){return deshi__memory_generic_allocate(size, str8_lit("deshi_allocator"), 0);}
FORCE_INLINE void* deshi__memory_generic_reallocate_allocator(void* ptr, upt new_size){return deshi__memory_generic_reallocate(ptr, new_size, str8_lit("deshi_allocator"), 0);}
FORCE_INLINE void deshi__memory_generic_zero_free_allocator(void* ptr){return deshi__memory_generic_zero_free(ptr, str8_lit("deshi_allocator"), 0);}
global Allocator deshi_allocator_{
	deshi__memory_generic_allocate_allocator,
	Allocator_ChangeMemory_Noop,
	Allocator_ChangeMemory_Noop,
	deshi__memory_generic_zero_free_allocator,
	deshi__memory_generic_reallocate_allocator
};
global Allocator* deshi_allocator = &deshi_allocator_;

FORCE_INLINE void* deshi__memory_temp_allocate_allocator(upt size){return deshi__memory_temp_allocate(size, str8_lit("deshi_allocator"), 0);}
FORCE_INLINE void* deshi__memory_temp_reallocate_allocator(void* ptr, upt size){return deshi__memory_temp_reallocate(ptr, size, str8_lit("deshi_allocator"), 0);}
global Allocator deshi_temp_allocator_{
	deshi__memory_temp_allocate_allocator,
	Allocator_ChangeMemory_Noop,
	Allocator_ChangeMemory_Noop,
	Allocator_ReleaseMemory_Noop,
	deshi__memory_temp_reallocate_allocator
};
global Allocator* deshi_temp_allocator = &deshi_temp_allocator_;


#endif //DESHI_MEMORY_H
