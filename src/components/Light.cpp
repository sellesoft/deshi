#pragma once
#include "Component.h"
#include "../math/Vector3.h"

struct Light : public Component {
	Vector3 position;
	Vector3 direction; //TODO change this to Quat
	float strength;
	//Geometry* shape;

	//OpenGL's tutorial used int but we may want to experiment
	//with using float later on 
	std::vector<int> depthTexture;

	Light(const Vector3& position, const Vector3& direction, float strength = 1.f) {
		this->position = position;
		this->direction = direction;
		this->strength = strength;
	}
};