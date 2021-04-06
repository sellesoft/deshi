#include "MeshComp.h"
#include "../../core.h"
#include "../Transform.h"
#include "../../scene/Model.h"
#include "../../scene/Scene.h"

#include "../../EntityAdmin.h"

MeshComp::MeshComp() {
	send = new Sender();
	layer = CL1_RENDCANVAS;
	r = admin->renderer;
}

MeshComp::MeshComp(Mesh* m) {
	this->m = m;
	name = "MeshComp";
	send = new Sender();
	layer = CL1_RENDCANVAS;
	//r = admin->renderer;
}

void MeshComp::ToggleVisibility() {
	mesh_visible = !mesh_visible;
	admin->renderer->UpdateMeshVisibility(MeshID, mesh_visible);
}

void MeshComp::ReceiveEvent(Event event) {
	switch (event) {
		case TEST_EVENT:
		PRINT("MeshComp receieved event");
		break;
	}
}

void MeshComp::ChangeMaterialShader(u32 s) {
	std::vector<u32> ids = admin->renderer->GetMaterialIDs(MeshID);
	
	for (u32 id : ids) {
		admin->renderer->UpdateMaterialShader(id, s);
	}
}

void MeshComp::ChangeMaterialTexture(u32 t) {
	std::vector<u32> ids = admin->renderer->GetMaterialIDs(MeshID);

	for (u32 id : ids) {
		admin->renderer->UpdateMaterialTexture(id, 0, t);
	}
}

//this should only be used when the entity is not controlling the Mesh
void MeshComp::UpdateMeshTransform(Vector3 position, Vector3 rotation, Vector3 scale) {
	admin->renderer->UpdateMeshMatrix(MeshID, Matrix4::TransformationMatrix(position, rotation, scale));
}

void MeshComp::Update() {
	//update mesh's transform with entities tranform
	if(ENTITY_CONTROL) admin->renderer->UpdateMeshMatrix(MeshID, entity->transform.TransformMatrix());
}