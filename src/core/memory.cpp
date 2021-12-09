namespace Memory{
	//NOTE Memory Layout: 
	//|                       Total Size                          |
	//|                  Main Heap                  |  Temp Arena |
	//|      Heap Arena      |      Heap Arena      | Item | Item |
	//| Header |    Memory   | Header |    Memory   |      |      |
	//|        | Item | Item |        | Item | Item |      |      |
	
	///////////////////
	//// @internal ////
	///////////////////
#define MEMORY_CHECK_HEAPS false
#define MEMORY_ARENA_MIN_SIZE Kilobytes(4)
#define MEMORY_ARENA_BYTE_ALIGNMENT 8
#define MEMORY_LARGE_GENERAL_ALLOCATION_SIZE Megabytes(4)
	local Heap   main_heap;
	local Arena  temp_arena;
	local Arena* generic_arena;
	
#define HeapNodeInsertNext(x,node) ((node)->next=(x)->next,(node)->prev=(x),(node)->next->prev=(node),(x)->next=(node))
#define HeapNodeInsertPrev(x,node) ((node)->prev=(x)->prev,(node)->next=(x),(node)->prev->next=(node),(x)->prev=(node))
#define HeapNodeRemove(node) ((node)->next->prev=(node)->prev,(node)->prev->next=(node)->next)
	
	FORCE_INLINE void ZeroBytes(void* ptr, upt bytes){
		memset(ptr, 0, bytes);
	}
	
#if MEMORY_CHECK_HEAPS
	local void DEBUGAssertHeapIsGood(Heap* heap){
		Assert(heap->order.next != 0, "First heap order node is invalid");
		Assert(heap->order.prev != 0, "First heap order node is invalid");
		Assert(heap->empty.next != 0, "First heap empty node is invalid");
		Assert(heap->empty.prev != 0, "First heap empty node is invalid");
		for(Node* node = &heap->order; ; ){
			Assert(node->next->prev == node, "Heap order node is invalid");
			Assert(node->prev->next == node, "Heap order node is invalid");
			node = node->next;
			if(node == &heap->order) break;
		}
		for(Node* node = &heap->empty; ; ){
			Assert(node->next->prev == node, "Heap empty node is invalid");
			Assert(node->prev->next == node, "Heap empty node is invalid");
			node = node->next;
			if(node == &heap->empty) break;
		}
	}
#else //MEMORY_CHECK_HEAPS
# define DEBUGAssertHeapIsGood(heap) UNUSED_VAR(heap)
#endif //MEMORY_CHECK_HEAPS
	
	local void DEBUGPrintMainHeapNodes(){
		string heap_order = "Order: ", heap_empty = "Empty: ";
		if(main_heap.used > 0){
			for(HeapNode* node = (HeapNode*)main_heap.start; ;){
				heap_order += to_string("%p", node); heap_order += " -> ";
				heap_empty += (node->empty.next) ? to_string("%p", node) : "                ";
				heap_empty += " -> ";
				
				if(node->order.next == &main_heap.order) break;
				node = CastFromMember(HeapNode, order, node->order.next);
			}
		}
		Log("memory",heap_order); Log("memory",heap_empty); Log("","----------------------------------------------");
	}
	
	local void DEBUGDrawMemory(){
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
		DEBUGAssertHeapIsGood(&main_heap);
		if(bytes == 0) return 0;
		upt reserve_size = ClampMin(RoundUpTo(bytes + sizeof(HeapNode), MEMORY_ARENA_BYTE_ALIGNMENT), MEMORY_ARENA_MIN_SIZE);
		Assert(main_heap.start, "Attempted to create an arena before Memory::Init() has been called");
		Assert(main_heap.used + reserve_size <= main_heap.size, "Attempted to use more than max main heap size");
		
		//check if there are any empty nodes that can hold the new arena
		if(main_heap.empty.next != &main_heap.empty){
			for(Node* n = main_heap.empty.next; n != &main_heap.empty; n = n->next){
				HeapNode* node = CastFromMember(HeapNode, empty, n);
				if(node->arena.size >= reserve_size){
					u8* arena_start = (u8*)node + sizeof(HeapNode);
					upt leftover_size = node->arena.size - reserve_size;
					
					//make new empty node after new order node if there is enough space
					if(leftover_size > sizeof(HeapNode)){
						HeapNode* new_node = (HeapNode*)(arena_start + reserve_size - sizeof(HeapNode));
						HeapNodeInsertNext(&node->order, &new_node->order); DEBUGAssertHeapIsGood(&main_heap);
						HeapNodeInsertNext(&node->empty, &new_node->empty); DEBUGAssertHeapIsGood(&main_heap);
						new_node->arena.size = leftover_size - sizeof(HeapNode);
					}
					
					//convert empty node to order node
					HeapNodeRemove(&node->empty); DEBUGAssertHeapIsGood(&main_heap);
					node->empty.next = 0;
					node->empty.prev = 0;
					node->arena.start  = arena_start;
					node->arena.cursor = arena_start;
					node->arena.size   = reserve_size - sizeof(HeapNode);
					node->arena.used   = 0;
					main_heap.cursor += reserve_size;
					main_heap.used   += reserve_size;
					
					DEBUGAssertHeapIsGood(&main_heap); //DEBUGPrintMainHeapNodes();
					return &node->arena;
				}
			}
		}
		
		//if we cant replace an empty node, make a new order node for the new arena
		Assert(main_heap.cursor + reserve_size <= main_heap.start + main_heap.size, "Attempted to use more than max main heap size");
		HeapNode* new_node = (HeapNode*)main_heap.cursor;
		HeapNodeInsertPrev(&main_heap.order, &new_node->order); DEBUGAssertHeapIsGood(&main_heap);
		new_node->arena.start  = (u8*)new_node + sizeof(HeapNode);
		new_node->arena.cursor = new_node->arena.start;
		new_node->arena.size   = reserve_size - sizeof(HeapNode);
		//new_node->arena.used   = 0; //NOTE new memory is zero
		main_heap.cursor += reserve_size;
		main_heap.used   += reserve_size;
		
		DEBUGAssertHeapIsGood(&main_heap); //DEBUGPrintMainHeapNodes(); 
		return &new_node->arena;
	}
	
	void DeleteArena(Arena* arena){
		DEBUGAssertHeapIsGood(&main_heap);
		if(arena == 0) return;
		Assert((u8*)arena >= main_heap.start && (u8*)arena < main_heap.cursor, "Attempted to delete an arena outside the main heap");
		
		void* zero_pointer = arena->start;
		upt   zero_amount = arena->size;
		HeapNode* node = CastFromMember(HeapNode, arena, arena);
		upt overall_size = arena->size + sizeof(HeapNode);
		if(node->order.next == &main_heap.order){ //last node
			HeapNodeRemove(&node->order); DEBUGAssertHeapIsGood(&main_heap);
			main_heap.cursor -= overall_size;
			zero_pointer = node; 
			zero_amount = overall_size;
		}else{ //not the last node, so try to merge with nearby empty nodes
			HeapNodeInsertNext(&main_heap.empty, &node->empty); DEBUGAssertHeapIsGood(&main_heap);
			
			//try to merge next empty into current empty 
			HeapNode* next = CastFromMember(HeapNode, order, node->order.next);
			if((&node->order != &main_heap.order) && (&next->order != &main_heap.order)
			   && (node->empty.next != 0) && (node->empty.prev != 0)
			   && (next->empty.next != 0) && (next->empty.prev != 0)){
				u8* ptr = (u8*)(node + 1) + node->arena.size;
				if(PointerDifference(ptr, next) == 0){
					if(next->order.next == &main_heap.order) main_heap.cursor -= next->arena.size + sizeof(HeapNode);
					HeapNodeRemove(&next->order); DEBUGAssertHeapIsGood(&main_heap);
					HeapNodeRemove(&next->empty); DEBUGAssertHeapIsGood(&main_heap);
					HeapNodeRemove(&node->empty); DEBUGAssertHeapIsGood(&main_heap);
					node->arena.size += next->arena.size + sizeof(HeapNode);
					HeapNodeInsertNext(&main_heap.empty, &node->empty); DEBUGAssertHeapIsGood(&main_heap);
					zero_pointer = node+1;
					zero_amount  = node->arena.size;
				}
			}
			
			//try to merge current empty into prev empty 
			HeapNode* prev = CastFromMember(HeapNode, order, node->order.prev);
			if(&prev->order != &main_heap.order && &node->order != &main_heap.order
			   && prev->empty.next != 0 && prev->empty.prev != 0
			   && node->empty.next != 0 && node->empty.prev != 0){
				u8* ptr = (u8*)(prev + 1) + prev->arena.size;
				if(PointerDifference(ptr, node) == 0){
					if(node->order.next == &main_heap.order) main_heap.cursor -= node->arena.size + sizeof(HeapNode);
					HeapNodeRemove(&node->order); DEBUGAssertHeapIsGood(&main_heap);
					HeapNodeRemove(&node->empty); DEBUGAssertHeapIsGood(&main_heap);
					HeapNodeRemove(&prev->empty); DEBUGAssertHeapIsGood(&main_heap);
					prev->arena.size += node->arena.size + sizeof(HeapNode);
					HeapNodeInsertNext(&main_heap.empty, &prev->empty); DEBUGAssertHeapIsGood(&main_heap);
					zero_pointer = prev+1;
					zero_amount  = prev->arena.size;
				}
			}
		}
		
		main_heap.used -= overall_size;
		ZeroBytes(zero_pointer, zero_amount); 
		DEBUGAssertHeapIsGood(&main_heap); //DEBUGPrintMainHeapNodes();
	}
	
	void* Allocate(upt bytes){
		DEBUGAssertHeapIsGood(&main_heap);
		if(bytes == 0) return 0;
		Assert(generic_arena, "Attempted to allocate before Memory::Init() has been called");
		Assert(main_heap.used + bytes <= main_heap.size, "Attempted to use more than max main heap size");
		
		//if the allocation is large, create an arena for it rather than using the generic arena
		Arena* arena = (bytes > MEMORY_LARGE_GENERAL_ALLOCATION_SIZE) ? CreateArena(bytes) : generic_arena;
		Assert(arena->cursor + bytes <= arena->start + arena->size, "Attempted to use more than max arena size");
		
		void* result = arena->cursor + sizeof(upt);
		*((upt*)arena->cursor) = bytes; //place allocation size at cursor
		arena->cursor += bytes + sizeof(upt);
		arena->used += bytes + sizeof(upt);
		
		DEBUGAssertHeapIsGood(&main_heap);
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
	
	void ZeroFree(void* ptr){
		DEBUGAssertHeapIsGood(&main_heap);
		if(ptr == 0) return;
		Assert(ptr >= main_heap.start && ptr <= main_heap.start + main_heap.used, "Attempted to free a pointer outside the main heap");
		
		upt* size = ((upt*)ptr - 1);
		if(*size > MEMORY_LARGE_GENERAL_ALLOCATION_SIZE){
			DeleteArena(&((HeapNode*)size - 1)->arena);
		}else{
			ZeroBytes(size, *size);
		}
		DEBUGAssertHeapIsGood(&main_heap);
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
		DEBUGAssertHeapIsGood(&main_heap);
		
		temp_arena.start  = main_heap.start + main_heap.size;
		temp_arena.cursor = temp_arena.start;
		temp_arena.size   = temp_size;
		temp_arena.used   = 0;
		
		generic_arena = CreateArena(Megabytes(64)-sizeof(HeapNode));
	}
	
	void Update(){
		//DEBUGDrawMemory();
		ZeroBytes(temp_arena.start, temp_arena.used);
		temp_arena.cursor = temp_arena.start;
		temp_arena.used = 0;
	}
}; //namespace Memory