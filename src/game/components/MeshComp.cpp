#include "MeshComp.h"
#include "../../core.h"
#include "Transform.h"
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

void MeshComp::ReceiveEvent(Event event) {
	switch (event) {
	case TEST_EVENT:
		PRINT("Transform receieved event");
		break;
	}
}

void MeshComp::Update() {
	
}