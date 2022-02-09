#include <typeinfo>
#include <cstdio>
#include <ctime>
#include "kigu/common.h"

#define DESHI_TEST_CORE_TODO(name) printf("[DESHI-TEST] TODO:   core/"name"\n")
#define DESHI_TEST_CORE_PASSED(name) printf("[DESHI-TEST] PASSED: core/"name"\n")

#include "armature.h"
local void TEST_deshi_core_armature(){
	DESHI_TEST_CORE_TODO("armature");
}

#include "camera.h"
local void TEST_deshi_core_camera(){
	DESHI_TEST_CORE_TODO("camera");
}

#include "commands.h"
local void TEST_deshi_core_commands(){
	DESHI_TEST_CORE_TODO("commands");
}

#include "console.h"
local void TEST_deshi_core_console(){
	DESHI_TEST_CORE_TODO("console");
}

#include "font.h"
local void TEST_deshi_core_font(){
	DESHI_TEST_CORE_TODO("font");
}

#include "input.h"
local void TEST_deshi_core_input(){
	DESHI_TEST_CORE_TODO("input");
}

#include "io.h"
local void TEST_deshi_core_io(){
	DESHI_TEST_CORE_TODO("io");
}

#include "logger.h"
local void TEST_deshi_core_logging(){
	DESHI_TEST_CORE_TODO("logging");
}

#include "memory.h"
local void TEST_deshi_core_memory(){
	Heap* arena_heap = memory_expose_arena_heap();
	AssertAlways(arena_heap);
	
	Heap* generic_heap = memory_expose_generic_heap();
	AssertAlways(generic_heap);
	
	Arena* temp_arena = memory_expose_temp_arena();
	AssertAlways(temp_arena);
	
	//arena creation
	Arena* arena1 = memory_create_arena(Kilobytes(4));
	AssertAlways(arena1);
	AssertAlways(arena1->start);
	AssertAlways(arena1->cursor);
	AssertAlways(arena1->used == 0);
	AssertAlways(arena1->size >= Kilobytes(4));
	
	//arena deletion
	memory_delete_arena(arena1);
	
	//deleting an arena in between two arenas
	arena1 = memory_create_arena(Kilobytes(8));
	Arena* arena2 = memory_create_arena(Kilobytes(16));
	Arena* arena3 = memory_create_arena(Kilobytes(8));
	memory_delete_arena(arena2);
	
	//clearing an arena
	memset(arena1->start, '3', 1024);
	arena1->cursor += 1024;
	arena1->used += 1024;
	memory_clear_arena(arena1);
	AssertAlways(*arena1->start == 0);
	AssertAlways(arena1->cursor == arena1->start);
	AssertAlways(arena1->used == 0);
	
	//filling the gap with arenas
	arena2 = memory_create_arena(Kilobytes(4));
	Arena* arena4 = memory_create_arena(Kilobytes(4));
	
	//arenas should merge right then left
	memory_delete_arena(arena1);
	memory_delete_arena(arena2);
	memory_delete_arena(arena3);
	memory_delete_arena(arena4);
	
	//generic allocation creation and deletion
	char* string1 = (char*)memory_alloc(64);
	forI(52){ string1[i] = 'a'+char(i); }
	char* string2 = (char*)memory_alloc(256);
	char* string3 = (char*)memory_alloc(56);
	memset(string2, '2', 128);
	memset(string3, '3', 50);
	memory_zfree(string2);
	memory_zfree(string1);
	
	//big memory generic allocation creation and deletion (should make its own arena)
	void* big_block = memory_alloc(Kilobytes(512));
	memset(big_block, 7, Kilobytes(64));
	memory_zfree(string3);
	memory_zfree(big_block);
	
	//arena growing with nothing as last node
	arena1 = memory_create_arena(Kilobytes(4));
	arena1 = memory_grow_arena(arena1, Kilobytes(4));
	
	//arena growing into a gap
	arena2 = memory_create_arena(Kilobytes(8));
	arena3 = memory_create_arena(Kilobytes(8));
	memory_delete_arena(arena2);
	arena1 = memory_grow_arena(arena1, Kilobytes(4));
	
	//grow arena but it needs to move
	arena1 = memory_grow_arena(arena1, Kilobytes(16));
	
	//delete grown arena
	memory_delete_arena(arena1);
	memory_delete_arena(arena3);
	
	//generic reallocation of a big block
	big_block = memory_alloc(Megabytes(8));
	memset(big_block, 7, Kilobytes(1));
	big_block = memory_realloc(big_block, Megabytes(16));
	memory_zfree(big_block);
	
	//generic reallocation of a big block where new size is less than previous
	big_block = memory_alloc(Megabytes(8));
	memset(big_block, 7, Kilobytes(1));
	big_block = memory_realloc(big_block, Megabytes(4));
	memory_zfree(big_block);
	
	//generic reallocation of a big block where new size is the same as previous
	big_block = memory_alloc(Megabytes(8));
	memset(big_block, 7, Kilobytes(1));
	big_block = memory_realloc(big_block, Megabytes(8));
	memory_zfree(big_block);
	
	//generic reallocation of small block into big block
	string1 = (char*)memory_alloc(64);
	memset(string1, '1', 32);
	string1 = (char*)memory_realloc(string1, Kilobytes(512));
	memory_zfree(string1);
	
	//generic reallocation with nothing to the right
	string1 = (char*)memory_alloc(64);
	memset(string1, '1', 32);
	string1 = (char*)memory_realloc(string1, 128);
	memory_zfree(string1);
	
	//generic reallocation with nothing to the right where new size is less than previous
	string1 = (char*)memory_alloc(64);
	memset(string1, '1', 48);
	string1 = (char*)memory_realloc(string1, 32);
	memory_zfree(string1);
	
	//generic rellocation with stuff to the right where new size is less than previous
	string1 = (char*)memory_alloc(64);
	memset(string1, '1', 48);
	string2 = (char*)memory_alloc(64);
	memset(string2, '2', 48);
	string1 = (char*)memory_realloc(string1, 32);
	string3 = (char*)memory_alloc(16);
	memset(string3, '3', 8);
	memory_zfree(string1);
	memory_zfree(string2);
	memory_zfree(string3);
	
	//generic rellocation with stuff to the right where new size is the same as previous
	string1 = (char*)memory_alloc(64);
	memset(string1, '1', 32);
	string2 = (char*)memory_alloc(64);
	memset(string2, '2', 32);
	string1 = (char*)memory_realloc(string1, 64);
	memory_zfree(string1);
	memory_zfree(string2);
	
	//generic rellocation with stuff to the right but its empty and can hold the growth
	string1 = (char*)memory_alloc(64);
	memset(string1, '1', 32);
	string2 = (char*)memory_alloc(128);
	memset(string2, '2', 32);
	string3 = (char*)memory_alloc(64);
	memset(string3, '3', 32);
	memory_zfree(string2);
	string1 = (char*)memory_realloc(string1, 128);
	memory_zfree(string1);
	memory_zfree(string3);
	
	//generic rellocation with stuff to the right and needs to move
	string1 = (char*)memory_alloc(64);
	memset(string1, '1', 32);
	string2 = (char*)memory_alloc(64);
	memset(string2, '2', 32);
	string1 = (char*)memory_realloc(string1, 128);
	memory_zfree(string1);
	memory_zfree(string2);
	
	{//// allocation info ////
#if BUILD_INTERNAL
		//alloc info array
		carray<AllocInfo> active = deshi__memory_allocinfo_active_expose();
		AssertAlways(active.count >= 3);
		for(int i=1; i<active.count-1; ++i){
			AssertAlways(active[i].address > active[i-1].address);
		}
		
		//default names
		AssertAlways(equals(cstr_lit("Arena Heap"),   deshi__memory_allocinfo_get(arena_heap).name));
		AssertAlways(equals(cstr_lit("Generic Heap"), deshi__memory_allocinfo_get(generic_heap).name));
		AssertAlways(equals(cstr_lit("Temp Arena"),   deshi__memory_allocinfo_get(temp_arena).name));
		
		//check arena heap defaults
		AllocInfo info = deshi__memory_allocinfo_get(arena_heap);
		AssertAlways(info.address == arena_heap);
		AssertAlways(info.creation_frame == 0);
		AssertAlways(info.deletion_frame == -1);
		AssertAlways(equals(info.name, cstr_lit("Arena Heap")));
		AssertAlways(info.type == Type_Heap);
		
		//changing name and type
		deshi__memory_allocinfo_set(arena_heap, cstr_lit("blah"), Type_Arena);
		info = deshi__memory_allocinfo_get(arena_heap);
		AssertAlways(info.address == arena_heap);
		AssertAlways(info.creation_frame == 0);
		AssertAlways(info.deletion_frame == -1);
		AssertAlways(equals(info.name, cstr_lit("blah")));
		AssertAlways(info.type == Type_Arena);
		
		//reset to default
		deshi__memory_allocinfo_set(arena_heap, cstr_lit("Arena Heap"), Type_Heap);
		AssertAlways(equals(cstr_lit("Arena Heap"), deshi__memory_allocinfo_get(arena_heap).name));
		AssertAlways(Type_Heap == deshi__memory_allocinfo_get(arena_heap).type);
		
		//check arena allocation defaults
		Arena* arena = memory_create_arena(Kilobytes(4));
		info = deshi__memory_allocinfo_get(arena);
		AssertAlways(info.address == arena);
		AssertAlways(info.creation_frame == DeshTime->updateCount);
		AssertAlways(info.deletion_frame == -1);
		AssertAlways(equals(info.name, cstr_lit("")));
		AssertAlways(info.type == 0);
		
		//change arena allocation
		deshi__memory_allocinfo_set(arena, cstr_lit("an arena"), Type_Arena);
		info = deshi__memory_allocinfo_get(arena);
		AssertAlways(info.address == arena);
		AssertAlways(info.creation_frame == DeshTime->updateCount);
		AssertAlways(info.deletion_frame == -1);
		AssertAlways(equals(info.name, cstr_lit("an arena")));
		AssertAlways(info.type == Type_Arena);
		
		//check after arena allocaton is freed
		memory_delete_arena(arena);
		info = deshi__memory_allocinfo_get(arena);
		AssertAlways(info.address == arena);
		AssertAlways(info.creation_frame == DeshTime->updateCount);
		AssertAlways(info.deletion_frame == DeshTime->updateCount);
		AssertAlways(equals(info.name, cstr_lit("an arena")));
		AssertAlways(info.type == Type_Arena);
		
		//check generic allocation defaults
		void* alloc = memory_alloc(sizeof(Allocator));
		info = deshi__memory_allocinfo_get(alloc);
		AssertAlways(info.address == alloc);
		AssertAlways(info.creation_frame == DeshTime->updateCount);
		AssertAlways(info.deletion_frame == -1);
		AssertAlways(equals(info.name, cstr_lit("")));
		AssertAlways(info.type == 0);
		
		//change generic allocation
		deshi__memory_allocinfo_set(alloc, cstr_lit("some allocator"), Type_Allocator);
		info = deshi__memory_allocinfo_get(alloc);
		AssertAlways(info.address == alloc);
		AssertAlways(info.creation_frame == DeshTime->updateCount);
		AssertAlways(info.deletion_frame == -1);
		AssertAlways(equals(info.name, cstr_lit("some allocator")));
		AssertAlways(info.type == Type_Allocator);
		
		//check after generic allocaton is freed
		memory_zfree(alloc);
		info = deshi__memory_allocinfo_get(alloc);
		AssertAlways(info.address == alloc);
		AssertAlways(info.creation_frame == DeshTime->updateCount);
		AssertAlways(info.deletion_frame == DeshTime->updateCount);
		AssertAlways(equals(info.name, cstr_lit("some allocator")));
		AssertAlways(info.type == Type_Allocator);
#endif //BUILD_INTERNAL
	}
	
	Log("memory-testing","Start  expecting testing errors starting here -----------------------------------------");
	Logger::PushIndent();
	{//// default to libc when running out of memory in arena heap ////
		//use up all but 1KB of arena heap for setup
		arena1 = memory_create_arena((arena_heap->size - (arena_heap->cursor - arena_heap->start)) - Kilobytes(1));
		defer{ memory_delete_arena(arena1); };
		
		//create libc arena
		arena2 = memory_create_arena(Kilobytes(32));
		memset(arena2->start, 7, 128);
		arena2->used = 128;
		
		//delete libc arena
		memory_delete_arena(arena2);
		
		//grow libc arena
		arena2 = memory_create_arena(Kilobytes(32));
		memset(arena2->start, 7, 128);
		arena2->used = 128;
		arena2 = memory_grow_arena(arena2, Kilobytes(32));
		memory_delete_arena(arena2);
		
		//grow arena into libc arena
		arena2 = memory_create_arena(256);
		memset(arena2->start, 7, 128);
		arena2->used = 128;
		arena2 = memory_grow_arena(arena2, Kilobytes(8));
		memory_delete_arena(arena2);
	}
	
	{//// default to libc when running out of memory in generic heap ////
		array<void*> setup;
		void *alloc2, *alloc3;
		
		//use up all but 1KB of generic heap for setup
		while((generic_heap->cursor + Kilobytes(63)) < (generic_heap->start + generic_heap->size)){ setup.add(memory_alloc(Kilobytes(63))); }
		while((generic_heap->cursor + Kilobytes(1)) < (generic_heap->start + generic_heap->size)){ setup.add(memory_alloc(Kilobytes(1))); }
		defer{ forE(setup){ memory_zfree(*it); } };
		
		//create libc alloc
		alloc2 = memory_alloc(Kilobytes(4));
		memset(alloc2, 7, 128);
		
		//delete libc alloc
		memory_zfree(alloc2);
		
		//grow libc alloc
		alloc2 = memory_alloc(Kilobytes(4));
		memset(alloc2, 7, 128);
		alloc3 = memory_realloc(alloc2, Kilobytes(8));
		memset((u8*)alloc3 + 128, 8, 128);
		memory_zfree(alloc3);
		
		//shrink libc alloc
		alloc2 = memory_alloc(Kilobytes(4));
		memset(alloc2, 7, 128);
		alloc3 = memory_realloc(alloc2, Kilobytes(2));
		memset((u8*)alloc3 + 128, 8, 128);
		memory_zfree(alloc3);
		
		//grow alloc into libc alloc
		alloc2 = memory_alloc(256);
		memset(alloc2, 7, 128);
		alloc3 = memory_realloc(alloc2, Kilobytes(4));
		memset((u8*)alloc3 + 128, 8, 128);
		memory_zfree(alloc3);
	}
	
	{//// default to libc when running out of memory in temp arena ////
		memory_clear_temp();
		void *alloc1, *alloc2, *alloc3;
		
		//use up all but 1KB of temp arena for setup
		alloc1 = memory_talloc((temp_arena->size - (temp_arena->cursor - temp_arena->start)) - Kilobytes(1));
		defer{ memory_clear_temp(); };
		
		//create libc temp alloc
		alloc2 = memory_talloc(Kilobytes(4));
		memset(alloc2, 7, 128);
		free((upt*)alloc2 - 1);
		
		//grow libc temp alloc
		alloc2 = memory_talloc(Kilobytes(4));
		memset(alloc2, 7, 128);
		alloc3 = memory_trealloc(alloc2, Kilobytes(8));
		free((upt*)alloc3 - 1);
		
		//shrink libc temp alloc
		alloc2 = memory_talloc(Kilobytes(4));
		memset(alloc2, 7, 128);
		alloc3 = memory_trealloc(alloc2, Kilobytes(2));
		free((upt*)alloc3 - 1);
		
		//grow temp alloc into libc temp alloc
		alloc2 = memory_talloc(256);
		memset(alloc2, 7, 128);
		alloc3 = memory_trealloc(alloc2, Kilobytes(4));
		free((upt*)alloc3 - 1);
	}
	Logger::PopIndent();
	Log("memory-testing","Finish expecting testing errors starting here -----------------------------------------");
	
	DESHI_TEST_CORE_TODO("memory");
}

#include "renderer.h"
local void TEST_deshi_core_renderer(){
	DESHI_TEST_CORE_TODO("renderer");
}

#include "storage.h"
local void TEST_deshi_core_storage(){
	DESHI_TEST_CORE_TODO("storage");
}

#include "time.h"
local void TEST_deshi_core_time(){
	DESHI_TEST_CORE_TODO("time");
}

#include "ui.h"
local void TEST_deshi_core_ui(){
	DESHI_TEST_CORE_TODO("ui");
}

#include "window.h"
local void TEST_deshi_core_window(){
	DESHI_TEST_CORE_TODO("window");
}

local void TEST_deshi_core(){
	TEST_deshi_core_armature();
	TEST_deshi_core_camera();
	TEST_deshi_core_commands();
	TEST_deshi_core_console();
	TEST_deshi_core_font();
	TEST_deshi_core_input();
	TEST_deshi_core_io();
	TEST_deshi_core_logging();
	TEST_deshi_core_memory();
	TEST_deshi_core_renderer();
	TEST_deshi_core_storage();
	TEST_deshi_core_time();
	TEST_deshi_core_ui();
	TEST_deshi_core_window();
}