#include "MeshComp.h"
#include "../../core.h"
#include "../Transform.h"
#include "../../scene/Model.h"
#include "../../scene/Scene.h"

#include "../../EntityAdmin.h"

MeshComp::MeshComp() {
	cpystr(name, "MeshComp", 63);
	send = new Sender();
	layer = CL1_RENDCANVAS;
	sortid = 5;
}

MeshComp::MeshComp(Mesh* m, u32 meshID, u32 instanceID) {
	this->mesh = m;
	this->meshID = meshID;
	this->instanceID = instanceID;
	
	cpystr(name, "MeshComp", 63);
	send = new Sender();
	layer = CL1_RENDCANVAS;
	sortid = 5;
}

void MeshComp::ToggleVisibility() {
	mesh_visible = !mesh_visible;
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

std::string MeshComp::Save() {
	return TOSTRING("meshID: ", meshID, "\n",
					"mesh_visible: ", mesh_visible, "\n",
					"ENTITY_CONTROL: ", ENTITY_CONTROL, "\n");
}

//this should only be used when the entity is not controlling the Mesh
void MeshComp::UpdateMeshTransform(Vector3 position, Vector3 rotation, Vector3 scale) {
	DengRenderer->UpdateMeshMatrix(meshID, Matrix4::TransformationMatrix(position, rotation, scale));
}

void MeshComp::Update() {
	//update mesh's transform with entities tranform
	if(ENTITY_CONTROL) DengRenderer->UpdateMeshMatrix(meshID, entity->transform.TransformMatrix());
}

void MeshComp::Load(std::vector<Entity>& entities, const char* data, u32& cursor, u32 count){
	u32 entityID = 0xFFFFFFFF;
	u32 instanceID = -1;
	u32 meshID = -1;
	b32 visible = 0;
	b32 entity_control = 0;
	for_n(i,count){
		memcpy(&entityID, data+cursor, sizeof(u32)); 
		cursor += sizeof(u32);
		if(entityID >= entities.size()) {
			ERROR("Failed to load mesh component at pos '", cursor-sizeof(u32),
				  "' because it has an invalid entity ID: ", entityID);
			continue;
		}
		
		memcpy(&instanceID, data+cursor, sizeof(u32)*2 + sizeof(b32)*2); 
		cursor += sizeof(u32)*2 + sizeof(b32)*2;
		
		MeshComp* mc = new MeshComp();
		mc->mesh = DengRenderer->GetMeshPtr(meshID);
		mc->instanceID = instanceID;
		mc->meshID = meshID;
		mc->mesh_visible = visible;
		mc->ENTITY_CONTROL = entity_control;
		entities[entityID].AddComponent(mc);
	}
}