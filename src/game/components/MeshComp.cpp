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
	model     = Storage::NullModel();
	mesh      = model->mesh;
	armature  = model->armature;
	transform = mat4::IDENTITY;
	visible   = true;
	control   = false;
}

ModelInstance::ModelInstance(Model* _model){
	type  = ComponentType_ModelInstance;
	layer = ComponentLayer_Canvas;
	model     = _model;
	mesh      = model->mesh;
	armature  = model->armature;
	transform = mat4::IDENTITY;
	visible   = true;
	control   = false;
}

ModelInstance::ModelInstance(Mesh* _mesh){
	type  = ComponentType_ModelInstance;
	layer = ComponentLayer_Canvas;
	model     = Storage::CreateModelFromMesh(_mesh).second;
	mesh      = model->mesh;
	armature  = model->armature;
	transform = mat4::IDENTITY;
	visible   = true;
	control   = false;
}

ModelInstance::~ModelInstance(){}

void ModelInstance::ChangeModel(Model* _model){
	model    = _model;
	mesh     = model->mesh;
	armature = model->armature;
}

void ModelInstance::ChangeModel(Mesh* _mesh){
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
	if(!control) transform = entity->transform.TransformMatrix();
	if(visible)  Render::DrawModel(model, transform);
}

std::string ModelInstance::SaveTEXT(){
	return TOSTDSTRING("\n>mesh"
					   "\nname    \"", model->name,"\""
					   "\nvisible ", (visible) ? "true" : "false",
					   "\ncontrol ", (control) ? "true" : "false",
					   "\n");
}

void ModelInstance::LoadDESH(Admin* admin, const char* data, u32& cursor, u32 count){
	ERROR("ModelInstance::LoadDESH not setup");
}
