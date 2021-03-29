#include "MeshComp.h"
#include "../../core.h"
#include "Transform.h"
#include "../../scene/Model.h"
#include "../../scene/Scene.h"

#include "../../EntityAdmin.h"

MeshComp::MeshComp() {
	send = new Sender();
	layer = CL1_RENDCANVAS;
}

MeshComp::MeshComp(Mesh* m) {
	this->m = m;
	name = "MeshComp";
	send = new Sender();
	layer = CL1_RENDCANVAS;
}

void MeshComp::ToggleVisibility() {
	mesh_visible = !mesh_visible;
	admin->renderer->UpdateMeshVisibility(MeshID, mesh_visible);
}

void MeshComp::ReceiveEvent(Event event) {
	switch (event) {
	case TEST_EVENT:
		PRINT("Transform receieved event");
		break;
	}
}

void MeshComp::Update() {
	//update mesh's transform with entities tranform
	admin->renderer->UpdateMeshMatrix(MeshID, entity->transform->TranformMatrix());
}