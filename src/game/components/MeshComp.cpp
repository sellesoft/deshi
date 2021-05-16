#include "MeshComp.h"
#include "../../core.h"
#include "../admin.h"
#include "../Transform.h"
#include "../../scene/Model.h"
#include "../../scene/Scene.h"

#include "../systems/CanvasSystem.h"
#include "../../geometry/Geometry.h"

MeshComp::MeshComp() {
	admin = g_admin;
	cpystr(name, "MeshComp", 63);
	sender = new Sender();
	layer = ComponentLayer_Canvas;
	comptype = ComponentType_MeshComp;
	
	this->mesh = 0;
	this->meshID = 0;
	this->instanceID = -1;
}

MeshComp::MeshComp(u32 meshID, u32 instanceID) {
	admin = g_admin;
	cpystr(name, "MeshComp", 63);
	sender = new Sender();
	layer = ComponentLayer_Canvas;
	comptype = ComponentType_MeshComp;
	
	
	this->meshID = meshID;
	this->instanceID = instanceID;
	this->mesh = DengRenderer->GetMeshPtr(meshID);
}

MeshComp::MeshComp(Mesh* m, u32 meshID, u32 instanceID) {
	admin = g_admin;
	cpystr(name, "MeshComp", 63);
	sender = new Sender();
	layer = ComponentLayer_Canvas;
	comptype = ComponentType_MeshComp;
	
	this->mesh = m;
	this->meshID = meshID;
	this->instanceID = instanceID;
}

MeshComp::~MeshComp() {
	DengRenderer->RemoveMesh(meshID);
}

void MeshComp::ToggleVisibility() {
	mesh_visible = !mesh_visible;
	DengRenderer->UpdateMeshVisibility(meshID, mesh_visible);
}

void MeshComp::Visible(bool visible) {
	mesh_visible = visible;
	DengRenderer->UpdateMeshVisibility(meshID, mesh_visible);
}

void MeshComp::ReceiveEvent(Event event) {
	switch (event) {
		case TEST_EVENT:
		LOG("MeshComp receieved event");
		break;
	}
}

//this should only be used when the entity is not controlling the Mesh
void MeshComp::UpdateMeshTransform(Vector3 position, Vector3 rotation, Vector3 scale) {
	DengRenderer->UpdateMeshMatrix(meshID, Matrix4::TransformationMatrix(position, rotation, scale));
}

void MeshComp::ChangeMesh(u32 newMeshIdx){
	if(newMeshIdx == meshID) return;
	if(newMeshIdx > DengRenderer->meshes.size()) return ERROR("ChangeMesh: There is no mesh with index: ", newMeshIdx);
	if(!DengRenderer->meshes[newMeshIdx].base) return ERROR("ChangeMesh: You can only change the mesh to a base mesh");
	
	Matrix4 oldMat = DengRenderer->GetMeshMatrix(meshID);
	DengRenderer->RemoveMesh(meshID);
	meshID = DengRenderer->CreateMesh(newMeshIdx, oldMat);
	mesh = DengRenderer->GetMeshPtr(meshID);
}

void MeshComp::ChangeMaterialShader(u32 s) {
	std::vector<u32> ids = DengRenderer->GetMaterialIDs(meshID);
	for (u32 id : ids) {
		DengRenderer->UpdateMaterialShader(id, s);
	}
}

void MeshComp::ChangeMaterialTexture(u32 t) {
	std::vector<u32> ids = DengRenderer->GetMaterialIDs(meshID);
	
	for (u32 id : ids) {
		DengRenderer->UpdateMaterialTexture(id, 0, t);
	}
}

template<class T>
bool isthisin(T test, std::vector<T> vec) {
	for (T t : vec) if (test == t) return true;
	return false;
}

void MeshComp::Init() {
	DengRenderer->UpdateMeshVisibility(meshID, mesh_visible);
}

void MeshComp::Update() {
	ASSERT(mesh->vertexCount, "Mesh has no vertices");
	if (g_admin->selectedEntity == EntityAt(entityID)) {
		if(g_admin->fast_outline){
			DengRenderer->SetSelectedMesh(meshID);
		}else{
			std::vector<Vector2> outline = mesh->GenerateOutlinePoints(EntityAt(entityID)->transform.TransformMatrix(), DengCamera->projMat, DengCamera->viewMat, DengWindow->dimensions, admin->mainCamera->position);
			for (int i = 0; i < outline.size(); i += 2) {
				ImGui::DebugDrawLine(outline[i], outline[i + 1], Color::CYAN);
			}
		}
	}
	
	//update mesh's transform with entities tranform
	if(ENTITY_CONTROL) DengRenderer->UpdateMeshMatrix(meshID, EntityAt(entityID)->transform.TransformMatrix());
}

std::vector<char> MeshComp::Save() {
	std::vector<char> out;
	return out;
}

void MeshComp::Load(EntityAdmin* admin, const char* data, u32& cursor, u32 count){
	u32 entityID = 0xFFFFFFFF;
	u32 meshID = 0xFFFFFFFF;
	u32 instanceID = 0xFFFFFFFF;
	for_n(i,count){
		memcpy(&entityID, data+cursor, sizeof(u32)); cursor += sizeof(u32);
		if(entityID >= admin->entities.size()) {
			ERROR("Failed to load mesh component at pos '", cursor-sizeof(u32),
				  "' because it has an invalid entity ID: ", entityID); continue;
		}
		
		memcpy(&instanceID, data+cursor, sizeof(u32)); cursor += sizeof(u32);
		memcpy(&meshID,     data+cursor, sizeof(u32)); cursor += sizeof(u32);
		if(meshID >= DengRenderer->meshes.size()){
			ERROR("Failed to load mesh component at pos '", cursor-sizeof(u32),
				  "' because it has an invalid mesh ID: ", meshID); continue;
		}
		
		MeshComp* c = new MeshComp(meshID, instanceID);
		memcpy(&c->mesh_visible,   data+cursor, sizeof(b32)); cursor += sizeof(b32);
		memcpy(&c->ENTITY_CONTROL, data+cursor, sizeof(b32)); cursor += sizeof(b32);
		EntityAt(entityID)->AddComponent(c);
		c->layer_index = admin->freeCompLayers[c->layer].add(c);
	}
}

////////////////////
//// MeshComp2D ////
////////////////////


MeshComp2D::MeshComp2D(u32 id) {
	admin = g_admin;
	cpystr(name, "MeshComp", 63);
	sender = new Sender();
	layer = ComponentLayer_Canvas;
	
	this->twodID = id;
}

void MeshComp2D::Init() {
	DengRenderer->UpdateMeshVisibility(twodID, visible);
}

void MeshComp2D::ToggleVisibility() {
	visible = !visible;
	DengRenderer->UpdateMeshVisibility(twodID, visible);
}

void MeshComp2D::ReceiveEvent(Event event) {}

void MeshComp2D::Update() {}

std::vector<char> MeshComp2D::Save() {
	std::vector<char> out;
	return out;
}

void MeshComp2D::Load(EntityAdmin* admin, const char* data, u32& cursor, u32 count){
	ERROR("MeshComp2D::Load not setup");
}