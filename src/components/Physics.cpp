#pragma once
#include "Component.h"
#include "Transform.h"
#include "../math/Vector3.h"

struct Physics : public Component {
	Vector3 position;
	Vector3 rotation;

	Vector3 velocity;
	Vector3 acceleration;
	Vector3 rotVelocity;
	Vector3 rotAcceleration;

	float elasticity; //less than 1 in most cases
	float mass;

	std::vector<Vector3> forces;
	Vector3 inputVector;

	bool isStatic = false;

	Physics(Vector3 position, Vector3 rotation, Vector3 velocity = Vector3::ZERO, Vector3 acceleration = Vector3::ZERO, Vector3 rotVeloctiy = Vector3::ZERO, 
						Vector3 rotAcceleration = Vector3::ZERO, float elasticity = .5f, float mass = 1.f, bool isStatic = false){
		this->position = position;
		this->rotation = rotation;
		this->velocity = velocity;
		this->acceleration = acceleration;
		this->rotVelocity = rotVeloctiy;
		this->rotAcceleration = rotAcceleration;
		this->elasticity = elasticity;
		this->mass = mass;
		this->isStatic = isStatic;
	}

	Physics(Vector3 position, Vector3 rotation, float mass, float elasticity){
		this->position = position;
		this->rotation = rotation;
		this->velocity = Vector3::ZERO;
		this->acceleration = Vector3::ZERO;
		this->rotVelocity = Vector3::ZERO;
		this->rotAcceleration = Vector3::ZERO;
		this->mass = mass;
		this->elasticity = elasticity;
	}
};