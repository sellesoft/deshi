#include "Transform.h"
#include "../../EntityAdmin.h"
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

inline Vector3 Transform::Up() {
	return (Vector3::UP * Matrix4::RotationMatrix(rotation)).normalized();
}

inline Vector3 Transform::Right() {
	return (Vector3::RIGHT * Matrix4::RotationMatrix(rotation)).normalized();
}

inline Vector3 Transform::Forward() {
	return (Vector3::FORWARD * Matrix4::RotationMatrix(rotation)).normalized();
}

void Transform::ReceiveEvent(Event event) {
	switch (event) {
	case TEST_EVENT:
		PRINT("Transform receieved event");
		break;
	}
}


void Transform::Update() {
	send->SendEvent(TEST_EVENT);
}
