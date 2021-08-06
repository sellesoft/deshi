#include "UndoManager.h"
#include "Transform.h"
#include "../core/input.h"
#include "../core/console.h"

void UndoManager::Init(){}

void UndoManager::Reset(){
	SUCCESS("Resetting undos/redos");
	undos.clear();
	redos.clear();
}

//select data layout:
//0x00  void*      | selected entity pointer
//0x08  void*      | old selection
//0x10  void*      | new selection
void UndoManager::AddUndoSelect(void** sel, void* oldEnt, void* newEnt){
	Assert(sizeof(u32)*2 == sizeof(void*), "assume ptr is 8 bytes");
	EditAction edit; edit.type = EditActionType_Select;
	memcpy(edit.data + 0, &sel,    sizeof(u32)*2);
	memcpy(edit.data + 2, &oldEnt, sizeof(u32)*2);
	memcpy(edit.data + 4, &newEnt, sizeof(u32)*2);
	undos.push_back(edit);
	redos.clear();
}
void UndoSelect(EditAction* edit){
	void** sel;
	memcpy(&sel, ((u32*)edit->data) + 0, sizeof(u32)*2);
	memcpy( sel, ((u32*)edit->data) + 2, sizeof(u32)*2);
}
void RedoSelect(EditAction* edit){
	void** sel;
	memcpy(&sel, ((u32*)edit->data) + 0, sizeof(u32)*2);
	memcpy( sel, ((u32*)edit->data) + 4, sizeof(u32)*2);
}

//translate data layout:
//0x00  Transform* | transform
//0x08  vec3       | old position
//0x14  vec3       | new position
void UndoManager::AddUndoTranslate(Transform* t, vec3* oldPos, vec3* newPos){
	Assert(sizeof(u32)*2 == sizeof(void*), "assume ptr is 8 bytes");
	EditAction edit; edit.type = EditActionType_Translate;
	memcpy(edit.data + 0, &t,     sizeof(u32)*2);
	memcpy(edit.data + 2, oldPos, sizeof(u32)*3);
	memcpy(edit.data + 5, newPos, sizeof(u32)*3);
	undos.push_back(edit);
	redos.clear();
}
void UndoTranslate(EditAction* edit){
	Transform* t; memcpy(&t, edit->data, sizeof(u32)*2);
	t->position = vec3(((f32*)edit->data) + 2);
}
void RedoTranslate(EditAction* edit){
	Transform* t; memcpy(&t, edit->data, sizeof(u32)*2);
	t->position = vec3(((f32*)edit->data) + 5);
}

//rotate data layout:
//0x00  Transform* | transform
//0x08  vec3       | old rotation
//0x14  vec3       | new rotation
void UndoManager::AddUndoRotate(Transform* t, vec3* oldRot, vec3* newRot){
	Assert(sizeof(u32)*2 == sizeof(void*), "assume ptr is 8 bytes");
	EditAction edit; edit.type = EditActionType_Rotate;
	memcpy(edit.data + 0, &t,     sizeof(u32)*2);
	memcpy(edit.data + 2, oldRot, sizeof(u32)*3);
	memcpy(edit.data + 5, newRot, sizeof(u32)*3);
	undos.push_back(edit);
	redos.clear();
}
void UndoRotate(EditAction* edit){
	Transform* t; memcpy(&t, edit->data, sizeof(u32)*2);
	t->rotation = vec3(((f32*)edit->data) + 2);
}
void RedoRotate(EditAction* edit){
	Transform* t; memcpy(&t, edit->data, sizeof(u32)*2);
	t->rotation = vec3(((f32*)edit->data) + 5);
}

//scale data layout:
//0x00  Transform* | transform
//0x08  vec3       | old scale
//0x14  vec3       | new scale
void UndoManager::AddUndoScale(Transform* t, vec3* oldScale, vec3* newScale){
	Assert(sizeof(u32)*2 == sizeof(void*), "assume ptr is 8 bytes");
	EditAction edit; edit.type = EditActionType_Scale;
	memcpy(edit.data + 0, &t,       sizeof(u32)*2);
	memcpy(edit.data + 2, oldScale, sizeof(u32)*3);
	memcpy(edit.data + 5, newScale, sizeof(u32)*3);
	undos.push_back(edit);
	redos.clear();
}
void UndoScale(EditAction* edit){
	Transform* t; memcpy(&t, edit->data, sizeof(u32)*2);
	t->scale = vec3(((f32*)edit->data) + 2);
}
void RedoScale(EditAction* edit){
	Transform* t; memcpy(&t, edit->data, sizeof(u32)*2);
	t->scale = vec3(((f32*)edit->data) + 5);
}

//create data layout:
void UndoManager::AddUndoCreate(){
	
}
void UndoCreate(EditAction* edit){
	
}
void RedoCreate(EditAction* edit){
	
}

//delete data layout:
void UndoManager::AddUndoDelete(){
	
}
void UndoDelete(EditAction* edit){
	
}
void RedoDelete(EditAction* edit){
	
}

void UndoManager::Undo(u32 count){
	forI((count < undos.size()) ? count : undos.size()){
		u32 n = undos.size()-i-1;
		switch(undos[n].type){
			case(EditActionType_Select):   { UndoSelect(&undos[n]);    }break;
			case(EditActionType_Translate):{ UndoTranslate(&undos[n]); }break;
			case(EditActionType_Rotate):   { UndoRotate(&undos[n]);    }break;
			case(EditActionType_Scale):    { UndoScale(&undos[n]);     }break;
			case(EditActionType_Create):   { UndoCreate(&undos[n]);    }break;
			case(EditActionType_Delete):   { UndoDelete(&undos[n]);    }break;
		}
		redos.push_back(undos.back());
		undos.pop_back();
	}
}

void UndoManager::Redo(u32 count){
	forI((count < redos.size()) ? count : redos.size()){
		u32 n = redos.size()-i-1;
		switch(redos[n].type){
			case(EditActionType_Select):   { RedoSelect(&redos[n]);    }break;
			case(EditActionType_Translate):{ RedoTranslate(&redos[n]); }break;
			case(EditActionType_Rotate):   { RedoRotate(&redos[n]);    }break;
			case(EditActionType_Scale):    { RedoScale(&redos[n]);     }break;
			case(EditActionType_Create):   { RedoCreate(&redos[n]);    }break;
			case(EditActionType_Delete):   { RedoDelete(&redos[n]);    }break;
		}
		undos.push_back(redos.back());
		redos.pop_back();
	}
}