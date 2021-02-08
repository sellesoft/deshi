#include "Transform.h"

#include "../EntityAdmin.h"

Transform::Transform() {
	position = Vector3::ZERO;
	//rotation = Quaternion();
	rotation = Vector3::ZERO;
	scale = Vector3::ONE;
	prevPosition = position;
	prevRotation = rotation;

	layer = TRANSFORM_LAYER;
}

/*Transform(const Vector3& position, const Quaternion& rotation, const Vector3& scale) {
	this->position = position;
	this->rotation = rotation;
	this->scale = scale;
}*/

Transform::Transform(const Vector3& position, const Vector3& rotation, const Vector3& scale) {
	this->position = position;
	this->rotation = rotation;
	this->scale = scale;
	prevPosition = position;
	prevRotation = rotation;

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
