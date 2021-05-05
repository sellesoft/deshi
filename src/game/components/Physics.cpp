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
	
	cpystr(name, "Physics", 63);
	layer = SystemLayer_Physics;
	comptype = ComponentType_Physics;
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
	
	cpystr(name, "Physics", 63);
	layer = SystemLayer_Physics;
	comptype = ComponentType_Physics;
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
	
	cpystr(name, "Physics", 63);
	layer = SystemLayer_Physics;
	comptype = ComponentType_Physics;
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

std::vector<char> Physics::Save() {
	std::vector<char> out;
	return out;
}

void Physics::Load(EntityAdmin* admin, const char* data, u32& cursor, u32 count){
	u32 entityID = 0xFFFFFFFF;
	vec3 position{}, rotation{}, velocity{}, accel{}, rotVel{}, rotAccel{};
	f32 elasticity = 0.f, mass = 0.f;
	b32 staticPos = false, staticRot = false;
	for_n(i,count){
		memcpy(&entityID, data+cursor, sizeof(u32)); 
		cursor += sizeof(u32);
		if(entityID >= admin->entities.size()) {
			ERROR("Failed to load physics component at pos '", cursor-sizeof(u32),
				  "' because it has an invalid entity ID: ", entityID);
			continue;
		}
		
		memcpy(&position,   data+cursor, sizeof(vec3)); cursor += sizeof(vec3);
		memcpy(&rotation,   data+cursor, sizeof(vec3)); cursor += sizeof(vec3);
		memcpy(&velocity,   data+cursor, sizeof(vec3)); cursor += sizeof(vec3);
		memcpy(&accel,      data+cursor, sizeof(vec3)); cursor += sizeof(vec3);
		memcpy(&rotVel,     data+cursor, sizeof(vec3)); cursor += sizeof(vec3);
		memcpy(&rotAccel,   data+cursor, sizeof(vec3)); cursor += sizeof(vec3);
		memcpy(&elasticity, data+cursor, sizeof(f32));  cursor += sizeof(f32);
		memcpy(&mass,       data+cursor, sizeof(f32));  cursor += sizeof(f32);
		memcpy(&staticPos,  data+cursor, sizeof(b32));  cursor += sizeof(b32);
		Physics* c = new Physics(position, rotation, velocity, accel, rotVel, rotAccel, elasticity, mass, staticPos);
		admin->entities[entityID].AddComponent(c);
		c->layer_index = admin->freeCompLayers[c->layer].add(c);
		c->Init(admin);
	}
}