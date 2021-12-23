//NOTE Memory Layout: 
//|                                Total Size                                 |
//|                            Main Heap                        |  Temp Arena |
//| Generic Arena |      Heap Arena      |      Heap Arena      | Item | Item |
//| Chunk | Chunk | Header |    Memory   | Header |    Memory   |      |      |
//|       |       |        | Item | Item |        | Item | Item |      |      |

namespace Memory{
	////////////////
	//// @utils ////
	////////////////
#define MEMORY_POINTER_SIZE sizeof(void*)
#define MEMORY_BYTE_ALIGNMENT (2*MEMORY_POINTER_SIZE)
#define MEMORY_BYTE_ALIGNMENT_MASK (MEMORY_BYTE_ALIGNMENT-1)
#define MEMORY_DO_HEAP_CHECKS true
#define MEMORY_DO_HEAP_PRINTS false
	
	FORCE_INLINE void ZeroBytes(void* ptr, upt bytes){
		memset(ptr, 0, bytes);
	}
	
	
	////////////////
	//// @arena ////
	////////////////
	local ArenaHeap arena_heap;
	
#define MEMORY_ARENA_MINIMUM_ALLOCATION_SIZE ((sizeof(ArenaHeapNode) + MEMORY_BYTE_ALIGNMENT_MASK) & ~(MEMORY_BYTE_ALIGNMENT_MASK))
#define MEMORY_ARENA_ALIGNMENT MEMORY_ARENA_MINIMUM_ALLOCATION_SIZE
	
#if !MEMORY_DO_HEAP_CHECKS
#  define DEBUG_CheckArenaHeapNodes(heap) 
#  define DEBUG_CheckArenaHeapArenas(heap)
#else //MEMORY_DO_HEAP_CHECKS
	function void DEBUG_CheckArenaHeapNodes(ArenaHeap* heap){
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
	
	function void DEBUG_CheckArenaHeapArenas(ArenaHeap* heap){
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
#endif //MEMORY_DO_HEAP_CHECKS
#if !MEMORY_DO_HEAP_PRINTS
#  define DEBUG_PrintArenaHeapNodes(heap)
#else //MEMORY_DO_HEAP_PRINTS
	function void DEBUG_PrintArenaHeapNodes(ArenaHeap* heap){
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
#endif //MEMORY_DO_HEAP_PRINTS
	
	function inline void UpdateArenaHeapCursor(ArenaHeap* heap){ //NOTE this relies on empty nodes having correct arena.start positions
		ArenaHeapNode* last_order_node = CastFromMember(ArenaHeapNode, order, arena_heap.order.prev);
		heap->cursor = last_order_node->arena.start + last_order_node->arena.size;
	}
	
	Arena* CreateArena(upt size){
		DEBUG_CheckArenaHeapNodes(&arena_heap);
		if(size == 0) return 0;
		Assert(arena_heap.start, "Attempted to create an arena before Memory::Init() has been called");
		upt aligned_size = ClampMin(RoundUpTo(size, MEMORY_ARENA_ALIGNMENT), MEMORY_ARENA_MINIMUM_ALLOCATION_SIZE);
		Assert(arena_heap.used + aligned_size <= arena_heap.size, "Attempted to use more than max arena heap size");
		
		//check if there are any empty nodes that can hold the new arena
		for(Node* n = arena_heap.empty.next; n != &arena_heap.empty; n = n->next){
			ArenaHeapNode* node = CastFromMember(ArenaHeapNode, empty, n);
			if(node->arena.size >= aligned_size){
				upt leftover_size = node->arena.size - aligned_size;
				Assert(leftover_size % sizeof(ArenaHeapNode) == 0, "Memory was not aligned correctly");
				
				//make new empty node after new order node if there is enough space
				if(leftover_size > sizeof(ArenaHeapNode)){
					ArenaHeapNode* new_node = (ArenaHeapNode*)(node->arena.start + aligned_size);
					NodeInsertNext(&node->order, &new_node->order); DEBUG_CheckArenaHeapNodes(&arena_heap);
					NodeInsertNext(&node->empty, &new_node->empty); DEBUG_CheckArenaHeapNodes(&arena_heap);
					new_node->arena.start = (u8*)(new_node+1);
					new_node->arena.size = leftover_size - sizeof(ArenaHeapNode);
					arena_heap.used += sizeof(ArenaHeapNode);
				}else{
					aligned_size += leftover_size;
				}
				
				//convert empty node to order node
				NodeRemove(&node->empty); DEBUG_CheckArenaHeapNodes(&arena_heap);
				node->empty.next = 0;
				node->empty.prev = 0;
				//node->arena.start = (u8*)(node+1); //NOTE arena starts are correct in empty nodes
				node->arena.cursor = node->arena.start;
				node->arena.size   = aligned_size;
				node->arena.used   = 0;
				arena_heap.used += aligned_size;
				UpdateArenaHeapCursor(&arena_heap); //NOTE an empty node can't be the last order node //TODO(delle) comment this out then test
				
				DEBUG_CheckArenaHeapNodes(&arena_heap); DEBUG_CheckArenaHeapArenas(&arena_heap); DEBUG_PrintArenaHeapNodes(&arena_heap);
				return &node->arena;
			}
		}
		
		//if we cant replace an empty node, make a new order node for the new arena
		Assert(arena_heap.cursor + aligned_size + sizeof(ArenaHeapNode) <= arena_heap.start + arena_heap.size, "Attempted to use more than max arena heap size");
		ArenaHeapNode* new_node = (ArenaHeapNode*)arena_heap.cursor;
		NodeInsertPrev(&arena_heap.order, &new_node->order); DEBUG_CheckArenaHeapNodes(&arena_heap);
		new_node->arena.start  = (u8*)new_node + sizeof(ArenaHeapNode);
		new_node->arena.cursor = new_node->arena.start;
		new_node->arena.size   = aligned_size;
		//new_node->arena.used   = 0; //NOTE new memory is already zero
		arena_heap.used   += aligned_size + sizeof(ArenaHeapNode);
		UpdateArenaHeapCursor(&arena_heap);
		
		DEBUG_CheckArenaHeapNodes(&arena_heap); DEBUG_CheckArenaHeapArenas(&arena_heap); DEBUG_PrintArenaHeapNodes(&arena_heap); 
		return &new_node->arena;
	}
	
	Arena* GrowArena(Arena* arena, upt size){
		DEBUG_CheckArenaHeapNodes(&arena_heap); DEBUG_CheckArenaHeapArenas(&arena_heap);
		if(size == 0) return arena;
		if(arena == 0) return 0;
		Assert((u8*)arena >= arena_heap.start && (u8*)arena < arena_heap.cursor, "Attempted to grow an arena that's outside the arena heap");
		Assert(arena_heap.used + size <= arena_heap.size, "Attempted to use more than max arena heap size");
		
		//check if the next node is empty and can hold the grown size, or if we need to make a new arena
		Arena* result = arena;
		upt aligned_size = RoundUpTo(size, MEMORY_ARENA_ALIGNMENT);
		ArenaHeapNode* node = CastFromMember(ArenaHeapNode, arena, arena);
		ArenaHeapNode* next = CastFromMember(ArenaHeapNode, order, node->order.next);
		if(&next->order == &arena_heap.order){ //we are the last node
			Assert(arena_heap.cursor + size <= arena_heap.start + arena_heap.size, "Attempted to use more than max arena heap size");
			upt growth_size = (arena_heap.cursor + aligned_size <= arena_heap.start + arena_heap.size) ? aligned_size : size;
			
			arena->size += growth_size;
			arena_heap.used += growth_size;
			UpdateArenaHeapCursor(&arena_heap);
		}else if((next->empty.next != 0) && (next->empty.prev != 0) && (next->arena.size >= size)){ //next node is empty and can hold the growth
			upt growth_size = ((u8*)next + aligned_size <= arena_heap.start + arena_heap.size) ? aligned_size : size;
			
			//make new empty+order node after current node if there is enough space
			upt leftover_size = next->arena.size - growth_size + sizeof(ArenaHeapNode);
			if(leftover_size > sizeof(ArenaHeapNode)){
				ArenaHeapNode* new_node = (ArenaHeapNode*)((u8*)next + growth_size);
				NodeInsertNext(&next->order, &new_node->order); DEBUG_CheckArenaHeapNodes(&arena_heap);
				NodeInsertNext(&next->empty, &new_node->empty); DEBUG_CheckArenaHeapNodes(&arena_heap);
				new_node->arena.start = (u8*)(new_node+1);
				new_node->arena.size = leftover_size - sizeof(ArenaHeapNode);
				arena_heap.used += sizeof(ArenaHeapNode);
			}else{
				growth_size += leftover_size;
			}
			
			//add empty node space to current node
			NodeRemove(&next->order); DEBUG_CheckArenaHeapNodes(&arena_heap);
			NodeRemove(&next->empty); DEBUG_CheckArenaHeapNodes(&arena_heap);
			ZeroBytes(next, sizeof(ArenaHeapNode)); //NOTE the node's memory should already be zeroed
			arena->size += growth_size;
			arena_heap.used += (growth_size - sizeof(ArenaHeapNode));
			UpdateArenaHeapCursor(&arena_heap);
		}else{ //need to move memory in order to fit new size
			upt growth_size = (arena_heap.cursor + arena->size + aligned_size <= arena_heap.start + arena_heap.size) ? aligned_size : size;
			Arena* new_arena = CreateArena(arena->size + growth_size);
			memcpy(new_arena->start, arena->start, arena->used);
			new_arena->used = arena->used;
			new_arena->cursor = new_arena->start + (arena->cursor - arena->start);
			result = new_arena;
			DeleteArena(arena);
		}
		
		DEBUG_CheckArenaHeapNodes(&arena_heap); DEBUG_CheckArenaHeapArenas(&arena_heap); DEBUG_PrintArenaHeapNodes(&arena_heap); 
		return result;
	}
	
	void ClearArena(Arena* arena){
		ZeroBytes(arena->start, arena->used);
		arena->cursor = arena->start;
		arena->used = 0;
	}
	
	void DeleteArena(Arena* arena){
		DEBUG_CheckArenaHeapNodes(&arena_heap); DEBUG_CheckArenaHeapArenas(&arena_heap);
		if(arena == 0) return;
		Assert((u8*)arena >= arena_heap.start && (u8*)arena < arena_heap.cursor, "Attempted to delete an arena outside the main heap");
		
		ArenaHeapNode* node = CastFromMember(ArenaHeapNode, arena, arena);
		void* zero_pointer = arena->start;
		upt   zero_amount  = arena->size;
		upt   used_amount  = arena->size;
		
		//insert current node into empty
		NodeInsertNext(&arena_heap.empty, &node->empty); DEBUG_CheckArenaHeapNodes(&arena_heap);
		
		//try to merge next empty into current empty
		ArenaHeapNode* next = CastFromMember(ArenaHeapNode, order, node->order.next);
		if(   (&node->order != &arena_heap.order)
		   && (&next->order != &arena_heap.order)
		   && (node->empty.next != 0) && (node->empty.prev != 0)
		   && (next->empty.next != 0) && (next->empty.prev != 0)
		   && (PointerDifference(node->arena.start + node->arena.size, next) == 0)){
			NodeRemove(&next->order); DEBUG_CheckArenaHeapNodes(&arena_heap);
			NodeRemove(&next->empty); DEBUG_CheckArenaHeapNodes(&arena_heap);
			NodeRemove(&node->empty); DEBUG_CheckArenaHeapNodes(&arena_heap); //NOTE remove and reinsert as first empty node for locality
			//UpdateArenaHeapCursor(&arena_heap); //NOTE cursor should not change as next is empty and cant be last
			node->arena.size += next->arena.size + sizeof(ArenaHeapNode);
			NodeInsertNext(&arena_heap.empty, &node->empty); DEBUG_CheckArenaHeapNodes(&arena_heap);
			zero_pointer = node+1;
			zero_amount  = node->arena.size;
			used_amount += sizeof(ArenaHeapNode);
		}
		
		//try to merge current empty into prev empty
		ArenaHeapNode* prev = CastFromMember(ArenaHeapNode, order, node->order.prev);
		if(   (&prev->order != &arena_heap.order)
		   && (&node->order != &arena_heap.order)
		   && (prev->empty.next != 0) && (prev->empty.prev != 0)
		   && (node->empty.next != 0) && (node->empty.prev != 0)
		   && (PointerDifference(prev->arena.start + prev->arena.size, node) == 0)){
			NodeRemove(&node->order); DEBUG_CheckArenaHeapNodes(&arena_heap);
			NodeRemove(&node->empty); DEBUG_CheckArenaHeapNodes(&arena_heap);
			NodeRemove(&prev->empty); DEBUG_CheckArenaHeapNodes(&arena_heap);
			UpdateArenaHeapCursor(&arena_heap);
			prev->arena.size += node->arena.size + sizeof(ArenaHeapNode);
			NodeInsertNext(&arena_heap.empty, &prev->empty); DEBUG_CheckArenaHeapNodes(&arena_heap);
			zero_pointer = prev+1;
			zero_amount  = prev->arena.size;
			used_amount += sizeof(ArenaHeapNode);
			node = prev;
		}
		
		//remove the last order node if its empty
		if(node->order.next == &arena_heap.order){
			NodeRemove(&node->order); DEBUG_CheckArenaHeapNodes(&arena_heap);
			NodeRemove(&node->empty); DEBUG_CheckArenaHeapNodes(&arena_heap);
			arena_heap.cursor = (u8*)node;
			zero_pointer = node; 
			zero_amount  = node->arena.size + sizeof(ArenaHeapNode);
			used_amount += sizeof(ArenaHeapNode);
		}
		
		arena_heap.used -= used_amount;
		ZeroBytes(zero_pointer, zero_amount);
		DEBUG_CheckArenaHeapNodes(&arena_heap); DEBUG_CheckArenaHeapArenas(&arena_heap); DEBUG_PrintArenaHeapNodes(&arena_heap);
	}
	
	
	//////////////////
	//// @generic ////
	//////////////////
	local Arena*       _generic_arena; //generic_heap is stored here; not used otherwise
	local GenericHeap* generic_heap;
	
#define MEMORY_GENERIC_CHUNK_MEMORY_OFFSET ((upt)OffsetOfMember(Chunk, node))
#define MEMORY_GENERIC_CHUNK_INUSE_OVERHEAD MEMORY_GENERIC_CHUNK_MEMORY_OFFSET
#define MEMORY_GENERIC_MINIMUM_ALLOCATION_SIZE ((sizeof(Chunk) + MEMORY_BYTE_ALIGNMENT_MASK) & ~(MEMORY_BYTE_ALIGNMENT_MASK))
#define MEMORY_GENERIC_MAXIMUM_ALLOCATION_SIZE Kilobytes(64)
	
#define ChunkToMemory(chunk)\
((void*)((u8*)(chunk) + MEMORY_GENERIC_CHUNK_MEMORY_OFFSET))
#define MemoryToChunk(memory)\
((Chunk*)((u8*)(memory) - MEMORY_GENERIC_CHUNK_MEMORY_OFFSET))
	
	//NOTE Chunk Flags !ref: https://github.com/lattera/glibc/blob/895ef79e04a953cac1493863bcae29ad85657ee1/malloc/malloc.c#L1224
	//  Chunk flags are stored in the lower bits of the chunk's size variable, and this doesn't cause problems b/c the size 
	//  is always greater than 8 bytes on 32bit and 16 bytes on on 64bit (MEMORY_GENERIC_MINIMUM_ALLOCATION_SIZE).
	//  To get the size, we mask off the bits holding these flags.
	//  ISARENAD and PREVINUSE should never be used together
	//
	//  PREVINUSE (0x1): when previous adjacent chunk is in use
	//  ISARENAD  (0x2): when the chunk was large enough to use an Arena rather than bins
#define MEMORY_GENERIC_PREVINUSE_FLAG 0x1
#define MEMORY_GENERIC_ISARENAD_FLAG 0x2
#define MEMORY_GENERIC_CHUNK_SIZE_BITS (MEMORY_GENERIC_PREVINUSE_FLAG | MEMORY_GENERIC_ISARENAD_FLAG)
#define MEMORY_GENERIC_EXTRACT_SIZE_BITMASK (~MEMORY_GENERIC_CHUNK_SIZE_BITS)
	
#define PrevChunkIsInUse(chunk_ptr)\
((chunk_ptr)->size & MEMORY_GENERIC_PREVINUSE_FLAG)
#define ChunkIsArenad(chunk_ptr)\
((chunk_ptr)->size & MEMORY_GENERIC_ISARENAD_FLAG)
#define GetChunkSize(chunk_ptr)\
((chunk_ptr)->size & MEMORY_GENERIC_EXTRACT_SIZE_BITMASK)
#define GetNextOrderChunk(chunk_ptr)\
((Chunk*)((u8*)(chunk_ptr) + GetChunkSize(chunk_ptr)))
#define GetPrevOrderChunk(chunk_ptr)\
((chunk_ptr)->prev)
#define GetChunkAtOffset(chunk_ptr,offset)\
((Chunk*)((u8*)(chunk_ptr) + (offset)))
#define ChunkIsInUse(chunk_ptr)\
(((Chunk*)((u8*)(chunk_ptr) + GetChunkSize(chunk_ptr)))->size & MEMORY_GENERIC_PREVINUSE_FLAG)
	
	
#if !MEMORY_DO_HEAP_CHECKS
#  define DEBUG_CheckGenericHeap(heap)
#else //MEMORY_DO_HEAP_CHECKS
	function void DEBUG_CheckGenericHeap(GenericHeap* heap){
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
			for(Chunk* chunk = (Chunk*)heap->start; ; chunk = GetNextOrderChunk(chunk)){
				Assert((u8*)chunk < heap->cursor, "All chunks must be below the cursor");
				Assert(GetChunkSize(chunk) >= MEMORY_GENERIC_MINIMUM_ALLOCATION_SIZE, "Chunk size is less than minimum");
				Assert(GetChunkSize(chunk) % MEMORY_BYTE_ALIGNMENT == 0, "Chunk size is not aligned correctly");
				if(chunk != heap->last_chunk){
					Assert(GetNextOrderChunk(chunk)->prev == chunk, "Next order chunk is not correctly pointing to current chunk");
					if(!PrevChunkIsInUse(GetNextOrderChunk(chunk))){
						overall_used += sizeof(Chunk);
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
#endif //MEMORY_DO_HEAP_CHECKS
#if !MEMORY_DO_HEAP_PRINTS
#  define DEBUG_PrintGenericArenaNodes(heap)
#else //MEMORY_DO_HEAP_PRINTS
	function void DEBUG_PrintGenericArenaNodes(GenericHeap* heap){
		if(heap->initialized && heap->used > 0){
			string heap_order = "Order: ", heap_empty = "Empty: ";
			for(Chunk* chunk = (Chunk*)heap->start; ; chunk = GetNextOrderChunk(chunk)){
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
#endif //MEMORY_DO_HEAP_PRINTS
	
	void* Allocate(upt requested_size){
		DEBUG_CheckGenericHeap(generic_heap);
		if(requested_size == 0) return 0;
		Assert(generic_heap, "Attempted to allocate before Memory::Init() has been called");
		
		//include chunk overhead, align to the byte alignment, and clamp the minimum
		upt aligned_size = ClampMin(RoundUpTo(requested_size + MEMORY_GENERIC_CHUNK_INUSE_OVERHEAD, MEMORY_BYTE_ALIGNMENT), MEMORY_GENERIC_MINIMUM_ALLOCATION_SIZE);
		
		//if the allocation is large, create an arena for it rather than using the generic heap
		if(aligned_size >= MEMORY_GENERIC_MAXIMUM_ALLOCATION_SIZE){
			Arena* arena = CreateArena(aligned_size);
			arena->used = aligned_size;
			Assert(arena->size >= aligned_size, "Arena size must be greater than requested size");
			Assert(arena->start == (u8*)(CastFromMember(ArenaHeapNode, arena, arena) + 1), "Arena start must be right after the ArenaHeapNode");
			Chunk* chunk = (Chunk*)arena->start;
			//chunk->prev = 0; //NOTE fresh memory is already zero
			chunk->size = arena->size | MEMORY_GENERIC_ISARENAD_FLAG;
			return ChunkToMemory(chunk);
		}
		
		//check if there are any empty chunks that can hold the allocation
		for(Node* node = generic_heap->empty_nodes.next; node != &generic_heap->empty_nodes; node = node->next){
			Chunk* chunk = CastFromMember(Chunk, node, node);
			upt chunk_size = GetChunkSize(chunk); //NOTE remember that chunk size includes the overhead
			if(chunk_size >= aligned_size){
				upt leftover_size = chunk_size - aligned_size;
				Assert(leftover_size % MEMORY_BYTE_ALIGNMENT == 0, "Memory was not aligned correctly");
				Chunk* next = GetNextOrderChunk(chunk);
				
				//make new empty chunk after current chunk if there is enough space for an empty chunk
				//NOTE '>=' rather than '>' because empty chunks use 8/16 bytes (Node) that in-use chunks dont (useful for small allocations)
				if(leftover_size >= MEMORY_GENERIC_MINIMUM_ALLOCATION_SIZE){
					Chunk* new_chunk = GetChunkAtOffset(chunk, aligned_size);
					NodeInsertNext(&generic_heap->empty_nodes, &new_chunk->node);
					new_chunk->prev = chunk;
					new_chunk->size = leftover_size; //NOTE this gets OR'd with MEMORY_GENERIC_PREVINUSE_FLAG below
					next->prev = new_chunk;
					next = new_chunk;
					generic_heap->used += sizeof(Chunk);
				}else{
					aligned_size += leftover_size;
				}
				
				//convert empty node to order node  //NOTE chunk->prev doesnt need to change
				NodeRemove(&chunk->node);
				chunk->node = {0}; //zero this data since it will be used by the allocation
				chunk->size = (PrevChunkIsInUse(chunk)) ? aligned_size | MEMORY_GENERIC_PREVINUSE_FLAG : aligned_size;
				next->size |= MEMORY_GENERIC_PREVINUSE_FLAG; //set PREVINUSE flag on next chunk
				generic_heap->used += aligned_size - sizeof(Chunk);
				
				DEBUG_CheckGenericHeap(generic_heap); DEBUG_PrintGenericArenaNodes(generic_heap);
				return &chunk->node;
			}
		}
		
		//if we cant replace an empty node, make a new order node for the allocation
		Assert(generic_heap->cursor + aligned_size <= generic_heap->start + generic_heap->size, "Attempted to use more than max generic heap size");
		Chunk* new_chunk = (Chunk*)generic_heap->cursor;
		new_chunk->prev = generic_heap->last_chunk;
		new_chunk->size = (generic_heap->last_chunk != 0) ? aligned_size | MEMORY_GENERIC_PREVINUSE_FLAG : aligned_size;
		generic_heap->cursor    += aligned_size;
		generic_heap->used      += aligned_size;
		generic_heap->last_chunk = new_chunk;
		
		DEBUG_CheckGenericHeap(generic_heap); DEBUG_PrintGenericArenaNodes(generic_heap);
		return &new_chunk->node;
	}
	
	void* Reallocate(void* ptr, upt requested_size){
		DEBUG_CheckGenericHeap(generic_heap); 
		if(ptr == 0) return Allocate(requested_size);
		if(requested_size == 0){ ZeroFree(ptr); return 0; }
		Assert(generic_heap, "Attempted to allocate before Memory::Init() has been called");
		Assert(ptr > arena_heap.start && ptr < arena_heap.cursor, "Attempted to reallocate a pointer outside the main heap");
		
		//include chunk overhead, align to the byte alignment, and clamp the minimum
		upt aligned_size = ClampMin(RoundUpTo(requested_size + MEMORY_GENERIC_CHUNK_INUSE_OVERHEAD, MEMORY_BYTE_ALIGNMENT), MEMORY_GENERIC_MINIMUM_ALLOCATION_SIZE);
		Chunk* chunk = MemoryToChunk(ptr);
		spt difference = (spt)GetChunkSize(chunk) - (spt)aligned_size;
		
		//previous allocation was an arena
		if(ChunkIsArenad(chunk)){ 
			if(aligned_size <= GetChunkSize(chunk)) return ptr; //do nothing if less than previous size
			Arena* arena = &((ArenaHeapNode*)chunk - 1)->arena;
			arena = GrowArena(arena, aligned_size - arena->size);
			Assert(arena->size >= aligned_size, "Arena size must be greater than requested size");
			Assert(arena->start == (u8*)(CastFromMember(ArenaHeapNode, arena, arena) + 1), "Arena start must be right after the ArenaHeapNode");
			arena->used = aligned_size;
			chunk = (Chunk*)arena->start;
			//chunk->prev = 0; //NOTE it should already be zero
			chunk->size = arena->size | MEMORY_GENERIC_ISARENAD_FLAG;
			DEBUG_CheckGenericHeap(generic_heap); DEBUG_PrintGenericArenaNodes(generic_heap);
			return &chunk->node;
		}
		Assert(ptr > generic_heap->start && ptr < generic_heap->cursor, "Attempted to reallocate a pointer outside the generic heap");
		
		//new allocation needs to be an arena
		if(aligned_size >= MEMORY_GENERIC_MAXIMUM_ALLOCATION_SIZE){
			//NOTE since its larger than MEMORY_GENERIC_MAXIMUM_ALLOCATION_SIZE and not an arena already, it cant be less than previous size
			Arena* arena = CreateArena(aligned_size);
			Assert(arena->size >= aligned_size, "Arena size must be greater than requested size");
			Assert(arena->start == (u8*)(CastFromMember(ArenaHeapNode, arena, arena) + 1), "Arena start must be right after the ArenaHeapNode");
			arena->used = aligned_size;
			memcpy(arena->start, chunk, GetChunkSize(chunk));
			chunk = (Chunk*)arena->start;
			chunk->prev = 0; //NOTE fresh memory is already zero
			chunk->size = arena->size | MEMORY_GENERIC_ISARENAD_FLAG;
			ZeroFree(ptr);
			DEBUG_CheckGenericHeap(generic_heap); DEBUG_PrintGenericArenaNodes(generic_heap);
			return &chunk->node;
		}
		
		//there is no used memory after this, so just adjust chunk size and heap cursor
		if(chunk == generic_heap->last_chunk){
			if(difference != 0){
				generic_heap->cursor -= difference;
				if(difference > 0) ZeroBytes(generic_heap->cursor, difference);
				generic_heap->used   -= difference;
				chunk->size = (PrevChunkIsInUse(chunk)) ? aligned_size | MEMORY_GENERIC_PREVINUSE_FLAG : aligned_size;
			}
			DEBUG_CheckGenericHeap(generic_heap); DEBUG_PrintGenericArenaNodes(generic_heap);
			return &chunk->node;
		}
		
		//if requested size is less than previous size and there is enough space for a new chunk, make a new chunk
		if(difference >= (spt)MEMORY_GENERIC_MINIMUM_ALLOCATION_SIZE){
			Chunk* next = GetNextOrderChunk(chunk);
			Chunk* new_chunk = GetChunkAtOffset(chunk, aligned_size);
			NodeInsertNext(&generic_heap->empty_nodes, &new_chunk->node);
			new_chunk->prev = chunk;
			new_chunk->size = (upt)difference | MEMORY_GENERIC_PREVINUSE_FLAG;
			chunk->size -= difference;
			next->prev = new_chunk;
			next->size &= ~(MEMORY_GENERIC_PREVINUSE_FLAG); //remove INUSE flag from next chunk
			generic_heap->used -= difference;
			generic_heap->used += sizeof(Chunk);
			DEBUG_CheckGenericHeap(generic_heap); DEBUG_PrintGenericArenaNodes(generic_heap);
			return &chunk->node;
		}
		
		//if requested size is less than or equal to previous size and there is not enough space for a new chunk, do nothing
		if(difference >= 0){
			Assert(ptr > generic_heap->start && ptr < generic_heap->cursor, "Attempted to reallocate a pointer outside the generic heap");
			return ptr;
		}
		
		//requested size is greater than previous size and we need to move memory in order to fit the new size
		Assert(ptr > generic_heap->start && ptr < generic_heap->cursor, "Attempted to reallocate a pointer outside the generic heap");
		void* new_ptr = Allocate(requested_size);
		Assert(GetChunkSize(MemoryToChunk(new_ptr)) >= GetChunkSize(chunk) - MEMORY_GENERIC_CHUNK_INUSE_OVERHEAD, "New chunk size must be greater than previous size");
		memcpy(new_ptr, &chunk->node, GetChunkSize(chunk) - MEMORY_GENERIC_CHUNK_INUSE_OVERHEAD);
		chunk = MemoryToChunk(new_ptr);
		ZeroFree(ptr);
		DEBUG_CheckGenericHeap(generic_heap); DEBUG_PrintGenericArenaNodes(generic_heap);
		return &chunk->node;
	}
	
	void ZeroFree(void* ptr){
		DEBUG_CheckGenericHeap(generic_heap);
		if(ptr == 0) return;
		Assert(ptr > arena_heap.start && ptr < arena_heap.cursor, "Attempted to free a pointer outside the main heap");
		
		Chunk* chunk = MemoryToChunk(ptr);
		
		if(ChunkIsArenad(chunk)){
			DeleteArena(&((ArenaHeapNode*)chunk - 1)->arena);
			DEBUG_CheckGenericHeap(generic_heap); DEBUG_PrintGenericArenaNodes(generic_heap);
			return;
		}
		
		Assert(ptr > generic_heap->start && ptr < generic_heap->cursor, "Attempted to free a pointer outside the generic heap");
		void* zero_pointer = chunk+1;
		upt   zero_amount  = GetChunkSize(chunk) - sizeof(Chunk);
		upt   used_amount  = GetChunkSize(chunk) - sizeof(Chunk);
		
		//insert current chunk into heap's empty nodes
		NodeInsertNext(&generic_heap->empty_nodes, &chunk->node);
		
		//try to merge next empty into current empty
		Chunk* next = GetNextOrderChunk(chunk);
		if(   (chunk != generic_heap->last_chunk) //we are not the last chunk
		   && (next  != generic_heap->last_chunk) //next is not the last chunk
		   && (!ChunkIsInUse(next))){             //next is empty
			Chunk* next_next = GetNextOrderChunk(next);
			next_next->prev = chunk;
			NodeRemove(&next->node);
			chunk->size += GetChunkSize(next);
			//NOTE zero_pointer doesnt change
			zero_amount += sizeof(Chunk); //NOTE next's memory is already zeroed, so only zero the chunk header
			used_amount += sizeof(Chunk);
			next = next_next;
		}
		
		//try to merge current empty into prev empty
		Chunk* prev = GetPrevOrderChunk(chunk);
		if(   (prev != 0)                  //we are not the first chunk
		   && (!PrevChunkIsInUse(chunk))){ //previous chunk is empty
			if(chunk == generic_heap->last_chunk){
				generic_heap->last_chunk = prev;
			}else{
				next->prev = prev;
			}
			NodeRemove(&chunk->node);
			NodeRemove(&prev->node); //NOTE remove and reinsert as first empty node for locality
			NodeInsertNext(&generic_heap->empty_nodes, &prev->node);
			prev->size += chunk->size; //NOTE not GetChunkSize(chunk) b/c we know that PREVINUSE flag is not there
			zero_pointer = chunk; //NOTE prev's memory is already zeroed, so only zero chunk
			zero_amount  = GetChunkSize(chunk);
			used_amount += sizeof(Chunk);
			chunk = prev;
		}
		
		//remove the last order chunk if its empty
		if(chunk == generic_heap->last_chunk){
			NodeRemove(&chunk->node);
			generic_heap->last_chunk = chunk->prev;
			generic_heap->cursor = (u8*)chunk;
			zero_pointer = chunk;
			zero_amount  = GetChunkSize(chunk);
			used_amount += sizeof(Chunk);
		}else{
			Assert((u8*)next == (u8*)chunk + GetChunkSize(chunk), "Next is invalid at this point");
			next->size &= ~(MEMORY_GENERIC_PREVINUSE_FLAG); //remove INUSE flag from next chunk
		}
		
		generic_heap->used -= used_amount;
		ZeroBytes(zero_pointer, zero_amount);
		DEBUG_CheckGenericHeap(generic_heap); DEBUG_PrintGenericArenaNodes(generic_heap);
	}
	
	
	////////////////////
	//// @temporary ////
	////////////////////
	local Arena temp_arena;
	
	void* TempAllocate(upt size){
		if(size == 0) return 0;
		Assert(temp_arena.used + size <= temp_arena.size, "Attempted to use more than max temp arena size");
		
		void* result = temp_arena.cursor + sizeof(upt);
		*((upt*)temp_arena.cursor) = size; //place allocation size at cursor
		temp_arena.cursor += size + sizeof(upt);
		temp_arena.used += size + sizeof(upt);
		return result;
	}
	
	
	/////////////////
	//// @expose ////
	/////////////////
	ArenaHeap* ExposeArenaHeap(){
		return &arena_heap;
	}
	
	GenericHeap* ExposeGenericHeap(){
		return generic_heap;
	}
	
	Arena* ExposeTempArena(){
		return &temp_arena;
	}
	
	
	///////////////
	//// @init ////
	///////////////
	void Init(upt main_size, upt temp_size){
		deshiStage |= DS_MEMORY;
		
		void* base_address = 0;
		u8*   allocation = 0;
		u64   total_size = main_size + temp_size;
		
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
		Assert(allocation != 0, "Failed to allocate memory");
		
		arena_heap.start  = allocation;
		arena_heap.cursor = allocation;
		arena_heap.size   = main_size;
		arena_heap.used   = 0;
		arena_heap.order.next = arena_heap.order.prev = &arena_heap.order;
		arena_heap.empty.next = arena_heap.empty.prev = &arena_heap.empty;
		arena_heap.initialized = true;
		DEBUG_CheckArenaHeapNodes(&arena_heap);
		
		_generic_arena = CreateArena(Megabytes(64));
		generic_heap = (GenericHeap*)_generic_arena->start;
		generic_heap->start  = (u8*)(generic_heap+1);
		generic_heap->cursor = generic_heap->start;
		generic_heap->used   = 0;
		generic_heap->size   = _generic_arena->size - sizeof(GenericHeap);
		generic_heap->empty_nodes.next = generic_heap->empty_nodes.prev = &generic_heap->empty_nodes;
		generic_heap->last_chunk = 0;
		generic_heap->initialized = true;
		DEBUG_CheckGenericHeap(generic_heap);
		
		temp_arena.start  = arena_heap.start + arena_heap.size;
		temp_arena.cursor = temp_arena.start;
		temp_arena.size   = temp_size;
		temp_arena.used   = 0;
	}
	
	
	/////////////////
	//// @udpate ////
	/////////////////
	void Update(){
		ZeroBytes(temp_arena.start, temp_arena.used);
		temp_arena.cursor = temp_arena.start;
		temp_arena.used = 0;
	}
}; //namespace Memory

void* TempAllocator_Resize(void* ptr, upt size){
	void* a = Memory::TempAllocate(size);
	memcpy(a, ptr, *((upt*)ptr-1)); 
	return a;
}