namespace Memory{
	//NOTE Memory Layout: 
	//|                       Total Size                          |
	//|                  Main Heap                  |  Temp Arena |
	//|      Heap Arena      |      Heap Arena      | Item | Item |
	//| Header |    Memory   | Header |    Memory   |      |      |
	//|        | Item | Item |        | Item | Item |      |      |
	
	//TODO(delle) maybe convert out of memory asserts to errors?
	//TODO(delle) add macro interface over functions to track where they are called from (file, line, function)
	//TODO(delle) standardize naming of bytes vs size
	//TODO(delle) convert generic_arena to a heap with custom HeapNodes
	
	///////////////////
	//// @internal ////
	///////////////////
#define MEMORY_CHECK_HEAPS true
#define MEMORY_ARENA_MIN_SIZE Kilobytes(4)
#define MEMORY_LARGE_GENERIC_ALLOCATION_SIZE Kilobytes(256)
	local Heap   main_heap;
	local Arena  temp_arena;
	local Arena* generic_arena;
	local b32    initialized = false;
	
#define HeapNodeInsertNext(x,node) ((node)->next=(x)->next,(node)->prev=(x),(node)->next->prev=(node),(x)->next=(node))
#define HeapNodeInsertPrev(x,node) ((node)->prev=(x)->prev,(node)->next=(x),(node)->prev->next=(node),(x)->prev=(node))
#define HeapNodeRemove(node) ((node)->next->prev=(node)->prev,(node)->prev->next=(node)->next)
	
	FORCE_INLINE void ZeroBytes(void* ptr, upt bytes){
		memset(ptr, 0, bytes);
	}
	
	function inline void UpdateHeapCursor(Heap* heap){ //NOTE this relies on empty nodes having correct arena.start positions
		HeapNode* last_order_node = CastFromMember(HeapNode, order, main_heap.order.prev);
		heap->cursor = last_order_node->arena.start + last_order_node->arena.size;
	}
	
#if MEMORY_CHECK_HEAPS
	function void DEBUGAssertHeapNodesAreGood(Heap* heap){
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
	
	function void DEBUGAssertHeapArenasAreGood(Heap* heap){
		upt overall_used = 0;
		for(Node* order = &heap->order; ; ){
			if(order != &heap->order){
				if(order->next != &heap->order){
					HeapNode* node = CastFromMember(HeapNode, order, order);
					overall_used += node->arena.size + sizeof(HeapNode);
					HeapNode* next = CastFromMember(HeapNode, order, order->next);
					Assert(node->arena.start + node->arena.size == (u8*)next, "Heap node arena is not sized correctly");
				}else{
					HeapNode* node = CastFromMember(HeapNode, order, order);
					overall_used += node->arena.size + sizeof(HeapNode);
					Assert(node->arena.start + node->arena.size == heap->cursor, "Heap cursor is not in the right spot");
					break;
				}
			}
			order = order->next;
			if(order == &heap->order) break;
		}
		//Assert(overall_used == heap->used, "Heap used is incorrect"); //TODO(delle) add this back
	}
	
	function void DEBUGPrintHeapNodes(Heap* heap){
		if(!initialized) return;
		string heap_order = "Order: ", heap_empty = "Empty: ";
		if(heap->used > 0){
			for(HeapNode* node = (HeapNode*)heap->start; ;){
				heap_order += to_string("%p", node); heap_order += " -> ";
				heap_empty += (node->empty.next) ? to_string("%p", node) : "                ";
				heap_empty += " -> ";
				
				if(node->order.next == &heap->order) break;
				node = CastFromMember(HeapNode, order, node->order.next);
			}
		}
		Log("memory",heap_order); Log("memory",heap_empty); Log("","----------------------------------------------");
	}
#else //MEMORY_CHECK_HEAPS
#  define DEBUGAssertHeapNodesAreGood(heap) 
#  define DEBUGAssertHeapArenasAreGood(heap)
#  define DEBUGPrintHeapNodes(heap)
#endif //MEMORY_CHECK_HEAPS
	
	function void DEBUGDrawMemory(){
		UI::PushColor(UIStyleCol_WindowBg, color(128,128,128,128));
		UI::Begin("debug__memory", {100,100}, {480,480});
		//TODO this
		UI::End();
		UI::PopColor();
	}
	
	////////////////////
	//// @interface ////
	////////////////////
	Arena* CreateArena(upt bytes){
		DEBUGAssertHeapNodesAreGood(&main_heap);
		if(bytes == 0) return 0;
		Assert(main_heap.start, "Attempted to create an arena before Memory::Init() has been called");
		upt aligned_size = ClampMin(RoundUpTo(bytes, sizeof(HeapNode)), MEMORY_ARENA_MIN_SIZE);
		Assert(main_heap.used + aligned_size <= main_heap.size, "Attempted to use more than max main heap size");
		
		//check if there are any empty nodes that can hold the new arena
		if(main_heap.empty.next != &main_heap.empty){
			for(Node* n = main_heap.empty.next; n != &main_heap.empty; n = n->next){
				HeapNode* node = CastFromMember(HeapNode, empty, n);
				if(node->arena.size >= aligned_size){
					upt leftover_size = node->arena.size - aligned_size;
					Assert(leftover_size % sizeof(HeapNode) == 0, "Memory was not aligned correctly");
					
					//make new empty node after new order node if there is enough space
					if(leftover_size > sizeof(HeapNode)){
						HeapNode* new_node = (HeapNode*)(node->arena.start + aligned_size);
						HeapNodeInsertNext(&node->order, &new_node->order); DEBUGAssertHeapNodesAreGood(&main_heap);
						HeapNodeInsertNext(&node->empty, &new_node->empty); DEBUGAssertHeapNodesAreGood(&main_heap);
						new_node->arena.start = (u8*)(new_node+1);
						new_node->arena.size = leftover_size - sizeof(HeapNode);
						main_heap.used += sizeof(HeapNode);
					}else{
						aligned_size += leftover_size;
					}
					
					//convert empty node to order node
					HeapNodeRemove(&node->empty); DEBUGAssertHeapNodesAreGood(&main_heap);
					node->empty.next = 0;
					node->empty.prev = 0;
					//node->arena.start = (u8*)(node+1); //NOTE arena starts are correct in empty nodes
					node->arena.cursor = node->arena.start;
					node->arena.size   = aligned_size;
					node->arena.used   = 0;
					main_heap.used += aligned_size;
					UpdateHeapCursor(&main_heap);
					
					DEBUGAssertHeapNodesAreGood(&main_heap); DEBUGAssertHeapArenasAreGood(&main_heap); DEBUGPrintHeapNodes(&main_heap);
					return &node->arena;
				}
			}
		}
		
		//if we cant replace an empty node, make a new order node for the new arena
		Assert(main_heap.cursor + aligned_size + sizeof(HeapNode) <= main_heap.start + main_heap.size, "Attempted to use more than max main heap size");
		HeapNode* new_node = (HeapNode*)main_heap.cursor;
		HeapNodeInsertPrev(&main_heap.order, &new_node->order); DEBUGAssertHeapNodesAreGood(&main_heap);
		new_node->arena.start  = (u8*)new_node + sizeof(HeapNode);
		new_node->arena.cursor = new_node->arena.start;
		new_node->arena.size   = aligned_size;
		//new_node->arena.used   = 0; //NOTE new memory is already zero
		main_heap.used   += aligned_size + sizeof(HeapNode);
		UpdateHeapCursor(&main_heap);
		
		DEBUGAssertHeapNodesAreGood(&main_heap); DEBUGAssertHeapArenasAreGood(&main_heap); DEBUGPrintHeapNodes(&main_heap); 
		return &new_node->arena;
	}
	
	Arena* GrowArena(Arena* arena, upt bytes){
		DEBUGAssertHeapNodesAreGood(&main_heap); DEBUGAssertHeapArenasAreGood(&main_heap);
		if(bytes == 0) return arena;
		if(arena == 0) return 0;
		Assert((u8*)arena >= main_heap.start && (u8*)arena < main_heap.cursor, "Attempted to grow an arena that's outside the main heap");
		Assert(main_heap.used + bytes <= main_heap.size, "Attempted to use more than max main heap size");
		
		//check if the next node is empty and can hold the grown size, or if we need to make a new arena
		Arena* result = arena;
		upt aligned_bytes = RoundUpTo(bytes, sizeof(HeapNode));
		HeapNode* node = CastFromMember(HeapNode, arena, arena);
		HeapNode* next = CastFromMember(HeapNode, order, node->order.next);
		if(&next->order == &main_heap.order){ //we are the last node
			Assert(main_heap.cursor + bytes <= main_heap.start + main_heap.size, "Attempted to use more than max main heap size");
			upt growth_bytes = (main_heap.cursor + aligned_bytes <= main_heap.start + main_heap.size) ? aligned_bytes : bytes;
			
			arena->size += growth_bytes;
			main_heap.used += growth_bytes;
			UpdateHeapCursor(&main_heap);
		}else if((next->empty.next != 0) && (next->empty.prev != 0) && (next->arena.size >= bytes)){ //next node is empty and can hold the growth
			upt growth_bytes = ((u8*)next + aligned_bytes <= main_heap.start + main_heap.size) ? aligned_bytes : bytes;
			
			//make new empty+order node after current node if there is enough space
			upt leftover_size = next->arena.size - growth_bytes + sizeof(HeapNode);
			if(leftover_size > sizeof(HeapNode)){
				HeapNode* new_node = (HeapNode*)((u8*)next + growth_bytes);
				HeapNodeInsertNext(&next->order, &new_node->order); DEBUGAssertHeapNodesAreGood(&main_heap);
				HeapNodeInsertNext(&next->empty, &new_node->empty); DEBUGAssertHeapNodesAreGood(&main_heap);
				new_node->arena.start = (u8*)(new_node+1);
				new_node->arena.size = leftover_size - sizeof(HeapNode);
				main_heap.used += sizeof(HeapNode);
			}else{
				growth_bytes += leftover_size;
			}
			
			//add empty node space to current node
			HeapNodeRemove(&next->order); DEBUGAssertHeapNodesAreGood(&main_heap);
			HeapNodeRemove(&next->empty); DEBUGAssertHeapNodesAreGood(&main_heap);
			ZeroBytes(next, sizeof(HeapNode)); //NOTE the node's memory should already be zeroed
			arena->size += growth_bytes;
			main_heap.used += (growth_bytes - sizeof(HeapNode));
			UpdateHeapCursor(&main_heap);
		}else{ //need to move memory in order to fit new size
			upt growth_bytes = (main_heap.cursor + arena->size + aligned_bytes <= main_heap.start + main_heap.size) ? aligned_bytes : bytes;
			Arena* new_arena = CreateArena(arena->size + growth_bytes);
			memcpy(new_arena->start, arena->start, arena->used);
			new_arena->used = arena->used;
			new_arena->cursor = new_arena->start + (arena->cursor - arena->start);
			result = new_arena;
			DeleteArena(arena);
		}
		
		DEBUGAssertHeapNodesAreGood(&main_heap); DEBUGAssertHeapArenasAreGood(&main_heap); DEBUGPrintHeapNodes(&main_heap); 
		return result;
	}
	
	void DeleteArena(Arena* arena){
		DEBUGAssertHeapNodesAreGood(&main_heap); DEBUGAssertHeapArenasAreGood(&main_heap);
		if(arena == 0) return;
		Assert((u8*)arena >= main_heap.start && (u8*)arena < main_heap.cursor, "Attempted to delete an arena outside the main heap");
		
		HeapNode* node = CastFromMember(HeapNode, arena, arena);
		void* zero_pointer = arena->start;
		upt   zero_amount = arena->size;
		upt   used_amount = arena->size;
		
		//insert current node into empty
		HeapNodeInsertNext(&main_heap.empty, &node->empty); DEBUGAssertHeapNodesAreGood(&main_heap);
		
		//try to merge next empty into current empty 
		HeapNode* next = CastFromMember(HeapNode, order, node->order.next);
		if(   (&node->order != &main_heap.order) 
		   && (&next->order != &main_heap.order)
		   && (node->empty.next != 0) && (node->empty.prev != 0)
		   && (next->empty.next != 0) && (next->empty.prev != 0)
		   && (PointerDifference(node->arena.start + node->arena.size, next) == 0)){
			HeapNodeRemove(&next->order); DEBUGAssertHeapNodesAreGood(&main_heap);
			HeapNodeRemove(&next->empty); DEBUGAssertHeapNodesAreGood(&main_heap);
			HeapNodeRemove(&node->empty); DEBUGAssertHeapNodesAreGood(&main_heap);
			UpdateHeapCursor(&main_heap);
			node->arena.size += next->arena.size + sizeof(HeapNode);
			HeapNodeInsertNext(&main_heap.empty, &node->empty); DEBUGAssertHeapNodesAreGood(&main_heap);
			zero_pointer = node+1;
			zero_amount  = node->arena.size;
			used_amount += sizeof(HeapNode);
		}
		
		//try to merge current empty into prev empty 
		HeapNode* prev = CastFromMember(HeapNode, order, node->order.prev);
		if(   (&prev->order != &main_heap.order) 
		   && (&node->order != &main_heap.order)
		   && (prev->empty.next != 0) && (prev->empty.prev != 0)
		   && (node->empty.next != 0) && (node->empty.prev != 0)
		   && (PointerDifference(prev->arena.start + prev->arena.size, node) == 0)){
			HeapNodeRemove(&node->order); DEBUGAssertHeapNodesAreGood(&main_heap);
			HeapNodeRemove(&node->empty); DEBUGAssertHeapNodesAreGood(&main_heap);
			HeapNodeRemove(&prev->empty); DEBUGAssertHeapNodesAreGood(&main_heap);
			UpdateHeapCursor(&main_heap);
			prev->arena.size += node->arena.size + sizeof(HeapNode);
			HeapNodeInsertNext(&main_heap.empty, &prev->empty); DEBUGAssertHeapNodesAreGood(&main_heap);
			zero_pointer = prev+1;
			zero_amount  = prev->arena.size;
			used_amount += sizeof(HeapNode);
			node = prev;
		}
		
		//remove the last order node if its empty
		if(node->order.next == &main_heap.order){
			HeapNodeRemove(&node->order); DEBUGAssertHeapNodesAreGood(&main_heap);
			HeapNodeRemove(&node->empty); DEBUGAssertHeapNodesAreGood(&main_heap);
			main_heap.cursor = (u8*)node;
			zero_pointer = node; 
			zero_amount  = node->arena.size + sizeof(HeapNode);
			used_amount += sizeof(HeapNode);
		}
		
		main_heap.used -= used_amount;
		ZeroBytes(zero_pointer, zero_amount);
		DEBUGAssertHeapNodesAreGood(&main_heap); DEBUGAssertHeapArenasAreGood(&main_heap); DEBUGPrintHeapNodes(&main_heap);
	}
	
	void* Allocate(upt bytes){
		DEBUGAssertHeapNodesAreGood(&main_heap); DEBUGAssertHeapArenasAreGood(&main_heap);
		if(bytes == 0) return 0;
		Assert(generic_arena, "Attempted to allocate before Memory::Init() has been called");
		Assert(main_heap.used + bytes <= main_heap.size, "Attempted to use more than max main heap size");
		
		//if the allocation is large, create an arena for it rather than using the generic arena
		Arena* arena = (bytes >= MEMORY_LARGE_GENERIC_ALLOCATION_SIZE) ? CreateArena(bytes) : generic_arena;
		Assert(arena->cursor + bytes <= arena->start + arena->size, "Attempted to use more than max arena size");
		
		void* result = arena->cursor + sizeof(upt);
		*((upt*)arena->cursor) = bytes; //place allocation size at cursor
		arena->cursor += bytes + sizeof(upt);
		arena->used += bytes + sizeof(upt);
		
		DEBUGAssertHeapNodesAreGood(&main_heap); DEBUGAssertHeapArenasAreGood(&main_heap);
		return result;
	}
	
	void* TempAllocate(upt bytes){
		if(bytes == 0) return 0;
		Assert(temp_arena.used + bytes <= temp_arena.size, "Attempted to use more than max temp arena size");
		
		void* result = temp_arena.cursor + sizeof(upt);
		*((upt*)temp_arena.cursor) = bytes; //place allocation size at cursor
		temp_arena.cursor += bytes + sizeof(upt);
		temp_arena.used += bytes + sizeof(upt);
		return result;
	}
	
	void* Reallocate(void* ptr, upt bytes){
		DEBUGAssertHeapNodesAreGood(&main_heap); DEBUGAssertHeapArenasAreGood(&main_heap);
		if(ptr == 0) return Allocate(bytes);
		if(bytes == 0){ ZeroFree(ptr); return 0; }
		Assert(ptr > main_heap.start && ptr < main_heap.start + main_heap.used, "Attempted to reallocate a pointer outside the main heap");
		Assert(generic_arena, "Attempted to allocate before Memory::Init() has been called");
		
		upt* size = ((upt*)ptr - 1);
		if(*size >= MEMORY_LARGE_GENERIC_ALLOCATION_SIZE){ //previous allocation was an arena
			if(bytes <= *size) return ptr; //do nothing if less than previous size
			Arena* arena = &((HeapNode*)size - 1)->arena;
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
			Assert(main_heap.used + (*size - bytes) <= main_heap.size, "Attempted to use more than max main heap size");
			void* new_ptr = Allocate(bytes);
			memcpy(new_ptr, ptr, *size);
			size = (upt*)new_ptr - 1;
			ZeroFree(ptr);
		}
		
		DEBUGAssertHeapNodesAreGood(&main_heap); DEBUGAssertHeapArenasAreGood(&main_heap);
		return size+1;
	}
	
	void ZeroFree(void* ptr){
		DEBUGAssertHeapNodesAreGood(&main_heap); DEBUGAssertHeapArenasAreGood(&main_heap);
		if(ptr == 0) return;
		Assert(ptr > main_heap.start && ptr < main_heap.start + main_heap.used, "Attempted to free a pointer outside the main heap");
		
		upt* size = ((upt*)ptr - 1);
		if(*size >= MEMORY_LARGE_GENERIC_ALLOCATION_SIZE){
			DeleteArena(&((HeapNode*)size - 1)->arena);
		}else{
			if((u8*)ptr + *size == generic_arena->cursor) generic_arena->cursor -= *size + sizeof(upt);
			generic_arena->used -= *size + sizeof(upt);
			ZeroBytes(size, *size);
		}
		DEBUGAssertHeapNodesAreGood(&main_heap); DEBUGAssertHeapArenasAreGood(&main_heap);
	}
	
	void Init(upt main_size, upt temp_size){
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
		
		main_heap.start  = allocation;
		main_heap.cursor = allocation;
		main_heap.size   = main_size;
		main_heap.used   = 0;
		main_heap.order.next = main_heap.order.prev = &main_heap.order;
		main_heap.empty.next = main_heap.empty.prev = &main_heap.empty;
		DEBUGAssertHeapNodesAreGood(&main_heap);
		
		temp_arena.start  = main_heap.start + main_heap.size;
		temp_arena.cursor = temp_arena.start;
		temp_arena.size   = temp_size;
		temp_arena.used   = 0;
		
		generic_arena = CreateArena(Megabytes(64)-sizeof(HeapNode));
		initialized = true;
	}
	
	void Update(){
		//DEBUGDrawMemory();
		ZeroBytes(temp_arena.start, temp_arena.used);
		temp_arena.cursor = temp_arena.start;
		temp_arena.used = 0;
	}
}; //namespace Memory
