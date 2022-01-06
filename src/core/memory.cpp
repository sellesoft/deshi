//NOTE Memory Layout: 
//|                                Total Size                                 |
//|                            Main Heap                        |  Temp Arena |
//| Generic Arena |      Heap Arena      |      Heap Arena      | Item | Item |
//| Chunk | Chunk | Header |    Memory   | Header |    Memory   |      |      |
//|       |       |        | Item | Item |        | Item | Item |      |      |

////////////////
//// @utils ////
////////////////
#define MEMORY_CHECK_HEAPS true
#define MEMORY_PRINT_ARENA_CHUNKS false
#define MEMORY_PRINT_ARENA_ACTIONS false
#define MEMORY_PRINT_GENERIC_CHUNKS false
#define MEMORY_PRINT_GENERIC_ACTIONS false
#define MEMORY_PRINT_TEMP_ACTIONS false

#define MEMORY_POINTER_SIZE sizeof(void*)
#define MEMORY_BYTE_ALIGNMENT (2*MEMORY_POINTER_SIZE)
#define MEMORY_BYTE_ALIGNMENT_MASK (MEMORY_BYTE_ALIGNMENT-1)
#define MEMORY_CHUNK_OVERHEAD MEMORY_CHUNK_MEMORY_OFFSET
#define MEMORY_ARENA_OVERHEAD (MEMORY_CHUNK_OVERHEAD + sizeof(Arena))
#define MEMORY_MIN_CHUNK_SIZE ((sizeof(MemChunk) + MEMORY_BYTE_ALIGNMENT_MASK) & ~(MEMORY_BYTE_ALIGNMENT_MASK))
#define MEMORY_CHUNK_ALIGNMENT MEMORY_MIN_CHUNK_SIZE
#define MEMORY_MAX_GENERIC_SIZE (Kilobytes(64) - 1)

//NOTE Chunk Flags !ref: https://github.com/lattera/glibc/blob/895ef79e04a953cac1493863bcae29ad85657ee1/malloc/malloc.c#L1224
//  Chunk flags are stored in the lower bits of the chunk's size variable, and this doesn't cause problems b/c the size 
//  is always greater than 8 bytes on 32bit and 16 bytes on on 64bit (MEMORY_GENERIC_MIN_ALLOC_SIZE).
//  To get the size, we mask off the bits holding these flags.
//  ISARENAD and PREVINUSE should never be used together
//
//  PREVINUSE (0x1): previous adjacent chunk is in use //TODO maybe just change to INUSE/EMPTY?
//  ISARENAD  (0x2): the chunk was large enough to use an Arena
//  LIBC      (0x4): deshi ran out of memory and had to call libc
#define MEMORY_PREVINUSE_FLAG 0x1
#define MEMORY_ISARENAD_FLAG 0x2
#define MEMORY_LIBC_FLAG 0x4

#define MEMORY_CHUNK_SIZE_BITS (MEMORY_PREVINUSE_FLAG | MEMORY_ISARENAD_FLAG | MEMORY_LIBC_FLAG)
#define MEMORY_EXTRACT_SIZE_BITMASK (~MEMORY_CHUNK_SIZE_BITS)

#define GetChunkSize(chunk_ptr)\
((chunk_ptr)->size & MEMORY_EXTRACT_SIZE_BITMASK)
#define GetNextOrderChunk(chunk_ptr)\
((MemChunk*)((u8*)(chunk_ptr) + GetChunkSize(chunk_ptr)))
#define GetPrevOrderChunk(chunk_ptr)\
((chunk_ptr)->prev)
#define GetChunkAtOffset(chunk_ptr,offset)\
((MemChunk*)((u8*)(chunk_ptr) + (offset)))
#define PrevChunkIsInUse(chunk_ptr)\
((chunk_ptr)->size & MEMORY_PREVINUSE_FLAG)
#define ChunkIsInUse(chunk_ptr)\
(((MemChunk*)((u8*)(chunk_ptr) + GetChunkSize(chunk_ptr)))->size & MEMORY_PREVINUSE_FLAG)
#define ChunkIsArenad(chunk_ptr)\
((chunk_ptr)->size & MEMORY_ISARENAD_FLAG)
#define ChunkIsLibc(chunk_ptr)\
((chunk_ptr)->size & MEMORY_LIBC_FLAG)

#if MEMORY_CHECK_HEAPS
local void
DEBUG_CheckHeap(Heap* heap){
	Assert(PointerDifference(heap->cursor, heap->start) % MEMORY_BYTE_ALIGNMENT == 0, "Memory alignment is invalid");
	Assert(PointerDifference(heap->cursor, heap->start) >= heap->used, "Heap used amount is greater than cursor offset");
	Assert(heap->empty_nodes.next != 0 && heap->empty_nodes.prev != 0, "First heap empty node is invalid");
	for(Node* node = &heap->empty_nodes; ; ){
		Assert(node->next->prev == node && node->prev->next == node, "Heap empty node is invalid");
		node = node->next;
		if(node == &heap->empty_nodes) break;
	}
	
	if(heap->initialized && heap->used > 0){
		upt overall_used = 0;
		for(MemChunk* chunk = (MemChunk*)heap->start; ; chunk = GetNextOrderChunk(chunk)){
			Assert((u8*)chunk < heap->cursor, "All chunks must be below the cursor");
			Assert(GetChunkSize(chunk) >= MEMORY_MIN_CHUNK_SIZE, "Chunk size is less than minimum");
			Assert(GetChunkSize(chunk) % MEMORY_BYTE_ALIGNMENT == 0, "Chunk size is not aligned correctly");
			if(chunk != heap->last_chunk){
				Assert(GetNextOrderChunk(chunk)->prev == chunk, "Next order chunk is not correctly pointing to current chunk");
				if(!PrevChunkIsInUse(GetNextOrderChunk(chunk))){
					overall_used += sizeof(MemChunk);
				}else{
					overall_used += GetChunkSize(chunk);
				}
			}else{
				overall_used += GetChunkSize(chunk);
				break;
			}
		}
		Assert(overall_used == heap->used, "Heap used amount is invalid");
	}
}
#else //MEMORY_CHECK_HEAPS
#  define DEBUG_CheckHeap(...)
#endif //MEMORY_CHECK_HEAPS

local void
DEBUG_PrintHeapChunks(Heap* heap, char* heap_name){
	if(heap->initialized && heap->used > 0){
		string heap_order = "Order: ", heap_empty = "Empty: ";
		for(MemChunk* chunk = (MemChunk*)heap->start; ; chunk = GetNextOrderChunk(chunk)){
			heap_order += to_string("0x%p", chunk); heap_order += " -> ";
			if(chunk != heap->last_chunk){
				if(!ChunkIsInUse(chunk)){
					heap_empty += to_string("0x%p", chunk);
					heap_empty += " -> ";
				}else{
					heap_empty += "                  ";
					heap_empty += " -> ";
				}
			}else{
				heap_empty += "                  ";
				heap_empty += " -> ";
				break;
			}
		}
		Log("memory","---------------------",(heap_name) ? heap_name : "","-------------------------");
		Log("memory",heap_order); Log("memory",heap_empty);
	}
}

////////////////
//// @arena ////
////////////////
local Heap deshi__arena_heap_;
local Heap* deshi__arena_heap = &deshi__arena_heap_;

#if MEMORY_CHECK_HEAPS
local void
DEBUG_CheckArenaHeapArenas(){
	if(deshi__arena_heap->initialized && deshi__arena_heap->used > 0){
		for(MemChunk* chunk = (MemChunk*)deshi__arena_heap->start; chunk != deshi__arena_heap->last_chunk; ){
			MemChunk* next = GetNextOrderChunk(chunk);
			if(PrevChunkIsInUse(next)){
				Arena* arena = ChunkToArena(chunk);
				Assert(PointerDifference(arena->start + arena->size, next) == 0, "Arena start and size dont point to the next chunk");
			}
			chunk = next;
		}
	}
}
#else //MEMORY_CHECK_HEAPS
#  define DEBUG_CheckArenaHeapArenas()
#endif //MEMORY_CHECK_HEAPS

#if MEMORY_PRINT_ARENA_CHUNKS
FORCE_INLINE void DEBUG_PrintArenaHeapChunks(){ DEBUG_PrintHeapChunks(deshi__arena_heap,"Arena Heap"); }
#else //MEMORY_PRINT_ARENA_CHUNKS
#  define DEBUG_PrintArenaHeapChunks()
#endif //MEMORY_PRINT_ARENA_CHUNKS

local Arena*
CreateArenaLibc(upt aligned_size){ //NOTE expects pre-aligned size with arena overhead
	MemChunk* chunk = (MemChunk*)calloc(1, aligned_size);
	Assert(chunk, "libc failed to allocate memory");
	chunk->size = aligned_size | MEMORY_LIBC_FLAG;
	Arena* arena = ChunkToArena(chunk);
	arena->start  = (u8*)(arena+1);
	arena->cursor = arena->start;
	arena->size   = aligned_size - MEMORY_ARENA_OVERHEAD;
	return arena;
}

Arena*
deshi__memory_arena_create(upt requested_size, char* file, upt line){
	DEBUG_CheckHeap(deshi__arena_heap);
	if(requested_size == 0) return 0;
	Assert(deshi__arena_heap && deshi__arena_heap->initialized, "Attempted to create an arena before memory_init() has been called");
	
	//include chunk and arena overhead, align to the byte alignment (no need to clamp min b/c arena overhead fits Node)
	upt aligned_size = RoundUpTo(requested_size + MEMORY_ARENA_OVERHEAD, MEMORY_BYTE_ALIGNMENT);
	Arena* result = 0;
	
	//check if there are any empty nodes that can hold the new arena
	for(Node* node = deshi__arena_heap->empty_nodes.next; node != &deshi__arena_heap->empty_nodes; node = node->next){
		MemChunk* chunk = CastFromMember(MemChunk, node, node);
		upt chunk_size = GetChunkSize(chunk); //NOTE remember that chunk size includes the overhead
		if(chunk_size >= aligned_size){
			upt leftover_size = chunk_size - aligned_size;
			Assert(leftover_size % MEMORY_BYTE_ALIGNMENT == 0, "Memory was not aligned correctly");
			MemChunk* next = GetNextOrderChunk(chunk);
			
			//make new empty chunk after current chunk if there is enough space for arena overhead
			if(leftover_size > MEMORY_ARENA_OVERHEAD){
				MemChunk* new_chunk = GetChunkAtOffset(chunk, aligned_size);
				NodeInsertNext(&deshi__arena_heap->empty_nodes, &new_chunk->node);
				new_chunk->prev = chunk;
				new_chunk->size = leftover_size; //NOTE this gets OR'd with MEMORY_GENERIC_PREVINUSE_FLAG below
				next->prev = new_chunk;
				next = new_chunk;
				deshi__arena_heap->used += sizeof(MemChunk);
			}else{
				aligned_size += leftover_size;
			}
			
			//convert empty node to order node
			NodeRemove(&chunk->node);
			//chunk->node = {0}; //NOTE not necessary since its overwritten below
			chunk->size = (PrevChunkIsInUse(chunk)) ? aligned_size | MEMORY_PREVINUSE_FLAG : aligned_size;
			next->size |= MEMORY_PREVINUSE_FLAG; //set PREVINUSE flag on next chunk
			deshi__arena_heap->used += aligned_size - sizeof(MemChunk);
			
			result = ChunkToArena(chunk);
			result->start  = (u8*)(result+1);
			result->cursor = result->start;
			result->size   = aligned_size - MEMORY_ARENA_OVERHEAD;
			result->used   = 0;
			
#if MEMORY_PRINT_ARENA_ACTIONS
			Logf("memory","Created an arena[0x%p] with %zu bytes (triggered at %s:%zu)", result, result->size, file, line);
#endif //MEMORY_PRINT_ARENA_ACTIONS
			
			Assert(result->size >= requested_size, "Arena size was not greater than or equal to requested size");
			DEBUG_CheckHeap(deshi__arena_heap); DEBUG_CheckArenaHeapArenas(); DEBUG_PrintArenaHeapChunks();
			return result;
		}
	}
	
	//if we cant replace an empty node, make a new order node for the allocation (if there is space)
	if(deshi__arena_heap->cursor + aligned_size > deshi__arena_heap->start + deshi__arena_heap->size){
		LogfE("memory","Deshi ran out of main memory when attempting to create an arena (triggered at %s:%zu); defaulting to libc calloc.", file, line);
		
		result = CreateArenaLibc(aligned_size);
		
#if MEMORY_PRINT_ARENA_ACTIONS
		Logf("memory","Created a libc arena[0x%p] with %zu bytes (triggered at %s:%zu)", result, result->size, file, line);
#endif //MEMORY_PRINT_ARENA_ACTIONS
		
		Assert(result->size >= requested_size, "Arena size was not greater than or equal to requested size");
		return result;
	}
	
	MemChunk* new_chunk = (MemChunk*)deshi__arena_heap->cursor;
	new_chunk->prev = deshi__arena_heap->last_chunk;
	new_chunk->size = (deshi__arena_heap->last_chunk != 0) ? aligned_size | MEMORY_PREVINUSE_FLAG : aligned_size;
	deshi__arena_heap->cursor    += aligned_size;
	deshi__arena_heap->used      += aligned_size;
	deshi__arena_heap->last_chunk = new_chunk;
	
	result = ChunkToArena(new_chunk);
	result->start  = (u8*)(result+1);
	result->cursor = result->start;
	result->size   = aligned_size - MEMORY_ARENA_OVERHEAD;
	result->used   = 0;
	
#if MEMORY_PRINT_ARENA_ACTIONS
	Logf("memory","Created an arena[0x%p] with %zu bytes (triggered at %s:%zu)", result, result->size, file, line);
#endif //MEMORY_PRINT_ARENA_ACTIONS
	
	Assert(result->size >= requested_size, "Arena size was not greater than or equal to requested size");
	DEBUG_CheckHeap(deshi__arena_heap); DEBUG_CheckArenaHeapArenas(); DEBUG_PrintArenaHeapChunks();
	return result;
}

Arena*
deshi__memory_arena_grow(Arena* arena, upt size, char* file, upt line){
	DEBUG_CheckHeap(deshi__arena_heap); DEBUG_CheckArenaHeapArenas();
	if(size == 0) return arena;
	if(arena == 0) return 0;
	Assert(deshi__arena_heap && deshi__arena_heap->initialized, "Attempted to grow an arena before memory_init() has been called");
	
	MemChunk* chunk = ArenaToChunk(arena);
	Arena* result = arena;
	upt aligned_size = RoundUpTo(size, MEMORY_BYTE_ALIGNMENT);
	
	//if already using libc, keep using it
	if(ChunkIsLibc(chunk)){
		upt old_size   = arena->size;
		upt old_cursor = arena->cursor - arena->start;
		chunk = (MemChunk*)realloc(chunk, MEMORY_ARENA_OVERHEAD + old_size + size);
		Assert(chunk, "libc failed to allocate memory");
		chunk->size += size;
		
		result = ChunkToArena(chunk);
		result->start  = (u8*)(result+1);
		result->cursor = result->start + old_cursor;
		result->size  += size;
		ZeroMemory(result->start + old_size, size);
		
		memory_set_address_name(result, memory_get_address_name(arena));
#if MEMORY_PRINT_ARENA_ACTIONS
		Logf("memory","Grew a libc arena[0x%p]%s to libc [0x%p] with %zu bytes (triggered at %s:%zu)", arena, (memory_get_address_name(arena)) ? memory_get_address_name(arena).str : "", result, size, file, line);
#endif //MEMORY_PRINT_ARENA_ACTIONS
		memory_set_address_name((result != arena) ? arena : 0, cstring{});
		
		return result;
	}
	Assert((u8*)arena > deshi__arena_heap->start && (u8*)arena < deshi__arena_heap->cursor, "Attempted to grow an arena that's outside the arena heap and missing the libc flag");
	
	//current chunk is the last chunk, so grow current
	if(chunk == deshi__arena_heap->last_chunk){
		if(deshi__arena_heap->cursor + aligned_size > deshi__arena_heap->start + deshi__arena_heap->size){
			LogfE("memory","Deshi ran out of main memory when attempting to grow an arena[0x%p]%s of size %zu bytes with %zu bytes (triggered at %s:%zu); defaulting to libc calloc.", arena, (memory_get_address_name(arena)) ? memory_get_address_name(arena).str : "", arena->size, size, file, line);
			
			result = CreateArenaLibc(arena->size + size);
			result->cursor += (arena->cursor - arena->start);
			result->used    = arena->used;
			memcpy(result->start, arena->start, arena->size);
			
			memory_set_address_name(result, memory_get_address_name(arena));
#if MEMORY_PRINT_ARENA_ACTIONS
			Logf("memory","Grew an arena   [0x%p]%s to libc [0x%p] with %zu bytes (triggered at %s:%zu)", arena, (memory_get_address_name(arena)) ? memory_get_address_name(arena).str : "", result, size, file, line);
#endif //MEMORY_PRINT_ARENA_ACTIONS
			
			deshi__memory_arena_delete(arena, file, line);
			return result;
		}
		
		chunk->size += aligned_size;
		arena->size += aligned_size;
		deshi__arena_heap->cursor += aligned_size;
		deshi__arena_heap->used   += aligned_size;
		
#if MEMORY_PRINT_ARENA_ACTIONS
		Logf("memory","Grew an arena   [0x%p]%s with %zu bytes (triggered at %s:%zu)", arena, (memory_get_address_name(arena)) ? memory_get_address_name(arena).str : "", aligned_size, file, line);
#endif //MEMORY_PRINT_ARENA_ACTIONS
		
		DEBUG_CheckHeap(deshi__arena_heap); DEBUG_CheckArenaHeapArenas(); DEBUG_PrintArenaHeapChunks();
		return result;
	}
	
	//next chunk is empty and can hold the growth, so grow into next
	MemChunk* next = GetNextOrderChunk(chunk);
	upt next_chunk_size = GetChunkSize(next);
	if((next != deshi__arena_heap->last_chunk) && !ChunkIsInUse(next) && (next_chunk_size >= aligned_size)){
		upt leftover_size = next_chunk_size - aligned_size;
		Assert(leftover_size % MEMORY_BYTE_ALIGNMENT == 0, "Memory was not aligned correctly");
		
		//make new empty chunk after current chunk if there is enough space for arena overhead
		MemChunk* next_next = GetNextOrderChunk(next);
		if(leftover_size > MEMORY_ARENA_OVERHEAD){
			MemChunk* new_chunk = GetChunkAtOffset(chunk, GetChunkSize(chunk) + aligned_size);
			NodeInsertNext(&deshi__arena_heap->empty_nodes, &new_chunk->node);
			new_chunk->prev = chunk;
			new_chunk->size = leftover_size | MEMORY_PREVINUSE_FLAG;
			next_next->prev = new_chunk;
			deshi__arena_heap->used += sizeof(MemChunk);
		}else{
			aligned_size += leftover_size;
		}
		
		//zero next's overhead for use by current chunk and grow current chunk
		NodeRemove(&next->node);
		next->prev = 0;
		next->size = 0;
		next->node = {0};
		chunk->size += aligned_size;
		result->size += aligned_size;
		deshi__arena_heap->used += aligned_size - sizeof(MemChunk);
		
#if MEMORY_PRINT_ARENA_ACTIONS
		Logf("memory","Grew an arena   [0x%p]%s with %zu bytes (triggered at %s:%zu)", arena, (memory_get_address_name(arena)) ? memory_get_address_name(arena).str : "", aligned_size, file, line);
#endif //MEMORY_PRINT_ARENA_ACTIONS
		
		DEBUG_CheckHeap(deshi__arena_heap); DEBUG_CheckArenaHeapArenas(); DEBUG_PrintArenaHeapChunks();
		return result;
	}
	
	//need to move memory in order to fit new size
	result = deshi__memory_arena_create(arena->size + size, file, line);
	memcpy(result->start, arena->start, arena->used);
	result->used   = arena->used;
	result->cursor = result->start + (arena->cursor - arena->start);
	
	memory_set_address_name(result, memory_get_address_name(arena));
#if MEMORY_PRINT_ARENA_ACTIONS
	Logf("memory","Grew an arena   [0x%p]%s to [0x%p] with %zu bytes (triggered at %s:%zu)", arena, (memory_get_address_name(arena)) ? memory_get_address_name(arena).str : "", result, aligned_size, file, line);
#endif //MEMORY_PRINT_ARENA_ACTIONS
	
	deshi__memory_arena_delete(arena, file, line);
	
	DEBUG_CheckHeap(deshi__arena_heap); DEBUG_CheckArenaHeapArenas(); DEBUG_PrintArenaHeapChunks();
	return result;
}

void
deshi__memory_arena_clear(Arena* arena, char* file, upt line){
#if MEMORY_PRINT_ARENA_ACTIONS
	Logf("memory","Cleared an arena[0x%p]%s with %zu bytes (triggered at %s:%zu)", arena, (memory_get_address_name(arena)) ? memory_get_address_name(arena).str : "", arena->size, file, line);
#endif //MEMORY_PRINT_ARENA_ACTIONS
	
	ZeroMemory(arena->start, arena->used);
	arena->cursor = arena->start;
	arena->used   = 0;
}

void
deshi__memory_arena_delete(Arena* arena, char* file, upt line){
	DEBUG_CheckHeap(deshi__arena_heap); DEBUG_CheckArenaHeapArenas();
	if(arena == 0) return;
	Assert(deshi__arena_heap && deshi__arena_heap->initialized, "Attempted to delete an arena before memory_init() has been called");
	
	MemChunk* chunk = ArenaToChunk(arena);
	
	//if allocated from libc, free with libc
	if(ChunkIsLibc(chunk)){
		free(chunk);
		
#if MEMORY_PRINT_ARENA_ACTIONS
		Logf("memory","Deleted a libc arena[0x%p]%s with %zu bytes (triggered at %s:%zu)", arena, (memory_get_address_name(arena)) ? memory_get_address_name(arena).str : "", arena->size, file, line);
#endif //MEMORY_PRINT_ARENA_ACTIONS
		memory_set_address_name(arena, cstring{});
		
		return;
	}
	Assert((u8*)arena > deshi__arena_heap->start && (u8*)arena < deshi__arena_heap->cursor, "Attempted to delete an arena outside the main heap and missing the libc flag");
	
	upt   chunk_size   = GetChunkSize(chunk);
	void* zero_pointer = chunk+1;
	upt   zero_amount  = chunk_size - sizeof(MemChunk);
	upt   used_amount  = zero_amount;
	
	//insert current chunk into heap's empty nodes (as first empty node for locality)
	NodeInsertNext(&deshi__arena_heap->empty_nodes, &chunk->node);
	
	//try to merge next empty into current empty
	MemChunk* next = GetNextOrderChunk(chunk);
	if(   (chunk != deshi__arena_heap->last_chunk) //we are not the last chunk
	   && (next  != deshi__arena_heap->last_chunk) //next is not the last chunk
	   && (!ChunkIsInUse(next))){                  //next is empty
		MemChunk* next_next = GetNextOrderChunk(next);
		next_next->prev = chunk;
		NodeRemove(&next->node);
		chunk->size += GetChunkSize(next);
		//NOTE zero_pointer doesnt change
		zero_amount += sizeof(MemChunk); //NOTE next's memory is already zeroed, so only zero the chunk header
		used_amount += sizeof(MemChunk);
		next = next_next;
	}
	
	//try to merge current empty into prev empty
	MemChunk* prev = GetPrevOrderChunk(chunk);
	if(   (prev != 0)                  //we are not the first chunk
	   && (!PrevChunkIsInUse(chunk))){ //previous chunk is empty
		if(chunk == deshi__arena_heap->last_chunk){
			deshi__arena_heap->last_chunk = prev;
		}else{
			next->prev = prev;
		}
		NodeRemove(&chunk->node);
		NodeRemove(&prev->node); //NOTE remove and reinsert as first empty node for locality
		NodeInsertNext(&deshi__arena_heap->empty_nodes, &prev->node);
		prev->size += chunk->size; //NOTE not GetChunkSize(chunk) b/c we know that PREVINUSE flag is not there
		zero_pointer = chunk; //NOTE prev's memory is already zeroed, so only zero chunk
		zero_amount  = GetChunkSize(chunk);
		used_amount += sizeof(MemChunk);
		chunk = prev;
	}
	
	//remove the last order chunk if its empty
	if(chunk == deshi__arena_heap->last_chunk){
		NodeRemove(&chunk->node);
		deshi__arena_heap->last_chunk = chunk->prev;
		deshi__arena_heap->cursor = (u8*)chunk;
		zero_pointer = chunk;
		zero_amount  = GetChunkSize(chunk);
		used_amount += sizeof(MemChunk);
	}else{
		Assert((u8*)next == (u8*)chunk + GetChunkSize(chunk), "Next is invalid at this point");
		next->size &= ~(MEMORY_PREVINUSE_FLAG); //remove INUSE flag from next chunk
	}
	
#if MEMORY_PRINT_ARENA_ACTIONS
	Logf("memory","Deleted an arena[0x%p]%s with %zu bytes (triggered at %s:%zu)", arena, (memory_get_address_name(arena)) ? memory_get_address_name(arena).str : "", arena->size, file, line);
#endif //MEMORY_PRINT_ARENA_ACTIONS
	memory_set_address_name(arena, cstring{});
	
	deshi__arena_heap->used -= used_amount;
	ZeroMemory(zero_pointer, zero_amount);
	
	DEBUG_CheckHeap(deshi__arena_heap); DEBUG_CheckArenaHeapArenas(); DEBUG_PrintArenaHeapChunks();
}

Heap*
deshi__memory_arena_expose(){
	return deshi__arena_heap;
}


//////////////////
//// @generic ////
//////////////////
local Arena* deshi__generic_arena; //deshi__generic_heap is stored here; not used otherwise
local Heap* deshi__generic_heap;

#if MEMORY_PRINT_GENERIC_CHUNKS
FORCE_INLINE void DEBUG_PrintGenericHeapChunks(){ DEBUG_PrintHeapChunks(deshi__generic_heap,"Generic Heap"); }
#else //MEMORY_PRINT_GENERIC_CHUNKS
#  define DEBUG_PrintGenericHeapChunks()
#endif //MEMORY_PRINT_GENERIC_CHUNKS

local void*
AllocateLibc(upt aligned_size){ //NOTE expects pre-aligned size with chunk
	MemChunk* new_chunk = (MemChunk*)calloc(1, aligned_size);
	Assert(new_chunk, "Libc failed to allocate memory");
	new_chunk->size = aligned_size | MEMORY_LIBC_FLAG;
	return ChunkToMemory(new_chunk);
}

local void* 
ReallocateLibc(void* ptr, upt aligned_size){ //NOTE expects pre-aligned size with chunk
	MemChunk* chunk = MemoryToChunk(ptr);
	upt old_size = GetChunkSize(chunk);
	chunk = (MemChunk*)realloc(chunk, aligned_size);
	Assert(chunk, "libc failed to allocate memory");
	chunk->size = aligned_size | MEMORY_LIBC_FLAG;
	if(aligned_size > old_size) ZeroMemory((u8*)chunk + old_size, aligned_size - old_size);
	return ChunkToMemory(chunk);
}

local void
FreeLibc(void* ptr){
	MemChunk* chunk = MemoryToChunk(ptr);
	Assert(ChunkIsLibc(chunk), "This was not allocated using libc");
	free(chunk);
}

void*
deshi__memory_generic_allocate(upt requested_size, char* file, upt line){
	DEBUG_CheckHeap(deshi__generic_heap);
	if(requested_size == 0) return 0;
	Assert(deshi__generic_heap && deshi__generic_heap->initialized, "Attempted to allocate before memory_init() has been called");
	
	//include chunk overhead, align to the byte alignment, and clamp the minimum
	upt aligned_size = ClampMin(RoundUpTo(requested_size + MEMORY_CHUNK_OVERHEAD, MEMORY_BYTE_ALIGNMENT), MEMORY_MIN_CHUNK_SIZE);
	void* result = 0;
	
	//if the allocation is large, create an arena for it rather than using the generic heap
	//NOTE when generic allocations use arena, the layout ends up like Chunk -> Arena -> Chunk
	//     where the first chunk is for the arena and the second is for the generic allocation
	if(aligned_size > MEMORY_MAX_GENERIC_SIZE){
		Arena* arena = deshi__memory_arena_create(aligned_size, file, line);
		arena->used = aligned_size;
		Assert(arena->size >= aligned_size, "Arena size must be greater than requested size");
		
		MemChunk* chunk = (MemChunk*)arena->start;
		chunk->size = arena->size | MEMORY_ISARENAD_FLAG;
		result = ChunkToMemory(chunk);
		
#if MEMORY_PRINT_GENERIC_ACTIONS
		Logf("memory","Created an allocation[0x%p] with %zu bytes (triggered at %s:%zu)", result, requested_size, file, line);
#endif //MEMORY_PRINT_GENERIC_ACTIONS
		
		return result;
	}
	
	//check if there are any empty chunks that can hold the allocation
	for(Node* node = deshi__generic_heap->empty_nodes.next; node != &deshi__generic_heap->empty_nodes; node = node->next){
		MemChunk* chunk = CastFromMember(MemChunk, node, node);
		upt chunk_size = GetChunkSize(chunk); //NOTE remember that chunk size includes the overhead
		if(chunk_size >= aligned_size){
			upt leftover_size = chunk_size - aligned_size;
			Assert(leftover_size % MEMORY_BYTE_ALIGNMENT == 0, "Memory was not aligned correctly");
			MemChunk* next = GetNextOrderChunk(chunk);
			
			//make new empty chunk after current chunk if there is enough space for an empty chunk
			//NOTE '>=' rather than '>' because empty chunks use 8/16 bytes (Node) that in-use chunks dont (useful for small allocations)
			if(leftover_size >= MEMORY_MIN_CHUNK_SIZE){
				MemChunk* new_chunk = GetChunkAtOffset(chunk, aligned_size);
				NodeInsertNext(&deshi__generic_heap->empty_nodes, &new_chunk->node);
				new_chunk->prev = chunk;
				new_chunk->size = leftover_size; //NOTE this gets OR'd with MEMORY_GENERIC_PREVINUSE_FLAG below
				next->prev = new_chunk;
				next = new_chunk;
				deshi__generic_heap->used += sizeof(MemChunk);
			}else{
				aligned_size += leftover_size;
			}
			
			//convert empty node to order node  //NOTE chunk->prev doesnt need to change
			NodeRemove(&chunk->node);
			chunk->node = {0}; //zero this data since it will be used by the allocation
			chunk->size = (PrevChunkIsInUse(chunk)) ? aligned_size | MEMORY_PREVINUSE_FLAG : aligned_size;
			next->size |= MEMORY_PREVINUSE_FLAG; //set PREVINUSE flag on next chunk
			deshi__generic_heap->used += aligned_size - sizeof(MemChunk);
			result = ChunkToMemory(chunk);
			
#if MEMORY_PRINT_GENERIC_ACTIONS
			Logf("memory","Created an allocation[0x%p] with %zu bytes (triggered at %s:%zu)", result, aligned_size, file, line);
#endif //MEMORY_PRINT_GENERIC_ACTIONS
			
			DEBUG_CheckHeap(deshi__generic_heap); DEBUG_PrintGenericHeapChunks();
			return result;
		}
	}
	
	//if we cant replace an empty node, make a new order node for the allocation (if there is space)
	if(deshi__generic_heap->cursor + aligned_size > deshi__generic_heap->start + deshi__generic_heap->size){
		LogfE("memory","Deshi ran out of generic memory when attempting to allocate %zu bytes (triggered at %s:%zu); defaulting to libc calloc.", requested_size, file, line);
		result = AllocateLibc(aligned_size);
		
#if MEMORY_PRINT_GENERIC_ACTIONS
		Logf("memory","Created a libc allocation[0x%p] with %zu bytes (triggered at %s:%zu)", result, requested_size, file, line);
#endif //MEMORY_PRINT_GENERIC_ACTIONS
		
		return result;
	}
	
	MemChunk* new_chunk = (MemChunk*)deshi__generic_heap->cursor;
	new_chunk->prev = deshi__generic_heap->last_chunk;
	new_chunk->size = (deshi__generic_heap->last_chunk != 0) ? aligned_size | MEMORY_PREVINUSE_FLAG : aligned_size;
	deshi__generic_heap->cursor    += aligned_size;
	deshi__generic_heap->used      += aligned_size;
	deshi__generic_heap->last_chunk = new_chunk;
	result = ChunkToMemory(new_chunk);
	
#if MEMORY_PRINT_GENERIC_ACTIONS
	Logf("memory","Created an allocation[0x%p] with %zu bytes (triggered at %s:%zu)", result, requested_size, file, line);
#endif //MEMORY_PRINT_GENERIC_ACTIONS
	
	DEBUG_CheckHeap(deshi__generic_heap); DEBUG_PrintGenericHeapChunks();
	return result;
}

void*
deshi__memory_generic_reallocate(void* ptr, upt requested_size, char* file, upt line){
	DEBUG_CheckHeap(deshi__generic_heap); 
	if(ptr == 0) return deshi__memory_generic_allocate(requested_size, file, line);
	if(requested_size == 0){ deshi__memory_generic_zero_free(ptr, file, line); return 0; }
	Assert(deshi__generic_heap && deshi__generic_heap->initialized, "Attempted to allocate before memory_init() has been called");
	
	//include chunk overhead, align to the byte alignment, and clamp the minimum
	upt aligned_size = ClampMin(RoundUpTo(requested_size + MEMORY_CHUNK_OVERHEAD, MEMORY_BYTE_ALIGNMENT), MEMORY_MIN_CHUNK_SIZE);
	MemChunk* chunk = MemoryToChunk(ptr);
	void* result = ptr;
	
	if(ChunkIsLibc(chunk)){
		result = ReallocateLibc(ptr, aligned_size);
		
		memory_set_address_name(result, memory_get_address_name(ptr));
#if MEMORY_PRINT_GENERIC_ACTIONS
		Logf("memory","Reallocated a libc allocation[0x%p]%s to [0x%p] with %zu bytes (triggered at %s:%zu)", ptr, (memory_get_address_name(ptr)) ? memory_get_address_name(ptr).str : "", result, requested_size, file, line);
#endif //MEMORY_PRINT_GENERIC_ACTIONS
		memory_set_address_name((ptr != result) ? ptr : 0, cstring{});
		return result;
	}
	Assert(ptr > deshi__arena_heap->start && ptr < deshi__arena_heap->cursor, "Attempted to reallocate a pointer outside the main heap and missing the libc flag");
	
	//previous allocation was an arena, so grow arena if new size is greater
	//NOTE when generic allocations use arena, the layout ends up like Chunk -> Arena -> Chunk
	//     where the first chunk is for the arena and the second is for the generic allocation
	if(ChunkIsArenad(chunk)){ 
		if(aligned_size <= GetChunkSize(chunk)) return ptr; //do nothing if less than previous size
		
		Arena* arena = (Arena*)(chunk-1);
		arena = deshi__memory_arena_grow(arena, aligned_size - arena->size, file, line);
		Assert(arena->size >= aligned_size, "Arena size must be greater than requested size");
		arena->used = aligned_size;
		
		chunk = (MemChunk*)arena->start;
		chunk->size = arena->size | MEMORY_ISARENAD_FLAG;
		result = ChunkToMemory(chunk);
		
		memory_set_address_name(result, memory_get_address_name(ptr));
#if MEMORY_PRINT_GENERIC_ACTIONS
		Logf("memory","Reallocated an allocation[0x%p]%s to [0x%p] with %zu bytes (triggered at %s:%zu)", ptr, (memory_get_address_name(ptr)) ? memory_get_address_name(ptr).str : "", result, requested_size, file, line);
#endif //MEMORY_PRINT_GENERIC_ACTIONS
		memory_set_address_name((ptr != result) ? ptr : 0, cstring{});
		DEBUG_CheckHeap(deshi__generic_heap); DEBUG_PrintGenericHeapChunks();
		return result;
	}
	Assert(ptr > deshi__generic_heap->start && ptr < deshi__generic_heap->cursor, "Attempted to reallocate a pointer outside the generic heap and missing the libc flag");
	
	//new allocation needs to be an arena and wasnt before, so copy memory to new arena and free old allocation
	if(aligned_size > MEMORY_MAX_GENERIC_SIZE){
		//NOTE since its larger than MEMORY_MAX_GENERIC_SIZE and not an arena already, it cant be less than previous size
		Arena* arena = deshi__memory_arena_create(aligned_size, file, line);
		arena->used = aligned_size;
		memcpy(arena->start, chunk, GetChunkSize(chunk));
		
		chunk = (MemChunk*)arena->start;
		chunk->prev = 0;
		chunk->size = arena->size | MEMORY_ISARENAD_FLAG;
		result = ChunkToMemory(chunk);
		
		memory_set_address_name(result, memory_get_address_name(ptr));
#if MEMORY_PRINT_GENERIC_ACTIONS
		Logf("memory","Reallocated an allocation[0x%p]%s to [0x%p] with %zu bytes (triggered at %s:%zu)", ptr, (memory_get_address_name(ptr)) ? memory_get_address_name(ptr).str : "", result, requested_size, file, line);
#endif //MEMORY_PRINT_GENERIC_ACTIONS
		
		deshi__memory_generic_zero_free(ptr, file, line);
		
		DEBUG_CheckHeap(deshi__generic_heap); DEBUG_PrintGenericHeapChunks();
		return result;
	}
	
	//if requested size is the same as previous size, do nothing
	spt difference = (spt)GetChunkSize(chunk) - (spt)aligned_size;
	if(difference == 0){
		return ptr;
	}
	
	//there is no used memory after this, so just adjust chunk size and heap cursor
	if(chunk == deshi__generic_heap->last_chunk){
		//if out of memory, default to libc
		if((deshi__generic_heap->cursor - difference) > (deshi__generic_heap->start + deshi__generic_heap->size)){
			LogfE("memory","Deshi ran out of generic memory when attempting to reallocate a ptr[0x%p]%s (triggered at %s:%zu); defaulting to libc calloc.", ptr, (memory_get_address_name(ptr)) ? memory_get_address_name(ptr).str : "", file, line);
			
			result = AllocateLibc(aligned_size);
			MemChunk* new_chunk = MemoryToChunk(result);
			memcpy(&new_chunk->node, &chunk->node, GetChunkSize(chunk) - MEMORY_CHUNK_OVERHEAD);
			
			memory_set_address_name(result, memory_get_address_name(ptr));
#if MEMORY_PRINT_GENERIC_ACTIONS
			Logf("memory","Reallocated an allocation[0x%p]%s to libc [0x%p] with %zu bytes (triggered at %s:%zu)", ptr, (memory_get_address_name(ptr)) ? memory_get_address_name(ptr).str : "", result, requested_size, file, line);
#endif //MEMORY_PRINT_GENERIC_ACTIONS
			
			deshi__memory_generic_zero_free(ptr);
			return result;
		}
		
		deshi__generic_heap->cursor -= difference;
		if(difference > 0) ZeroMemory(deshi__generic_heap->cursor, difference);
		deshi__generic_heap->used   -= difference;
		chunk->size = (PrevChunkIsInUse(chunk)) ? aligned_size | MEMORY_PREVINUSE_FLAG : aligned_size;
		
#if MEMORY_PRINT_GENERIC_ACTIONS
		Logf("memory","Reallocated an allocation[0x%p]%s with %zu bytes (triggered at %s:%zu)", ptr, (memory_get_address_name(ptr)) ? memory_get_address_name(ptr).str : "", requested_size, file, line);
#endif //MEMORY_PRINT_GENERIC_ACTIONS
		
		DEBUG_CheckHeap(deshi__generic_heap); DEBUG_PrintGenericHeapChunks();
		return ptr;
	}
	
	//if requested size is less than previous size and there is enough space for a new chunk, make a new chunk
	if(difference >= (spt)MEMORY_MIN_CHUNK_SIZE){
		MemChunk* next = GetNextOrderChunk(chunk);
		MemChunk* new_chunk = GetChunkAtOffset(chunk, aligned_size);
		NodeInsertNext(&deshi__generic_heap->empty_nodes, &new_chunk->node);
		new_chunk->prev = chunk;
		new_chunk->size = (upt)difference | MEMORY_PREVINUSE_FLAG;
		chunk->size -= difference;
		next->prev = new_chunk;
		next->size &= ~(MEMORY_PREVINUSE_FLAG); //remove INUSE flag from next chunk
		deshi__generic_heap->used -= difference;
		deshi__generic_heap->used += sizeof(MemChunk);
		
#if MEMORY_PRINT_GENERIC_ACTIONS
		Logf("memory","Reallocated an allocation[0x%p]%s with %zu bytes (triggered at %s:%zu)", ptr, (memory_get_address_name(ptr)) ? memory_get_address_name(ptr).str : "", requested_size, file, line);
#endif //MEMORY_PRINT_GENERIC_ACTIONS
		
		DEBUG_CheckHeap(deshi__generic_heap); DEBUG_PrintGenericHeapChunks();
		return ptr;
	}
	
	//if requested size is less than previous size and there is not enough space for a new chunk, do nothing
	if(difference > 0){
		return ptr;
	}
	
	//if requested size is greater than previous size and next chunk is empty and next chunk can hold growth, grow into that chunk
	MemChunk* next = GetNextOrderChunk(chunk);
	upt next_chunk_size = GetChunkSize(next);
	if((next != deshi__generic_heap->last_chunk) && !ChunkIsInUse(next) && (next_chunk_size >= aligned_size)){
		upt leftover_size = next_chunk_size + difference;
		Assert(leftover_size % MEMORY_BYTE_ALIGNMENT == 0, "Memory was not aligned correctly");
		
		//make new empty chunk after current chunk if there is enough space for an empty chunk
		//NOTE '>=' rather than '>' because empty chunks use 8/16 bytes (Node) that in-use chunks dont (useful for small allocations)
		MemChunk* next_next = GetNextOrderChunk(next);
		if(leftover_size >= MEMORY_MIN_CHUNK_SIZE){
			MemChunk* new_chunk = GetChunkAtOffset(chunk, aligned_size);
			NodeInsertNext(&deshi__generic_heap->empty_nodes, &new_chunk->node);
			new_chunk->prev = chunk;
			new_chunk->size = leftover_size | MEMORY_PREVINUSE_FLAG;
			next_next->prev = new_chunk;
			deshi__generic_heap->used += sizeof(MemChunk);
		}else{
			aligned_size += leftover_size;
		}
		
		//zero next's overhead for use by current chunk and grow current chunk
		NodeRemove(&next->node);
		next->prev = 0;
		next->size = 0;
		next->node = {0};
		chunk->size = (PrevChunkIsInUse(chunk)) ? aligned_size | MEMORY_PREVINUSE_FLAG : aligned_size;
		deshi__generic_heap->used += -difference - sizeof(MemChunk);
		
#if MEMORY_PRINT_GENERIC_ACTIONS
		Logf("memory","Reallocated an allocation[0x%p]%s with %zu bytes (triggered at %s:%zu)", ptr, (memory_get_address_name(ptr)) ? memory_get_address_name(ptr).str : "", aligned_size, file, line);
#endif //MEMORY_PRINT_GENERIC_ACTIONS
		
		DEBUG_CheckHeap(deshi__generic_heap); DEBUG_PrintGenericHeapChunks();
		return ptr;
	}
	
	//requested size is greater than previous size and we need to move memory in order to fit the new size
	result = deshi__memory_generic_allocate(requested_size, file, line);
	Assert(GetChunkSize(MemoryToChunk(result)) >= GetChunkSize(chunk) - MEMORY_CHUNK_OVERHEAD, "New chunk size must be greater than previous size");
	memcpy(result, &chunk->node, GetChunkSize(chunk) - MEMORY_CHUNK_OVERHEAD);
	chunk = MemoryToChunk(result);
	
	memory_set_address_name(result, memory_get_address_name(ptr));
#if MEMORY_PRINT_GENERIC_ACTIONS
	Logf("memory","Reallocated an allocation[0x%p%s] to [0x%p] with %zu bytes (triggered at %s:%zu)", ptr, (memory_get_address_name(ptr)) ? memory_get_address_name(ptr).str : "", result, requested_size, file, line);
#endif //MEMORY_PRINT_GENERIC_ACTIONS
	
	deshi__memory_generic_zero_free(ptr, file, line);
	
	DEBUG_CheckHeap(deshi__generic_heap); DEBUG_PrintGenericHeapChunks();
	return result;
}

void
deshi__memory_generic_zero_free(void* ptr, char* file, upt line){
	DEBUG_CheckHeap(deshi__generic_heap);
	if(ptr == 0) return;
	
	MemChunk* chunk = MemoryToChunk(ptr);
	
	//if allocated from libc, free with libc
	if(ChunkIsLibc(chunk)){
		FreeLibc(ptr);
		
#if MEMORY_PRINT_GENERIC_ACTIONS
		Logf("memory","Freed a libc allocation[0x%p]%s (triggered at %s:%zu)", ptr, (memory_get_address_name(ptr)) ? memory_get_address_name(ptr).str : "", file, line);
#endif //MEMORY_PRINT_GENERIC_ACTIONS
		memory_set_address_name(ptr, cstring{});
		
		return;
	}
	Assert(ptr > deshi__arena_heap->start && ptr < deshi__arena_heap->cursor, "Attempted to free a pointer outside the main heap and missing the libc flag");
	
	//if allocation used an arena, delete the arena
	//NOTE when generic allocations use arena, the layout ends up like Chunk -> Arena -> Chunk
	//     where the first chunk is for the arena and the second is for the generic allocation
	if(ChunkIsArenad(chunk)){
		deshi__memory_arena_delete((Arena*)chunk - 1, file, line);
		
#if MEMORY_PRINT_GENERIC_ACTIONS
		Logf("memory","Freed an allocation  [0x%p]%s (triggered at %s:%zu)", ptr, (memory_get_address_name(ptr)) ? memory_get_address_name(ptr).str : "", file, line);
#endif //MEMORY_PRINT_GENERIC_ACTIONS
		memory_set_address_name(ptr, cstring{});
		
		DEBUG_CheckHeap(deshi__generic_heap); DEBUG_PrintGenericHeapChunks();
		return;
	}
	Assert(ptr > deshi__generic_heap->start && ptr < deshi__generic_heap->cursor, "Attempted to free a pointer outside the generic heap");
	
	upt   chunk_size   = GetChunkSize(chunk);
	void* zero_pointer = chunk+1;
	upt   zero_amount  = chunk_size - sizeof(MemChunk);
	upt   used_amount  = zero_amount;
	
	//insert current chunk into heap's empty nodes (as first empty node for locality)
	NodeInsertNext(&deshi__generic_heap->empty_nodes, &chunk->node);
	
	//try to merge next empty into current empty
	MemChunk* next = GetNextOrderChunk(chunk);
	if(   (chunk != deshi__generic_heap->last_chunk) //we are not the last chunk
	   && (next  != deshi__generic_heap->last_chunk) //next is not the last chunk
	   && (!ChunkIsInUse(next))){                    //next is empty
		MemChunk* next_next = GetNextOrderChunk(next);
		next_next->prev = chunk;
		NodeRemove(&next->node);
		chunk->size += GetChunkSize(next);
		//NOTE zero_pointer doesnt change
		zero_amount += sizeof(MemChunk); //NOTE next's memory is already zeroed, so only zero the chunk header
		used_amount += sizeof(MemChunk);
		next = next_next;
	}
	
	//try to merge current empty into prev empty
	MemChunk* prev = GetPrevOrderChunk(chunk);
	if(   (prev != 0)                  //we are not the first chunk
	   && (!PrevChunkIsInUse(chunk))){ //previous chunk is empty
		if(chunk == deshi__generic_heap->last_chunk){
			deshi__generic_heap->last_chunk = prev;
		}else{
			next->prev = prev;
		}
		NodeRemove(&chunk->node);
		NodeRemove(&prev->node); //NOTE remove and reinsert as first empty node for locality
		NodeInsertNext(&deshi__generic_heap->empty_nodes, &prev->node);
		prev->size += chunk->size; //NOTE not GetChunkSize(chunk) b/c we know that PREVINUSE flag is not there
		zero_pointer = chunk; //NOTE prev's memory is already zeroed, so only zero chunk
		zero_amount  = GetChunkSize(chunk);
		used_amount += sizeof(MemChunk);
		chunk = prev;
	}
	
	//remove the last order chunk if its empty
	if(chunk == deshi__generic_heap->last_chunk){
		NodeRemove(&chunk->node);
		deshi__generic_heap->last_chunk = chunk->prev;
		deshi__generic_heap->cursor = (u8*)chunk;
		zero_pointer = chunk;
		zero_amount  = GetChunkSize(chunk);
		used_amount += sizeof(MemChunk);
	}else{
		Assert((u8*)next == (u8*)chunk + GetChunkSize(chunk), "Next is invalid at this point");
		next->size &= ~(MEMORY_PREVINUSE_FLAG); //remove INUSE flag from next chunk
	}
	
	deshi__generic_heap->used -= used_amount;
	ZeroMemory(zero_pointer, zero_amount);
	
#if MEMORY_PRINT_GENERIC_ACTIONS
	Logf("memory","Freed an allocation  [0x%p]%s (triggered at %s:%zu)", ptr, (memory_get_address_name(ptr)) ? memory_get_address_name(ptr).str : "", file, line);
#endif //MEMORY_PRINT_GENERIC_ACTIONS
	memory_set_address_name(ptr, cstring{});
	
	DEBUG_CheckHeap(deshi__generic_heap); DEBUG_PrintGenericHeapChunks();
}

Heap*
deshi__memory_generic_expose(){
	return deshi__generic_heap;
}

////////////////////
//// @temporary ////
////////////////////
local Arena deshi__temp_arena_;
local Arena* deshi__temp_arena = &deshi__temp_arena_;

void*
deshi__memory_temp_allocate(upt size, char* file, upt line){
	if(size == 0) return 0;
	Assert(deshi__temp_arena, "Attempted to temp allocate before memory_init() has been called");
	
	upt aligned_size = RoundUpTo(size + sizeof(upt), MEMORY_BYTE_ALIGNMENT);
	if(deshi__temp_arena->used + aligned_size > deshi__temp_arena->size){
		LogfE("memory","Deshi ran out of temporary memory when attempting to allocate %zu bytes (triggered at %s:%zu); defaulting to libc calloc which will not be automatically freed by clearing the temporary storage (aka: a memory leak)!", size, file, line);
		upt* size_ptr = (upt*)calloc(1, aligned_size);
		Assert(size_ptr, "libc failed to allocate memory");
		*size_ptr = aligned_size | 0x1;
		return size_ptr+1;
	}
	
	void* result = deshi__temp_arena->cursor + sizeof(upt);
	*((upt*)deshi__temp_arena->cursor) = aligned_size; //place allocation size at cursor
	deshi__temp_arena->cursor += aligned_size;
	deshi__temp_arena->used += aligned_size;
	
#if MEMORY_PRINT_TEMP_ACTIONS
	Logf("memory","Created a temp allocation[0x%p] with %zu bytes (triggered at %s:%zu)", result, aligned_size, file, line);
#endif //MEMORY_PRINT_TEMP_ACTIONS
	return result;
}

void*
deshi__memory_temp_reallocate(void* ptr, upt size, char* file, upt line){
	if(size == 0) return 0;
	if(ptr == 0) return 0;
	
#if MEMORY_PRINT_TEMP_ACTIONS
	Logf("memory","Reallocating a temp ptr[0x%p]%s to %zu bytes (triggered at %s:%zu)", ptr, (memory_get_address_name(ptr)) ? memory_get_address_name(ptr).str : "", size, file, line);
#endif //MEMORY_PRINT_TEMP_ACTIONS
	
	upt* size_ptr = (upt*)ptr - 1;
	upt  prev_size = *size_ptr; //including overhead and flags
	upt  aligned_size = RoundUpTo(size + sizeof(upt), MEMORY_BYTE_ALIGNMENT);
	
	//allocation was done with libc
	if(HasFlag(prev_size, 0x1)){
		size_ptr = (upt*)realloc(size_ptr, aligned_size);
		Assert(size_ptr, "libc failed to allocate memory");
		*size_ptr = aligned_size | 0x1;
		return size_ptr+1;
	}
	
	//if new size is less than previous size, just change the size header
	if(aligned_size <= prev_size){
		*size_ptr = aligned_size;
		return ptr;
	}
	
	//if its greater, then move the memory to a new allocation
	void* new_ptr = deshi__memory_temp_allocate(size, file, line);
	memcpy(new_ptr, ptr, prev_size - sizeof(upt)); 
	return new_ptr;
}

void
deshi__memory_temp_clear(){
#if MEMORY_PRINT_TEMP_ACTIONS
	Logf("memory","Clearing temporary memory which used %zu bytes", deshi__temp_arena->used);
#endif //MEMORY_PRINT_TEMP_ACTIONS
	
	memory_clear_arena(deshi__temp_arena);
}

Arena*
deshi__memory_temp_expose(){
	return deshi__temp_arena;
}


////////////////
//// @debug ////
////////////////
#if DESHI_INTERNAL
#  define MEMORY_NAMING_MAX_COUNT 4096 //arbitrary limit

local Arena* deshi__naming_arena; //we don't use arena cursor here, so it's value will be invalid

local b32
AddressNameInfo_LessThan(const AddressNameInfo& a, const AddressNameInfo& b){
	return a.address < b.address;
}

void
deshi__memory_naming_set(void* address, cstring name){
	if(address == 0) return;
	
	//binary search for address index (or index to insert at)
	carray<AddressNameInfo> arr{(AddressNameInfo*)deshi__naming_arena->start, deshi__naming_arena->used / sizeof(AddressNameInfo)};
	spt index = -1;
	spt middle = 0;
	if(arr.count > 0){
		spt left  = 0;
		spt right = arr.count-1;
		while(left <= right){
			middle = left + ((right - left) / 2);
			if(arr[middle].address == address){
				index = middle;
				break;
			}
			if(arr[middle].address < address){
				left = middle + 1;
				middle = left + ((right - left) / 2);
			}else{
				right = middle - 1;
			}
		}
	}
	
	if(name){ //set address name
		if(index != -1){ //info already exists
			//arr[index].address = address; //NOTE the address doesnt change
			arr[index].name = name;
		}else{ //make a new info
			if(deshi__naming_arena->used >= deshi__naming_arena->size){ 
				LogE("memory","Address naming arena is out of space"); 
				return; 
			}
			array_insert(arr, AddressNameInfo{ address, name }, middle);
			deshi__naming_arena->used += sizeof(AddressNameInfo);
		}
	}else if(index != -1){ //remove address name
		array_remove_ordered(arr, index);
		deshi__naming_arena->used -= sizeof(AddressNameInfo);
	}
}

cstring
memory_get_address_name(void* address){
	if(address == 0) return {};
	if(deshi__naming_arena->used == 0) return {};
	
	carray<AddressNameInfo> arr{
		(AddressNameInfo*)deshi__naming_arena->start,
		deshi__naming_arena->used / sizeof(AddressNameInfo)
	};
	upt index = binary_search(arr, AddressNameInfo{address}, AddressNameInfo_LessThan);
	return (index != -1) ? arr[index].name : cstring{};
}

Arena*
deshi__memory_naming_expose(){
	return deshi__naming_arena;
}

void
deshi__memory_draw(){
	auto bytes_sigfigs = [](upt bytes, char& character, f32& divisor){
		if(bytes >= Kilobytes(1)){
			character = 'K'; divisor = Kilobytes(1);
			if(bytes >= Megabytes(1)){
				character = 'M'; divisor = Megabytes(1);
			}
		}
	};
	
	UI::PushColor(UIStyleCol_Border,    Color_Grey);
	UI::PushColor(UIStyleCol_Separator, Color_Grey);
	UI::PushVar(UIStyleVar_WindowPadding, vec2::ZERO);
	UI::PushVar(UIStyleVar_ItemSpacing,   vec2::ZERO);
	{UI::Begin("deshi_memory", DeshWindow->dimensions/4.f, DeshWindow->dimensions/2.f, UIWindowFlags_NoScroll);
		UIWindow* window = UI::GetWindow();
		char used_char = ' ', size_char = ' ';
		f32  used_divisor = 1.f, size_divisor = 1.f;
		
		//left panel: generic heap
		UI::SetNextWindowSize({window->width*.5f, window->height*.9f});
		{UI::BeginChild("deshi_memory_generic", vec2::ZERO, UIWindowFlags_NoScroll | UIWindowFlags_NoBorder | UIWindowFlags_NoResize);
			bytes_sigfigs(deshi__generic_heap->used, used_char, used_divisor);
			bytes_sigfigs(deshi__generic_heap->size, size_char, size_divisor);
			UI::TextF("Generic Heap    %.2f %cB / %.2f %cB", (f32)deshi__generic_heap->used / used_divisor, used_char, (f32)deshi__generic_heap->size / size_divisor, size_char);
			UI::RectFilled(UI::GetPositionForNextItem(), UI::GetWindowRemainingSpace(), Color_VeryDarkRed);
			//TODO this
		}UI::EndChild();
		
		//right panel: arena heap
		UI::SameLine();
		UI::SetNextWindowSize({window->width*.5f, window->height*.9f});
		{UI::BeginChild("deshi_memory_arena", vec2::ZERO, UIWindowFlags_NoScroll | UIWindowFlags_NoBorder | UIWindowFlags_NoResize);
			bytes_sigfigs(deshi__arena_heap->used, used_char, used_divisor);
			bytes_sigfigs(deshi__arena_heap->size, size_char, size_divisor);
			UI::TextF("Arena Heap    %.2f %cB / %.2f %cB", (f32)deshi__arena_heap->used / used_divisor, used_char, (f32)deshi__arena_heap->size / size_divisor, size_char);
			UI::RectFilled(UI::GetPositionForNextItem(), UI::GetWindowRemainingSpace(), Color_VeryDarkGreen);
			//TODO this
		}UI::EndChild();
		
		//bottom panel: temp arena
		UI::SetNextWindowSize({window->width, window->height*.1f});
		{UI::BeginChild("deshi_memory_temp", vec2::ZERO, UIWindowFlags_NoScroll | UIWindowFlags_NoBorder | UIWindowFlags_NoResize);
			bytes_sigfigs(deshi__temp_arena->used, used_char, used_divisor);
			bytes_sigfigs(deshi__temp_arena->size, size_char, size_divisor);
			UI::TextF("Temporary Memory    %.2f %cB / %.2f %cB", (f32)deshi__temp_arena->used / used_divisor, used_char, (f32)deshi__temp_arena->size / size_divisor, size_char);
			
			UI::RectFilled(UI::GetPositionForNextItem(), UI::GetWindowRemainingSpace(), Color_VeryDarkCyan);
			UI::RectFilled(UI::GetPositionForNextItem(), UI::GetWindowRemainingSpace() * vec2{((f32)deshi__temp_arena->used / (f32)deshi__temp_arena->size), 1.f}, Color_DarkCyan);
		}UI::EndChild();
	}UI::End();
	UI::PopVar(2);
	UI::PopColor(2);
}
#endif //DESHI_INTERNAL


////////////////
//// @state ////
////////////////
void
deshi__memory_init(upt main_size, upt temp_size){
	void* base_address = 0;
	u8*   allocation   = 0;
	u64   total_size   = main_size + temp_size;
	
#if DESHI_INTERNAL
	base_address = (void*)Terabytes(2);
#endif //DESHI_INTERNAL
	
#if   DESHI_WINDOWS
	allocation = (u8*)VirtualAlloc((LPVOID)base_address, (SIZE_T)total_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
#elif DESHI_LINUX //DESHI_WINDOWS
#  error not implemented
	allocation = (u8*)calloc(1, total_size);
#elif DESHI_MAC   //DESHI_LINUX
#  error not implemented
	allocation = (u8*)calloc(1, total_size);
#endif            //DESHI_MAC
	Assert(allocation, "Failed to allocate memory from the OS");
	
	deshi__arena_heap->start      = allocation;
	deshi__arena_heap->cursor     = allocation;
	deshi__arena_heap->size       = main_size;
	deshi__arena_heap->used       = 0;
	deshi__arena_heap->empty_nodes.next = deshi__arena_heap->empty_nodes.prev = &deshi__arena_heap->empty_nodes;
	deshi__arena_heap->last_chunk = 0;
	deshi__arena_heap->initialized = true;
	DEBUG_CheckHeap(deshi__arena_heap);
	
	deshi__generic_arena = memory_create_arena(Megabytes(64)+sizeof(Heap));
	deshi__generic_heap = (Heap*)deshi__generic_arena->start;
	deshi__generic_heap->start      = (u8*)(deshi__generic_heap+1);
	deshi__generic_heap->cursor     = deshi__generic_heap->start;
	deshi__generic_heap->used       = 0;
	deshi__generic_heap->size       = deshi__generic_arena->size - sizeof(Heap);
	deshi__generic_heap->empty_nodes.next = deshi__generic_heap->empty_nodes.prev = &deshi__generic_heap->empty_nodes;
	deshi__generic_heap->last_chunk = 0;
	deshi__generic_heap->initialized = true;
	DEBUG_CheckHeap(deshi__generic_heap);
	
	deshi__temp_arena->start  = deshi__arena_heap->start + deshi__arena_heap->size;
	deshi__temp_arena->cursor = deshi__temp_arena->start;
	deshi__temp_arena->size   = temp_size;
	deshi__temp_arena->used   = 0;
	
#if DESHI_INTERNAL
	deshi__naming_arena = memory_create_arena(MEMORY_NAMING_MAX_COUNT*sizeof(AddressNameInfo));
	memory_set_address_name(deshi__arena_heap,   cstr_lit("Arena Heap"));
	memory_set_address_name(deshi__generic_heap, cstr_lit("Generic Heap"));
	memory_set_address_name(deshi__temp_arena,   cstr_lit("Temp Arena"));
	memory_set_address_name(deshi__naming_arena, cstr_lit("Naming Arena"));
#endif //DESHI_INTERNAL
	
	deshiStage |= DS_MEMORY;
}
