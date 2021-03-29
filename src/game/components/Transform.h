#pragma once
#ifndef COMPONENT_TRANSFORM_H
#define COMPONENT_TRANSFORM_H

#include "Component.h"
#include "../../math/Vector3.h"
#include "../../math/Matrix4.h"

struct Transform : public Component {
	Vector3 position = Vector3::ZERO;
	Vector3 rotation = Vector3::ZERO;
	Vector3 scale    = Vector3::ONE;
	
	Vector3 prevPosition = Vector3::ZERO;
	Vector3 prevRotation = Vector3::ZERO;
	
	Transform();
	Transform(Vector3 position, Vector3 rotation, Vector3 scale);
	
	inline Vector3 Up();
	inline Vector3 Right();
	inline Vector3 Forward();
	inline Matrix4 TranformMatrix();
	
	void Update() override;
	void ReceiveEvent(Event event) override;
};

inline Vector3 Transform::Up() {
	return (Vector3::UP * Matrix4::RotationMatrix(rotation)).normalized();
}

inline Vector3 Transform::Right() {
	return (Vector3::RIGHT * Matrix4::RotationMatrix(rotation)).normalized();
}

inline Vector3 Transform::Forward() {
	return (Vector3::FORWARD * Matrix4::RotationMatrix(rotation)).normalized();
}

inline Matrix4 Transform::TranformMatrix() {
	return Matrix4::TransformationMatrix(position, rotation, scale);
}

#endif //COMPONENT_TRANSFORM_H