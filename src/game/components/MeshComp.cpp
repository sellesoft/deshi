#include "MeshComp.h"
#include "../../core.h"
#include "Transform.h"
#include "../../EntityAdmin.h"

MeshComp::MeshComp() {
	//empty for adding in commands
	entity->GetComponent<Transform>()->send->AddReceiver(this);
}

MeshComp::MeshComp(Mesh* m) {
	this->m = m;
	name = "MeshComp";

	entity->GetComponent<Transform>()->send->AddReceiver(this);
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