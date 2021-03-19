#pragma once
#ifndef COMPONENT_TRANSFORM_H
#define COMPONENT_TRANSFORM_H

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
	
	Transform();
	
	/*Transform(const Vector3& position, const Quaternion& rotation, const Vector3& scale) {
		this->position = position;
		this->rotation = rotation;
		this->scale = scale;
	}*/
	
	Transform(const Vector3& position, const Vector3& rotation, const Vector3& scale);
	
	inline Vector3 Up();
	
	inline Vector3 Right();
	
	inline Vector3 Forward();
	
	void Update() override;
};

#endif //COMPONENT_TRANSFORM_H