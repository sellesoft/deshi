#include "MeshComp.h"
#include "../admin.h"
#include "../Transform.h"
#include "../../core/renderer.h"
#include "../../core/storage.h"
#include "../../core/console.h"
#include "../../utils/debug.h"

ModelInstance::ModelInstance(){
	type  = ComponentType_ModelInstance;
	layer = ComponentLayer_Canvas;
	model     = Storage::CopyModel(Storage::NullModel()).second;
	mesh      = model->mesh;
	armature  = model->armature;
	transform = mat4::IDENTITY;
	visible   = true;
	override  = false;
}

ModelInstance::ModelInstance(Model* _model){
	type  = ComponentType_ModelInstance;
	layer = ComponentLayer_Canvas;
	model     = Storage::CopyModel(_model).second;
	mesh      = model->mesh;
	armature  = model->armature;
	transform = mat4::IDENTITY;
	visible   = true;
	override  = false;
}

ModelInstance::ModelInstance(Mesh* _mesh){
	type  = ComponentType_ModelInstance;
	layer = ComponentLayer_Canvas;
	model     = Storage::CreateModelFromMesh(_mesh).second;
	mesh      = model->mesh;
	armature  = model->armature;
	transform = mat4::IDENTITY;
	visible   = true;
	override  = false;
}

ModelInstance::~ModelInstance(){
	Storage::DeleteModel(model);
}

void ModelInstance::ChangeModel(Model* _model){
	Storage::DeleteModel(model);
	model    = Storage::CopyModel(_model).second;
	mesh     = model->mesh;
	armature = model->armature;
}

void ModelInstance::ChangeModel(Mesh* _mesh){
	Storage::DeleteModel(model);
	model    = Storage::CreateModelFromMesh(_mesh).second;
	mesh     = model->mesh;
	armature = model->armature;
}

void ModelInstance::ReceiveEvent(Event event){
	if(event == Event_ModelVisibleToggle){
		ToggleVisibility();
	}
}

void ModelInstance::Update(){
	if(!override) transform = entity->transform.TransformMatrix();
	if(visible) Render::DrawModel(model, transform);
}

std::string ModelInstance::SaveTEXT(){
	return TOSTDSTRING("\n>mesh"
					"\nname     \"", model->name,"\""
					"\nvisible  ", (visible) ? "true" : "false",
					"\noverride ", (override) ? "true" : "false",
					"\n");
	//!Incomplete convert all meshcomp saves
}

void ModelInstance::LoadDESH(Admin* admin, const char* data, u32& cursor, u32 count){
	ERROR("ModelInstance::LoadDESH not setup");
}
