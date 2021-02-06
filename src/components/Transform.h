#pragma once
#include "Component.h"
#include "../math/Vector3.h"
#include "../math/Matrix4.h"

struct Transform : public Component {
	Vector3 position;
	//Quaternion rotation;
	Vector3 rotation;
	Vector3 scale;

	Vector3 lookDir = Vector3::ZERO;

	Vector3 prevPosition;
	Vector3 prevRotation;

	Transform() {
		position = Vector3::ZERO;
		//rotation = Quaternion();
		rotation = Vector3::ZERO;
		scale = Vector3::ONE;
		prevPosition = position;
		prevRotation = rotation;
	}

	/*Transform(const Vector3& position, const Quaternion& rotation, const Vector3& scale) {
		this->position = position;
		this->rotation = rotation;
		this->scale = scale;
	}*/

	Transform(const Vector3& position, const Vector3& rotation, const Vector3& scale) {
		this->position = position;
		this->rotation = rotation;
		this->scale = scale;
		prevPosition = position;
		prevRotation = rotation;
	}

	inline Vector3 Up() {
		return (Vector3::UP * Matrix4::RotationMatrix(rotation)).normalized();
	}

	inline Vector3 Right() {
		return (Vector3::RIGHT * Matrix4::RotationMatrix(rotation)).normalized();
	}

	inline Vector3 Forward() {
		return (Vector3::FORWARD * Matrix4::RotationMatrix(rotation)).normalized();
	}

	void Update() override;
};