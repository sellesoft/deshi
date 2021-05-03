#include "MeshComp.h"
#include "../../core.h"
#include "../Transform.h"
#include "../../scene/Model.h"
#include "../../scene/Scene.h"

#include "../systems/CanvasSystem.h"
#include "../../geometry/Geometry.h"

#include "../../EntityAdmin.h"

MeshComp::MeshComp() {
	this->mesh = 0;
	this->meshID = -1;
	this->instanceID = -1;
	cpystr(name, "MeshComp", 63);
	send = new Sender();
	layer = ComponentLayer_Canvas;
}

MeshComp::MeshComp(u32 meshID, u32 instanceID) {
	this->mesh = DengRenderer->GetMeshPtr(meshID);
	this->meshID = meshID;
	this->instanceID = instanceID;
	
	cpystr(name, "MeshComp", 63);
	send = new Sender();
	layer = ComponentLayer_Canvas;
}

MeshComp::MeshComp(Mesh* m, u32 meshID, u32 instanceID) {
	this->mesh = m;
	this->meshID = meshID;
	this->instanceID = instanceID;
	
	cpystr(name, "MeshComp", 63);
	send = new Sender();
	layer = ComponentLayer_Canvas;
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

//this should only be used when the entity is not controlling the Mesh
void MeshComp::UpdateMeshTransform(Vector3 position, Vector3 rotation, Vector3 scale) {
	DengRenderer->UpdateMeshMatrix(meshID, Matrix4::TransformationMatrix(position, rotation, scale));
}

template<class T>
bool isthisin(T test, std::vector<T> vec) {
	for (T t : vec) if (test == t) return true;
	return false;
}

void MeshComp::Init(EntityAdmin* a) {
	admin = a;
}

void MeshComp::Update() {
	ASSERT(mesh->vertexCount, "Mesh has no vertices");
	if (g_admin->selectedEntity == &admin->entities[entityID]) {
		std::vector<Vector2> outline = mesh->GenerateOutlinePoints(admin->entities[entityID].transform.TransformMatrix(), DengCamera->projMat, DengCamera->viewMat, DengWindow->dimensions, admin->mainCamera->position);
		for (int i = 0; i < outline.size(); i += 2) {
			ImGui::DebugDrawLine(outline[i], outline[i + 1], Color::CYAN);
		}
	}
	
	
	//update mesh's transform with entities tranform
	if(ENTITY_CONTROL) DengRenderer->UpdateMeshMatrix(meshID, admin->entities[entityID].transform.TransformMatrix());
}

std::vector<char> MeshComp::Save() {
	std::vector<char> out;
	return out;
}

void MeshComp::Load(std::vector<Entity>& entities, const char* data, u32& cursor, u32 count){
	u32 entityID = 0xFFFFFFFF;
	for_n(i,count){
		memcpy(&entityID, data+cursor, sizeof(u32)); 
		cursor += sizeof(u32);
		if(entityID >= entities.size()) {
			ERROR("Failed to load mesh component at pos '", cursor-sizeof(u32),
				  "' because it has an invalid entity ID: ", entityID);
			continue;
		}
		
		MeshComp* mc = new MeshComp();
		memcpy(&mc->instanceID,     data+cursor, sizeof(u32)); cursor += sizeof(u32);
		memcpy(&mc->meshID,         data+cursor, sizeof(u32)); cursor += sizeof(u32);
		memcpy(&mc->mesh_visible,   data+cursor, sizeof(b32)); cursor += sizeof(b32);
		memcpy(&mc->ENTITY_CONTROL, data+cursor, sizeof(b32)); cursor += sizeof(b32);
		mc->mesh = DengRenderer->GetMeshPtr(mc->meshID);
		entities[entityID].AddComponent(mc);
		DengRenderer->UpdateMeshVisibility(mc->meshID, mc->mesh_visible);
		DengRenderer->UpdateMeshMatrix(mc->meshID, entities[entityID].transform.TransformMatrix());
	}
}

////////////////////
//// MeshComp2D ////
////////////////////


MeshComp2D::MeshComp2D(u32 id) {
	this->twodID = id;
	
	cpystr(name, "MeshComp", 63);
	send = new Sender();
	layer = ComponentLayer_Canvas;
}

void MeshComp2D::Init(EntityAdmin* a) {
	admin = a;
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

void MeshComp2D::Load(std::vector<Entity>& entities, const char* data, u32& cursor, u32 count){}