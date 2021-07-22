#include "Physics.h"

#include "../admin.h"
#include "../../core/console.h"
#include "../../utils/debug.h"

Physics::Physics() {
	layer = SystemLayer_Physics;
	type = ComponentType_Physics;
	
	position = Vector3::ZERO;
	rotation = Vector3::ZERO;
	velocity = Vector3::ZERO;
	acceleration = Vector3::ZERO;
	rotVelocity = Vector3::ZERO;
	rotAcceleration = Vector3::ZERO;
	elasticity = .2f;
	mass = 1;
	staticPosition = false;
}

Physics::Physics(Vector3 position, Vector3 rotation, Vector3 velocity, Vector3 acceleration, Vector3 rotVeloctiy,
				 Vector3 rotAcceleration, float elasticity, float mass, bool staticPosition) {
	layer = SystemLayer_Physics;
	type = ComponentType_Physics;
	
	this->position = position;
	this->rotation = rotation;
	this->velocity = velocity;
	this->acceleration = acceleration;
	this->rotVelocity = rotVeloctiy;
	this->rotAcceleration = rotAcceleration;
	this->elasticity = elasticity;
	this->mass = mass;
	this->staticPosition = staticPosition;
}

//for loading only really
Physics::Physics(Vector3 position, Vector3 rotation, Vector3 velocity, Vector3 acceleration, Vector3 rotVeloctiy, Vector3 rotAcceleration, float elasticity,
				 float mass, bool staticPosition, bool staticRotation, bool twoDphys, float kineticFricCoef, float staticFricCoef) {
	layer = SystemLayer_Physics;
	type = ComponentType_Physics;
	
	this->position = position;
	this->rotation = rotation;
	this->velocity = velocity;
	this->acceleration = acceleration;
	this->rotVelocity = rotVeloctiy;
	this->rotAcceleration = rotAcceleration;
	this->elasticity = elasticity;
	this->mass = mass;
	this->staticPosition = staticPosition;
	this->staticRotation = staticRotation;
	this->twoDphys = twoDphys;
	this->kineticFricCoef = kineticFricCoef;
	this->staticFricCoef = staticFricCoef;
	
	
}

Physics::Physics(Vector3 position, Vector3 rotation, float mass, float elasticity) {
	layer = SystemLayer_Physics;
	type = ComponentType_Physics;
	
	this->position = position;
	this->rotation = rotation;
	this->velocity = Vector3::ZERO;
	this->acceleration = Vector3::ZERO;
	this->rotVelocity = Vector3::ZERO;
	this->rotAcceleration = Vector3::ZERO;
	this->mass = mass;
	this->elasticity = elasticity;
}

void Physics::AddInput(Vector3 input) {
	inputVector += input;
}

void Physics::AddForce(Physics* creator, Vector3 force) {
	forces.push_back(force);
	if(creator) { creator->forces.push_back(-force); }
}

void Physics::AddFrictionForce(Physics* creator, float frictionCoef, float grav) {
	forces.push_back(-velocity.normalized() * frictionCoef * mass * grav);
}

void Physics::AddImpulse(Physics* creator, Vector3 impulse) {
	velocity += impulse / mass;
	if (creator) { creator->velocity -= impulse / creator->mass; }
}

void Physics::AddImpulseNomass(Physics* creator, Vector3 impulse) {
	velocity += impulse;
	if (creator) { creator->velocity -= impulse; }
}

std::string Physics::SaveTEXT(){
	return TOSTRING("\n>physics"
					"\nvelocity         (",velocity.x,",",velocity.y,",",velocity.z,")"
					"\nacceleration     (",acceleration.x,",",acceleration.y,",",acceleration.z,")"
					"\nrot_velocity     (",rotVelocity.x,",",rotVelocity.y,",",rotVelocity.z,")"
					"\nrot_acceleration (",rotAcceleration.x,",",rotAcceleration.y,",",rotAcceleration.z,")"
					"\nelasticity       ", elasticity,
					"\nmass             ", mass,
					"\nfriction_kinetic ", kineticFricCoef,
					"\nfriction_static  ", staticFricCoef,
					"\nstatic_position  ", (staticPosition) ? "true" : "false",
					"\nstatic_rotation  ", (staticRotation) ? "true" : "false",
					"\ntwod             ", (twoDphys) ? "true" : "false",
					"\n");
}

void Physics::LoadDESH(Admin* admin, const char* data, u32& cursor, u32 count){
	ERROR("Physics::LoadDESH not setup");
}