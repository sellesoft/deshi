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
	model    = DengScene->NullModel();
	mesh     = model->mesh;
	armature = model->armature;
	visible  = true;
}

ModelInstance::ModelInstance(Model* _model){
	type  = ComponentType_ModelInstance;
	layer = ComponentLayer_Canvas;
	model    = _model;
	mesh     = model->mesh;
	armature = model->armature;
	visible  = true;
}

ModelInstance::~ModelInstance(){
	DengScene->DeleteModel(model);
}

void ModelInstance::ToggleVisibility(){
	visible = !visible;
}

void ModelInstance::ReceiveEvent(Event event){}

void ModelInstance::Init(){}

void ModelInstance::Update(){
	if(visible){
		Render::DrawModel(model, entity->transform.TransformMatrix());
	}
}

std::string ModelInstance::SaveTEXT(){
	return TOSTRING("\n>mesh"
					"\nname    \"", model->name , "\""
					"\nvisible ", (visible) ? "true" : "false",
					"\n");
}

void ModelInstance::LoadDESH(Admin* admin, const char* data, u32& cursor, u32 count){
	ERROR("ModelInstance::LoadDESH not setup");
}
