/*Index:
@memory_vars
@memory_utils
@memory_heap
@memory_arena
@memory_pool
@memory_generic
@memory_temp
@memory_debug
@memory_state
*/


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @memory_vars
#if MEMORY_TRACK_ALLOCS
local arrayT<AllocInfo> alloc_infos_active(stl_allocator); //uses libc so it is external the system
local arrayT<AllocInfo> alloc_infos_inactive(stl_allocator);
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
#else
#  define DEBUG_CheckHeap(heap) (void)0
#endif //MEMORY_CHECK_HEAPS


#if MEMORY_PRINT_ARENA_CHUNKS || MEMORY_PRINT_GENERIC_CHUNKS
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
#else
#  define DEBUG_PrintHeapChunks(heap,heap_name) (void)0
#endif //MEMORY_PRINT_ARENA_CHUNKS || MEMORY_PRINT_GENERIC_CHUNKS


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
		alloc_infos_active.insert(AllocInfo{address, file, line, DeshTime->frame, upt(-1), str8_lit(""), 0}, middle);
	}else{
		alloc_infos_active.add(AllocInfo{address, file, line, DeshTime->frame, upt(-1), str8_lit(""), 0});
	}
	return &alloc_infos_active[middle];
}

local void
DEBUG_AllocInfo_Deletion(void* address){DPZoneScoped;
	b32 AllocInfo_LessThan(const AllocInfo& a, const AllocInfo& b){ return a.address < b.address; }
	b32 AllocInfo_GreaterThan(const AllocInfo& a, const AllocInfo& b){ return a.address > b.address; }
	
	if(address == 0) return;
	upt index = binary_search(alloc_infos_active, AllocInfo{address}, AllocInfo_LessThan);
	if(index != -1){
		alloc_infos_active[index].deletion_frame = DeshTime->frame;
		alloc_infos_inactive.add(alloc_infos_active[index]);
		bubble_sort(alloc_infos_inactive, AllocInfo_GreaterThan); //TODO(delle) use binary_insertion_sort_low_to_high after testing it
		alloc_infos_active.remove(index);
	}
}
#else
#  define DEBUG_AllocInfo_Creation(address,file,line) (void)0
#  define DEBUG_AllocInfo_Deletion(address) (void)0
#endif //MEMORY_TRACK_ALLOCS


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @memory_heap
#if MEMORY_PRINT_HEAP_ACTIONS
#  define DEBUG_PrintHeapAction(text,...) Logf("memory",text " (triggered at %s:%zu)", __VA_ARGS__, file.str, line)
#else
#  define DEBUG_PrintHeapAction(text,...) (void)0
#endif //MEMORY_PRINT_HEAP_ACTIONS


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
	DEBUG_PrintHeapAction("Initted a heap[0x%p] with %zu bytes", result, bytes);
	
	return result;
}


void
deshi__memory_heap_deinit(Heap* heap, str8 file, upt line){DPZoneScoped;
	DEBUG_PrintHeapAction("Deinitted a heap[0x%p]%s with %zu bytes", heap, deshi__memory_allocinfo_get(heap).name.str, heap->size);
	
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
			//NOTE(delle) '>=' rather than '>' because empty chunks use 8/16 bytes (Node) that in-use chunks dont (useful for small allocations)
			if(leftover_size >= MEMORY_MIN_CHUNK_SIZE){
				NodeRemove(&chunk->node); //NOTE(delle) remove this early so new_chunk doesnt break next's nodes before removal
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
			
			//convert empty node to order node  //NOTE(delle) chunk->prev doesnt need to change
			chunk->size = aligned_size;
			heap->used += aligned_size - sizeof(MemChunk);
			result = ChunkToMemory(chunk);
			
			DEBUG_CheckHeap(heap);
			DEBUG_PrintHeapAction("Created an allocation[0x%p] in heap[0x%p]%s with %zu bytes", result, heap, deshi__memory_allocinfo_get(heap).name.str, aligned_size);
			
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
	
	DEBUG_CheckHeap(heap);
	DEBUG_PrintHeapAction("Created an allocation[0x%p] in heap[0x%p]%s with %zu bytes", result, heap, deshi__memory_allocinfo_get(heap).name.str, aligned_size);
	
	return result;
}


void
deshi__memory_heap_remove(Heap* heap, void* ptr, str8 file, upt line){DPZoneScoped;
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
		//NOTE(delle) zero_pointer doesnt change
		zero_amount += sizeof(MemChunk); //NOTE(delle) next's memory is already zeroed, so only zero the chunk header
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
		NodeRemove(&prev->node); //NOTE(delle) remove and reinsert as first empty node for locality
		NodeInsertNext(&heap->empty_nodes, &prev->node);
		prev->size += GetChunkSize(chunk);
		zero_pointer = chunk; //NOTE(delle) prev's memory is already zeroed, so only zero chunk
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
	
	//clear the used memory
	heap->used -= used_amount;
	ZeroMemory(zero_pointer, zero_amount);
	
	DEBUG_PrintHeapAction("Freed   an allocation[0x%p]%s in heap[0x%p]%s", ptr, info.name.str, heap, deshi__memory_allocinfo_get(heap).name.str);
	DEBUG_CheckHeap(heap);
}


void
deshi__memory_heap_clear(Heap* heap, str8 file, upt line){DPZoneScoped;
	if(g_memory->cleanup_happened) return;
	if(heap == 0) return;
	
	DEBUG_PrintHeapAction("Cleared a heap[0x%p]%s with %zu bytes", heap, deshi__memory_allocinfo_get(heap).name.str, heap->size);
	
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
#  define DEBUG_CheckArenaHeapArenas() (void)0
#endif //MEMORY_CHECK_HEAPS


#if MEMORY_PRINT_ARENA_CHUNKS
#  define DEBUG_PrintArenaHeapChunks() DEBUG_PrintHeapChunks(&g_memory->arena_heap,"Arena Heap")
#else //MEMORY_PRINT_ARENA_CHUNKS
#  define DEBUG_PrintArenaHeapChunks() (void)0
#endif //MEMORY_PRINT_ARENA_CHUNKS


#if MEMORY_PRINT_ARENA_ACTIONS
#  define DEBUG_PrintArenaAction(text,...) Logf("memory",text " (triggered at %s:%zu)", __VA_ARGS__, file.str, line)
#else
#  define DEBUG_PrintArenaAction(text,...) (void)0
#endif //MEMORY_PRINT_ARENA_ACTIONS


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
		upt chunk_size = GetChunkSize(chunk); //NOTE(delle) remember that chunk size includes the overhead
		if(chunk_size >= aligned_size){
			upt leftover_size = chunk_size - aligned_size;
			Assert(leftover_size % MEMORY_BYTE_ALIGNMENT == 0, "Memory was not aligned correctly");
			MemChunk* next = GetNextOrderChunk(chunk);
			
			//make new empty chunk after current chunk if there is enough space for arena overhead
			if(leftover_size > MEMORY_ARENA_OVERHEAD){
				NodeRemove(&chunk->node); //NOTE(delle) remove this early so new_chunk doesnt break chunk's nodes before removal
				
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
			//chunk->node = {0}; //NOTE(delle) not necessary since its overwritten below
			chunk->size = aligned_size;
			g_memory->arena_heap.used += aligned_size - sizeof(MemChunk);
			
			result = ChunkToArena(chunk);
			result->start  = (u8*)(result+1);
			result->cursor = result->start;
			result->size   = aligned_size - MEMORY_ARENA_OVERHEAD;
			result->used   = 0;
			
			DEBUG_CheckHeap(&g_memory->arena_heap);
			DEBUG_CheckArenaHeapArenas();
			DEBUG_PrintArenaHeapChunks();
			DEBUG_PrintArenaAction("Created an arena[0x%p] with %zu bytes", result, result->size);
			DEBUG_AllocInfo_Creation(result, file, line);
			Assert(result->size >= requested_size, "Arena size was not greater than or equal to requested size");
			
			return result;
		}
	}
	
	//if we cant replace an empty node, make a new order node for the allocation (if there is space)
	if(g_memory->arena_heap.cursor + aligned_size > g_memory->arena_heap.start + g_memory->arena_heap.size){
		LogfE("memory","Deshi ran out of main memory when attempting to create an arena with %zu bytes (triggered at %s:%zu); defaulting to libc calloc.", result->size, file.str, line);
		
		MemChunk* chunk = (MemChunk*)calloc(1, aligned_size);
		Assert(chunk, "libc failed to allocate memory");
		chunk->size = aligned_size | MEMORY_LIBC_FLAG;
		result = ChunkToArena(chunk);
		result->start  = (u8*)(result+1);
		result->cursor = result->start;
		result->size   = aligned_size - MEMORY_ARENA_OVERHEAD;
		
		DEBUG_PrintArenaAction("Created a libc arena[0x%p] with %zu bytes", result, result->size);
		DEBUG_AllocInfo_Creation(result, file, line);
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
	
	DEBUG_CheckHeap(&g_memory->arena_heap);
	DEBUG_CheckArenaHeapArenas();
	DEBUG_PrintArenaHeapChunks();
	DEBUG_PrintArenaAction("Created an arena[0x%p] with %zu bytes", result, result->size);
	DEBUG_AllocInfo_Creation(result, file, line);
	Assert(result->size >= requested_size, "Arena size was not greater than or equal to requested size");
	
	return result;
}


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
		
		DEBUG_PrintArenaAction("Grew a libc arena[0x%p]%s to libc [0x%p] with %zu bytes", arena, info.name.str, result, size);
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
			
			upt chunk_size = arena->size + size;
			MemChunk* chunk = (MemChunk*)calloc(1, chunk_size);
			Assert(chunk, "libc failed to allocate memory");
			chunk->size = chunk_size | MEMORY_LIBC_FLAG;
			result = ChunkToArena(chunk);
			result->start  = (u8*)(arena+1);
			result->cursor = result->start + (arena->cursor - arena->start);
			result->size   = chunk_size - MEMORY_ARENA_OVERHEAD;
			result->used   = arena->used;
			memcpy(result->start, arena->start, arena->size);
			
			DEBUG_PrintArenaAction("Grew an arena   [0x%p]%s to libc [0x%p] with %zu bytes", arena, info.name.str, result, size);
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
		
		DEBUG_PrintArenaAction("Grew an arena   [0x%p]%s with %zu bytes", arena, info.name.str, aligned_size);
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
			NodeRemove(&next->node); //NOTE(delle) remove this early so new_chunk doesnt break next's nodes before removal
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
		
		DEBUG_PrintArenaAction("Grew an arena   [0x%p]%s with %zu bytes", arena, info.name.str, aligned_size);
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
	
	DEBUG_PrintArenaAction("Grew an arena   [0x%p]%s to [0x%p] with %zu bytes", arena, info.name.str, result, aligned_size);
	DEBUG_CheckHeap(&g_memory->arena_heap);
	DEBUG_CheckArenaHeapArenas();
	DEBUG_PrintArenaHeapChunks();
	deshi__memory_allocinfo_set(result, info.name, info.type);
	
	deshi__memory_arena_delete(arena, file, line);
	return result;
}


void
deshi__memory_arena_clear(Arena* arena, str8 file, upt line){DPZoneScoped;
	if(g_memory->cleanup_happened) return;
	
	DEBUG_PrintArenaAction("Cleared an arena[0x%p]%s with %zu bytes", arena, deshi__memory_allocinfo_get(arena).name.str, arena->size);
	
	ZeroMemory(arena->start, arena->used);
	arena->cursor = arena->start;
	arena->used   = 0;
}


void
deshi__memory_arena_fit(Arena* arena, str8 file, upt line){DPZoneScoped;
	if(g_memory->cleanup_happened) return;
	if(arena->used >= arena->size) return;
	
	DEBUG_CheckHeap(&g_memory->arena_heap);
	DEBUG_CheckArenaHeapArenas();
	Assert(g_memory->arena_heap.initialized, "Attempted to fit an arena before memory_init() has been called");
	
	AllocInfo info = deshi__memory_allocinfo_get(arena);
	MemChunk* chunk = ArenaToChunk(arena);
	upt old_size = arena->size;
	upt aligned_size = RoundUpTo(arena->used, MEMORY_BYTE_ALIGNMENT);
	upt size_difference = old_size - aligned_size;
	
	//if using libc, call realloc to free the excess memory
	if(ChunkIsLibc(chunk)){
		arena->size = aligned_size;
		chunk->size = (aligned_size + MEMORY_ARENA_OVERHEAD) | MEMORY_LIBC_FLAG;
		chunk = (MemChunk*)realloc(chunk, chunk->size);
		Assert(chunk, "libc failed to reallocate memory (which it shouldn't need to do anyways)");
		
		DEBUG_PrintArenaAction("Fit a libc arena[0x%p]%s from %zu bytes to %zu bytes", arena, info.name.str, old_size, arena->size);
		
		return;
	}
	Assert((u8*)arena > g_memory->arena_heap.start && (u8*)arena < g_memory->arena_heap.cursor, "Attempted to grow an arena that's outside the arena heap and missing the libc flag");
	
	//fit the arena and chunk sizes
	arena->size = aligned_size;
	chunk->size = aligned_size + MEMORY_ARENA_OVERHEAD;
	if(arena->cursor > arena->start + arena->size){
		arena->cursor = arena->start + arena->size;
	}
	
	//current chunk is the last chunk, so readjust the heap cursor
	if(chunk == g_memory->arena_heap.last_chunk){
		g_memory->arena_heap.cursor -= size_difference;
		g_memory->arena_heap.used   -= size_difference;
		
		DEBUG_CheckHeap(&g_memory->arena_heap);
		DEBUG_CheckArenaHeapArenas();
		DEBUG_PrintArenaHeapChunks();
		DEBUG_PrintArenaAction("Fit an arena    [0x%p]%s from %zu bytes to %zu bytes", arena, info.name.str, old_size, arena->size);
		
		return;
	}
	
	//next chunk is empty, so shift it to after the fit chunk
	MemChunk* next = GetNextOrderChunk(chunk);
	if(ChunkIsEmpty(next)){
		MemChunk* new_next = GetChunkAtOffset(chunk, GetChunkSize(chunk));
		
		//if next is the last chunk, adjust heap's last chunk pointer
		//else, adjust next_next chunk's prev order chunk pointer and next free chunk's pointer
		if(next == g_memory->arena_heap.last_chunk){
			g_memory->arena_heap.last_chunk = new_next;
		}else{
			MemChunk* next_next = GetNextOrderChunk(next);
			next_next->prev = new_next;
		}
		
		//fill the new_next chunk
		new_next->prev = chunk;
		new_next->size = (GetChunkSize(next) + size_difference) | MEMORY_EMPTY_FLAG;
		
		//adjust free chunk linked-list
		NodeInsertPrev(&next->node, &new_next->node);
		NodeRemove(&next->node);
		
		//zero the old_next chunk memory
		ZeroMemory(next, sizeof(MemChunk));
		
		DEBUG_CheckHeap(&g_memory->arena_heap);
		DEBUG_CheckArenaHeapArenas();
		DEBUG_PrintArenaHeapChunks();
		DEBUG_PrintArenaAction("Fit an arena    [0x%p]%s from %zu bytes to %zu bytes", arena, info.name.str, old_size, arena->size);
		
		return;
	}
	
	//next chunk is not empty, so create a new empty chunk if there is enough space for arena overhead
	if(size_difference > MEMORY_ARENA_OVERHEAD){
		MemChunk* new_chunk = GetChunkAtOffset(chunk, GetChunkSize(chunk));
		new_chunk->prev = chunk;
		new_chunk->size = size_difference | MEMORY_EMPTY_FLAG;
		NodeInsertNext(&g_memory->arena_heap.empty_nodes, &new_chunk->node);
		next->prev = new_chunk;
		g_memory->arena_heap.used += sizeof(MemChunk);
	}
	
	DEBUG_CheckHeap(&g_memory->arena_heap);
	DEBUG_CheckArenaHeapArenas();
	DEBUG_PrintArenaHeapChunks();
	DEBUG_PrintArenaAction("Fit an arena    [0x%p]%s from %zu bytes to %zu bytes", arena, info.name.str, old_size, arena->size);
}


void
deshi__memory_arena_delete(Arena* arena, str8 file, upt line){DPZoneScoped;
	if(g_memory->cleanup_happened) return;
	if(arena == 0) return;
	
	DEBUG_CheckHeap(&g_memory->arena_heap);
	DEBUG_CheckArenaHeapArenas();
	Assert(g_memory->arena_heap.initialized, "Attempted to delete an arena before memory_init() has been called");
	
	AllocInfo info = deshi__memory_allocinfo_get(arena);
	MemChunk* chunk = ArenaToChunk(arena);
	
	DEBUG_AllocInfo_Deletion(arena);
	
	//if allocated from libc, free with libc
	if(ChunkIsLibc(chunk)){
		free(chunk);
		
		DEBUG_PrintArenaAction("Deleted a libc arena[0x%p]%s with %zu bytes", arena, info.name.str, arena->size);
		
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
		//NOTE(delle) zero_pointer doesnt change
		zero_amount += sizeof(MemChunk); //NOTE(delle) next's memory is already zeroed, so only zero the chunk header
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
		NodeRemove(&prev->node); //NOTE(delle) remove and reinsert as first empty node for locality
		NodeInsertNext(&g_memory->arena_heap.empty_nodes, &prev->node);
		prev->size += GetChunkSize(chunk);
		zero_pointer = chunk; //NOTE(delle) prev's memory is already zeroed, so only zero chunk
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
	
	DEBUG_CheckHeap(&g_memory->arena_heap);
	DEBUG_CheckArenaHeapArenas();
	DEBUG_PrintArenaHeapChunks();
	DEBUG_PrintArenaAction("Deleted an arena[0x%p]%s with %zu bytes", arena, info.name.str, arena->size);
}


void*
deshi__memory_arena_push(Arena* arena, upt size, str8 file, upt line){DPZoneScoped;
	if((arena->start + arena->size) - arena->cursor < size){
		AllocInfo info = deshi__memory_allocinfo_get(arena);
		LogE("memory","Failed to push ",size," bytes to arena[0x",(void*)arena,"]",info.name," (triggered at ",file,":",line,")");
		return 0;
	}
	
	void* result = arena->cursor;
	arena->cursor += size;
	arena->used   += size;
	return result;
}


void*
deshi__memory_arena_add(Arena* arena, void* item, upt size, str8 file, upt line){DPZoneScoped;
	if((arena->start + arena->size) - arena->cursor < size){
		AllocInfo info = deshi__memory_allocinfo_get(arena);
		LogE("memory","Failed to add ",size," bytes to arena[0x",(void*)arena,"]",info.name," (triggered at ",file,":",line,")");
		return 0;
	}
	
	void* result = arena->cursor;
	CopyMemory(result, item, size);
	arena->cursor += size;
	arena->used   += size;
	return result;
}


Heap*
deshi__memory_arena_expose(){DPZoneScoped;
	return &g_memory->arena_heap;
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @memory_pool
void*
deshi__memory_pool_init(void* pool, upt type_size, upt count){
	//calc chunk and alloc size
	upt chunk_size = Max(type_size, sizeof(void*));
	upt alloc_size = sizeof(PoolHeader) + (chunk_size * count);
	
	//allocate space for chunks and header
	pool = memory_alloc(sizeof(PoolHeader) + alloc_size);
	PoolHeader* header = memory_pool_header(pool);
	header->chunks_per_block = count;
	
	//set free chunk equal to the first chunk
	header->free_chunk = (void**)(pool);
	
	//setup the rest of the free chunk linked list
	for(u8* chunk = (u8*)(pool); chunk < (u8*)header + alloc_size - chunk_size; chunk += chunk_size){
		*(void**)chunk = chunk + chunk_size;
	}
	
	//return pool so the macro can assign to var
	return pool;
}


void
memory_pool_deinit(void* pool){
	//zfree the first block (and header)
	PoolHeader* header = memory_pool_header(pool);
	void** next_block = header->next_block;
	memory_zfree(header);
	
	//zfree the rest of the blocks by iterating the linked list
	if(next_block){
		while(*next_block){
			void* block = (void*)next_block;
			next_block = (void**)(*next_block);
			memory_zfree(block);
		}
	}
}


void
deshi__memory_pool_grow(void* pool, upt type_size, upt count){
	PoolHeader* header = memory_pool_header(pool);
	
	//calc chunk and alloc size
	upt chunk_size = Max(type_size, sizeof(void*));
	upt alloc_size = sizeof(void*) + (chunk_size * (count));
	
	//iterate to the last block
	void** last_block = header->next_block;
	if(last_block){
		while(*last_block){
			last_block = (void**)(*last_block);
		}
	}
	
	//allocate a new block and point previously last block to it
	if(last_block){
		*last_block = memory_alloc(alloc_size);
	}else{
		header->next_block = (void**)memory_alloc(alloc_size);
		last_block = header->next_block;
	}
	
	//point to the previous free chunk in the last chunk of the new block
	*((void**)((u8*)last_block + alloc_size - chunk_size)) = header->free_chunk;
	
	//set free chunk equal to the first chunk in the new block
	header->free_chunk = last_block+1;
	
	//setup the rest of the free chunk single linked list
	for(u8* chunk = (u8*)header->free_chunk; chunk < (u8*)last_block + alloc_size - chunk_size; chunk += chunk_size){
		*(void**)chunk = chunk + chunk_size;
	}
}


void*
deshi__memory_pool_push(void* pool, upt type_size){
	PoolHeader* header = memory_pool_header(pool);
	
	//grow if there are no free chunks
	if(header->free_chunk == 0){
		deshi__memory_pool_grow(pool, type_size, header->chunks_per_block);
	}
	
	//store the current free chunk
	void* result = header->free_chunk;
	
	//set free chunk to the next free chunk in the linked list
	header->free_chunk = (void**)*header->free_chunk;
	
	//zero the result memory
	*(void**)result = 0;
	
	header->count += 1;
	return result;
}


void
deshi__memory_pool_delete(void* pool, upt type_size, void* ptr){
	PoolHeader* header = memory_pool_header(pool);
	
	//zero the memory at ptr
	ZeroMemory(ptr, type_size);
	
	//create a new free chunk at ptr pointing to the previous free chunk
	void** chunk = (void**)ptr;
	*chunk = header->free_chunk;
	
	//set pool's free chunk to the new free chunk
	header->free_chunk = chunk;
	
	header->count -= 1;
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @memory_generic
#if MEMORY_PRINT_GENERIC_CHUNKS
FORCE_INLINE void DEBUG_PrintGenericHeapChunks(){ DEBUG_PrintHeapChunks(g_memory->generic_heap,"Generic Heap"); }
#else //MEMORY_PRINT_GENERIC_CHUNKS
#  define DEBUG_PrintGenericHeapChunks()
#endif //MEMORY_PRINT_GENERIC_CHUNKS


local void*
AllocateLibc(upt aligned_size){DPZoneScoped; //NOTE(delle) expects pre-aligned size with chunk
	MemChunk* new_chunk = (MemChunk*)calloc(1, aligned_size);
	Assert(new_chunk, "Libc failed to allocate memory");
	new_chunk->size = aligned_size | MEMORY_LIBC_FLAG;
	return ChunkToMemory(new_chunk);
}


local void* 
ReallocateLibc(void* ptr, upt aligned_size){DPZoneScoped; //NOTE(delle) expects pre-aligned size with chunk
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


#if MEMORY_PRINT_GENERIC_ACTIONS
#  define DEBUG_PrintGenericAction(text,...) Logf("memory",text " (triggered at %s:%zu)", __VA_ARGS__, file.str, line)
#else
#  define DEBUG_PrintGenericAction(text,...) (void)0
#endif //MEMORY_PRINT_GENERIC_ACTIONS


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
	//NOTE(delle) when generic allocations use arena, the layout ends up like Chunk -> Arena -> Chunk
	//  where the first chunk is for the arena and the second is for the generic allocation
	if(aligned_size > MEMORY_MAX_GENERIC_SIZE){
		Arena* arena = deshi__memory_arena_create(aligned_size, file, line);
		arena->used = aligned_size;
		Assert(arena->size >= aligned_size, "Arena size must be greater than requested size");
		
		MemChunk* chunk = (MemChunk*)arena->start;
		chunk->size = arena->size | MEMORY_ARENAD_FLAG;
		result = ChunkToMemory(chunk);
		
		DEBUG_PrintGenericAction("Created an allocation[0x%p] with %zu bytes", result, aligned_size);
		DEBUG_AllocInfo_Creation(result, file, line);
		
		return result;
	}
	
	//check if there are any empty chunks that can hold the allocation
	for(Node* node = g_memory->generic_heap->empty_nodes.next; node != &g_memory->generic_heap->empty_nodes; node = node->next){
		MemChunk* chunk = CastFromMember(MemChunk, node, node);
		upt chunk_size = GetChunkSize(chunk); //NOTE(delle) remember that chunk size includes the overhead
		if(chunk_size >= aligned_size){
			upt leftover_size = chunk_size - aligned_size;
			Assert(leftover_size % MEMORY_BYTE_ALIGNMENT == 0, "Memory was not aligned correctly");
			MemChunk* next = GetNextOrderChunk(chunk);
			
			//make new empty chunk after current chunk if there is enough space for an empty chunk
			//NOTE(delle) '>=' rather than '>' because empty chunks use 8/16 bytes (Node) that in-use chunks dont (useful for small allocations)
			if(leftover_size >= MEMORY_MIN_CHUNK_SIZE){
				NodeRemove(&chunk->node); //NOTE(delle) remove this early so new_chunk doesnt break next's nodes before removal
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
			
			//convert empty node to order node  //NOTE(delle) chunk->prev doesnt need to change
			chunk->size = aligned_size;
			g_memory->generic_heap->used += aligned_size - sizeof(MemChunk);
			result = ChunkToMemory(chunk);
			
			DEBUG_CheckHeap(g_memory->generic_heap);
			DEBUG_PrintGenericHeapChunks();
			DEBUG_PrintGenericAction("Created an allocation[0x%p] with %zu bytes", result, aligned_size);
			DEBUG_AllocInfo_Creation(result, file, line);
			
			return result;
		}
	}
	
	//if we cant replace an empty node, make a new order node for the allocation (if there is space)
	if(g_memory->generic_heap->cursor + aligned_size > g_memory->generic_heap->start + g_memory->generic_heap->size){
		LogfE("memory","Deshi ran out of generic memory when attempting to allocate %zu bytes (triggered at %s:%zu); defaulting to libc calloc.", requested_size, file.str, line);
		result = AllocateLibc(aligned_size);
		
		DEBUG_PrintGenericAction("Created an allocation[0x%p] with %zu bytes", result, aligned_size);
		DEBUG_AllocInfo_Creation(result, file, line);
		
		return result;
	}
	
	MemChunk* new_chunk = (MemChunk*)g_memory->generic_heap->cursor;
	new_chunk->prev = g_memory->generic_heap->last_chunk;
	new_chunk->size = aligned_size;
	g_memory->generic_heap->cursor    += aligned_size;
	g_memory->generic_heap->used      += aligned_size;
	g_memory->generic_heap->last_chunk = new_chunk;
	result = ChunkToMemory(new_chunk);
	
	DEBUG_CheckHeap(g_memory->generic_heap);
	DEBUG_PrintGenericHeapChunks();
	DEBUG_PrintGenericAction("Created an allocation[0x%p] with %zu bytes", result, aligned_size);
	DEBUG_AllocInfo_Creation(result, file, line);
	
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
		
		DEBUG_PrintGenericAction("Reallocated a libc allocation[0x%p]%s to [0x%p] with %zu bytes", ptr, info.name.str, result, requested_size);
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
	//NOTE(delle) when generic allocations use arena, the layout ends up like Chunk -> Arena -> Chunk
	//  where the first chunk is for the arena and the second is for the generic allocation
	if(ChunkIsArenad(chunk)){ 
		if(aligned_size <= GetChunkSize(chunk)) return ptr; //do nothing if less than previous size
		
		Arena* arena = (Arena*)(chunk-1);
		arena = deshi__memory_arena_grow(arena, aligned_size - arena->size, file, line);
		Assert(arena->size >= aligned_size, "Arena size must be greater than requested size");
		arena->used = aligned_size;
		
		chunk = (MemChunk*)arena->start;
		chunk->size = arena->size | MEMORY_ARENAD_FLAG;
		result = ChunkToMemory(chunk);
		
		DEBUG_CheckHeap(g_memory->generic_heap);
		DEBUG_PrintGenericHeapChunks();
		DEBUG_PrintGenericAction("Reallocated an allocation[0x%p]%s to [0x%p] with %zu bytes", ptr, info.name.str, result, requested_size);
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
		//NOTE(delle) since its larger than MEMORY_MAX_GENERIC_SIZE and not an arena already, it cant be less than previous size
		Arena* arena = deshi__memory_arena_create(aligned_size, file, line);
		arena->used = aligned_size;
		memcpy(arena->start, chunk, GetChunkSize(chunk));
		
		chunk = (MemChunk*)arena->start;
		chunk->prev = 0;
		chunk->size = arena->size | MEMORY_ARENAD_FLAG;
		result = ChunkToMemory(chunk);
		
		
		DEBUG_CheckHeap(g_memory->generic_heap);
		DEBUG_PrintGenericHeapChunks();
		DEBUG_PrintGenericAction("Reallocated an allocation[0x%p]%s to [0x%p] with %zu bytes", ptr, info.name.str, result, requested_size);
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
			
			DEBUG_PrintGenericAction("Reallocated an allocation[0x%p]%s to libc [0x%p] with %zu bytes", ptr, info.name.str, result, requested_size);
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
		
		DEBUG_CheckHeap(g_memory->generic_heap);
		DEBUG_PrintGenericHeapChunks();
		DEBUG_PrintGenericAction("Reallocated an allocation[0x%p]%s with %zu bytes without moving", ptr, info.name.str, requested_size);
		
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
		
		DEBUG_CheckHeap(g_memory->generic_heap);
		DEBUG_PrintGenericHeapChunks();
		DEBUG_PrintGenericAction("Reallocated an allocation[0x%p]%s with %zu bytes without moving", ptr, info.name.str, requested_size);
		
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
		//NOTE(delle) '>=' rather than '>' because empty chunks use 8/16 bytes (Node) that in-use chunks dont (useful for small allocations)
		MemChunk* next_next = GetNextOrderChunk(next);
		if(leftover_size >= MEMORY_MIN_CHUNK_SIZE){
			NodeRemove(&next->node); //NOTE(delle) remove this early so new_chunk doesnt break next's nodes before removal
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
		
		DEBUG_CheckHeap(g_memory->generic_heap);
		DEBUG_PrintGenericHeapChunks();
		DEBUG_PrintArenaAction("Reallocated an allocation[0x%p]%s with %zu bytes without moving", ptr, info.name.str, aligned_size);
		
		return ptr;
	}
	
	//requested size is greater than previous size and we need to move memory in order to fit the new size
	result = deshi__memory_generic_allocate(requested_size, file, line);
	Assert(GetChunkSize(MemoryToChunk(result)) >= GetChunkSize(chunk) - MEMORY_CHUNK_OVERHEAD, "New chunk size must be greater than previous size");
	memcpy(result, &chunk->node, GetChunkSize(chunk) - MEMORY_CHUNK_OVERHEAD);
	chunk = MemoryToChunk(result);
	
	DEBUG_CheckHeap(g_memory->generic_heap);
	DEBUG_PrintGenericHeapChunks();
	DEBUG_PrintArenaAction("Reallocated an allocation[0x%p%s] to [0x%p] with %zu bytes", ptr, info.name.str, result, requested_size);
	deshi__memory_allocinfo_set(result, info.name, info.type);
	
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
	Assert(!HasFlag(chunk->size, MEMORY_EMPTY_FLAG), "This pointer has already been freed.");
	
	DEBUG_AllocInfo_Deletion(ptr);
	
	//if allocated from libc, free with libc
	if(ChunkIsLibc(chunk)){
		FreeLibc(ptr);
		
		DEBUG_PrintGenericAction("Freed a libc allocation[0x%p]%s", ptr, info.name.str);
		
		return;
	}
	Assert(ptr > g_memory->arena_heap.start && ptr < g_memory->arena_heap.cursor, "Attempted to free a pointer outside the main heap and missing the libc flag");
	
	//if allocation used an arena, delete the arena
	//NOTE(delle) when generic allocations use arena, the layout ends up like Chunk -> Arena -> Chunk
	//  where the first chunk is for the arena and the second is for the generic allocation
	if(ChunkIsArenad(chunk)){
		deshi__memory_arena_delete((Arena*)chunk - 1, file, line);
		
		DEBUG_CheckHeap(g_memory->generic_heap);
		DEBUG_PrintGenericHeapChunks();
		DEBUG_PrintGenericAction("Freed   an allocation[0x%p]%s", ptr, info.name.str);
		
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
		//NOTE(delle) zero_pointer doesnt change
		zero_amount += sizeof(MemChunk); //NOTE(delle) next's memory is already zeroed, so only zero the chunk header
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
		NodeRemove(&prev->node); //NOTE(delle) remove and reinsert as first empty node for locality
		NodeInsertNext(&g_memory->generic_heap->empty_nodes, &prev->node);
		prev->size += GetChunkSize(chunk);
		zero_pointer = chunk; //NOTE(delle) prev's memory is already zeroed, so only zero chunk
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
	
	DEBUG_CheckHeap(g_memory->generic_heap);
	DEBUG_PrintGenericHeapChunks();
	DEBUG_PrintGenericAction("Freed   an allocation[0x%p]%s", ptr, info.name.str);
}


Heap*
deshi__memory_generic_expose(){DPZoneScoped;
	return g_memory->generic_heap;
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @memory_temp
#if MEMORY_PRINT_TEMP_ACTIONS
#  define DEBUG_PrintTempAction(text,...) Logf("memory",text " (triggered at %s:%zu)", __VA_ARGS__, file.str, line)
#else
#  define DEBUG_PrintTempAction(text,...) (void)0
#endif //MEMORY_PRINT_TEMP_ACTIONS


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
	
	DEBUG_PrintTempAction("Created a temp allocation[0x%p] with %zu bytes", result, aligned_size);
	
	return result;
}


void*
deshi__memory_temp_reallocate(void* ptr, upt size, str8 file, upt line){DPZoneScoped;
	if(g_memory->cleanup_happened) return 0;
	if(size == 0) return 0;
	if(ptr == 0) return 0;
	
	DEBUG_PrintTempAction("Reallocating a temp ptr[0x%p]%s to %zu bytes", ptr, deshi__memory_allocinfo_get(ptr).name.str, size);
	
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
	
	DEBUG_PrintTempAction("Clearing temporary memory which used %zu bytes", g_memory->temp_arena.used);
	
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
	if(g_memory->cleanup_happened) return;
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
		alloc_infos_active.insert(AllocInfo{address, str8_lit(""), 0, DeshTime->frame, upt(-1), name, type}, middle);
	}
#endif //MEMORY_TRACK_ALLOCS
}


AllocInfo
deshi__memory_allocinfo_get(void* address){DPZoneScoped;
	local AllocInfo null_alloc_info{0, str8_lit(""), 0, 0, upt(-1), str8_lit(""), 0};
#if MEMORY_TRACK_ALLOCS
	local AllocInfo test_alloc_info{0, str8_lit(""), 0, 0, upt(-1), str8_lit(""), 0};
	
	if(g_memory->cleanup_happened) return null_alloc_info;
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


void
deshi__memory_allocinfo_active_expose(AllocInfo** out_array, upt* out_size){DPZoneScoped;
#if MEMORY_TRACK_ALLOCS
	*out_array = alloc_infos_active.data;
	*out_size = alloc_infos_active.count;
#else
	*out_array = 0;
	*out_size = 0;
#endif //MEMORY_TRACK_ALLOCS
}


void
deshi__memory_allocinfo_inactive_expose(AllocInfo** out_array, upt* out_size){DPZoneScoped;
#if MEMORY_TRACK_ALLOCS
	*out_array = alloc_infos_inactive.data;
	*out_size = alloc_infos_inactive.count;
#else
	*out_array = 0;
	*out_size = 0;
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
memory_init(upt main_size, upt temp_size){DPZoneScoped;
	void* base_address = 0;
	u8*   allocation   = 0;
	u64   total_size   = main_size + temp_size + sizeof(MemoryContext);
	
#if BUILD_INTERNAL //NOTE(delle) when debugging, always allocate to the same address so addresses stay the same across sessions
	base_address = (void*)Terabytes(2);
#endif //BUILD_INTERNAL
	
	u32 retries = 0;
	u32 max_retries = 1000;
	while(allocation == 0 && retries < max_retries){
#if   DESHI_WINDOWS
		allocation = (u8*)VirtualAlloc((LPVOID)base_address, (SIZE_T)total_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
#elif DESHI_LINUX //DESHI_WINDOWS
		// TODO(sushi) confirm that MAP_PRIVATE is the behavoir we want 
		allocation = (u8*)mmap(base_address, total_size, PROT_READ|PROT_WRITE, MAP_PRIVATE, 0, 0);
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
	deshi__memory_allocinfo_set(&g_memory->arena_heap, str8_lit("Arena Heap"), 0);
	
	g_memory->generic_arena = memory_create_arena((main_size / 4) + sizeof(Heap));
	g_memory->generic_heap = (Heap*)g_memory->generic_arena->start;
	g_memory->generic_heap->start      = (u8*)(g_memory->generic_heap+1);
	g_memory->generic_heap->cursor     = g_memory->generic_heap->start;
	g_memory->generic_heap->used       = 0;
	g_memory->generic_heap->size       = g_memory->generic_arena->size - sizeof(Heap);
	g_memory->generic_heap->empty_nodes.next = g_memory->generic_heap->empty_nodes.prev = &g_memory->generic_heap->empty_nodes;
	g_memory->generic_heap->last_chunk = 0;
	g_memory->generic_heap->initialized = true;
	DEBUG_CheckHeap(g_memory->generic_heap);
	deshi__memory_allocinfo_set(g_memory->generic_heap, str8_lit("Generic Heap"), 0);
	
	g_memory->temp_arena.start  = g_memory->arena_heap.start + g_memory->arena_heap.size;
	g_memory->temp_arena.cursor = g_memory->temp_arena.start;
	g_memory->temp_arena.size   = temp_size;
	g_memory->temp_arena.used   = 0;
	deshi__memory_allocinfo_set(&g_memory->temp_arena, str8_lit("Temp Arena"), 0);
	
	deshiStage |= DS_MEMORY;
}


void
memory_cleanup(){DPZoneScoped;
	g_memory->cleanup_happened = true;
}