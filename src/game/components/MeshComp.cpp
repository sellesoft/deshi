#include "MeshComp.h"
#include "../admin.h"
#include "../Transform.h"
#include "../../core/renderer.h"
#include "../../scene/Model.h"
#include "../../scene/Scene.h"

MeshComp::MeshComp() {
	layer = ComponentLayer_Canvas;
	type  = ComponentType_MeshComp;
	
	this->mesh = 0;
	this->meshID = 0;
	this->instanceID = -1;
}

MeshComp::MeshComp(u32 meshID, u32 instanceID) {
	layer = ComponentLayer_Canvas;
	type = ComponentType_MeshComp;
	
	
	this->meshID = meshID;
	this->instanceID = instanceID;
	this->mesh = Render::GetMeshPtr(meshID);
}

MeshComp::~MeshComp() {
	Render::RemoveMesh(meshID);
}

void MeshComp::ToggleVisibility() {
	mesh_visible = !mesh_visible;
	Render::UpdateMeshVisibility(meshID, mesh_visible);
}

void MeshComp::Visible(bool visible) {
	mesh_visible = visible;
	Render::UpdateMeshVisibility(meshID, mesh_visible);
}

void MeshComp::ReceiveEvent(Event event) {
	switch (event) {}
}

//this should only be used when the entity is not controlling the Mesh
void MeshComp::UpdateMeshTransform(Vector3 position, Vector3 rotation, Vector3 scale) {
	Render::UpdateMeshMatrix(meshID, Matrix4::TransformationMatrix(position, rotation, scale));
}

void MeshComp::ChangeMesh(u32 newMeshIdx){
	if(newMeshIdx == meshID) return;
	if(newMeshIdx > Render::MeshCount()) return ERROR("ChangeMesh: There is no mesh with index: ", newMeshIdx);
	if(!Render::IsBaseMesh(newMeshIdx)) return ERROR("ChangeMesh: You can only change the mesh to a base mesh");
	
	Matrix4 oldMat = Render::GetMeshMatrix(meshID);
	Render::RemoveMesh(meshID);
	meshID = Render::CreateMesh(newMeshIdx, oldMat);
	mesh = Render::GetMeshPtr(meshID);
}

void MeshComp::ChangeMaterialShader(u32 s) {
	std::vector<u32> ids = Render::GetMaterialIDs(meshID);
	for (u32 id : ids) {
		Render::UpdateMaterialShader(id, s);
	}
}

void MeshComp::ChangeMaterialTexture(u32 t) {
	std::vector<u32> ids = Render::GetMaterialIDs(meshID);
	
	for (u32 id : ids) {
		Render::UpdateMaterialTexture(id, 0, t);
	}
}

void MeshComp::Init() {
	Render::UpdateMeshVisibility(meshID, mesh_visible);
}

void MeshComp::Update() {
	//update mesh's transform with entities tranform
	if(ENTITY_CONTROL) Render::UpdateMeshMatrix(meshID, entity->transform.TransformMatrix());
}

std::string MeshComp::SaveTEXT(){
	return TOSTDSTRING("\n>mesh"
					"\nid      ", meshID,
					"\nname    \"", Render::MeshName(meshID) , "\""
					"\nvisible ", (mesh_visible) ? "true" : "false",
					"\n");
}

void MeshComp::LoadDESH(Admin* admin, const char* data, u32& cursor, u32 count){
	ERROR("MeshComp::LoadDESH not setup");
}
