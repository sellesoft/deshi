//NOTE Memory Layout: 
//|                                Total Size                                 |
//|                            Main Heap                        |  Temp Arena |
//| Generic Arena |      Heap Arena      |      Heap Arena      | Item | Item |
//| Chunk | Chunk | Header |    Memory   | Header |    Memory   |      |      |
//|       |       |        | Item | Item |        | Item | Item |      |      |

////////////////
//// @utils ////
////////////////
#define MEMORY_POINTER_SIZE sizeof(void*)
#define MEMORY_BYTE_ALIGNMENT (2*MEMORY_POINTER_SIZE)
#define MEMORY_BYTE_ALIGNMENT_MASK (MEMORY_BYTE_ALIGNMENT-1)
#define MEMORY_DO_HEAP_CHECKS true
#define MEMORY_DO_HEAP_PRINTS false
#define MEMORY_DO_ARENA_PRINTS false
#define MEMORY_DO_GENERIC_PRINTS false
#define MEMORY_DO_TEMP_PRINTS false

////////////////
//// @arena ////
////////////////
local ArenaHeap deshi__arena_heap_;
local ArenaHeap* deshi__arena_heap = &deshi__arena_heap_;

#define MEMORY_ARENA_MIN_ALLOC_SIZE ((sizeof(ArenaHeapNode) + MEMORY_BYTE_ALIGNMENT_MASK) & ~(MEMORY_BYTE_ALIGNMENT_MASK))
#define MEMORY_ARENA_ALIGNMENT MEMORY_ARENA_MIN_ALLOC_SIZE
#define MEMORY_ARENA_LIBC_FLAG 0x1 //had to use libc b/c we ran out of memory

#if MEMORY_DO_HEAP_CHECKS
local void
DEBUG_CheckArenaHeapNodes(ArenaHeap* heap){
	Assert(heap->order.next != 0 && heap->order.prev != 0, "First heap order node is invalid");
	Assert(heap->empty.next != 0 && heap->empty.prev != 0, "First heap empty node is invalid");
	for(Node* node = &heap->order; ; ){
		Assert(node->next->prev == node && node->prev->next == node, "Heap order node is invalid");
		node = node->next;
		if(node == &heap->order) break;
	}
	for(Node* node = &heap->empty; ; ){
		Assert(node->next->prev == node && node->prev->next == node, "Heap empty node is invalid");
		node = node->next;
		if(node == &heap->empty) break;
	}
}

local void
DEBUG_CheckArenaHeapArenas(ArenaHeap* heap){
	Assert(PointerDifference(heap->cursor, heap->start) % MEMORY_BYTE_ALIGNMENT == 0, "Memory alignment is invalid");
	Assert(PointerDifference(heap->cursor, heap->start) >= heap->used, "Heap used amount is greater than cursor offset");
	upt overall_used = 0;
	for(Node* order = &heap->order; ; ){
		if(order != &heap->order){
			if(order->next != &heap->order){
				ArenaHeapNode* node = CastFromMember(ArenaHeapNode, order, order);
				overall_used += node->arena.size + sizeof(ArenaHeapNode);
				ArenaHeapNode* next = CastFromMember(ArenaHeapNode, order, order->next);
				Assert(node->arena.start + node->arena.size == (u8*)next, "Heap node arena is not sized correctly");
			}else{ //last node
				ArenaHeapNode* node = CastFromMember(ArenaHeapNode, order, order);
				overall_used += node->arena.size + sizeof(ArenaHeapNode);
				Assert(node->arena.start + node->arena.size == heap->cursor, "Heap cursor is not in the right spot");
				break;
			}
		}
		order = order->next;
		if(order == &heap->order) break;
	}
	//Assert(overall_used == heap->used, "Heap used is incorrect"); //TODO(delle) add this back
}
#else //MEMORY_DO_HEAP_CHECKS
#  define DEBUG_CheckArenaHeapNodes(heap) 
#  define DEBUG_CheckArenaHeapArenas(heap)
#endif //MEMORY_DO_HEAP_CHECKS
#if MEMORY_DO_HEAP_PRINTS
local void
DEBUG_PrintArenaHeapNodes(ArenaHeap* heap){
	if(heap->initialized && heap->used > 0){
		string heap_order = "Order: ", heap_empty = "Empty: ";
		for(ArenaHeapNode* node = (ArenaHeapNode*)heap->start; ;){
			heap_order += to_string("%p", node); heap_order += " -> ";
			heap_empty += (node->empty.next) ? to_string("%p", node) : "                ";
			heap_empty += " -> ";
			
			if(node->order.next == &heap->order) break;
			node = CastFromMember(ArenaHeapNode, order, node->order.next);
		}
		Log("memory-arena",heap_order); Log("memory-arena",heap_empty); Log("","----------------------------------------------");
	}
}
#else //MEMORY_DO_HEAP_PRINTS
#  define DEBUG_PrintArenaHeapNodes(heap)
#endif //MEMORY_DO_HEAP_PRINTS

//TODO(delle) remove this
local inline void
UpdateArenaHeapCursor(ArenaHeap* heap){ //NOTE this relies on empty nodes having correct arena.start positions
	ArenaHeapNode* last_order_node = CastFromMember(ArenaHeapNode, order, deshi__arena_heap->order.prev);
	heap->cursor = last_order_node->arena.start + last_order_node->arena.size;
}

local inline b32
ArenaUsedLibc(Arena* arena){
	return HasFlag(*((upt*)(arena+1)), MEMORY_ARENA_LIBC_FLAG);
}

local Arena*
CreateArenaLibc(upt size){
	Arena* ptr = (Arena*)calloc(1, sizeof(Arena) + MEMORY_BYTE_ALIGNMENT + size);
	Assert(ptr, "Libc failed to allocate memory");
	*((upt*)(ptr+1)) = (upt)ptr | MEMORY_ARENA_LIBC_FLAG; //place special value for checking when deleting/growing
	ptr->start  = (u8*)ptr + sizeof(Arena) + MEMORY_BYTE_ALIGNMENT;
	ptr->cursor = ptr->start;
	ptr->size   = size;
	return ptr;
}

Arena*
deshi__memory_arena_create(upt requested_size, char* file, upt line){
	DEBUG_CheckArenaHeapNodes(deshi__arena_heap);
	if(requested_size == 0) return 0;
	Assert(deshi__arena_heap && deshi__arena_heap->initialized, "Attempted to create an arena before Memory::Init() has been called");
	
#if MEMORY_DO_ARENA_PRINTS
	Logf("memory","Creating an arena with %lld bytes (triggered at %s:%lld)", requested_size, file, line);
#endif
	
	upt aligned_size = ClampMin(RoundUpTo(requested_size, MEMORY_ARENA_ALIGNMENT), MEMORY_ARENA_MIN_ALLOC_SIZE);
	
	//check if there are any empty nodes that can hold the new arena
	for(Node* n = deshi__arena_heap->empty.next; n != &deshi__arena_heap->empty; n = n->next){
		ArenaHeapNode* node = CastFromMember(ArenaHeapNode, empty, n);
		if(node->arena.size >= aligned_size){
			upt leftover_size = node->arena.size - aligned_size;
			Assert(leftover_size % sizeof(ArenaHeapNode) == 0, "Memory was not aligned correctly");
			
			//make new empty node after new order node if there is enough space
			if(leftover_size > sizeof(ArenaHeapNode)){
				ArenaHeapNode* new_node = (ArenaHeapNode*)(node->arena.start + aligned_size);
				NodeInsertNext(&node->order, &new_node->order); DEBUG_CheckArenaHeapNodes(deshi__arena_heap);
				NodeInsertNext(&node->empty, &new_node->empty); DEBUG_CheckArenaHeapNodes(deshi__arena_heap);
				new_node->arena.start = (u8*)(new_node+1);
				new_node->arena.size = leftover_size - sizeof(ArenaHeapNode);
				deshi__arena_heap->used += sizeof(ArenaHeapNode);
			}else{
				aligned_size += leftover_size;
			}
			
			//convert empty node to order node
			NodeRemove(&node->empty); DEBUG_CheckArenaHeapNodes(deshi__arena_heap);
			node->empty.next = 0;
			node->empty.prev = 0;
			//node->arena.start = (u8*)(node+1); //NOTE arena starts are correct in empty nodes
			node->arena.cursor = node->arena.start;
			node->arena.size   = aligned_size;
			node->arena.used   = 0;
			deshi__arena_heap->used += aligned_size;
			//UpdateArenaHeapCursor(deshi__arena_heap); //NOTE an empty node can't be the last order node so heap cursor wont change
			
			DEBUG_CheckArenaHeapNodes(deshi__arena_heap); DEBUG_CheckArenaHeapArenas(deshi__arena_heap); DEBUG_PrintArenaHeapNodes(deshi__arena_heap);
			return &node->arena;
		}
	}
	
	//if making a new arena and out of memory, default to libc
	if(deshi__arena_heap->cursor + aligned_size + sizeof(ArenaHeapNode) > deshi__arena_heap->start + deshi__arena_heap->size){
		LogfE("memory","Deshi ran out of main memory when attempting to create an arena (triggered at %s:%lld); defaulting to libc calloc.", file, line);
		return CreateArenaLibc(requested_size);
	}
	
	//if we cant replace an empty node, make a new order node for the new arena
	ArenaHeapNode* new_node = (ArenaHeapNode*)deshi__arena_heap->cursor;
	NodeInsertPrev(&deshi__arena_heap->order, &new_node->order); DEBUG_CheckArenaHeapNodes(deshi__arena_heap);
	new_node->arena.start  = (u8*)new_node + sizeof(ArenaHeapNode);
	new_node->arena.cursor = new_node->arena.start;
	new_node->arena.size   = aligned_size;
	//new_node->arena.used   = 0; //NOTE new memory is already zero
	deshi__arena_heap->used   += aligned_size + sizeof(ArenaHeapNode);
	UpdateArenaHeapCursor(deshi__arena_heap);
	
	DEBUG_CheckArenaHeapNodes(deshi__arena_heap); DEBUG_CheckArenaHeapArenas(deshi__arena_heap); DEBUG_PrintArenaHeapNodes(deshi__arena_heap); 
	return &new_node->arena;
}

Arena*
deshi__memory_arena_grow(Arena* arena, upt size, char* file, upt line){
	DEBUG_CheckArenaHeapNodes(deshi__arena_heap); DEBUG_CheckArenaHeapArenas(deshi__arena_heap);
	if(size == 0) return arena;
	if(arena == 0) return 0;
	Assert(deshi__arena_heap && deshi__arena_heap->initialized, "Attempted to grow an arena before Memory::Init() has been called");
	
#if MEMORY_DO_ARENA_PRINTS
	Logf("memory","Growing an arena[0x%p \"%s\"] of size %lld bytes with %lld bytes (triggered at %s:%lld)", arena, (deshi__memory_naming_get(arena)) ? deshi__memory_naming_get(arena).str : "", arena->size, size, file, line);
#endif
	
	//if out of memory, default to libc
	if(ArenaUsedLibc(arena)){
		upt old_size   = arena->size;
		upt old_cursor = arena->cursor - arena->start;
		arena = (Arena*)realloc(arena, sizeof(Arena) + MEMORY_BYTE_ALIGNMENT + old_size + size);
		arena->start = (u8*)arena + sizeof(Arena) + MEMORY_BYTE_ALIGNMENT;
		arena->cursor = arena->start + old_cursor;
		arena->size += size;
		ZeroMemory(arena->start + old_size, arena->size - old_size);
		return arena;
	}
	Assert((u8*)arena >= deshi__arena_heap->start && (u8*)arena < deshi__arena_heap->cursor, "Attempted to grow an arena that's outside the arena heap and missing the libc flag");
	
	//check if the next node is empty and can hold the grown size, or if we need to make a new arena
	Arena* result = arena;
	upt aligned_size = RoundUpTo(size, MEMORY_ARENA_ALIGNMENT);
	ArenaHeapNode* node = CastFromMember(ArenaHeapNode, arena, arena);
	ArenaHeapNode* next = CastFromMember(ArenaHeapNode, order, node->order.next);
	
	//current node is the last node
	if(&next->order == &deshi__arena_heap->order){
		if(deshi__arena_heap->cursor + aligned_size > deshi__arena_heap->start + deshi__arena_heap->size){
			LogfE("memory","Deshi ran out of main memory when attempting to grow an arena[0x%p \"%s\"] of size %lld bytes with %lld bytes (triggered at %s:%lld); defaulting to libc calloc.", arena, (deshi__memory_naming_get(arena)) ? deshi__memory_naming_get(arena).str : "", arena->size, size, file, line);
			result = CreateArenaLibc(arena->size + size);
			result->cursor += (arena->cursor - arena->start);
			result->used = arena->used;
			memcpy(result->start, arena->start, arena->size);
			deshi__memory_arena_delete(arena, file, line);
			return result;
		}
		
		arena->size += aligned_size;
		deshi__arena_heap->used += aligned_size;
		UpdateArenaHeapCursor(deshi__arena_heap);
		
		DEBUG_CheckArenaHeapNodes(deshi__arena_heap); DEBUG_CheckArenaHeapArenas(deshi__arena_heap); DEBUG_PrintArenaHeapNodes(deshi__arena_heap); 
		return result;
	}
	
	//next node is empty and can hold the growth
	if((next->empty.next != 0) && (next->empty.prev != 0) && (next->arena.size >= aligned_size)){
		//make new empty+order node after current node if there is enough space
		upt leftover_size = next->arena.size - aligned_size + sizeof(ArenaHeapNode);
		if(leftover_size > sizeof(ArenaHeapNode)){
			ArenaHeapNode* new_node = (ArenaHeapNode*)((u8*)next + aligned_size);
			NodeInsertNext(&next->order, &new_node->order); DEBUG_CheckArenaHeapNodes(deshi__arena_heap);
			NodeInsertNext(&next->empty, &new_node->empty); DEBUG_CheckArenaHeapNodes(deshi__arena_heap);
			new_node->arena.start = (u8*)(new_node+1);
			new_node->arena.size  = leftover_size - sizeof(ArenaHeapNode);
			deshi__arena_heap->used += sizeof(ArenaHeapNode);
		}else{
			aligned_size += leftover_size;
		}
		
		//add empty node space to current node
		NodeRemove(&next->order); DEBUG_CheckArenaHeapNodes(deshi__arena_heap);
		NodeRemove(&next->empty); DEBUG_CheckArenaHeapNodes(deshi__arena_heap);
		ZeroMemory(next, sizeof(ArenaHeapNode)); //NOTE only zero header since the node's memory should already be zeroed
		arena->size += aligned_size;
		deshi__arena_heap->used += (aligned_size - sizeof(ArenaHeapNode));
		UpdateArenaHeapCursor(deshi__arena_heap);
		
		DEBUG_CheckArenaHeapNodes(deshi__arena_heap); DEBUG_CheckArenaHeapArenas(deshi__arena_heap); DEBUG_PrintArenaHeapNodes(deshi__arena_heap); 
		return result;
	}
	
	//need to move memory in order to fit new size
	Arena* new_arena = deshi__memory_arena_create(arena->size + size, file, line);
	memcpy(new_arena->start, arena->start, arena->used);
	new_arena->used   = arena->used;
	new_arena->cursor = new_arena->start + (arena->cursor - arena->start);
	result = new_arena;
	deshi__memory_arena_delete(arena, file, line);
	
	DEBUG_CheckArenaHeapNodes(deshi__arena_heap); DEBUG_CheckArenaHeapArenas(deshi__arena_heap); DEBUG_PrintArenaHeapNodes(deshi__arena_heap); 
	return result;
}

void
deshi__memory_arena_clear(Arena* arena, char* file, upt line){
#if MEMORY_DO_ARENA_PRINTS
	Logf("memory","Clearing an arena[0x%p \"%s\"] of size %lld bytes (triggered at %s:%lld)", arena, (deshi__memory_naming_get(arena)) ? deshi__memory_naming_get(arena).str : "", arena->size, file, line);
#endif
	
	ZeroMemory(arena->start, arena->used);
	arena->cursor = arena->start;
	arena->used   = 0;
}

void
deshi__memory_arena_delete(Arena* arena, char* file, upt line){
	DEBUG_CheckArenaHeapNodes(deshi__arena_heap); DEBUG_CheckArenaHeapArenas(deshi__arena_heap);
	if(arena == 0) return;
	Assert(deshi__arena_heap && deshi__arena_heap->initialized, "Attempted to delete an arena before Memory::Init() has been called");
	
#if MEMORY_DO_ARENA_PRINTS
	Logf("memory","Deleting an arena[0x%p \"%s\"] of size %lld bytes (triggered at %s:%lld)", arena, (deshi__memory_naming_get(arena)) ? deshi__memory_naming_get(arena).str : "", arena->size, file, line);
#endif
	
	if(ArenaUsedLibc(arena)){ free(arena); return; }
	Assert((u8*)arena >= deshi__arena_heap->start && (u8*)arena < deshi__arena_heap->cursor, "Attempted to delete an arena outside the main heap and missing the libc flag");
	
	ArenaHeapNode* node = CastFromMember(ArenaHeapNode, arena, arena);
	void* zero_pointer = arena->start;
	upt   zero_amount  = arena->size;
	upt   used_amount  = arena->size;
	
	//insert current node into empty
	NodeInsertNext(&deshi__arena_heap->empty, &node->empty); DEBUG_CheckArenaHeapNodes(deshi__arena_heap);
	
	//try to merge next empty into current empty
	ArenaHeapNode* next = CastFromMember(ArenaHeapNode, order, node->order.next);
	if(   (&node->order != &deshi__arena_heap->order)
	   && (&next->order != &deshi__arena_heap->order)
	   && (node->empty.next != 0) && (node->empty.prev != 0)
	   && (next->empty.next != 0) && (next->empty.prev != 0)
	   && (PointerDifference(node->arena.start + node->arena.size, next) == 0)){
		NodeRemove(&next->order); DEBUG_CheckArenaHeapNodes(deshi__arena_heap);
		NodeRemove(&next->empty); DEBUG_CheckArenaHeapNodes(deshi__arena_heap);
		NodeRemove(&node->empty); DEBUG_CheckArenaHeapNodes(deshi__arena_heap); //NOTE remove and reinsert as first empty node for locality
		//UpdateArenaHeapCursor(deshi__arena_heap); //NOTE cursor should not change as next is empty and cant be last
		node->arena.size += next->arena.size + sizeof(ArenaHeapNode);
		NodeInsertNext(&deshi__arena_heap->empty, &node->empty); DEBUG_CheckArenaHeapNodes(deshi__arena_heap);
		zero_pointer = node+1;
		zero_amount  = node->arena.size;
		used_amount += sizeof(ArenaHeapNode);
	}
	
	//try to merge current empty into prev empty
	ArenaHeapNode* prev = CastFromMember(ArenaHeapNode, order, node->order.prev);
	if(   (&prev->order != &deshi__arena_heap->order)
	   && (&node->order != &deshi__arena_heap->order)
	   && (prev->empty.next != 0) && (prev->empty.prev != 0)
	   && (node->empty.next != 0) && (node->empty.prev != 0)
	   && (PointerDifference(prev->arena.start + prev->arena.size, node) == 0)){
		NodeRemove(&node->order); DEBUG_CheckArenaHeapNodes(deshi__arena_heap);
		NodeRemove(&node->empty); DEBUG_CheckArenaHeapNodes(deshi__arena_heap);
		NodeRemove(&prev->empty); DEBUG_CheckArenaHeapNodes(deshi__arena_heap);
		UpdateArenaHeapCursor(deshi__arena_heap);
		prev->arena.size += node->arena.size + sizeof(ArenaHeapNode);
		NodeInsertNext(&deshi__arena_heap->empty, &prev->empty); DEBUG_CheckArenaHeapNodes(deshi__arena_heap);
		zero_pointer = prev+1;
		zero_amount  = prev->arena.size;
		used_amount += sizeof(ArenaHeapNode);
		node = prev;
	}
	
	//remove the last order node if its empty
	if(node->order.next == &deshi__arena_heap->order){
		NodeRemove(&node->order); DEBUG_CheckArenaHeapNodes(deshi__arena_heap);
		NodeRemove(&node->empty); DEBUG_CheckArenaHeapNodes(deshi__arena_heap);
		deshi__arena_heap->cursor = (u8*)node;
		zero_pointer = node; 
		zero_amount  = node->arena.size + sizeof(ArenaHeapNode);
		used_amount += sizeof(ArenaHeapNode);
	}
	
	deshi__arena_heap->used -= used_amount;
	ZeroMemory(zero_pointer, zero_amount);
	DEBUG_CheckArenaHeapNodes(deshi__arena_heap); DEBUG_CheckArenaHeapArenas(deshi__arena_heap); DEBUG_PrintArenaHeapNodes(deshi__arena_heap);
}

ArenaHeap*
deshi__memory_arena_expose(){
	return deshi__arena_heap;
}


//////////////////
//// @generic ////
//////////////////
typedef GenericHeapNode MemoryChunk; //just an easier name than GenericHeapNode
local Arena*       deshi__generic_arena; //deshi__generic_heap is stored here; not used otherwise
local GenericHeap* deshi__generic_heap;

#define MEMORY_GENERIC_CHUNK_MEMORY_OFFSET ((upt)OffsetOfMember(MemoryChunk, node))
#define MEMORY_GENERIC_CHUNK_INUSE_OVERHEAD MEMORY_GENERIC_CHUNK_MEMORY_OFFSET
#define MEMORY_GENERIC_MIN_ALLOC_SIZE ((sizeof(MemoryChunk) + MEMORY_BYTE_ALIGNMENT_MASK) & ~(MEMORY_BYTE_ALIGNMENT_MASK))
#define MEMORY_GENERIC_MAX_ALLOC_SIZE Kilobytes(64)

#define ChunkToMemory(chunk)\
((void*)((u8*)(chunk) + MEMORY_GENERIC_CHUNK_MEMORY_OFFSET))
#define MemoryToChunk(memory)\
((MemoryChunk*)((u8*)(memory) - MEMORY_GENERIC_CHUNK_MEMORY_OFFSET))

//NOTE Chunk Flags !ref: https://github.com/lattera/glibc/blob/895ef79e04a953cac1493863bcae29ad85657ee1/malloc/malloc.c#L1224
//  Chunk flags are stored in the lower bits of the chunk's size variable, and this doesn't cause problems b/c the size 
//  is always greater than 8 bytes on 32bit and 16 bytes on on 64bit (MEMORY_GENERIC_MIN_ALLOC_SIZE).
//  To get the size, we mask off the bits holding these flags.
//  ISARENAD and PREVINUSE should never be used together
//
//  PREVINUSE (0x1): previous adjacent chunk is in use
//  ISARENAD  (0x2): the chunk was large enough to use an Arena rather than bins
//  LIBC      (0x4): deshi ran out of memory and had to call libc
#define MEMORY_GENERIC_PREVINUSE_FLAG 0x1
#define MEMORY_GENERIC_ISARENAD_FLAG 0x2
#define MEMORY_GENERIC_LIBC_FLAG 0x4
#define MEMORY_GENERIC_CHUNK_SIZE_BITS (MEMORY_GENERIC_PREVINUSE_FLAG | MEMORY_GENERIC_ISARENAD_FLAG)
#define MEMORY_GENERIC_EXTRACT_SIZE_BITMASK (~MEMORY_GENERIC_CHUNK_SIZE_BITS)

#define PrevChunkIsInUse(chunk_ptr)\
((chunk_ptr)->size & MEMORY_GENERIC_PREVINUSE_FLAG)
#define GetChunkSize(chunk_ptr)\
((chunk_ptr)->size & MEMORY_GENERIC_EXTRACT_SIZE_BITMASK)
#define GetNextOrderChunk(chunk_ptr)\
((MemoryChunk*)((u8*)(chunk_ptr) + GetChunkSize(chunk_ptr)))
#define GetPrevOrderChunk(chunk_ptr)\
((chunk_ptr)->prev)
#define GetChunkAtOffset(chunk_ptr,offset)\
((MemoryChunk*)((u8*)(chunk_ptr) + (offset)))
#define ChunkIsInUse(chunk_ptr)\
(((MemoryChunk*)((u8*)(chunk_ptr) + GetChunkSize(chunk_ptr)))->size & MEMORY_GENERIC_PREVINUSE_FLAG)
#define ChunkIsArenad(chunk_ptr)\
((chunk_ptr)->size & MEMORY_GENERIC_ISARENAD_FLAG)
#define ChunkIsLibc(chunk_ptr)\
((chunk_ptr)->size & MEMORY_GENERIC_LIBC_FLAG)


#if MEMORY_DO_HEAP_CHECKS
local void
DEBUG_CheckGenericHeap(GenericHeap* heap){
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
		for(MemoryChunk* chunk = (MemoryChunk*)heap->start; ; chunk = GetNextOrderChunk(chunk)){
			Assert((u8*)chunk < heap->cursor, "All chunks must be below the cursor");
			Assert(GetChunkSize(chunk) >= MEMORY_GENERIC_MIN_ALLOC_SIZE, "Chunk size is less than minimum");
			Assert(GetChunkSize(chunk) % MEMORY_BYTE_ALIGNMENT == 0, "Chunk size is not aligned correctly");
			if(chunk != heap->last_chunk){
				Assert(GetNextOrderChunk(chunk)->prev == chunk, "Next order chunk is not correctly pointing to current chunk");
				if(!PrevChunkIsInUse(GetNextOrderChunk(chunk))){
					overall_used += sizeof(MemoryChunk);
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
#else //MEMORY_DO_HEAP_CHECKS
#  define DEBUG_CheckGenericHeap(heap)
#endif //MEMORY_DO_HEAP_CHECKS
#if MEMORY_DO_HEAP_PRINTS
local void
DEBUG_PrintGenericArenaNodes(GenericHeap* heap){
	if(heap->initialized && heap->used > 0){
		string heap_order = "Order: ", heap_empty = "Empty: ";
		for(MemoryChunk* chunk = (MemoryChunk*)heap->start; ; chunk = GetNextOrderChunk(chunk)){
			heap_order += to_string("%p", chunk); heap_order += " -> ";
			if(chunk != heap->last_chunk){
				if(!ChunkIsInUse(chunk)){
					heap_empty += to_string("%p", chunk);
					heap_empty += " -> ";
				}else{
					heap_empty += "                ";
					heap_empty += " -> ";
				}
			}else{
				heap_empty += "                ";
				heap_empty += " -> ";
				break;
			}
		}
		Log("memory-generic",heap_order); Log("memory-generic",heap_empty); Log("","----------------------------------------------");
	}
}
#else //MEMORY_DO_HEAP_PRINTS
#  define DEBUG_PrintGenericArenaNodes(heap)
#endif //MEMORY_DO_HEAP_PRINTS

local void*
AllocateLibc(upt aligned_size){
	MemoryChunk* new_chunk = (MemoryChunk*)calloc(1, aligned_size);
	Assert(new_chunk, "Libc failed to allocate memory");
	new_chunk->size = aligned_size | MEMORY_GENERIC_LIBC_FLAG;
	return &new_chunk->node;
}

local void* 
ReallocateLibc(void* ptr, upt aligned_size){
	MemoryChunk* chunk = MemoryToChunk(ptr);
	upt old_size = GetChunkSize(chunk);
	chunk = (MemoryChunk*)realloc(chunk, aligned_size);
	chunk->size = aligned_size | MEMORY_GENERIC_LIBC_FLAG;
	if(aligned_size > old_size) ZeroMemory((u8*)chunk + old_size, aligned_size - old_size);
	return &chunk->node;
}

local void
FreeLibc(void* ptr){
	MemoryChunk* chunk = MemoryToChunk(ptr);
	Assert(ChunkIsLibc(chunk), "This was not allocated using libc");
	free(chunk);
}

void*
deshi__memory_generic_allocate(upt requested_size, char* file, upt line){
	DEBUG_CheckGenericHeap(deshi__generic_heap);
	if(requested_size == 0) return 0;
	Assert(deshi__generic_heap && deshi__generic_heap->initialized, "Attempted to allocate before Memory::Init() has been called");
	
#if MEMORY_DO_GENERIC_PRINTS
	Logf("memory","Creating an allocation of %lld bytes (triggered at %s:%lld)", requested_size, file, line);
#endif
	
	//include chunk overhead, align to the byte alignment, and clamp the minimum
	upt aligned_size = ClampMin(RoundUpTo(requested_size + MEMORY_GENERIC_CHUNK_INUSE_OVERHEAD, MEMORY_BYTE_ALIGNMENT), MEMORY_GENERIC_MIN_ALLOC_SIZE);
	
	//if the allocation is large, create an arena for it rather than using the generic heap
	if(aligned_size >= MEMORY_GENERIC_MAX_ALLOC_SIZE){
		Arena* arena = deshi__memory_arena_create(aligned_size, file, line);
		arena->used = aligned_size;
		Assert(arena->size >= aligned_size, "Arena size must be greater than requested size");
		MemoryChunk* chunk = (MemoryChunk*)arena->start;
		//chunk->prev = 0; //NOTE fresh memory is already zero
		chunk->size = arena->size | MEMORY_GENERIC_ISARENAD_FLAG;
		return ChunkToMemory(chunk);
	}
	
	//check if there are any empty chunks that can hold the allocation
	for(Node* node = deshi__generic_heap->empty_nodes.next; node != &deshi__generic_heap->empty_nodes; node = node->next){
		MemoryChunk* chunk = CastFromMember(MemoryChunk, node, node);
		upt chunk_size = GetChunkSize(chunk); //NOTE remember that chunk size includes the overhead
		if(chunk_size >= aligned_size){
			upt leftover_size = chunk_size - aligned_size;
			Assert(leftover_size % MEMORY_BYTE_ALIGNMENT == 0, "Memory was not aligned correctly");
			MemoryChunk* next = GetNextOrderChunk(chunk);
			
			//make new empty chunk after current chunk if there is enough space for an empty chunk
			//NOTE '>=' rather than '>' because empty chunks use 8/16 bytes (Node) that in-use chunks dont (useful for small allocations)
			if(leftover_size >= MEMORY_GENERIC_MIN_ALLOC_SIZE){
				MemoryChunk* new_chunk = GetChunkAtOffset(chunk, aligned_size);
				NodeInsertNext(&deshi__generic_heap->empty_nodes, &new_chunk->node);
				new_chunk->prev = chunk;
				new_chunk->size = leftover_size; //NOTE this gets OR'd with MEMORY_GENERIC_PREVINUSE_FLAG below
				next->prev = new_chunk;
				next = new_chunk;
				deshi__generic_heap->used += sizeof(MemoryChunk);
			}else{
				aligned_size += leftover_size;
			}
			
			//convert empty node to order node  //NOTE chunk->prev doesnt need to change
			NodeRemove(&chunk->node);
			chunk->node = {0}; //zero this data since it will be used by the allocation
			chunk->size = (PrevChunkIsInUse(chunk)) ? aligned_size | MEMORY_GENERIC_PREVINUSE_FLAG : aligned_size;
			next->size |= MEMORY_GENERIC_PREVINUSE_FLAG; //set PREVINUSE flag on next chunk
			deshi__generic_heap->used += aligned_size - sizeof(MemoryChunk);
			
			DEBUG_CheckGenericHeap(deshi__generic_heap); DEBUG_PrintGenericArenaNodes(deshi__generic_heap);
			return &chunk->node;
		}
	}
	
	//if we cant replace an empty node, make a new order node for the allocation (if there is space)
	if(deshi__generic_heap->cursor + aligned_size > deshi__generic_heap->start + deshi__generic_heap->size){
		LogfE("memory","Deshi ran out of generic memory when attempting to allocate %lld bytes (triggered at %s:%lld); defaulting to libc calloc.", requested_size, file, line);
		return AllocateLibc(aligned_size);
	}
	
	MemoryChunk* new_chunk = (MemoryChunk*)deshi__generic_heap->cursor;
	new_chunk->prev = deshi__generic_heap->last_chunk;
	new_chunk->size = (deshi__generic_heap->last_chunk != 0) ? aligned_size | MEMORY_GENERIC_PREVINUSE_FLAG : aligned_size;
	deshi__generic_heap->cursor    += aligned_size;
	deshi__generic_heap->used      += aligned_size;
	deshi__generic_heap->last_chunk = new_chunk;
	
	DEBUG_CheckGenericHeap(deshi__generic_heap); DEBUG_PrintGenericArenaNodes(deshi__generic_heap);
	return &new_chunk->node;
}

void*
deshi__memory_generic_reallocate(void* ptr, upt requested_size, char* file, upt line){
	DEBUG_CheckGenericHeap(deshi__generic_heap); 
	if(ptr == 0) return deshi__memory_generic_allocate(requested_size, file, line);
	if(requested_size == 0){ deshi__memory_generic_zero_free(ptr, file, line); return 0; }
	Assert(deshi__generic_heap && deshi__generic_heap->initialized, "Attempted to allocate before Memory::Init() has been called");
	
#if MEMORY_DO_GENERIC_PRINTS
	Logf("memory","Reallocating a ptr[0x%p \"%s\"] to %lld bytes (triggered at %s:%lld)", ptr, (deshi__memory_naming_get(ptr)) ? deshi__memory_naming_get(ptr).str : "", requested_size, file, line);
#endif
	
	//include chunk overhead, align to the byte alignment, and clamp the minimum
	upt aligned_size = ClampMin(RoundUpTo(requested_size + MEMORY_GENERIC_CHUNK_INUSE_OVERHEAD, MEMORY_BYTE_ALIGNMENT), MEMORY_GENERIC_MIN_ALLOC_SIZE);
	MemoryChunk* chunk = MemoryToChunk(ptr);
	
	if(ChunkIsLibc(chunk)){ return ReallocateLibc(ptr, aligned_size); }
	Assert(ptr > deshi__arena_heap->start && ptr < deshi__arena_heap->cursor, "Attempted to reallocate a pointer outside the main heap and missing the libc flag");
	
	//previous allocation was an arena
	if(ChunkIsArenad(chunk)){ 
		if(aligned_size <= GetChunkSize(chunk)) return ptr; //do nothing if less than previous size
		Arena* arena = &((ArenaHeapNode*)chunk - 1)->arena;
		arena = deshi__memory_arena_grow(arena, aligned_size - arena->size, file, line);
		Assert(arena->size >= aligned_size, "Arena size must be greater than requested size");
		arena->used = aligned_size;
		chunk = (MemoryChunk*)arena->start;
		//chunk->prev = 0; //NOTE it should already be zero
		chunk->size = arena->size | MEMORY_GENERIC_ISARENAD_FLAG;
		DEBUG_CheckGenericHeap(deshi__generic_heap); DEBUG_PrintGenericArenaNodes(deshi__generic_heap);
		return &chunk->node;
	}
	Assert(ptr > deshi__generic_heap->start && ptr < deshi__generic_heap->cursor, "Attempted to reallocate a pointer outside the generic heap and missing the libc flag");
	
	//new allocation needs to be an arena
	if(aligned_size >= MEMORY_GENERIC_MAX_ALLOC_SIZE){
		//NOTE since its larger than MEMORY_GENERIC_MAX_ALLOC_SIZE and not an arena already, it cant be less than previous size
		Arena* arena = deshi__memory_arena_create(aligned_size, file, line);
		Assert(arena->size >= aligned_size, "Arena size must be greater than requested size");
		arena->used = aligned_size;
		memcpy(arena->start, chunk, GetChunkSize(chunk));
		chunk = (MemoryChunk*)arena->start;
		chunk->prev = 0; //NOTE fresh memory is already zero
		chunk->size = arena->size | MEMORY_GENERIC_ISARENAD_FLAG;
		deshi__memory_generic_zero_free(ptr, file, line);
		DEBUG_CheckGenericHeap(deshi__generic_heap); DEBUG_PrintGenericArenaNodes(deshi__generic_heap);
		return &chunk->node;
	}
	
	spt difference = (spt)GetChunkSize(chunk) - (spt)aligned_size;
	
	//there is no used memory after this, so just adjust chunk size and heap cursor
	if(chunk == deshi__generic_heap->last_chunk){
		if(difference != 0){
			//if out of memory, default to libc
			if((deshi__generic_heap->cursor - difference) > (deshi__generic_heap->start + deshi__generic_heap->size)){
				LogfE("memory","Deshi ran out of generic memory when attempting to reallocate a ptr[0x%p \"%s\"] (triggered at %s:%lld); defaulting to libc calloc.", ptr, (deshi__memory_naming_get(ptr)) ? deshi__memory_naming_get(ptr).str : "", file, line);
				void* new_ptr = AllocateLibc(aligned_size);
				MemoryChunk* new_chunk = MemoryToChunk(new_ptr);
				memcpy(&new_chunk->node, &chunk->node, GetChunkSize(chunk) - MEMORY_GENERIC_CHUNK_INUSE_OVERHEAD);
				deshi__memory_generic_zero_free(ptr);
				return new_ptr;
			}
			
			deshi__generic_heap->cursor -= difference;
			if(difference > 0) ZeroMemory(deshi__generic_heap->cursor, difference);
			deshi__generic_heap->used   -= difference;
			chunk->size = (PrevChunkIsInUse(chunk)) ? aligned_size | MEMORY_GENERIC_PREVINUSE_FLAG : aligned_size;
		}
		DEBUG_CheckGenericHeap(deshi__generic_heap); DEBUG_PrintGenericArenaNodes(deshi__generic_heap);
		return &chunk->node;
	}
	
	//if requested size is less than previous size and there is enough space for a new chunk, make a new chunk
	if(difference >= (spt)MEMORY_GENERIC_MIN_ALLOC_SIZE){
		MemoryChunk* next = GetNextOrderChunk(chunk);
		MemoryChunk* new_chunk = GetChunkAtOffset(chunk, aligned_size);
		NodeInsertNext(&deshi__generic_heap->empty_nodes, &new_chunk->node);
		new_chunk->prev = chunk;
		new_chunk->size = (upt)difference | MEMORY_GENERIC_PREVINUSE_FLAG;
		chunk->size -= difference;
		next->prev = new_chunk;
		next->size &= ~(MEMORY_GENERIC_PREVINUSE_FLAG); //remove INUSE flag from next chunk
		deshi__generic_heap->used -= difference;
		deshi__generic_heap->used += sizeof(MemoryChunk);
		DEBUG_CheckGenericHeap(deshi__generic_heap); DEBUG_PrintGenericArenaNodes(deshi__generic_heap);
		return &chunk->node;
	}
	
	//if requested size is less than or equal to previous size and there is not enough space for a new chunk, do nothing
	if(difference >= 0){
		return ptr;
	}
	
	//requested size is greater than previous size and we need to move memory in order to fit the new size
	void* new_ptr = deshi__memory_generic_allocate(requested_size, file, line);
	Assert(GetChunkSize(MemoryToChunk(new_ptr)) >= GetChunkSize(chunk) - MEMORY_GENERIC_CHUNK_INUSE_OVERHEAD, "New chunk size must be greater than previous size");
	memcpy(new_ptr, &chunk->node, GetChunkSize(chunk) - MEMORY_GENERIC_CHUNK_INUSE_OVERHEAD);
	chunk = MemoryToChunk(new_ptr);
	deshi__memory_generic_zero_free(ptr, file, line);
	DEBUG_CheckGenericHeap(deshi__generic_heap); DEBUG_PrintGenericArenaNodes(deshi__generic_heap);
	return &chunk->node;
}

void
deshi__memory_generic_zero_free(void* ptr, char* file, upt line){
	DEBUG_CheckGenericHeap(deshi__generic_heap);
	if(ptr == 0) return;
	
#if MEMORY_DO_GENERIC_PRINTS
	Logf("memory","Zero freeing a ptr[0x%p \"%s\"] (triggered at %s:%lld)", ptr, (deshi__memory_naming_get(ptr)) ? deshi__memory_naming_get(ptr).str : "", file, line);
#endif
	
	MemoryChunk* chunk = MemoryToChunk(ptr);
	
	if(ChunkIsLibc(chunk)){
		FreeLibc(ptr);
		return;
	}
	
	Assert(ptr > deshi__arena_heap->start && ptr < deshi__arena_heap->cursor, "Attempted to free a pointer outside the main heap and missing the libc flag");
	
	if(ChunkIsArenad(chunk)){
		deshi__memory_arena_delete(&((ArenaHeapNode*)chunk - 1)->arena, file, line);
		DEBUG_CheckGenericHeap(deshi__generic_heap); DEBUG_PrintGenericArenaNodes(deshi__generic_heap);
		return;
	}
	
	Assert(ptr > deshi__generic_heap->start && ptr < deshi__generic_heap->cursor, "Attempted to free a pointer outside the generic heap");
	void* zero_pointer = chunk+1;
	upt   zero_amount  = GetChunkSize(chunk) - sizeof(MemoryChunk);
	upt   used_amount  = GetChunkSize(chunk) - sizeof(MemoryChunk);
	
	//insert current chunk into heap's empty nodes
	NodeInsertNext(&deshi__generic_heap->empty_nodes, &chunk->node);
	
	//try to merge next empty into current empty
	MemoryChunk* next = GetNextOrderChunk(chunk);
	if(   (chunk != deshi__generic_heap->last_chunk) //we are not the last chunk
	   && (next  != deshi__generic_heap->last_chunk) //next is not the last chunk
	   && (!ChunkIsInUse(next))){             //next is empty
		MemoryChunk* next_next = GetNextOrderChunk(next);
		next_next->prev = chunk;
		NodeRemove(&next->node);
		chunk->size += GetChunkSize(next);
		//NOTE zero_pointer doesnt change
		zero_amount += sizeof(MemoryChunk); //NOTE next's memory is already zeroed, so only zero the chunk header
		used_amount += sizeof(MemoryChunk);
		next = next_next;
	}
	
	//try to merge current empty into prev empty
	MemoryChunk* prev = GetPrevOrderChunk(chunk);
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
		used_amount += sizeof(MemoryChunk);
		chunk = prev;
	}
	
	//remove the last order chunk if its empty
	if(chunk == deshi__generic_heap->last_chunk){
		NodeRemove(&chunk->node);
		deshi__generic_heap->last_chunk = chunk->prev;
		deshi__generic_heap->cursor = (u8*)chunk;
		zero_pointer = chunk;
		zero_amount  = GetChunkSize(chunk);
		used_amount += sizeof(MemoryChunk);
	}else{
		Assert((u8*)next == (u8*)chunk + GetChunkSize(chunk), "Next is invalid at this point");
		next->size &= ~(MEMORY_GENERIC_PREVINUSE_FLAG); //remove INUSE flag from next chunk
	}
	
	deshi__generic_heap->used -= used_amount;
	ZeroMemory(zero_pointer, zero_amount);
	DEBUG_CheckGenericHeap(deshi__generic_heap); DEBUG_PrintGenericArenaNodes(deshi__generic_heap);
}

GenericHeap*
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
	Assert(deshi__temp_arena, "Attempted to temp allocate before Memory::Init() has been called");
	
#if MEMORY_DO_TEMP_PRINTS
	Logf("memory","Creating a temp allocation of %lld bytes (triggered at %s:%lld)", size, file, line);
#endif
	
	upt aligned_size = RoundUpTo(size + sizeof(upt), MEMORY_BYTE_ALIGNMENT);
	if(deshi__temp_arena->used + aligned_size > deshi__temp_arena->size){
		LogfE("memory","Deshi ran out of temporary memory when attempting to allocate %lld bytes (triggered at %s:%lld); defaulting to libc calloc which will not be automatically freed by clearing the temporary storage (aka: a memory leak)!", size, file, line);
		upt* size_ptr = (upt*)calloc(1, aligned_size);
		*size_ptr = aligned_size | 0x1;
		return size_ptr+1;
	}
	
	void* result = deshi__temp_arena->cursor + sizeof(upt);
	*((upt*)deshi__temp_arena->cursor) = aligned_size; //place allocation size at cursor
	deshi__temp_arena->cursor += aligned_size;
	deshi__temp_arena->used += aligned_size;
	return result;
}

void*
deshi__memory_temp_reallocate(void* ptr, upt size, char* file, upt line){
	if(size == 0) return 0;
	if(ptr == 0) return 0;
	
#if MEMORY_DO_TEMP_PRINTS
	Logf("memory","Reallocating a temp ptr[0x%p \"%s\"] to %lld bytes of (triggered at %s:%lld)", ptr, (deshi__memory_naming_get(ptr)) ? deshi__memory_naming_get(ptr).str : "", size, file, line);
#endif
	
	upt* size_ptr = (upt*)ptr - 1;
	upt  prev_size = *size_ptr; //including overhead and flags
	upt  aligned_size = RoundUpTo(size + sizeof(upt), MEMORY_BYTE_ALIGNMENT);
	
	//allocation was done with libc
	if(HasFlag(prev_size, 0x1)){
		size_ptr = (upt*)realloc(size_ptr, aligned_size);
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
#if MEMORY_DO_TEMP_PRINTS
	Logf("memory","Clearing temporary memory which used %lld bytes", deshi__temp_arena->used);
#endif
	
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
deshi__memory_naming_set(void* address, cstring name, Type type){
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
			arr[index].type = type;
		}else{ //make a new info
			if(deshi__naming_arena->used >= deshi__naming_arena->size){ 
				LogE("memory","Address naming arena is out of space."); 
				return; 
			}
			array_insert(arr, AddressNameInfo{ address, name, type }, middle);
			deshi__naming_arena->used += sizeof(AddressNameInfo);
		}
	}else if(index != -1){ //remove address name
		array_remove_ordered(arr, index);
		deshi__naming_arena->used -= sizeof(AddressNameInfo);
	}
}

cstring
deshi__memory_naming_get(void* address){
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
		if(bytes > Kilobytes(1)){
			character = 'K'; divisor = Kilobytes(1);
			if(bytes > Megabytes(1)){
				character = 'M'; divisor = Megabytes(1);
			}
		}
	};
	
	UI::PushColor(UIStyleCol_Border,    Color_Grey);
	UI::PushColor(UIStyleCol_Separator, Color_Grey);
	UI::PushVar(UIStyleVar_WindowPadding,    vec2::ZERO);
	UI::PushVar(UIStyleVar_ItemSpacing,      vec2::ZERO);
	UI::PushVar(UIStyleVar_WindowBorderSize, 0);
	{UI::Begin("deshi_memory", DeshWindow->dimensions/4.f, DeshWindow->dimensions/2.f, UIWindowFlags_NoScroll | UIWindowFlags_NoBorder);
		UIWindow* window = UI::GetWindow();
		char used_char = ' ', size_char = ' ';
		f32  used_divisor = 1.f, size_divisor = 1.f;
		
		//left panel: generic heap
		UI::SetNextWindowSize({window->width*.5f, window->height*.9f});
		{UI::BeginChild("deshi_memory_generic", vec2::ZERO, UIWindowFlags_NoScroll | UIWindowFlags_NoBorder | UIWindowFlags_NoResize);
			bytes_sigfigs(deshi__generic_heap->used, used_char, used_divisor);
			bytes_sigfigs(deshi__generic_heap->size, size_char, size_divisor);
			UI::TextF("Generic Heap    %.2f %cB / %.2f %cB", (f32)deshi__generic_heap->used / used_divisor, used_char, (f32)deshi__generic_heap->size / size_divisor, size_char);
			UI::RectFilled({0,UI::GetPositionForNextItem().y}, UI::GetWindowRemainingSpace(), Color_VeryDarkRed);
			
			f32 row_bytes = ceil(sqrtf((f32)deshi__generic_heap->size));
			
		}UI::EndChild();
		
		//right panel: arena heap
		UI::SameLine();
		UI::SetNextWindowSize({window->width*.5f, window->height*.9f});
		{UI::BeginChild("deshi_memory_arena", vec2::ZERO, UIWindowFlags_NoScroll | UIWindowFlags_NoBorder | UIWindowFlags_NoResize);
			bytes_sigfigs(deshi__arena_heap->used, used_char, used_divisor);
			bytes_sigfigs(deshi__arena_heap->size, size_char, size_divisor);
			UI::TextF("Arena Heap    %.2f %cB / %.2f %cB", (f32)deshi__arena_heap->used / used_divisor, used_char, (f32)deshi__arena_heap->size / size_divisor, size_char);
			UI::RectFilled({0,UI::GetPositionForNextItem().y}, UI::GetWindowRemainingSpace(), Color_VeryDarkGreen);
			
			
		}UI::EndChild();
		
		//bottom panel: temp arena
		UI::SetNextWindowSize({window->width, window->height*.1f});
		{UI::BeginChild("deshi_memory_temp", vec2::ZERO, UIWindowFlags_NoScroll | UIWindowFlags_NoBorder | UIWindowFlags_NoResize);
			bytes_sigfigs(deshi__temp_arena->used, used_char, used_divisor);
			bytes_sigfigs(deshi__temp_arena->size, size_char, size_divisor);
			UI::TextF("Temporary Memory    %.2f %cB / %.2f %cB", (f32)deshi__temp_arena->used / used_divisor, used_char, (f32)deshi__temp_arena->size / size_divisor, size_char);
			UI::RectFilled({0,UI::GetPositionForNextItem().y}, UI::GetWindowRemainingSpace(), Color_VeryDarkCyan);
			
		}UI::EndChild();
	}UI::End();
	UI::PopVar(3);
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
	
	deshi__arena_heap->start  = allocation;
	deshi__arena_heap->cursor = allocation;
	deshi__arena_heap->size   = main_size;
	deshi__arena_heap->used   = 0;
	deshi__arena_heap->order.next = deshi__arena_heap->order.prev = &deshi__arena_heap->order;
	deshi__arena_heap->empty.next = deshi__arena_heap->empty.prev = &deshi__arena_heap->empty;
	deshi__arena_heap->initialized = true;
	DEBUG_CheckArenaHeapNodes(deshi__arena_heap);
	
	deshi__generic_arena = memory_create_arena(Megabytes(64));
	deshi__generic_heap = (GenericHeap*)deshi__generic_arena->start;
	deshi__generic_heap->start  = (u8*)(deshi__generic_heap+1);
	deshi__generic_heap->cursor = deshi__generic_heap->start;
	deshi__generic_heap->used   = 0;
	deshi__generic_heap->size   = deshi__generic_arena->size - sizeof(GenericHeap);
	deshi__generic_heap->empty_nodes.next = deshi__generic_heap->empty_nodes.prev = &deshi__generic_heap->empty_nodes;
	deshi__generic_heap->last_chunk = 0;
	deshi__generic_heap->initialized = true;
	DEBUG_CheckGenericHeap(deshi__generic_heap);
	
	deshi__temp_arena->start  = deshi__arena_heap->start + deshi__arena_heap->size;
	deshi__temp_arena->cursor = deshi__temp_arena->start;
	deshi__temp_arena->size   = temp_size;
	deshi__temp_arena->used   = 0;
	
#if DESHI_INTERNAL
	deshi__naming_arena = memory_create_arena(MEMORY_NAMING_MAX_COUNT*sizeof(AddressNameInfo));
	memory_set_address_name(deshi__arena_heap,   cstr_lit("Arena Heap"), 0);
	memory_set_address_name(deshi__generic_heap, cstr_lit("Generic Heap"), 0);
	memory_set_address_name(deshi__temp_arena,   cstr_lit("Temp Arena"), 0);
	memory_set_address_name(deshi__naming_arena, cstr_lit("Naming Arena"), 0);
#endif //DESHI_INTERNAL
	
	deshiStage |= DS_MEMORY;
}
