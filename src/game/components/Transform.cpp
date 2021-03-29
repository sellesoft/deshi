#include "Transform.h"
#include "../../EntityAdmin.h"
#include "../../math/Math.h"
#include "Light.h"
#include "MeshComp.h"

Transform::Transform() {

	name = "Transform";
	layer = TRANSFORM_LAYER;
	send = new Sender();
}

Transform::Transform(Vector3 position, Vector3 rotation, Vector3 scale) {
	this->position = position;
	this->rotation = rotation;
	this->scale = scale;
	prevPosition = position;
	prevRotation = rotation;
	
	name = "Transform";

	layer = TRANSFORM_LAYER;
	send = new Sender();
}



void Transform::ReceiveEvent(Event event) {
	switch (event) {
	case TEST_EVENT:
		PRINT("Transform receieved event");
		break;
	}
}

void Transform::Update() {

}
