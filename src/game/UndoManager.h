#pragma once
#ifndef GAME_UNDOMANAGER_H
#define GAME_UNDOMANAGER_H

#include "../utils/defines.h"
#include <deque>

struct Transform;
struct Vector3;

enum struct EditActionType : u32{
	NONE, SELECT, TRANSLATE, ROTATE, SCALE, CREATE, DELETE
};

struct EditAction{ //48 bytes
	EditActionType type;
	u32 data[11];
};

//TODO(delle) handle going over MEMORY_LIMIT
//TODO(delle,Op) maybe use a vector with fixed size and store redos at back and use swap rather than construction/deletion
struct UndoManager{
	const u64 MEMORY_LIMIT = 8 * (1024*1024); //8MB = ~1 million undos
	
	std::deque<EditAction> undos = std::deque<EditAction>();
	std::deque<EditAction> redos = std::deque<EditAction>();
	
	void Init();
	void Reset();
	
	void AddUndoSelect(void** sel, void* oldEnt, void* newEnt);
	void AddUndoTranslate(Transform* t, Vector3* oldPos, Vector3* newPos);
	void AddUndoRotate(Transform* t, Vector3* oldPos, Vector3* newPos);
	void AddUndoScale(Transform* t, Vector3* oldPos, Vector3* newPos);
	void AddUndoCreate();
	void AddUndoDelete();
	
	void Undo(u32 count = 1);
	void Redo(u32 count = 1);
};

#endif //GAME_UNDOMANAGER_H
