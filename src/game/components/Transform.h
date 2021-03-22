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
	
	void Update() override;
};

#endif //COMPONENT_TRANSFORM_H