#include "MeshComp.h"
#include "../admin.h"
#include "../Transform.h"
#include "../../core/renderer.h"
#include "../../scene/Model.h"
#include "../../scene/Scene.h"

MeshComp::MeshComp() {
	admin = g_admin;
	cpystr(name, "MeshComp", DESHI_NAME_SIZE);
	sender = new Sender();
	layer = ComponentLayer_Canvas;
	comptype = ComponentType_MeshComp;
	
	this->mesh = 0;
	this->meshID = 0;
	this->instanceID = -1;
}

MeshComp::MeshComp(u32 meshID, u32 instanceID) {
	admin = g_admin;
	cpystr(name, "MeshComp", DESHI_NAME_SIZE);
	sender = new Sender();
	layer = ComponentLayer_Canvas;
	comptype = ComponentType_MeshComp;
	
	
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
	return TOSTRING("\n>mesh"
					"\nid      ", meshID,
					"\nname    \"", Render::MeshName(meshID) , "\""
					"\nvisible ", (mesh_visible) ? "true" : "false",
					"\n");
}

void MeshComp::LoadDESH(Admin* admin, const char* data, u32& cursor, u32 count){
	u32 entityID = -1, compID = -1, event = -1;
	u32 meshID = -1, instanceID = -1;
	forI(count){
		memcpy(&entityID, data+cursor, sizeof(u32)); cursor += sizeof(u32);
		if(entityID >= admin->entities.size()) {
			ERROR("Failed to load mesh component at pos '", cursor-sizeof(u32),
				  "' because it has an invalid entity ID: ", entityID); continue;
		}
		memcpy(&compID, data + cursor, sizeof(u32)); cursor += sizeof(u32);
		memcpy(&event, data + cursor, sizeof(u32)); cursor += sizeof(u32);
		
		memcpy(&instanceID, data+cursor, sizeof(u32)); cursor += sizeof(u32);
		memcpy(&meshID,     data+cursor, sizeof(u32)); cursor += sizeof(u32);
		if(meshID >= Render::MeshCount()){
			ERROR("Failed to load mesh component at pos '", cursor-sizeof(u32),
				  "' because it has an invalid mesh ID: ", meshID); continue;
		}
		
		MeshComp* c = new MeshComp(meshID, instanceID);
		memcpy(&c->mesh_visible,   data+cursor, sizeof(b32)); cursor += sizeof(b32);
		memcpy(&c->ENTITY_CONTROL, data+cursor, sizeof(b32)); cursor += sizeof(b32);
		EntityAt(entityID)->AddComponent(c);
		c->SetCompID(compID);
		c->SetEvent(event);
		c->layer_index = admin->freeCompLayers[c->layer].add(c);
	}
}

////////////////////
//// MeshComp2D ////
////////////////////


MeshComp2D::MeshComp2D(u32 id) {
	admin = g_admin;
	cpystr(name, "MeshComp", DESHI_NAME_SIZE);
	sender = new Sender();
	layer = ComponentLayer_Canvas;
	
	this->twodID = id;
}

void MeshComp2D::Init() {
	Render::UpdateMeshVisibility(twodID, visible);
}

void MeshComp2D::ToggleVisibility() {
	visible = !visible;
	Render::UpdateMeshVisibility(twodID, visible);
}

void MeshComp2D::ReceiveEvent(Event event) {}

void MeshComp2D::Update() {}

std::string MeshComp2D::SaveTEXT(){
	return TOSTRING("\n>mesh"
					"\nname    \"", Render::MeshName(twodID), "\""
					"\nvisible ", (visible) ? "true" : "false",
					"\n");
}

void MeshComp2D::LoadDESH(Admin* admin, const char* data, u32& cursor, u32 count){
	ERROR("MeshComp2D::Load not setup");
}