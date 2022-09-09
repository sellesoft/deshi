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
@memory_chunk
  MemChunk
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
  Heap
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
  Arena
  memory_create_arena(upt size) -> Arena*
  memory_grow_arena(Arena* arena, upt size) -> Arena*
  memory_clear_arena(Arena* arena) -> void
  memory_delete_arena(Arena* arena) -> void
  memory_expose_arena_heap() -> Heap*
@memory_pool
  PoolHeader
  memory_pool_header(void* pool) -> PoolHeader*
  memory_pool_count(void* pool) -> upt
  memory_pool_grow(T* pool, upt count) -> void
  memory_pool_init(T* pool, upt count) -> T*
  memory_pool_deinit(void* pool) -> void
  memory_pool_push(T* pool) -> void*
  memory_pool_delete(T* pool, void* ptr) -> void
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
  AllocInfo
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
  http://dmitrysoshnikov.com/compilers/writing-a-pool-allocator/
*/


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma once
#ifndef DESHI_MEMORY_H
#define DESHI_MEMORY_H
#include "kigu/common.h"
#include "kigu/node.h"

#define MEMORY_CHECK_HEAPS false
#define MEMORY_TRACK_ALLOCS false

#define MEMORY_PRINT_ARENA_CHUNKS false
#define MEMORY_PRINT_ARENA_ACTIONS false
#define MEMORY_PRINT_HEAP_ACTIONS false
#define MEMORY_PRINT_GENERIC_CHUNKS false
#define MEMORY_PRINT_GENERIC_ACTIONS false
#define MEMORY_PRINT_TEMP_ACTIONS false


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @memory_chunk
external struct MemChunk{
	MemChunk* prev; //pointer to previous order chunk
	upt       size; //size of this chunk (including this var and above vars as overhead)(OR'd with flags)
	Node      node; //user memory starts here when in use; points to free chunks when not
};

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
external struct Heap{
	u8*       start;
	u8*       cursor;
	upt       used;
	upt       size;
	Node      empty_nodes;
	MemChunk* last_chunk;
	b32       initialized;
};

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
external void* deshi__memory_heap_add_bytes(Heap* heap, upt bytes, str8 file, upt line);
#define memory_heap_add_bytes(heap,bytes) deshi__memory_heap_add_bytes(heap, bytes, str8_lit(__FILE__), __LINE__)

//Adds an `item` to an empty chunk in `heap`
//    if the heap is full, it will grow its internal space by a factor of 2
#define memory_heap_add(heap,item) memory_heap_add_bytes(heap, &item, sizeof(item))

//Removes the item at `ptr` from `heap` by turning it into an empty chunk
external void deshi__memory_heap_remove(Heap* heap, void* ptr, str8 file, upt line);
#define memory_heap_remove(heap,ptr) deshi__memory_heap_remove(heap, ptr, str8_lit(__FILE__), __LINE__)

//Resets a `heap` and zeroes its memory
external void deshi__memory_heap_clear(Heap* heap, str8 file, upt line);
#define memory_heap_clear(heap) deshi__memory_heap_clear(heap, str8_lit(__FILE__), __LINE__)


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @memory_arena
external struct Arena{
	u8* start;
	u8* cursor;
	upt size;
	upt used;
};

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
//// @memory_pool
// A memory pool (in this context) is a memory-owning data structure for storing homogenous data
// without invalidatng pointers on growth. The pool must first be init by passing a pointer of a
// type, from which the chunk size comes. A pool chunk represents a slot that an item can fill in
// the pool. When initting the pool, you specify how many chunks fit in one block. A pool block is
// just a contigious allocation of the chunks. Allocation from the pool happens in O(1) unless a
// growth operation is needed to create new free chunks. Fast allocation is possible because when
// deleting from the pool, we insert a free chunk where the memory was to the beginning of a singly-
// linked-list of free chunks. In the even a growth operation occurs during allocation, we generic
// allocate a new block of memory which is linked with the pool's header thru a singly-linked-list
// of all the blocks. As such, deinitting happens by iterating that list of blocks and freeing them.
//
// TL/DR: fast allocation, pointer persistence on growth, not-fully-contiguous

external struct PoolHeader{
	upt chunks_per_block;
	void** free_chunk; //linked list of void*
	void** next_block; //linked list of void*
	upt count;
};

//Returns the header for the pooled arena `pool`
#define memory_pool_header(pool) ((PoolHeader*)(pool) - 1)

//Returns the number of items (not chunks) in the pooled arena `pool`
#define memory_pool_count(pool) (memory_pool_header(pool)->count)

//Allocates a block of `count` chunks for the pooled arena `pool` and sets `PoolHeader.free_chunk` to the first new chunk
#define memory_pool_grow(pool,count) deshi__memory_pool_grow((pool), sizeof(*(pool)), (count))
external void deshi__memory_pool_grow(void* pool, upt type_size, upt count);

//Creates a pooled arena with `count` chunks-per-block (item slots) and sets `pool` equal to the first chunk
#define memory_pool_init(pool,count) ((pool) = memory_pool_init_wrapper(pool, sizeof(*(pool)), (count)))
external void* deshi__memory_pool_init(void* pool, upt type_size, upt count);
#if COMPILER_FEATURE_CPP //NOTE(delle) C can implicitly cast from void* to T*, but C++ can't so templates are required
template<class T> global inline T* memory_pool_init_wrapper(T*    pool, upt type_size, upt count){ return (T*)deshi__memory_pool_init(pool, type_size, count); }
#else
FORCE_INLINE                 void* memory_pool_init_wrapper(void* pool, upt type_size, upt count){ return     deshi__memory_pool_init(pool, type_size, count); }
#endif

//Deletes the pooled arena `pool`
external void memory_pool_deinit(void* pool);

//Returns a free chunk in `pool`, growing if necessary
#define memory_pool_push(pool) deshi__memory_pool_push(pool, sizeof(*(pool)))
external void* deshi__memory_pool_push(void* pool, upt type_size);

//Deletes the chunk at `ptr` in the pooled arena `pool`
#define memory_pool_delete(pool,ptr) deshi__memory_pool_delete((pool), sizeof(*(pool)), (ptr))
external void deshi__memory_pool_delete(void* pool, upt type_size, void* ptr);


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
external struct MemoryContext{
	b32    cleanup_happened;
	Heap   arena_heap;
	Arena* generic_arena; //generic_heap is stored here; not used otherwise
	Heap*  generic_heap;
	Arena  temp_arena;
};
extern MemoryContext* g_memory; //global memory pointer

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
