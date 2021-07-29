#include "MeshComp.h"
#include "../admin.h"
#include "../Transform.h"
#include "../../core/renderer.h"
#include "../../core/scene.h"
#include "../../core/console.h"
#include "../../utils/debug.h"

ModelInstance::ModelInstance(){
	type  = ComponentType_ModelInstance;
	layer = ComponentLayer_Canvas;
	model     = DengScene->CopyModel(DengScene->NullModel()).second;
	mesh      = model->mesh;
	armature  = model->armature;
	transform = Matrix4::IDENTITY;
	visible   = true;
	override  = false;
}

ModelInstance::ModelInstance(Model* _model){
	type  = ComponentType_ModelInstance;
	layer = ComponentLayer_Canvas;
	model     = DengScene->CopyModel(_model).second;
	mesh      = model->mesh;
	armature  = model->armature;
	transform = Matrix4::IDENTITY;
	visible   = true;
	override  = false;
}

ModelInstance::ModelInstance(Mesh* _mesh){
	type  = ComponentType_ModelInstance;
	layer = ComponentLayer_Canvas;
	model     = DengScene->CreateModelFromMesh(_mesh).second;
	mesh      = model->mesh;
	armature  = model->armature;
	transform = Matrix4::IDENTITY;
	visible   = true;
	override  = false;
}

ModelInstance::~ModelInstance(){
	DengScene->DeleteModel(model);
}

void ModelInstance::ChangeModel(Model* _model){
	DengScene->DeleteModel(model);
	model    = DengScene->CopyModel(_model).second;
	mesh     = model->mesh;
	armature = model->armature;
}

void ModelInstance::ChangeModel(Mesh* _mesh){
	DengScene->DeleteModel(model);
	model    = DengScene->CreateModelFromMesh(_mesh).second;
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
	return TOSTRING("\n>mesh"
					"\nname     \"", model->name,"\""
					"\nchecksum ", model->checksum,
					"\nvisible  ", (visible) ? "true" : "false",
					"\noverride ", (override) ? "true" : "false",
					"\n");
	//!Incomplete convert all meshcomp saves
}

void ModelInstance::LoadDESH(Admin* admin, const char* data, u32& cursor, u32 count){
	ERROR("ModelInstance::LoadDESH not setup");
}
