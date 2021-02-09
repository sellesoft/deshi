#include "Physics.h"

#include "../EntityAdmin.h"

Physics::Physics(Vector3 position, Vector3 rotation, Vector3 velocity, Vector3 acceleration, Vector3 rotVeloctiy,
		         Vector3 rotAcceleration, float elasticity, float mass, bool isStatic) {
	this->position = position;
	this->rotation = rotation;
	this->velocity = velocity;
	this->acceleration = acceleration;
	this->rotVelocity = rotVeloctiy;
	this->rotAcceleration = rotAcceleration;
	this->elasticity = elasticity;
	this->mass = mass;
	this->isStatic = isStatic;

	layer = PHYS_LAYER;
}

Physics::Physics(Vector3 position, Vector3 rotation, float mass, float elasticity) {
	this->position = position;
	this->rotation = rotation;
	this->velocity = Vector3::ZERO;
	this->acceleration = Vector3::ZERO;
	this->rotVelocity = Vector3::ZERO;
	this->rotAcceleration = Vector3::ZERO;
	this->mass = mass;
	this->elasticity = elasticity;

	layer = PHYS_LAYER;
}