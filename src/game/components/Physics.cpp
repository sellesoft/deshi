#include "Physics.h"

#include "../../EntityAdmin.h"

Physics::Physics() {
	position = Vector3::ZERO;
	rotation = Vector3::ZERO;
	velocity = Vector3::ZERO;
	acceleration = Vector3::ZERO;
	rotVelocity = Vector3::ZERO;
	rotAcceleration = Vector3::ZERO;
	elasticity = 1;
	mass = 1;
	isStatic = false;

	strncpy_s(name, "Physics", 63);
	sortid = 7;
}

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

	strncpy_s(name, "Physics", 63);

	layer = PHYS_LAYER;
	sortid = 7;
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

	strncpy_s(name, "Physics", 63);
	this->name[63] = '\0';

	layer = PHYS_LAYER;
	sortid = 7;
}

//////////////////////////
//// static functions ////
//////////////////////////

void Physics::AddInput(Vector3 input) {
	inputVector += input;
}

void Physics::AddForce(Physics* creator, Vector3 force) {
	forces.push_back(force);
	if(creator) { creator->forces.push_back(-force); }
}

void Physics::AddFrictionForce(Physics* creator, float frictionCoef, float gravity) {
	forces.push_back(-velocity.normalized() * frictionCoef * mass);// * gravity);
	if (creator) { 
		//TODO(delle,Ph) implement sliding friction between two objects
	}
}

void Physics::AddImpulse(Physics* creator, Vector3 impulse) {
	velocity += impulse / mass;
	if (creator) { creator->velocity -= impulse / creator->mass; }
}

void Physics::AddImpulseNomass(Physics* creator, Vector3 impulse) {
	velocity += impulse;
	if (creator) { creator->velocity -= impulse; }
}
