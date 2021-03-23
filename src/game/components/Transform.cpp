#include "Transform.h"
#include "../../EntityAdmin.h"

Transform::Transform() {

	name = "Transform";

	layer = TRANSFORM_LAYER;
}

Transform::Transform(Vector3 position, Vector3 rotation, Vector3 scale) {
	this->position = position;
	this->rotation = rotation;
	this->scale = scale;
	prevPosition = position;
	prevRotation = rotation;
	
	name = "Transform";

	layer = TRANSFORM_LAYER;
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

void Transform::Update() {
	
}
