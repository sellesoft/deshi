//NOTE Memory Layout: 
//|                       Total Size                          |
//|                  Main Heap                  |  Temp Arena |
//|      Heap Arena      |      Heap Arena      | Item | Item |
//| Header |    Memory   | Header |    Memory   |      |      |
//|        | Item | Item |        | Item | Item |      |      |

namespace Memory{
	////////////////
	//// @utils ////
	////////////////
#define MEMORY_POINTER_SIZE sizeof(void*)
#define MEMORY_BYTE_ALIGNMENT (2*MEMORY_POINTER_SIZE)
#define MEMORY_DO_HEAP_CHECKS false
	
	
	FORCE_INLINE void ZeroBytes(void* ptr, upt bytes){
		memset(ptr, 0, bytes);
	}
	
	////////////////
	//// @arena ////
	////////////////
#define MEMORY_ARENA_MIN_SIZE Kilobytes(4)
	
	
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
		b32 initialized;
	};
	
	
	local ArenaHeap arena_heap;
	
	
#if !MEMORY_DO_HEAP_CHECKS
#  define DEBUG_CheckHeapArenaNodes(heap) 
#  define DEBUG_CheckArenaHeapArenas(heap)
#  define DEBUG_PrintHeapArenaNodes(heap)
#else //MEMORY_DO_HEAP_CHECKS
	function void DEBUG_CheckHeapArenaNodes(ArenaHeap* heap){
		Assert(heap->order.next != 0 && heap->order.prev != 0, "First heap order node is invalid");
		Assert(heap->empty.next != 0 && heap->empty.prev != 0, "First heap empty node is invalid");
		for(Node* node = &heap->order; ; ){
			Assert(node->next->prev == node && node->prev->next == node, "ArenaHeap order node is invalid");
			node = node->next;
			if(node == &heap->order) break;
		}
		for(Node* node = &heap->empty; ; ){
			Assert(node->next->prev == node && node->prev->next == node, "ArenaHeap empty node is invalid");
			node = node->next;
			if(node == &heap->empty) break;
		}
	}
	
	function void DEBUG_CheckArenaHeapArenas(ArenaHeap* heap){
		upt overall_used = 0;
		for(Node* order = &heap->order; ; ){
			if(order != &heap->order){
				if(order->next != &heap->order){
					ArenaHeapNode* node = CastFromMember(ArenaHeapNode, order, order);
					overall_used += node->arena.size + sizeof(ArenaHeapNode);
					ArenaHeapNode* next = CastFromMember(ArenaHeapNode, order, order->next);
					Assert(node->arena.start + node->arena.size == (u8*)next, "Heap node arena is not sized correctly");
				}else{
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
	
	function void DEBUG_PrintHeapArenaNodes(ArenaHeap* heap){
		if(heap->initialized) return;
		string heap_order = "Order: ", heap_empty = "Empty: ";
		if(heap->used > 0){
			for(ArenaHeapNode* node = (ArenaHeapNode*)heap->start; ;){
				heap_order += to_string("%p", node); heap_order += " -> ";
				heap_empty += (node->empty.next) ? to_string("%p", node) : "                ";
				heap_empty += " -> ";
				
				if(node->order.next == &heap->order) break;
				node = CastFromMember(ArenaHeapNode, order, node->order.next);
			}
		}
		Log("memory",heap_order); Log("memory",heap_empty); Log("","----------------------------------------------");
	}
#endif //MEMORY_DO_HEAP_CHECKS
	
	function inline void UpdateArenaHeapCursor(ArenaHeap* heap){ //NOTE this relies on empty nodes having correct arena.start positions
		ArenaHeapNode* last_order_node = CastFromMember(ArenaHeapNode, order, arena_heap.order.prev);
		heap->cursor = last_order_node->arena.start + last_order_node->arena.size;
	}
	
	Arena* CreateArena(upt bytes){
		DEBUG_CheckHeapArenaNodes(&arena_heap);
		if(bytes == 0) return 0;
		Assert(arena_heap.start, "Attempted to create an arena before Memory::Init() has been called");
		upt aligned_size = ClampMin(RoundUpTo(bytes, sizeof(ArenaHeapNode)), MEMORY_ARENA_MIN_SIZE);
		Assert(arena_heap.used + aligned_size <= arena_heap.size, "Attempted to use more than max arena heap size");
		
		//check if there are any empty nodes that can hold the new arena
		if(arena_heap.empty.next != &arena_heap.empty){
			for(Node* n = arena_heap.empty.next; n != &arena_heap.empty; n = n->next){
				ArenaHeapNode* node = CastFromMember(ArenaHeapNode, empty, n);
				if(node->arena.size >= aligned_size){
					upt leftover_size = node->arena.size - aligned_size;
					Assert(leftover_size % sizeof(ArenaHeapNode) == 0, "Memory was not aligned correctly");
					
					//make new empty node after new order node if there is enough space
					if(leftover_size > sizeof(ArenaHeapNode)){
						ArenaHeapNode* new_node = (ArenaHeapNode*)(node->arena.start + aligned_size);
						NodeInsertNext(&node->order, &new_node->order); DEBUG_CheckHeapArenaNodes(&arena_heap);
						NodeInsertNext(&node->empty, &new_node->empty); DEBUG_CheckHeapArenaNodes(&arena_heap);
						new_node->arena.start = (u8*)(new_node+1);
						new_node->arena.size = leftover_size - sizeof(ArenaHeapNode);
						arena_heap.used += sizeof(ArenaHeapNode);
					}else{
						aligned_size += leftover_size;
					}
					
					//convert empty node to order node
					NodeRemove(&node->empty); DEBUG_CheckHeapArenaNodes(&arena_heap);
					node->empty.next = 0;
					node->empty.prev = 0;
					//node->arena.start = (u8*)(node+1); //NOTE arena starts are correct in empty nodes
					node->arena.cursor = node->arena.start;
					node->arena.size   = aligned_size;
					node->arena.used   = 0;
					arena_heap.used += aligned_size;
					UpdateArenaHeapCursor(&arena_heap);
					
					DEBUG_CheckHeapArenaNodes(&arena_heap); DEBUG_CheckArenaHeapArenas(&arena_heap); DEBUG_PrintHeapArenaNodes(&arena_heap);
					return &node->arena;
				}
			}
		}
		
		//if we cant replace an empty node, make a new order node for the new arena
		Assert(arena_heap.cursor + aligned_size + sizeof(ArenaHeapNode) <= arena_heap.start + arena_heap.size, "Attempted to use more than max arena heap size");
		ArenaHeapNode* new_node = (ArenaHeapNode*)arena_heap.cursor;
		NodeInsertPrev(&arena_heap.order, &new_node->order); DEBUG_CheckHeapArenaNodes(&arena_heap);
		new_node->arena.start  = (u8*)new_node + sizeof(ArenaHeapNode);
		new_node->arena.cursor = new_node->arena.start;
		new_node->arena.size   = aligned_size;
		//new_node->arena.used   = 0; //NOTE new memory is already zero
		arena_heap.used   += aligned_size + sizeof(ArenaHeapNode);
		UpdateArenaHeapCursor(&arena_heap);
		
		DEBUG_CheckHeapArenaNodes(&arena_heap); DEBUG_CheckArenaHeapArenas(&arena_heap); DEBUG_PrintHeapArenaNodes(&arena_heap); 
		return &new_node->arena;
	}
	
	Arena* GrowArena(Arena* arena, upt bytes){
		DEBUG_CheckHeapArenaNodes(&arena_heap); DEBUG_CheckArenaHeapArenas(&arena_heap);
		if(bytes == 0) return arena;
		if(arena == 0) return 0;
		Assert((u8*)arena >= arena_heap.start && (u8*)arena < arena_heap.cursor, "Attempted to grow an arena that's outside the arena heap");
		Assert(arena_heap.used + bytes <= arena_heap.size, "Attempted to use more than max arena heap size");
		
		//check if the next node is empty and can hold the grown size, or if we need to make a new arena
		Arena* result = arena;
		upt aligned_bytes = RoundUpTo(bytes, sizeof(ArenaHeapNode));
		ArenaHeapNode* node = CastFromMember(ArenaHeapNode, arena, arena);
		ArenaHeapNode* next = CastFromMember(ArenaHeapNode, order, node->order.next);
		if(&next->order == &arena_heap.order){ //we are the last node
			Assert(arena_heap.cursor + bytes <= arena_heap.start + arena_heap.size, "Attempted to use more than max arena heap size");
			upt growth_bytes = (arena_heap.cursor + aligned_bytes <= arena_heap.start + arena_heap.size) ? aligned_bytes : bytes;
			
			arena->size += growth_bytes;
			arena_heap.used += growth_bytes;
			UpdateArenaHeapCursor(&arena_heap);
		}else if((next->empty.next != 0) && (next->empty.prev != 0) && (next->arena.size >= bytes)){ //next node is empty and can hold the growth
			upt growth_bytes = ((u8*)next + aligned_bytes <= arena_heap.start + arena_heap.size) ? aligned_bytes : bytes;
			
			//make new empty+order node after current node if there is enough space
			upt leftover_size = next->arena.size - growth_bytes + sizeof(ArenaHeapNode);
			if(leftover_size > sizeof(ArenaHeapNode)){
				ArenaHeapNode* new_node = (ArenaHeapNode*)((u8*)next + growth_bytes);
				NodeInsertNext(&next->order, &new_node->order); DEBUG_CheckHeapArenaNodes(&arena_heap);
				NodeInsertNext(&next->empty, &new_node->empty); DEBUG_CheckHeapArenaNodes(&arena_heap);
				new_node->arena.start = (u8*)(new_node+1);
				new_node->arena.size = leftover_size - sizeof(ArenaHeapNode);
				arena_heap.used += sizeof(ArenaHeapNode);
			}else{
				growth_bytes += leftover_size;
			}
			
			//add empty node space to current node
			NodeRemove(&next->order); DEBUG_CheckHeapArenaNodes(&arena_heap);
			NodeRemove(&next->empty); DEBUG_CheckHeapArenaNodes(&arena_heap);
			ZeroBytes(next, sizeof(ArenaHeapNode)); //NOTE the node's memory should already be zeroed
			arena->size += growth_bytes;
			arena_heap.used += (growth_bytes - sizeof(ArenaHeapNode));
			UpdateArenaHeapCursor(&arena_heap);
		}else{ //need to move memory in order to fit new size
			upt growth_bytes = (arena_heap.cursor + arena->size + aligned_bytes <= arena_heap.start + arena_heap.size) ? aligned_bytes : bytes;
			Arena* new_arena = CreateArena(arena->size + growth_bytes);
			memcpy(new_arena->start, arena->start, arena->used);
			new_arena->used = arena->used;
			new_arena->cursor = new_arena->start + (arena->cursor - arena->start);
			result = new_arena;
			DeleteArena(arena);
		}
		
		DEBUG_CheckHeapArenaNodes(&arena_heap); DEBUG_CheckArenaHeapArenas(&arena_heap); DEBUG_PrintHeapArenaNodes(&arena_heap); 
		return result;
	}
	
	void DeleteArena(Arena* arena){
		DEBUG_CheckHeapArenaNodes(&arena_heap); DEBUG_CheckArenaHeapArenas(&arena_heap);
		if(arena == 0) return;
		Assert((u8*)arena >= arena_heap.start && (u8*)arena < arena_heap.cursor, "Attempted to delete an arena outside the main heap");
		
		ArenaHeapNode* node = CastFromMember(ArenaHeapNode, arena, arena);
		void* zero_pointer = arena->start;
		upt   zero_amount = arena->size;
		upt   used_amount = arena->size;
		
		//insert current node into empty
		NodeInsertNext(&arena_heap.empty, &node->empty); DEBUG_CheckHeapArenaNodes(&arena_heap);
		
		//try to merge next empty into current empty 
		ArenaHeapNode* next = CastFromMember(ArenaHeapNode, order, node->order.next);
		if(   (&node->order != &arena_heap.order) 
		   && (&next->order != &arena_heap.order)
		   && (node->empty.next != 0) && (node->empty.prev != 0)
		   && (next->empty.next != 0) && (next->empty.prev != 0)
		   && (PointerDifference(node->arena.start + node->arena.size, next) == 0)){
			NodeRemove(&next->order); DEBUG_CheckHeapArenaNodes(&arena_heap);
			NodeRemove(&next->empty); DEBUG_CheckHeapArenaNodes(&arena_heap);
			NodeRemove(&node->empty); DEBUG_CheckHeapArenaNodes(&arena_heap);
			UpdateArenaHeapCursor(&arena_heap);
			node->arena.size += next->arena.size + sizeof(ArenaHeapNode);
			NodeInsertNext(&arena_heap.empty, &node->empty); DEBUG_CheckHeapArenaNodes(&arena_heap);
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
			NodeRemove(&node->order); DEBUG_CheckHeapArenaNodes(&arena_heap);
			NodeRemove(&node->empty); DEBUG_CheckHeapArenaNodes(&arena_heap);
			NodeRemove(&prev->empty); DEBUG_CheckHeapArenaNodes(&arena_heap);
			UpdateArenaHeapCursor(&arena_heap);
			prev->arena.size += node->arena.size + sizeof(ArenaHeapNode);
			NodeInsertNext(&arena_heap.empty, &prev->empty); DEBUG_CheckHeapArenaNodes(&arena_heap);
			zero_pointer = prev+1;
			zero_amount  = prev->arena.size;
			used_amount += sizeof(ArenaHeapNode);
			node = prev;
		}
		
		//remove the last order node if its empty
		if(node->order.next == &arena_heap.order){
			NodeRemove(&node->order); DEBUG_CheckHeapArenaNodes(&arena_heap);
			NodeRemove(&node->empty); DEBUG_CheckHeapArenaNodes(&arena_heap);
			arena_heap.cursor = (u8*)node;
			zero_pointer = node; 
			zero_amount  = node->arena.size + sizeof(ArenaHeapNode);
			used_amount += sizeof(ArenaHeapNode);
		}
		
		arena_heap.used -= used_amount;
		ZeroBytes(zero_pointer, zero_amount);
		DEBUG_CheckHeapArenaNodes(&arena_heap); DEBUG_CheckArenaHeapArenas(&arena_heap); DEBUG_PrintHeapArenaNodes(&arena_heap);
	}
	
	//////////////////
	//// @generic ////
	//////////////////
#define MEMORY_LARGE_GENERIC_ALLOCATION_SIZE Kilobytes(256)
	
	
	local Arena* generic_arena;
	
	
	void* Allocate(upt bytes){
		DEBUG_CheckHeapArenaNodes(&arena_heap); DEBUG_CheckArenaHeapArenas(&arena_heap);
		if(bytes == 0) return 0;
		Assert(generic_arena, "Attempted to allocate before Memory::Init() has been called");
		Assert(arena_heap.used + bytes <= arena_heap.size, "Attempted to use more than max main heap size");
		
		//if the allocation is large, create an arena for it rather than using the generic arena
		Arena* arena = (bytes >= MEMORY_LARGE_GENERIC_ALLOCATION_SIZE) ? CreateArena(bytes) : generic_arena;
		Assert(arena->cursor + bytes <= arena->start + arena->size, "Attempted to use more than max arena size");
		
		void* result = arena->cursor + sizeof(upt);
		*((upt*)arena->cursor) = bytes; //place allocation size at cursor
		arena->cursor += bytes + sizeof(upt);
		arena->used += bytes + sizeof(upt);
		
		DEBUG_CheckHeapArenaNodes(&arena_heap); DEBUG_CheckArenaHeapArenas(&arena_heap);
		return result;
	}
	
	void* Reallocate(void* ptr, upt bytes){
		DEBUG_CheckHeapArenaNodes(&arena_heap); DEBUG_CheckArenaHeapArenas(&arena_heap);
		if(ptr == 0) return Allocate(bytes);
		if(bytes == 0){ ZeroFree(ptr); return 0; }
		Assert(ptr > arena_heap.start && ptr < arena_heap.start + arena_heap.used, "Attempted to reallocate a pointer outside the main heap");
		Assert(generic_arena, "Attempted to allocate before Memory::Init() has been called");
		
		upt* size = ((upt*)ptr - 1);
		if(*size >= MEMORY_LARGE_GENERIC_ALLOCATION_SIZE){ //previous allocation was an arena
			if(bytes <= *size) return ptr; //do nothing if less than previous size
			Arena* arena = &((ArenaHeapNode*)size - 1)->arena;
			arena = GrowArena(arena, bytes - *size);
			size = (upt*)arena->start;
			*size = arena->size - sizeof(upt);
		}else if(bytes >= MEMORY_LARGE_GENERIC_ALLOCATION_SIZE){ //new allocation needs to be an arena
			//NOTE since its larger than MEMORY_LARGE_GENERIC_ALLOCATION_SIZE and not an arena already, it cant be less than previous size
			Arena* arena = CreateArena(bytes);
			arena->used = *size + sizeof(upt);
			memcpy(arena->start, size, arena->used);
			size = (upt*)arena->start;
			*size = arena->size - sizeof(upt);
			ZeroFree(ptr);
		}else if((u8*)ptr + *size == generic_arena->cursor){ //there is no used memory after this
			if(bytes <= *size){ //move generic_arena cursor left by removed size
				generic_arena->cursor -= (*size - bytes);
				generic_arena->used -= (*size - bytes);
				*size = bytes;
			}else{ //move generic_arena cursor right by extra size
				Assert(generic_arena->cursor + (bytes - *size) <= generic_arena->start + generic_arena->size, "Attempted to use more than max generic arena size");
				generic_arena->cursor += (bytes - *size);
				generic_arena->used += (bytes - *size);
				*size = bytes;
			}
		}else{ //need to move memory in order to fit new size
			if(bytes <= *size) return ptr; //do nothing if less than previous size
			Assert(arena_heap.used + (*size - bytes) <= arena_heap.size, "Attempted to use more than max main heap size");
			void* new_ptr = Allocate(bytes);
			memcpy(new_ptr, ptr, *size);
			size = (upt*)new_ptr - 1;
			ZeroFree(ptr);
		}
		
		DEBUG_CheckHeapArenaNodes(&arena_heap); DEBUG_CheckArenaHeapArenas(&arena_heap);
		return size+1;
	}
	
	void ZeroFree(void* ptr){
		DEBUG_CheckHeapArenaNodes(&arena_heap); DEBUG_CheckArenaHeapArenas(&arena_heap);
		if(ptr == 0) return;
		Assert(ptr > arena_heap.start && ptr < arena_heap.cursor, "Attempted to free a pointer outside the main heap");
		
		upt* size = ((upt*)ptr - 1);
		if(*size >= MEMORY_LARGE_GENERIC_ALLOCATION_SIZE){
			DeleteArena(&((ArenaHeapNode*)size - 1)->arena);
		}else{
			if((u8*)ptr + *size == generic_arena->cursor) generic_arena->cursor -= *size + sizeof(upt);
			generic_arena->used -= *size + sizeof(upt);
			ZeroBytes(size, *size);
		}
		DEBUG_CheckHeapArenaNodes(&arena_heap); DEBUG_CheckArenaHeapArenas(&arena_heap);
	}
	
	////////////////////
	//// @temporary ////
	////////////////////
	local Arena temp_arena;
	
	void* TempAllocate(upt bytes){
		if(bytes == 0) return 0;
		Assert(temp_arena.used + bytes <= temp_arena.size, "Attempted to use more than max temp arena size");
		
		void* result = temp_arena.cursor + sizeof(upt);
		*((upt*)temp_arena.cursor) = bytes; //place allocation size at cursor
		temp_arena.cursor += bytes + sizeof(upt);
		temp_arena.used += bytes + sizeof(upt);
		return result;
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
		DEBUG_CheckHeapArenaNodes(&arena_heap);
		
		temp_arena.start  = arena_heap.start + arena_heap.size;
		temp_arena.cursor = temp_arena.start;
		temp_arena.size   = temp_size;
		temp_arena.used   = 0;
		
		generic_arena = CreateArena(Megabytes(64)-sizeof(ArenaHeapNode));
		initialized = true;
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

void* TempAllocator_Resize(void* ptr, upt bytes){
	void* a = Memory::TempAllocate(bytes); 
	memcpy(a, ptr, *((upt*)ptr-1)); 
	return a;
}