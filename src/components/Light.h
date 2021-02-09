#pragma once
#include "Component.h"
#include "../math/Vector3.h"

struct Light : public Component {
	Vector3 position;
	Vector3 direction; //TODO change this to Quat
	float strength;
	//Geometry* shape;

	Light(const Vector3& position, const Vector3& direction, float strength = 1.f);
	
	void Update() override;
};