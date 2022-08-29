/*Index:
@memory_vars
@memory_utils
@memory_heap
@memory_arena
@memory_generic
@memory_temp
@memory_debug
@memory_state
*/


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @memory_vars
#if MEMORY_TRACK_ALLOCS
local array<AllocInfo> alloc_infos_active(stl_allocator); //uses libc so it is external the system
local array<AllocInfo> alloc_infos_inactive(stl_allocator);
#endif //MEMORY_TRACK_ALLOCS


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @memory_utils
#define MEMORY_POINTER_SIZE sizeof(void*)
#define MEMORY_BYTE_ALIGNMENT (2*MEMORY_POINTER_SIZE)
#define MEMORY_BYTE_ALIGNMENT_MASK (MEMORY_BYTE_ALIGNMENT-1)
#define MEMORY_CHUNK_OVERHEAD MEMORY_CHUNK_MEMORY_OFFSET
#define MEMORY_ARENA_OVERHEAD (MEMORY_CHUNK_OVERHEAD + sizeof(Arena))
#define MEMORY_MIN_CHUNK_SIZE ((sizeof(MemChunk) + MEMORY_BYTE_ALIGNMENT_MASK) & ~(MEMORY_BYTE_ALIGNMENT_MASK))
#define MEMORY_CHUNK_ALIGNMENT MEMORY_MIN_CHUNK_SIZE
#define MEMORY_MAX_GENERIC_SIZE (Kilobytes(64) - 1)

#if MEMORY_CHECK_HEAPS
local void
DEBUG_CheckHeap(Heap* heap){DPZoneScoped;
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
		for(MemChunk* chunk = (MemChunk*)heap->start; ;chunk = GetNextOrderChunk(chunk)){
			Assert((u8*)chunk < heap->cursor, "All chunks must be below the cursor");
			Assert(GetChunkSize(chunk) >= MEMORY_MIN_CHUNK_SIZE, "Chunk size is less than minimum");
			Assert(GetChunkSize(chunk) % MEMORY_BYTE_ALIGNMENT == 0, "Chunk size is not aligned correctly");
			if(chunk != heap->last_chunk){
				Assert(GetNextOrderChunk(chunk)->prev == chunk, "Next order chunk is not correctly pointing to current chunk");
				if(ChunkIsEmpty(chunk)){
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
DEBUG_PrintHeapChunks(Heap* heap, char* heap_name = 0){DPZoneScoped;
	if(heap->initialized && heap->used > 0){
		string heap_order = "Order: ", heap_empty = "Empty: ";
		for(MemChunk* chunk = (MemChunk*)heap->start; ;chunk = GetNextOrderChunk(chunk)){
			heap_order += stringf("0x%p", chunk); heap_order += " -> ";
			if(chunk != heap->last_chunk){
				if(ChunkIsEmpty(chunk)){
					heap_empty += stringf("0x%p", chunk);
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

local b32 AllocInfo_LessThan(const AllocInfo& a, const AllocInfo& b){ return a.address < b.address; }
local b32 AllocInfo_GreaterThan(const AllocInfo& a, const AllocInfo& b){ return a.address > b.address; }

#if MEMORY_TRACK_ALLOCS
local AllocInfo*
DEBUG_AllocInfo_Creation(void* address, str8 file, upt line){DPZoneScoped;
	spt middle = 0;
	upt index = -1;
	if(alloc_infos_active.count > 0){
		spt left  = 0;
		spt right = alloc_infos_active.count-1;
		while(left <= right){
			middle = left + ((right - left) / 2);
			if(alloc_infos_active[middle].address == address){
				index = middle;
				break;
			}
			if(alloc_infos_active[middle].address < address){
				left = middle + 1;
				middle = left + ((right - left) / 2);
			}else{
				right = middle - 1;
			}
		}
		
		Assert(index == -1, "There is already an existing active AllocInfo with this address");
		alloc_infos_active.insert(AllocInfo{address, CodeLocation{{(char*)file.str, file.count}, u32(line), 0}, DeshTime->totalTime, upt(-1), cstr_lit("")}, middle);
	}else{
		alloc_infos_active.add(AllocInfo{address, CodeLocation{{(char*)file.str, file.count}, u32(line), 0}, DeshTime->totalTime, upt(-1), cstr_lit("")});
	}
	return &alloc_infos_active[middle];
}

local void
DEBUG_AllocInfo_Deletion(void* address){DPZoneScoped;
	if(address == 0) return;
	upt index = binary_search(alloc_infos_active, AllocInfo{address}, AllocInfo_LessThan);
	if(index != -1){
		alloc_infos_active[index].deletion_frame = DeshTime->updateCount;
		alloc_infos_inactive.add(alloc_infos_active[index]);
		bubble_sort(alloc_infos_inactive, AllocInfo_GreaterThan); //TODO use binary_insertion_sort_low_to_high after testing it
		alloc_infos_active.remove(index);
	}
}
#endif //MEMORY_TRACK_ALLOCS


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @memory_heap
Heap*
deshi__memory_heap_init_bytes(upt bytes, str8 file, upt line){DPZoneScoped;
	Heap* result = (Heap*)deshi__memory_generic_allocate(bytes+sizeof(Heap), file, line);
	result->start  = (u8*)(result+1);
	result->cursor = result->start;
	result->size   = bytes;
	result->empty_nodes.next = result->empty_nodes.prev = &result->empty_nodes;
	result->last_chunk = 0;
	result->initialized = true;
	DEBUG_CheckHeap(result);
#if MEMORY_PRINT_HEAP_ACTIONS
	Logf("memory","Initted a heap[0x%p] with %zu bytes (triggered at %s:%zu)", result, bytes, file.str, line);
#endif //MEMORY_PRINT_HEAP_ACTIONS
	return result;
}


void
deshi__memory_heap_deinit(Heap* heap, str8 file, upt line){DPZoneScoped;
#if MEMORY_PRINT_HEAP_ACTIONS
	AllocInfo info = memory_allocinfo_get(heap);
	Logf("memory","Deinitted a heap[0x%p]%s with %zu bytes (triggered at %s:%zu)", heap, info.name.str, arena->size, file.str, line);
#endif //MEMORY_PRINT_HEAP_ACTIONS
	
	deshi__memory_generic_zero_free(heap, file, line);
}


void*
deshi__memory_heap_add_bytes(Heap* heap, upt bytes, str8 file, upt line){DPZoneScoped;
	DEBUG_CheckHeap(heap);
	
	if(g_memory->cleanup_happened) return 0;
	if(bytes == 0) return 0;
	Assert(heap && heap->initialized, "Attempted to allocate before heap_init() has been called");
	
	//include chunk overhead, align to the byte alignment, and clamp the minimum
	upt aligned_size = ClampMin(RoundUpTo(bytes + MEMORY_CHUNK_OVERHEAD, MEMORY_BYTE_ALIGNMENT), MEMORY_MIN_CHUNK_SIZE);
	void* result = 0;
	
	//check if there are any empty chunks that can hold the allocation
	for(Node* node = heap->empty_nodes.next; node != &heap->empty_nodes; node = node->next){
		MemChunk* chunk = CastFromMember(MemChunk, node, node);
		upt chunk_size = GetChunkSize(chunk); //NOTE remember that chunk size includes the overhead
		if(chunk_size >= aligned_size){
			upt leftover_size = chunk_size - aligned_size;
			Assert(leftover_size % MEMORY_BYTE_ALIGNMENT == 0, "Memory was not aligned correctly");
			MemChunk* next = GetNextOrderChunk(chunk);
			
			//make new empty chunk after current chunk if there is enough space for an empty chunk
			//NOTE '>=' rather than '>' because empty chunks use 8/16 bytes (Node) that in-use chunks dont (useful for small allocations)
			if(leftover_size >= MEMORY_MIN_CHUNK_SIZE){
				NodeRemove(&chunk->node); //NOTE remove this early so new_chunk doesnt break next's nodes before removal
				chunk->node = {0};
				
				MemChunk* new_chunk = GetChunkAtOffset(chunk, aligned_size);
				NodeInsertNext(&heap->empty_nodes, &new_chunk->node);
				new_chunk->prev = chunk;
				new_chunk->size = leftover_size | MEMORY_EMPTY_FLAG;
				next->prev = new_chunk;
				next = new_chunk;
				heap->used += sizeof(MemChunk);
			}else{
				aligned_size += leftover_size;
				NodeRemove(&chunk->node);
				chunk->node = {0}; //zero this data since it will be used by the allocation
			}
			
			//convert empty node to order node //NOTE chunk->prev doesnt need to change
			chunk->size = aligned_size;
			heap->used += aligned_size - sizeof(MemChunk);
			result = ChunkToMemory(chunk);
			
#if MEMORY_PRINT_HEAP_ACTIONS
			AllocInfo info = memory_allocinfo_get(heap);
			Logf("memory","Created an allocation[0x%p] in heap[0x%p]%s with %zu bytes (triggered at %s:%zu)", result, heap, info.name.str, aligned_size, file.str, line);
#endif //MEMORY_PRINT_HEAP_ACTIONS
			DEBUG_CheckHeap(heap);
			return result;
		}
	}
	
	//if we cant replace an empty node, make a new order node for the allocation (if there is space)
	MemChunk* new_chunk = (MemChunk*)heap->cursor;
	new_chunk->prev = heap->last_chunk;
	new_chunk->size = aligned_size;
	heap->cursor    += aligned_size;
	heap->used      += aligned_size;
	heap->last_chunk = new_chunk;
	result = ChunkToMemory(new_chunk);
	
#if MEMORY_PRINT_HEAP_ACTIONS
	AllocInfo info = memory_allocinfo_get(heap);
	Logf("memory","Created an allocation[0x%p] in heap[0x%p]%s with %zu bytes (triggered at %s:%zu)", result, heap, info.name.str, aligned_size, file.str, line);
#endif //MEMORY_PRINT_HEAP_ACTIONS
	DEBUG_CheckHeap(heap);
	return result;
}


void
deshi__memory_heap_remove(Heap* heap, void* ptr){DPZoneScoped;
	if(g_memory->cleanup_happened) return;
	if(ptr == 0 || heap == 0) return;
	
	DEBUG_CheckHeap(heap);
	AllocInfo info = deshi__memory_allocinfo_get(ptr);
	MemChunk* chunk = MemoryToChunk(ptr);
	Assert(chunk->size > 0, "A chunk must always have a size");
	Assert(heap->initialized, "Attempted to remove before heap_init() has been called");
	
	upt   chunk_size   = GetChunkSize(chunk);
	void* zero_pointer = chunk+1;
	upt   zero_amount  = chunk_size - sizeof(MemChunk);
	upt   used_amount  = zero_amount;
	
	//insert current chunk into heap's empty nodes (as first empty node for locality)
	NodeInsertNext(&heap->empty_nodes, &chunk->node);
	chunk->size |= MEMORY_EMPTY_FLAG;
	
	//try to merge next empty into current empty
	MemChunk* next = GetNextOrderChunk(chunk);
	if((chunk != heap->last_chunk) && ChunkIsEmpty(next)){
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
	if((prev != 0) && ChunkIsEmpty(prev)){
		if(chunk == heap->last_chunk){
			heap->last_chunk = prev;
		}else{
			next->prev = prev;
		}
		NodeRemove(&chunk->node);
		NodeRemove(&prev->node); //NOTE remove and reinsert as first empty node for locality
		NodeInsertNext(&heap->empty_nodes, &prev->node);
		prev->size += GetChunkSize(chunk);
		zero_pointer = chunk; //NOTE prev's memory is already zeroed, so only zero chunk
		zero_amount  = GetChunkSize(chunk);
		used_amount += sizeof(MemChunk);
		chunk = prev;
	}
	
	//remove the last order chunk if its empty
	if(chunk == heap->last_chunk){
		NodeRemove(&chunk->node);
		heap->last_chunk = chunk->prev;
		heap->cursor = (u8*)chunk;
		zero_pointer = chunk;
		zero_amount  = GetChunkSize(chunk);
		used_amount += sizeof(MemChunk);
	}
	
	heap->used -= used_amount;
	ZeroMemory(zero_pointer, zero_amount);
	
#if MEMORY_PRINT_HEAP_ACTIONS
	AllocInfo info2 = memory_allocinfo_get(heap);
	Logf("memory","Freed an allocation  [0x%p]%s in heap[0x%p]%s (triggered at %s:%zu)", result, info.name.str, heap, info2.name.str, aligned_size, file.str, line);
#endif //MEMORY_PRINT_HEAP_ACTIONS
	DEBUG_CheckHeap(heap);
}


void
deshi__memory_heap_clear(Heap* heap){DPZoneScoped;
	if(g_memory->cleanup_happened) return;
	if(heap == 0) return;
	
#if MEMORY_PRINT_HEAP_ACTIONS
	AllocInfo info = memory_allocinfo_get(heap);
	Logf("memory","Cleared a heap[0x%p]%s with %zu bytes (triggered at %s:%zu)", heap, info.name.str, arena->size, file.str, line);
#endif //MEMORY_PRINT_ARENA_ACTIONS
	
	ZeroMemory(heap->start, heap->size);
	heap->cursor = heap->start;
	heap->used   = 0;
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @memory_arena
#if MEMORY_CHECK_HEAPS
local void
DEBUG_CheckArenaHeapArenas(){DPZoneScoped;
	if(g_memory->arena_heap.initialized && g_memory->arena_heap.used > 0){
		for(MemChunk* chunk = (MemChunk*)g_memory->arena_heap.start; chunk != g_memory->arena_heap.last_chunk; ){
			MemChunk* next = GetNextOrderChunk(chunk);
			if(!ChunkIsEmpty(chunk)){
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
FORCE_INLINE void DEBUG_PrintArenaHeapChunks(){ DEBUG_PrintHeapChunks(arena_heap,"Arena Heap"); }
#else //MEMORY_PRINT_ARENA_CHUNKS
#  define DEBUG_PrintArenaHeapChunks()
#endif //MEMORY_PRINT_ARENA_CHUNKS

local Arena*
CreateArenaLibc(upt aligned_size){DPZoneScoped; //NOTE expects pre-aligned size with arena overhead
	MemChunk* chunk = (MemChunk*)calloc(1, aligned_size);
	Assert(chunk, "libc failed to allocate memory");
	chunk->size = aligned_size | MEMORY_LIBC_FLAG;
	Arena* arena = ChunkToArena(chunk);
	arena->start  = (u8*)(arena+1);
	arena->cursor = arena->start;
	arena->size   = aligned_size - MEMORY_ARENA_OVERHEAD;
	return arena;
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @memory_arena_create
Arena*
deshi__memory_arena_create(upt requested_size, str8 file, upt line){DPZoneScoped;
	DEBUG_CheckHeap(&g_memory->arena_heap);
	
	if(g_memory->cleanup_happened) return 0;
	if(requested_size == 0) return 0;
	Assert(g_memory->arena_heap.initialized, "Attempted to create an arena before memory_init() has been called");
	
	//include chunk and arena overhead, align to the byte alignment (no need to clamp min b/c arena overhead fits Node)
	upt aligned_size = RoundUpTo(requested_size + MEMORY_ARENA_OVERHEAD, MEMORY_BYTE_ALIGNMENT);
	Arena* result = 0;
	
	//check if there are any empty nodes that can hold the new arena
	for(Node* node = g_memory->arena_heap.empty_nodes.next; node != &g_memory->arena_heap.empty_nodes; node = node->next){
		MemChunk* chunk = CastFromMember(MemChunk, node, node);
		upt chunk_size = GetChunkSize(chunk); //NOTE remember that chunk size includes the overhead
		if(chunk_size >= aligned_size){
			upt leftover_size = chunk_size - aligned_size;
			Assert(leftover_size % MEMORY_BYTE_ALIGNMENT == 0, "Memory was not aligned correctly");
			MemChunk* next = GetNextOrderChunk(chunk);
			
			//make new empty chunk after current chunk if there is enough space for arena overhead
			if(leftover_size > MEMORY_ARENA_OVERHEAD){
				NodeRemove(&chunk->node); //NOTE remove this early so new_chunk doesnt break chunk's nodes before removal
				
				MemChunk* new_chunk = GetChunkAtOffset(chunk, aligned_size);
				NodeInsertNext(&g_memory->arena_heap.empty_nodes, &new_chunk->node);
				new_chunk->prev = chunk;
				new_chunk->size = leftover_size | MEMORY_EMPTY_FLAG;
				next->prev = new_chunk;
				next = new_chunk;
				g_memory->arena_heap.used += sizeof(MemChunk);
			}else{
				aligned_size += leftover_size;
				NodeRemove(&chunk->node);
			}
			
			//convert empty node to order node
			//chunk->node = {0}; //NOTE not necessary since its overwritten below
			chunk->size = aligned_size;
			g_memory->arena_heap.used += aligned_size - sizeof(MemChunk);
			
			result = ChunkToArena(chunk);
			result->start  = (u8*)(result+1);
			result->cursor = result->start;
			result->size   = aligned_size - MEMORY_ARENA_OVERHEAD;
			result->used   = 0;
			
#if MEMORY_PRINT_ARENA_ACTIONS
			Logf("memory","Created an arena[0x%p] with %zu bytes (triggered at %s:%zu)", result, result->size, file.str, line);
#endif //MEMORY_PRINT_ARENA_ACTIONS
			DEBUG_CheckHeap(&g_memory->arena_heap);
			DEBUG_CheckArenaHeapArenas();
			DEBUG_PrintArenaHeapChunks();
#if MEMORY_TRACK_ALLOCS
			DEBUG_AllocInfo_Creation(result, file, line);
#endif //MEMORY_TRACK_ALLOCS
			
			Assert(result->size >= requested_size, "Arena size was not greater than or equal to requested size");
			return result;
		}
	}
	
	//if we cant replace an empty node, make a new order node for the allocation (if there is space)
	if(g_memory->arena_heap.cursor + aligned_size > g_memory->arena_heap.start + g_memory->arena_heap.size){
		LogfE("memory","Deshi ran out of main memory when attempting to create an arena (triggered at %s:%zu); defaulting to libc calloc.", file.str, line);
		
		result = CreateArenaLibc(aligned_size);
		
#if MEMORY_PRINT_ARENA_ACTIONS
		Logf("memory","Created a libc arena[0x%p] with %zu bytes (triggered at %s:%zu)", result, result->size, file.str, line);
#endif //MEMORY_PRINT_ARENA_ACTIONS
#if MEMORY_TRACK_ALLOCS
		DEBUG_AllocInfo_Creation(result, file, line);
#endif //MEMORY_TRACK_ALLOCS
		
		Assert(result->size >= requested_size, "Arena size was not greater than or equal to requested size");
		return result;
	}
	
	MemChunk* new_chunk = (MemChunk*)g_memory->arena_heap.cursor;
	new_chunk->prev = g_memory->arena_heap.last_chunk;
	new_chunk->size = aligned_size;
	g_memory->arena_heap.cursor    += aligned_size;
	g_memory->arena_heap.used      += aligned_size;
	g_memory->arena_heap.last_chunk = new_chunk;
	
	result = ChunkToArena(new_chunk);
	result->start  = (u8*)(result+1);
	result->cursor = result->start;
	result->size   = aligned_size - MEMORY_ARENA_OVERHEAD;
	result->used   = 0;
	
#if MEMORY_PRINT_ARENA_ACTIONS
	Logf("memory","Created an arena[0x%p] with %zu bytes (triggered at %s:%zu)", result, result->size, file.str, line);
#endif //MEMORY_PRINT_ARENA_ACTIONS
	DEBUG_CheckHeap(&g_memory->arena_heap);
	DEBUG_CheckArenaHeapArenas();
	DEBUG_PrintArenaHeapChunks();
#if MEMORY_TRACK_ALLOCS
	DEBUG_AllocInfo_Creation(result, file, line);
#endif //MEMORY_TRACK_ALLOCS
	
	Assert(result->size >= requested_size, "Arena size was not greater than or equal to requested size");
	return result;
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @memory_arena_grow
Arena*
deshi__memory_arena_grow(Arena* arena, upt size, str8 file, upt line){DPZoneScoped;
	DEBUG_CheckHeap(&g_memory->arena_heap);
	DEBUG_CheckArenaHeapArenas();
	
	if(g_memory->cleanup_happened) return 0;
	if(size == 0) return arena;
	if(arena == 0) return 0;
	Assert(g_memory->arena_heap.initialized, "Attempted to grow an arena before memory_init() has been called");
	
	upt aligned_size = RoundUpTo(size, MEMORY_BYTE_ALIGNMENT);
	AllocInfo info = deshi__memory_allocinfo_get(arena);
	MemChunk* chunk = ArenaToChunk(arena);
	Arena* result = arena;
	
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
		
#if MEMORY_PRINT_ARENA_ACTIONS
		Logf("memory","Grew a libc arena[0x%p]%s to libc [0x%p] with %zu bytes (triggered at %s:%zu)", arena, info.name.str, result, size, file.str, line);
#endif //MEMORY_PRINT_ARENA_ACTIONS
#if MEMORY_TRACK_ALLOCS
		if(arena != result){
			AllocInfo* new_info = DEBUG_AllocInfo_Creation(result, file, line);
			new_info->name = info.name;
			new_info->type = info.type;
			DEBUG_AllocInfo_Deletion(arena);
		}
#endif //MEMORY_TRACK_ALLOCS
		
		return result;
	}
	Assert((u8*)arena > g_memory->arena_heap.start && (u8*)arena < g_memory->arena_heap.cursor, "Attempted to grow an arena that's outside the arena heap and missing the libc flag");
	
	//current chunk is the last chunk, so grow current
	if(chunk == g_memory->arena_heap.last_chunk){
		if(g_memory->arena_heap.cursor + aligned_size > g_memory->arena_heap.start + g_memory->arena_heap.size){
			LogfE("memory","Deshi ran out of main memory when attempting to grow an arena[0x%p]%s of size %zu bytes with %zu bytes (triggered at %s:%zu); defaulting to libc calloc.", arena, info.name.str, arena->size, size, file.str, line);
			
			result = CreateArenaLibc(arena->size + size);
			result->cursor += (arena->cursor - arena->start);
			result->used    = arena->used;
			memcpy(result->start, arena->start, arena->size);
			
#if MEMORY_PRINT_ARENA_ACTIONS
			Logf("memory","Grew an arena   [0x%p]%s to libc [0x%p] with %zu bytes (triggered at %s:%zu)", arena, info.name.str, result, size, file.str, line);
#endif //MEMORY_PRINT_ARENA_ACTIONS
#if MEMORY_TRACK_ALLOCS
			AllocInfo* new_info = DEBUG_AllocInfo_Creation(result, file, line);
			new_info->name = info.name;
			new_info->type = info.type;
#endif //MEMORY_TRACK_ALLOCS
			
			deshi__memory_arena_delete(arena, file, line);
			return result;
		}
		
		chunk->size += aligned_size;
		arena->size += aligned_size;
		g_memory->arena_heap.cursor += aligned_size;
		g_memory->arena_heap.used   += aligned_size;
		
#if MEMORY_PRINT_ARENA_ACTIONS
		Logf("memory","Grew an arena   [0x%p]%s with %zu bytes (triggered at %s:%zu)", arena, info.name.str, aligned_size, file.str, line);
#endif //MEMORY_PRINT_ARENA_ACTIONS
		DEBUG_CheckHeap(&g_memory->arena_heap);
		DEBUG_CheckArenaHeapArenas();
		DEBUG_PrintArenaHeapChunks();
		
		return result;
	}
	
	//next chunk is empty and can hold the growth, so grow into next
	MemChunk* next = GetNextOrderChunk(chunk);
	upt next_chunk_size = GetChunkSize(next);
	if(ChunkIsEmpty(next) && (next_chunk_size >= aligned_size)){
		upt leftover_size = next_chunk_size - aligned_size;
		Assert(leftover_size % MEMORY_BYTE_ALIGNMENT == 0, "Memory was not aligned correctly");
		
		//make new empty chunk after current chunk if there is enough space for arena overhead
		MemChunk* next_next = GetNextOrderChunk(next);
		if(leftover_size > MEMORY_ARENA_OVERHEAD){
			NodeRemove(&next->node); //NOTE remove this early so new_chunk doesnt break next's nodes before removal
			next->prev = 0;
			next->size = 0;
			next->node = {0};
			
			MemChunk* new_chunk = GetChunkAtOffset(chunk, GetChunkSize(chunk) + aligned_size);
			NodeInsertNext(&g_memory->arena_heap.empty_nodes, &new_chunk->node);
			new_chunk->prev = chunk;
			new_chunk->size = leftover_size | MEMORY_EMPTY_FLAG;
			next_next->prev = new_chunk;
			g_memory->arena_heap.used += sizeof(MemChunk);
		}else{
			aligned_size += leftover_size;
			NodeRemove(&next->node);
			next->prev = 0;
			next->size = 0;
			next->node = {0};
		}
		
		//zero next's overhead for use by current chunk and grow current chunk
		chunk->size += aligned_size;
		result->size += aligned_size;
		g_memory->arena_heap.used += aligned_size - sizeof(MemChunk);
		
#if MEMORY_PRINT_ARENA_ACTIONS
		Logf("memory","Grew an arena   [0x%p]%s with %zu bytes (triggered at %s:%zu)", arena, info.name.str, aligned_size, file.str, line);
#endif //MEMORY_PRINT_ARENA_ACTIONS
		DEBUG_CheckHeap(&g_memory->arena_heap);
		DEBUG_CheckArenaHeapArenas();
		DEBUG_PrintArenaHeapChunks();
		
		return result;
	}
	
	//need to move memory in order to fit new size
	result = deshi__memory_arena_create(arena->size + size, file, line);
	memcpy(result->start, arena->start, arena->used);
	result->used   = arena->used;
	result->cursor = result->start + (arena->cursor - arena->start);
	
#if MEMORY_PRINT_ARENA_ACTIONS
	Logf("memory","Grew an arena   [0x%p]%s to [0x%p] with %zu bytes (triggered at %s:%zu)", arena, info.name.str, result, aligned_size, file.str, line);
#endif //MEMORY_PRINT_ARENA_ACTIONS
	DEBUG_CheckHeap(&g_memory->arena_heap);
	DEBUG_CheckArenaHeapArenas();
	DEBUG_PrintArenaHeapChunks();
#if MEMORY_TRACK_ALLOCS
	memory_allocinfo_set(result, info.name, info.type);
#endif //MEMORY_TRACK_ALLOCS
	
	deshi__memory_arena_delete(arena, file, line);
	return result;
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @memory_arena_clear
void
deshi__memory_arena_clear(Arena* arena, str8 file, upt line){DPZoneScoped;
	if(g_memory->cleanup_happened) return;
	
#if MEMORY_PRINT_ARENA_ACTIONS
	AllocInfo info = memory_allocinfo_get(arena);
	Logf("memory","Cleared an arena[0x%p]%s with %zu bytes (triggered at %s:%zu)", arena, info.name.str, arena->size, file.str, line);
#endif //MEMORY_PRINT_ARENA_ACTIONS
	
	ZeroMemory(arena->start, arena->used);
	arena->cursor = arena->start;
	arena->used   = 0;
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @memory_arena_delete
void
deshi__memory_arena_delete(Arena* arena, str8 file, upt line){DPZoneScoped;
	if(g_memory->cleanup_happened) return;
	if(arena == 0) return;
	
	DEBUG_CheckHeap(&g_memory->arena_heap);
	DEBUG_CheckArenaHeapArenas();
	Assert(g_memory->arena_heap.initialized, "Attempted to delete an arena before memory_init() has been called");
	
	AllocInfo info = deshi__memory_allocinfo_get(arena);
	MemChunk* chunk = ArenaToChunk(arena);
	
#if MEMORY_TRACK_ALLOCS
	DEBUG_AllocInfo_Deletion(arena);
#endif //MEMORY_TRACK_ALLOCS
	
	//if allocated from libc, free with libc
	if(ChunkIsLibc(chunk)){
		free(chunk);
		
#if MEMORY_PRINT_ARENA_ACTIONS
		Logf("memory","Deleted a libc arena[0x%p]%s with %zu bytes (triggered at %s:%zu)", arena, info.name.str, arena->size, file.str, line);
#endif //MEMORY_PRINT_ARENA_ACTIONS
		
		return;
	}
	Assert((u8*)arena > g_memory->arena_heap.start && (u8*)arena < g_memory->arena_heap.cursor, "Attempted to delete an arena outside the main heap and missing the libc flag");
	
	upt   chunk_size   = GetChunkSize(chunk);
	void* zero_pointer = chunk+1;
	upt   zero_amount  = chunk_size - sizeof(MemChunk);
	upt   used_amount  = zero_amount;
	
	//insert current chunk into heap's empty nodes (as first empty node for locality)
	NodeInsertNext(&g_memory->arena_heap.empty_nodes, &chunk->node);
	chunk->size |= MEMORY_EMPTY_FLAG;
	
	//try to merge next empty into current empty
	MemChunk* next = GetNextOrderChunk(chunk);
	if((chunk != g_memory->arena_heap.last_chunk) && ChunkIsEmpty(next)){
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
	if((prev != 0) && ChunkIsEmpty(prev)){
		if(chunk == g_memory->arena_heap.last_chunk){
			g_memory->arena_heap.last_chunk = prev;
		}else{
			next->prev = prev;
		}
		NodeRemove(&chunk->node);
		NodeRemove(&prev->node); //NOTE remove and reinsert as first empty node for locality
		NodeInsertNext(&g_memory->arena_heap.empty_nodes, &prev->node);
		prev->size += GetChunkSize(chunk);
		zero_pointer = chunk; //NOTE prev's memory is already zeroed, so only zero chunk
		zero_amount  = GetChunkSize(chunk);
		used_amount += sizeof(MemChunk);
		chunk = prev;
	}
	
	//remove the last order chunk if its empty
	if(chunk == g_memory->arena_heap.last_chunk){
		NodeRemove(&chunk->node);
		g_memory->arena_heap.last_chunk = chunk->prev;
		g_memory->arena_heap.cursor = (u8*)chunk;
		zero_pointer = chunk;
		zero_amount  = GetChunkSize(chunk);
		used_amount += sizeof(MemChunk);
	}
	
	g_memory->arena_heap.used -= used_amount;
	ZeroMemory(zero_pointer, zero_amount);
	
#if MEMORY_PRINT_ARENA_ACTIONS
	Logf("memory","Deleted an arena[0x%p]%s with %zu bytes (triggered at %s:%zu)", arena, info.name.str, arena->size, file.str, line);
#endif //MEMORY_PRINT_ARENA_ACTIONS
	DEBUG_CheckHeap(&g_memory->arena_heap);
	DEBUG_CheckArenaHeapArenas();
	DEBUG_PrintArenaHeapChunks();
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @memory_arena_other
Heap*
deshi__memory_arena_expose(){DPZoneScoped;
	return &g_memory->arena_heap;
}

template<typename T>
T* memory_arena_add(Arena* arena, const T& item){
	if(arena->cursor-(arena->start+arena->size) > -upt(sizeof(T))) return 0; //TODO maybe handle this better somehow
	memcpy(arena->cursor, &item, sizeof(T));
	arena->cursor+=sizeof(T);
	return (T*)(arena->cursor-sizeof(T));
}

template<typename T>
T* memory_arena_add_new(Arena* arena){
	if(arena->cursor-(arena->start+arena->size) > -upt(sizeof(T))) return 0; //TODO maybe handle this better somehow
	new(arena->cursor) T();
	arena->cursor+=sizeof(T);
	return (T*)(arena->cursor-sizeof(T));
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @memory_generic
#if MEMORY_PRINT_GENERIC_CHUNKS
FORCE_INLINE void DEBUG_PrintGenericHeapChunks(){ DEBUG_PrintHeapChunks(generic_heap,"Generic Heap"); }
#else //MEMORY_PRINT_GENERIC_CHUNKS
#  define DEBUG_PrintGenericHeapChunks()
#endif //MEMORY_PRINT_GENERIC_CHUNKS

local void*
AllocateLibc(upt aligned_size){DPZoneScoped; //NOTE expects pre-aligned size with chunk
	MemChunk* new_chunk = (MemChunk*)calloc(1, aligned_size);
	Assert(new_chunk, "Libc failed to allocate memory");
	new_chunk->size = aligned_size | MEMORY_LIBC_FLAG;
	return ChunkToMemory(new_chunk);
}

local void* 
ReallocateLibc(void* ptr, upt aligned_size){DPZoneScoped; //NOTE expects pre-aligned size with chunk
	MemChunk* chunk = MemoryToChunk(ptr);
	
	upt old_size = GetChunkSize(chunk);
	chunk = (MemChunk*)realloc(chunk, aligned_size);
	Assert(chunk, "libc failed to allocate memory");
	chunk->size = aligned_size | MEMORY_LIBC_FLAG;
	if(aligned_size > old_size) ZeroMemory((u8*)chunk + old_size, aligned_size - old_size);
	return ChunkToMemory(chunk);
}

local void
FreeLibc(void* ptr){DPZoneScoped;
	MemChunk* chunk = MemoryToChunk(ptr);
	Assert(ChunkIsLibc(chunk), "This was not allocated using libc");
	free(chunk);
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @memory_generic_alloc
void*
deshi__memory_generic_allocate(upt requested_size, str8 file, upt line){DPZoneScoped;
	DEBUG_CheckHeap(g_memory->generic_heap);
	
	if(g_memory->cleanup_happened) return 0;
	if(requested_size == 0) return 0;
	Assert(g_memory->generic_heap && g_memory->generic_heap->initialized, "Attempted to allocate before memory_init() has been called");
	
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
		chunk->size = arena->size | MEMORY_ARENAD_FLAG;
		result = ChunkToMemory(chunk);
		
		
#if MEMORY_PRINT_GENERIC_ACTIONS
		Logf("memory","Created an allocation[0x%p] with %zu bytes (triggered at %s:%zu)", result, requested_size, file.str, line);
#endif //MEMORY_PRINT_GENERIC_ACTIONS
#if MEMORY_TRACK_ALLOCS
		DEBUG_AllocInfo_Creation(result, file, line);
#endif //MEMORY_TRACK_ALLOCS
		
		return result;
	}
	
	//check if there are any empty chunks that can hold the allocation
	for(Node* node = g_memory->generic_heap->empty_nodes.next; node != &g_memory->generic_heap->empty_nodes; node = node->next){
		MemChunk* chunk = CastFromMember(MemChunk, node, node);
		upt chunk_size = GetChunkSize(chunk); //NOTE remember that chunk size includes the overhead
		if(chunk_size >= aligned_size){
			upt leftover_size = chunk_size - aligned_size;
			Assert(leftover_size % MEMORY_BYTE_ALIGNMENT == 0, "Memory was not aligned correctly");
			MemChunk* next = GetNextOrderChunk(chunk);
			
			//make new empty chunk after current chunk if there is enough space for an empty chunk
			//NOTE '>=' rather than '>' because empty chunks use 8/16 bytes (Node) that in-use chunks dont (useful for small allocations)
			if(leftover_size >= MEMORY_MIN_CHUNK_SIZE){
				NodeRemove(&chunk->node); //NOTE remove this early so new_chunk doesnt break next's nodes before removal
				chunk->node = {0};
				
				MemChunk* new_chunk = GetChunkAtOffset(chunk, aligned_size);
				NodeInsertNext(&g_memory->generic_heap->empty_nodes, &new_chunk->node);
				new_chunk->prev = chunk;
				new_chunk->size = leftover_size | MEMORY_EMPTY_FLAG;
				next->prev = new_chunk;
				next = new_chunk;
				g_memory->generic_heap->used += sizeof(MemChunk);
			}else{
				aligned_size += leftover_size;
				NodeRemove(&chunk->node);
				chunk->node = {0}; //zero this data since it will be used by the allocation
			}
			
			//convert empty node to order node //NOTE chunk->prev doesnt need to change
			chunk->size = aligned_size;
			g_memory->generic_heap->used += aligned_size - sizeof(MemChunk);
			result = ChunkToMemory(chunk);
			
			
#if MEMORY_PRINT_GENERIC_ACTIONS
			Logf("memory","Created an allocation[0x%p] with %zu bytes (triggered at %s:%zu)", result, aligned_size, file.str, line);
#endif //MEMORY_PRINT_GENERIC_ACTIONS
			DEBUG_CheckHeap(g_memory->generic_heap);
			DEBUG_PrintGenericHeapChunks();
#if MEMORY_TRACK_ALLOCS
			DEBUG_AllocInfo_Creation(result, file, line);
#endif //MEMORY_TRACK_ALLOCS
			
			return result;
		}
	}
	
	//if we cant replace an empty node, make a new order node for the allocation (if there is space)
	if(g_memory->generic_heap->cursor + aligned_size > g_memory->generic_heap->start + g_memory->generic_heap->size){
		LogfE("memory","Deshi ran out of generic memory when attempting to allocate %zu bytes (triggered at %s:%zu); defaulting to libc calloc.", requested_size, file.str, line);
		result = AllocateLibc(aligned_size);
		
		
#if MEMORY_PRINT_GENERIC_ACTIONS
		Logf("memory","Created a libc allocation[0x%p] with %zu bytes (triggered at %s:%zu)", result, requested_size, file.str, line);
#endif //MEMORY_PRINT_GENERIC_ACTIONS
#if MEMORY_TRACK_ALLOCS
		DEBUG_AllocInfo_Creation(result, file, line);
#endif //MEMORY_TRACK_ALLOCS
		
		return result;
	}
	
	MemChunk* new_chunk = (MemChunk*)g_memory->generic_heap->cursor;
	new_chunk->prev = g_memory->generic_heap->last_chunk;
	new_chunk->size = aligned_size;
	g_memory->generic_heap->cursor    += aligned_size;
	g_memory->generic_heap->used      += aligned_size;
	g_memory->generic_heap->last_chunk = new_chunk;
	result = ChunkToMemory(new_chunk);
	
	
#if MEMORY_PRINT_GENERIC_ACTIONS
	Logf("memory","Created an allocation[0x%p] with %zu bytes (triggered at %s:%zu)", result, requested_size, file.str, line);
#endif //MEMORY_PRINT_GENERIC_ACTIONS
	DEBUG_CheckHeap(g_memory->generic_heap);
	DEBUG_PrintGenericHeapChunks();
#if MEMORY_TRACK_ALLOCS
	DEBUG_AllocInfo_Creation(result, file, line);
#endif //MEMORY_TRACK_ALLOCS
	
	return result;
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @memory_generic_realloc
void*
deshi__memory_generic_reallocate(void* ptr, upt requested_size, str8 file, upt line){DPZoneScoped;
	if(g_memory->cleanup_happened) return 0;
	if(ptr == 0) return deshi__memory_generic_allocate(requested_size, file, line);
	if(requested_size == 0){ deshi__memory_generic_zero_free(ptr, file, line); return 0; }
	
	DEBUG_CheckHeap(g_memory->generic_heap);
	Assert(g_memory->generic_heap && g_memory->generic_heap->initialized, "Attempted to allocate before memory_init() has been called");
	
	//include chunk overhead, align to the byte alignment, and clamp the minimum
	upt aligned_size = ClampMin(RoundUpTo(requested_size + MEMORY_CHUNK_OVERHEAD, MEMORY_BYTE_ALIGNMENT), MEMORY_MIN_CHUNK_SIZE);
	AllocInfo info = deshi__memory_allocinfo_get(ptr);
	MemChunk* chunk = MemoryToChunk(ptr);
	void* result = ptr;
	
	if(ChunkIsLibc(chunk)){
		result = ReallocateLibc(ptr, aligned_size);
		
		
#if MEMORY_PRINT_GENERIC_ACTIONS
		Logf("memory","Reallocated a libc allocation[0x%p]%s to [0x%p] with %zu bytes (triggered at %s:%zu)", ptr, info.name.str, result, requested_size, file.str, line);
#endif //MEMORY_PRINT_GENERIC_ACTIONS
#if MEMORY_TRACK_ALLOCS
		if(ptr != result){
			AllocInfo* new_info = DEBUG_AllocInfo_Creation(result, file, line);
			new_info->name = info.name;
			new_info->type = info.type;
			DEBUG_AllocInfo_Deletion(ptr);
		}
#endif //MEMORY_TRACK_ALLOCS
		
		return result;
	}
	Assert(ptr > g_memory->arena_heap.start && ptr < g_memory->arena_heap.cursor, "Attempted to reallocate a pointer outside the main heap and missing the libc flag");
	
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
		chunk->size = arena->size | MEMORY_ARENAD_FLAG;
		result = ChunkToMemory(chunk);
		
		
#if MEMORY_PRINT_GENERIC_ACTIONS
		Logf("memory","Reallocated an allocation[0x%p]%s to [0x%p] with %zu bytes (triggered at %s:%zu)", ptr, info.name.str, result, requested_size, file.str, line);
#endif //MEMORY_PRINT_GENERIC_ACTIONS
		DEBUG_CheckHeap(g_memory->generic_heap);
		DEBUG_PrintGenericHeapChunks();
#if MEMORY_TRACK_ALLOCS
		if(ptr != result){
			AllocInfo* new_info = DEBUG_AllocInfo_Creation(result, file, line);
			new_info->name = info.name;
			new_info->type = info.type;
			DEBUG_AllocInfo_Deletion(ptr);
		}
#endif //MEMORY_TRACK_ALLOCS
		
		return result;
	}
	Assert(ptr > g_memory->generic_heap->start && ptr < g_memory->generic_heap->cursor, "Attempted to reallocate a pointer outside the generic heap and missing the libc flag");
	
	//new allocation needs to be an arena and wasnt before, so copy memory to new arena and free old allocation
	if(aligned_size > MEMORY_MAX_GENERIC_SIZE){
		//NOTE since its larger than MEMORY_MAX_GENERIC_SIZE and not an arena already, it cant be less than previous size
		Arena* arena = deshi__memory_arena_create(aligned_size, file, line);
		arena->used = aligned_size;
		memcpy(arena->start, chunk, GetChunkSize(chunk));
		
		chunk = (MemChunk*)arena->start;
		chunk->prev = 0;
		chunk->size = arena->size | MEMORY_ARENAD_FLAG;
		result = ChunkToMemory(chunk);
		
		
#if MEMORY_PRINT_GENERIC_ACTIONS
		Logf("memory","Reallocated an allocation[0x%p]%s to [0x%p] with %zu bytes (triggered at %s:%zu)", ptr, info.name.str, result, requested_size, file.str, line);
#endif //MEMORY_PRINT_GENERIC_ACTIONS
		DEBUG_CheckHeap(g_memory->generic_heap);
		DEBUG_PrintGenericHeapChunks();
#if MEMORY_TRACK_ALLOCS
		AllocInfo* new_info = DEBUG_AllocInfo_Creation(result, file, line);
		new_info->name = info.name;
		new_info->type = info.type;
#endif //MEMORY_TRACK_ALLOCS
		
		deshi__memory_generic_zero_free(ptr, file, line);
		return result;
	}
	
	//if requested size is the same as previous size, do nothing
	spt difference = (spt)GetChunkSize(chunk) - (spt)aligned_size;
	if(difference == 0){
		return ptr;
	}
	
	//there is no used memory after this, so just adjust chunk size and heap cursor
	if(chunk == g_memory->generic_heap->last_chunk){
		//if out of memory, default to libc
		if((g_memory->generic_heap->cursor - difference) > (g_memory->generic_heap->start + g_memory->generic_heap->size)){
			LogfE("memory","Deshi ran out of generic memory when attempting to reallocate a ptr[0x%p]%s (triggered at %s:%zu); defaulting to libc calloc.", ptr, info.name.str, file.str, line);
			
			result = AllocateLibc(aligned_size);
			MemChunk* new_chunk = MemoryToChunk(result);
			memcpy(&new_chunk->node, &chunk->node, GetChunkSize(chunk) - MEMORY_CHUNK_OVERHEAD);
			
			
#if MEMORY_PRINT_GENERIC_ACTIONS
			Logf("memory","Reallocated an allocation[0x%p]%s to libc [0x%p] with %zu bytes (triggered at %s:%zu)", ptr, info.name.str, result, requested_size, file.str, line);
#endif //MEMORY_PRINT_GENERIC_ACTIONS
#if MEMORY_TRACK_ALLOCS
			AllocInfo* new_info = DEBUG_AllocInfo_Creation(result, file, line);
			new_info->name = info.name;
			new_info->type = info.type;
#endif //MEMORY_TRACK_ALLOCS
			
			deshi__memory_generic_zero_free(ptr, file, line);
			return result;
		}
		
		g_memory->generic_heap->cursor -= difference;
		g_memory->generic_heap->used   -= difference;
		if(difference > 0) ZeroMemory(g_memory->generic_heap->cursor, difference);
		chunk->size = aligned_size;
		
#if MEMORY_PRINT_GENERIC_ACTIONS
		Logf("memory","Reallocated an allocation[0x%p]%s with %zu bytes (triggered at %s:%zu)", ptr, info.name.str, requested_size, file.str, line);
#endif //MEMORY_PRINT_GENERIC_ACTIONS
		DEBUG_CheckHeap(g_memory->generic_heap);
		DEBUG_PrintGenericHeapChunks();
		
		return ptr;
	}
	
	//if requested size is less than previous size and there is enough space for a new chunk, make a new chunk
	if(difference >= (spt)MEMORY_MIN_CHUNK_SIZE){
		MemChunk* next = GetNextOrderChunk(chunk);
		MemChunk* new_chunk = GetChunkAtOffset(chunk, aligned_size);
		NodeInsertNext(&g_memory->generic_heap->empty_nodes, &new_chunk->node);
		new_chunk->prev = chunk;
		new_chunk->size = (upt)difference | MEMORY_EMPTY_FLAG;
		chunk->size -= difference;
		next->prev = new_chunk;
		g_memory->generic_heap->used -= difference;
		g_memory->generic_heap->used += sizeof(MemChunk);
		
#if MEMORY_PRINT_GENERIC_ACTIONS
		Logf("memory","Reallocated an allocation[0x%p]%s with %zu bytes (triggered at %s:%zu)", ptr, info.name.str, requested_size, file.str, line);
#endif //MEMORY_PRINT_GENERIC_ACTIONS
		DEBUG_CheckHeap(g_memory->generic_heap);
		DEBUG_PrintGenericHeapChunks();
		
		return ptr;
	}
	
	//if requested size is less than previous size and there is not enough space for a new chunk, do nothing
	if(difference > 0){
		return ptr;
	}
	
	//if requested size is greater than previous size and next chunk is empty and next chunk can hold growth, grow into that chunk
	MemChunk* next = GetNextOrderChunk(chunk);
	upt next_chunk_size = GetChunkSize(next);
	if(ChunkIsEmpty(next) && (next_chunk_size >= aligned_size)){
		upt leftover_size = next_chunk_size + difference;
		Assert(leftover_size % MEMORY_BYTE_ALIGNMENT == 0, "Memory was not aligned correctly");
		
		//make new empty chunk after current chunk if there is enough space for an empty chunk
		//NOTE '>=' rather than '>' because empty chunks use 8/16 bytes (Node) that in-use chunks dont (useful for small allocations)
		MemChunk* next_next = GetNextOrderChunk(next);
		if(leftover_size >= MEMORY_MIN_CHUNK_SIZE){
			NodeRemove(&next->node); //NOTE remove this early so new_chunk doesnt break next's nodes before removal
			next->prev = 0;
			next->size = 0;
			next->node = {0};
			
			MemChunk* new_chunk = GetChunkAtOffset(chunk, aligned_size);
			NodeInsertNext(&g_memory->generic_heap->empty_nodes, &new_chunk->node);
			new_chunk->prev = chunk;
			new_chunk->size = leftover_size | MEMORY_EMPTY_FLAG;
			next_next->prev = new_chunk;
			g_memory->generic_heap->used += sizeof(MemChunk);
		}else{
			aligned_size += leftover_size;
			
			//zero next's overhead for use by current chunk 
			NodeRemove(&next->node);
			next->prev = 0;
			next->size = 0;
			next->node = {0};
		}
		
		//grow current chunk
		chunk->size = aligned_size;
		g_memory->generic_heap->used += -difference - sizeof(MemChunk);
		
#if MEMORY_PRINT_GENERIC_ACTIONS
		Logf("memory","Reallocated an allocation[0x%p]%s with %zu bytes (triggered at %s:%zu)", ptr, info.name.str, aligned_size, file.str, line);
#endif //MEMORY_PRINT_GENERIC_ACTIONS
		DEBUG_CheckHeap(g_memory->generic_heap);
		DEBUG_PrintGenericHeapChunks();
		
		return ptr;
	}
	
	//requested size is greater than previous size and we need to move memory in order to fit the new size
	result = deshi__memory_generic_allocate(requested_size, file, line);
	Assert(GetChunkSize(MemoryToChunk(result)) >= GetChunkSize(chunk) - MEMORY_CHUNK_OVERHEAD, "New chunk size must be greater than previous size");
	memcpy(result, &chunk->node, GetChunkSize(chunk) - MEMORY_CHUNK_OVERHEAD);
	chunk = MemoryToChunk(result);
	
#if MEMORY_PRINT_GENERIC_ACTIONS
	Logf("memory","Reallocated an allocation[0x%p%s] to [0x%p] with %zu bytes (triggered at %s:%zu)", ptr, info.name.str, result, requested_size, file.str, line);
#endif //MEMORY_PRINT_GENERIC_ACTIONS
	DEBUG_CheckHeap(g_memory->generic_heap);
	DEBUG_PrintGenericHeapChunks();
#if MEMORY_TRACK_ALLOCS
	memory_allocinfo_set(result, info.name, info.type);
#endif //MEMORY_TRACK_ALLOCS
	
	deshi__memory_generic_zero_free(ptr, file, line);
	return result;
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @memory_generic_zfree
void
deshi__memory_generic_zero_free(void* ptr, str8 file, upt line){DPZoneScoped;
	if(g_memory->cleanup_happened) return;
	if(ptr == 0) return;
	
	DEBUG_CheckHeap(g_memory->generic_heap);
	AllocInfo info = deshi__memory_allocinfo_get(ptr);
	MemChunk* chunk = MemoryToChunk(ptr);
	Assert(chunk->size > 0, "A chunk must always have a size");
	
#if MEMORY_TRACK_ALLOCS
	DEBUG_AllocInfo_Deletion(ptr);
#endif //MEMORY_TRACK_ALLOCS
	
	//if allocated from libc, free with libc
	if(ChunkIsLibc(chunk)){
		FreeLibc(ptr);
		
#if MEMORY_PRINT_GENERIC_ACTIONS
		Logf("memory","Freed a libc allocation[0x%p]%s (triggered at %s:%zu)", ptr, info.name.str, file.str, line);
#endif //MEMORY_PRINT_GENERIC_ACTIONS
		
		return;
	}
	Assert(ptr > g_memory->arena_heap.start && ptr < g_memory->arena_heap.cursor, "Attempted to free a pointer outside the main heap and missing the libc flag");
	
	//if allocation used an arena, delete the arena
	//NOTE when generic allocations use arena, the layout ends up like Chunk -> Arena -> Chunk
	//     where the first chunk is for the arena and the second is for the generic allocation
	if(ChunkIsArenad(chunk)){
		deshi__memory_arena_delete((Arena*)chunk - 1, file, line);
		
#if MEMORY_PRINT_GENERIC_ACTIONS
		Logf("memory","Freed an allocation  [0x%p]%s (triggered at %s:%zu)", ptr, info.name.str, file.str, line);
#endif //MEMORY_PRINT_GENERIC_ACTIONS
		DEBUG_CheckHeap(g_memory->generic_heap);
		DEBUG_PrintGenericHeapChunks();
		
		return;
	}
	Assert(ptr > g_memory->generic_heap->start && ptr < g_memory->generic_heap->cursor, "Attempted to free a pointer outside the generic heap");
	
	upt   chunk_size   = GetChunkSize(chunk);
	void* zero_pointer = chunk+1;
	upt   zero_amount  = chunk_size - sizeof(MemChunk);
	upt   used_amount  = zero_amount;
	
	//insert current chunk into heap's empty nodes (as first empty node for locality)
	NodeInsertNext(&g_memory->generic_heap->empty_nodes, &chunk->node);
	chunk->size |= MEMORY_EMPTY_FLAG;
	
	//try to merge next empty into current empty
	MemChunk* next = GetNextOrderChunk(chunk);
	if((chunk != g_memory->generic_heap->last_chunk) && ChunkIsEmpty(next)){
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
	if((prev != 0) && ChunkIsEmpty(prev)){
		if(chunk == g_memory->generic_heap->last_chunk){
			g_memory->generic_heap->last_chunk = prev;
		}else{
			next->prev = prev;
		}
		NodeRemove(&chunk->node);
		NodeRemove(&prev->node); //NOTE remove and reinsert as first empty node for locality
		NodeInsertNext(&g_memory->generic_heap->empty_nodes, &prev->node);
		prev->size += GetChunkSize(chunk);
		zero_pointer = chunk; //NOTE prev's memory is already zeroed, so only zero chunk
		zero_amount  = GetChunkSize(chunk);
		used_amount += sizeof(MemChunk);
		chunk = prev;
	}
	
	//remove the last order chunk if its empty
	if(chunk == g_memory->generic_heap->last_chunk){
		NodeRemove(&chunk->node);
		g_memory->generic_heap->last_chunk = chunk->prev;
		g_memory->generic_heap->cursor = (u8*)chunk;
		zero_pointer = chunk;
		zero_amount  = GetChunkSize(chunk);
		used_amount += sizeof(MemChunk);
	}
	
	g_memory->generic_heap->used -= used_amount;
	ZeroMemory(zero_pointer, zero_amount);
	
#if MEMORY_PRINT_GENERIC_ACTIONS
	Logf("memory","Freed an allocation  [0x%p]%s (triggered at %s:%zu)", ptr, info.name.str, file.str, line);
#endif //MEMORY_PRINT_GENERIC_ACTIONS
	DEBUG_CheckHeap(g_memory->generic_heap);
	DEBUG_PrintGenericHeapChunks();
}


Heap*
deshi__memory_generic_expose(){DPZoneScoped;
	return g_memory->generic_heap;
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @memory_temp
void*
deshi__memory_temp_allocate(upt size, str8 file, upt line){DPZoneScoped;
	if(g_memory->cleanup_happened) return 0;
	if(size == 0) return 0;
	Assert(g_memory->temp_arena.start, "Attempted to temp allocate before memory_init() has been called");
	
	upt aligned_size = RoundUpTo(size + sizeof(upt), MEMORY_BYTE_ALIGNMENT);
	if(g_memory->temp_arena.used + aligned_size > g_memory->temp_arena.size){
		LogfE("memory","Deshi ran out of temporary memory when attempting to allocate %zu bytes (triggered at %s:%zu); defaulting to libc calloc which will not be automatically freed by clearing the temporary storage (aka: a memory leak)!", size, file.str, line);
		upt* size_ptr = (upt*)calloc(1, aligned_size);
		Assert(size_ptr, "libc failed to allocate memory");
		*size_ptr = aligned_size | 0x1;
		return size_ptr+1;
	}
	
	void* result = g_memory->temp_arena.cursor + sizeof(upt);
	*((upt*)g_memory->temp_arena.cursor) = aligned_size; //place allocation size at cursor
	g_memory->temp_arena.cursor += aligned_size;
	g_memory->temp_arena.used += aligned_size;
	
#if MEMORY_PRINT_TEMP_ACTIONS
	Logf("memory","Created a temp allocation[0x%p] with %zu bytes (triggered at %s:%zu)", result, aligned_size, file.str, line);
#endif //MEMORY_PRINT_TEMP_ACTIONS
	return result;
}

void*
deshi__memory_temp_reallocate(void* ptr, upt size, str8 file, upt line){DPZoneScoped;
	if(g_memory->cleanup_happened) return 0;
	if(size == 0) return 0;
	if(ptr == 0) return 0;
	
#if MEMORY_PRINT_TEMP_ACTIONS
	AllocInfo info = deshi__memory_allocinfo_get(ptr);
	Logf("memory","Reallocating a temp ptr[0x%p]%s to %zu bytes (triggered at %s:%zu)", ptr, info.name.str, size, file.str, line);
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
deshi__memory_temp_clear(){DPZoneScoped;
	if(g_memory->cleanup_happened) return;
	
#if MEMORY_PRINT_TEMP_ACTIONS
	Logf("memory","Clearing temporary memory which used %zu bytes", g_memory->temp_arena.used);
#endif //MEMORY_PRINT_TEMP_ACTIONS
	g_memory->temp_arena.cursor = g_memory->temp_arena.start;
	g_memory->temp_arena.used = 0;
	memory_clear_arena(&g_memory->temp_arena);
}

Arena*
deshi__memory_temp_expose(){DPZoneScoped;
	return &g_memory->temp_arena;
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @memory_debug
void
deshi__memory_allocinfo_set(void* address, str8 name, Type type){DPZoneScoped;
#if MEMORY_TRACK_ALLOCS
	if(cleanup_happened) return;
	if(address == 0) return;
	
	//binary search for address index (or index to insert at)
	spt index = -1;
	spt middle = 0;
	if(alloc_infos_active.count > 0){
		spt left  = 0;
		spt right = alloc_infos_active.count-1;
		while(left <= right){
			middle = left + ((right - left) / 2);
			if(alloc_infos_active[middle].address == address){
				index = middle;
				break;
			}
			if(alloc_infos_active[middle].address < address){
				left = middle + 1;
				middle = left + ((right - left) / 2);
			}else{
				right = middle - 1;
			}
		}
	}
	
	if(index != -1){
		alloc_infos_active[index].name = name;
		alloc_infos_active[index].type = type;
	}else{
		alloc_infos_active.insert(AllocInfo{address, {}, DeshTime->updateCount, upt(-1), name, type}, middle);
	}
#endif //MEMORY_TRACK_ALLOCS
}

local AllocInfo null_alloc_info{0, {}, 0, upt(-1), str8_lit(""), 0};
local AllocInfo test_alloc_info{0, {}, 0, upt(-1), str8_lit(""), 0};
AllocInfo
deshi__memory_allocinfo_get(void* address){DPZoneScoped;
#if MEMORY_TRACK_ALLOCS
	if(cleanup_happened) return null_alloc_info;
	if(address == 0) return null_alloc_info;
	
	test_alloc_info.address = address;
	upt index = binary_search(alloc_infos_active, test_alloc_info, AllocInfo_LessThan);
	if(index != -1){
		return alloc_infos_active[index];
	}else{
		index = binary_search(alloc_infos_inactive, test_alloc_info, AllocInfo_LessThan);
		while(   (index < alloc_infos_inactive.count-1)
			  && (alloc_infos_inactive[index].address == alloc_infos_inactive[index+1].address)){
			++index;
		}
		if(index != -1){
			return alloc_infos_inactive[index];
		}
	}
	return null_alloc_info;
#else //MEMORY_TRACK_ALLOCS
	return null_alloc_info;
#endif //MEMORY_TRACK_ALLOCS
}

carray<AllocInfo>
deshi__memory_allocinfo_active_expose(){DPZoneScoped;
#if MEMORY_TRACK_ALLOCS
	return carray<AllocInfo>{alloc_infos_active.data, alloc_infos_active.count};
#else
	return carray<AllocInfo>{};
#endif //MEMORY_TRACK_ALLOCS
}

carray<AllocInfo>
deshi__memory_allocinfo_inactive_expose(){DPZoneScoped;
#if MEMORY_TRACK_ALLOCS
	return carray<AllocInfo>{alloc_infos_inactive.data, alloc_infos_inactive.count};
#else
	return carray<AllocInfo>{};
#endif //MEMORY_TRACK_ALLOCS
}

void
deshi__memory_draw(){DPZoneScoped;
	auto bytes_sigfigs = [](upt bytes, char& character, f32& divisor){
		if(bytes >= Kilobytes(1)){
			character = 'K'; divisor = Kilobytes(1);
			if(bytes >= Megabytes(1)){
				character = 'M'; divisor = Megabytes(1);
			}
		}
	};
	
	UI::PushColor(UIStyleCol_Border,             Color_Grey);
	UI::PushColor(UIStyleCol_Separator,          Color_Grey);
	UI::Begin(str8_lit("deshi_memory"), Vec2(DeshWindow->width,DeshWindow->height)/4.f, Vec2(DeshWindow->width,DeshWindow->height)/2.f, UIWindowFlags_NoScroll);{
		UIWindow* window = UI::GetWindow();
		UIStyle_old& style = UI::GetStyle();
		char used_char = ' ', size_char = ' ';
		f32  used_divisor = 1.f, size_divisor = 1.f;
		
		//UI::SetNextItemSize({MAX_F32, window->height*.9f});
		UI::BeginTabBar(str8_lit("deshi_memory_top_panel"), UITabBarFlags_NoIndent);{
			//left panel: generic heap
			if(UI::BeginTab(str8_lit("deshi_memory_generic"))){
				bytes_sigfigs(g_memory->generic_heap->used, used_char, used_divisor);
				bytes_sigfigs(g_memory->generic_heap->size, size_char, size_divisor);
				UI::Separator(style.fontHeight / 2.f);
				UI::TextF(str8_lit("Generic Heap    %.2f %cB / %.2f %cB"), (f32)g_memory->generic_heap->used / used_divisor, used_char, (f32)g_memory->generic_heap->size / size_divisor, size_char);
				UI::Separator(style.fontHeight/2.f);
				
				UI::PushColor(UIStyleCol_WindowBg,                Color_VeryDarkRed);
				UI::PushColor(UIStyleCol_ScrollBarDragger,        Color_DarkGrey);
				UI::PushColor(UIStyleCol_ScrollBarDraggerHovered, Color_Grey);
				UI::PushColor(UIStyleCol_ScrollBarDraggerActive,  Color_LightGrey);
				UI::PushColor(UIStyleCol_ScrollBarBg,             Color_VeryDarkRed);
				UI::PushColor(UIStyleCol_ScrollBarBgHovered,      Color_Grey);
				UI::PushColor(UIStyleCol_ScrollBarBgActive,       Color_LightGrey);
				UI::SetNextWindowSize({MAX_F32, MAX_F32});
				UI::BeginChild(str8_lit("deshi_memory_generic_timeline"), vec2::ZERO, UIWindowFlags_NoBorder | UIWindowFlags_NoResize | UIWindowFlags_NoMove);{
#if 0 //MEMORY_TRACK_ALLOCS //TODO update this to new alloc info arrays
					f32 alloc_height = 10.f;
					f32 frame_width  = 5.f;
					
					forI(alloc_infos.count){
						UI::TextF(str8_lit("0x%p"), alloc_infos[i].address);
						if(UI::IsLastItemHovered()){
							UI::PushColor(UIStyleCol_WindowBg, color(32,0,0,255));
							UI::BeginPopOut(str8_lit("deshi_memory_generic_hovered"), input_mouse_position - UI::GetWindow()->position, vec2::ZERO, UIWindowFlags_FitAllElements | UIWindowFlags_NoBorder | UIWindowFlags_NoInteract);{
								UI::TextF(str8_lit("Trigger: %s:%u"), alloc_infos[i].trigger.file.str, alloc_infos[i].trigger.line);
								UI::TextF(str8_lit("Name: %s"), alloc_infos[i].name.str);
								UI::TextF(str8_lit("Type: %u"), alloc_infos[i].type);
							}UI::EndPopOut();
							UI::PopColor();
						}
						
						//TODO draw rect for time the alloc has been alive
					}
#endif //MEMORY_TRACK_ALLOCS
				}UI::EndChild();
				UI::PopColor(7);
				
				UI::EndTab();
			}
			
			//right panel: arena heap
			if(UI::BeginTab(str8_lit("deshi_memory_arena"))){
				bytes_sigfigs(g_memory->arena_heap.used, used_char, used_divisor);
				bytes_sigfigs(g_memory->arena_heap.size, size_char, size_divisor);
				UI::Separator(style.fontHeight / 2.f);
				UI::TextF(str8_lit("Arena Heap    %.2f %cB / %.2f %cB"), (f32)g_memory->arena_heap.used / used_divisor, used_char, (f32)g_memory->arena_heap.size / size_divisor, size_char);
				UI::Separator(style.fontHeight/2.f);
				
				UI::PushColor(UIStyleCol_WindowBg, Color_VeryDarkGreen);
				UI::SetNextWindowSize({MAX_F32, MAX_F32});
				UI::BeginChild(str8_lit("deshi_memory_arena_treemap"), vec2::ZERO, UIWindowFlags_NoScroll | UIWindowFlags_NoBorder | UIWindowFlags_NoResize);{
					//TODO this
				}UI::EndChild();
				UI::PopColor();
				
				UI::EndTab();
			}
		}UI::EndTabBar();
		/*
		//bottom panel: temp arena
		UI::SetNextWindowSize({MAX_F32, window->height*.1f});
		UI::BeginChild("deshi_memory_bottom_panel", vec2::ZERO, UIWindowFlags_NoScroll | UIWindowFlags_NoBorder | UIWindowFlags_NoResize);{
			bytes_sigfigs(temp_arena->used, used_char, used_divisor);
			bytes_sigfigs(temp_arena->size, size_char, size_divisor);
			UI::TextF("Temporary Memory    %.2f %cB / %.2f %cB", (f32)temp_arena->used / used_divisor, used_char, (f32)temp_arena->size / size_divisor, size_char);
			
			UI::RectFilled(UI::GetWinCursor(), UI::GetWindowRemainingSpace(), Color_VeryDarkCyan);
			UI::RectFilled(UI::GetWinCursor(), UI::GetWindowRemainingSpace() * vec2{((f32)temp_arena->used / (f32)temp_arena->size), 1.f}, Color_DarkCyan);
		}UI::EndChild();
		*/
	}UI::End();
	UI::PopColor(2);
}

void 
deshi__memory_bytes_draw() {
	using namespace UI;
	
	Heap* heap = g_memory->generic_heap;
	u32 scale = 8;
	
	Begin(str8_lit("memory_bytes_draw"));
	
	u32 winw = GetMarginedRight() - GetMarginedLeft();
	
	persist f32 selected_chunk = 0;
	
	Slider(str8_lit("chunksel"), &selected_chunk, 0, 450);
	SameLine();
	if (Button(str8_lit("<-"))) selected_chunk = Max(--selected_chunk, 0.f);
	SameLine();
	if (Button(str8_lit("->"))) selected_chunk++;
	
	MemChunk* chunk = (MemChunk*)heap->start;
	forI(s32(selected_chunk)) chunk = GetNextOrderChunk(chunk);
	
	u8* mem = (u8*)ChunkToMemory(chunk);
	u32 count = GetChunkSize(chunk);
	
	forI(Min(count, u32(5000))) {
		u8 val = mem[i]; 
		u32 canfit = winw / scale;
		
		vec2 pos = Vec2((i % canfit) * scale, i * scale / winw * scale) + UI::GetWinCursor();
		RectFilled(pos, vec2::ONE * scale, color(val, val, val, 255));
		if (Math::PointInRectangle(input_mouse_position(), GetLastItemScreenPos(), GetLastItemSize())) {
			PushLayer(GetCenterLayer() + 1);
			vec2 mp = (input_mouse_position() - GetWindow()->position);
			string m = toStr(mem + i);
			RectFilled(mp + Vec2(0, -GetStyle().fontHeight * 2), CalcTextSize(str8{(u8*)m.str, (s64)m.count}), Color_VeryDarkGrey);
			TextOld(str8{(u8*)m.str, (s64)m.count}, mp + Vec2(0, -GetStyle().fontHeight * 2), UITextFlags_NoWrap);
			string v = toStr(val);
			RectFilled(mp + Vec2(0, -GetStyle().fontHeight), CalcTextSize(str8{(u8*)v.str, (s64)v.count}), Color_VeryDarkGrey);
			TextOld(str8{(u8*)v.str, (s64)v.count}, mp + Vec2(0, -GetStyle().fontHeight), UITextFlags_NoWrap);
			PopLayer();
		}
	}
	
	//Texture* memsnap = Storage::CreateTextureFromMemory(mem, (char*)chunk, ceil(sqrt(count)), ceil(sqrt(count)), ImageFormat_BW, TextureType_2D, TextureFilter_Nearest, TextureAddressMode_ClampToBlack, 1).second;
	//Image(memsnap);
	
	End();
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @memory_state
void
deshi__memory_init(upt main_size, upt temp_size){DPZoneScoped;
#if defined(TRACY_ENABLE) && defined(DESHI_WAIT_FOR_TRACY_CONNECTION)
	printf("TRACY_ENABLE and DESHI_WAIT_FOR_TRACY_CONNECTION both enabled. Waiting for connection...");
	while(!TracyIsConnected){}
#endif
	
	void* base_address = 0;
	u8*   allocation   = 0;
	u64   total_size   = main_size + temp_size + sizeof(MemoryContext);
	
#if BUILD_INTERNAL
	base_address = (void*)Terabytes(2);
#endif //BUILD_INTERNAL
	
	u32 retries = 0;
	u32 max_retries = 1000;
	while(allocation == 0 && retries < max_retries){
#if   DESHI_WINDOWS
		allocation = (u8*)VirtualAlloc((LPVOID)base_address, (SIZE_T)total_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
#elif DESHI_LINUX //DESHI_WINDOWS
#  error not implemented
		allocation = (u8*)calloc(1, total_size);
#elif DESHI_MAC   //DESHI_LINUX
#  error not implemented
		allocation = (u8*)calloc(1, total_size);
#endif            //DESHI_MAC
		retries++;
		if(!allocation) printf("[MEMORY-ERROR] Failed to allocate memory from OS. Retrying (%u)", retries);
	}
	Assert(allocation, "Failed to allocate memory from the OS");
	
	g_memory = (MemoryContext*)allocation;
	
	g_memory->arena_heap.start      = allocation+sizeof(MemoryContext);
	g_memory->arena_heap.cursor     = g_memory->arena_heap.start;
	g_memory->arena_heap.size       = main_size;
	g_memory->arena_heap.used       = 0;
	g_memory->arena_heap.empty_nodes.next = g_memory->arena_heap.empty_nodes.prev = &g_memory->arena_heap.empty_nodes;
	g_memory->arena_heap.last_chunk = 0;
	g_memory->arena_heap.initialized = true;
	DEBUG_CheckHeap(&g_memory->arena_heap);
	deshi__memory_allocinfo_set(&g_memory->arena_heap, str8_lit("Arena Heap"), Type_Heap);
	
	g_memory->generic_arena = memory_create_arena(Megabytes(64)+sizeof(Heap));
	g_memory->generic_heap = (Heap*)g_memory->generic_arena->start;
	g_memory->generic_heap->start      = (u8*)(g_memory->generic_heap+1);
	g_memory->generic_heap->cursor     = g_memory->generic_heap->start;
	g_memory->generic_heap->used       = 0;
	g_memory->generic_heap->size       = g_memory->generic_arena->size - sizeof(Heap);
	g_memory->generic_heap->empty_nodes.next = g_memory->generic_heap->empty_nodes.prev = &g_memory->generic_heap->empty_nodes;
	g_memory->generic_heap->last_chunk = 0;
	g_memory->generic_heap->initialized = true;
	DEBUG_CheckHeap(g_memory->generic_heap);
	deshi__memory_allocinfo_set(g_memory->generic_heap, str8_lit("Generic Heap"), Type_Heap);
	
	g_memory->temp_arena.start  = g_memory->arena_heap.start + g_memory->arena_heap.size;
	g_memory->temp_arena.cursor = g_memory->temp_arena.start;
	g_memory->temp_arena.size   = temp_size;
	g_memory->temp_arena.used   = 0;
	deshi__memory_allocinfo_set(&g_memory->temp_arena, str8_lit("Temp Arena"), Type_Arena);
	
	deshiStage |= DS_MEMORY;
}

void
deshi__memory_cleanup(){DPZoneScoped;
	g_memory->cleanup_happened = true;
}