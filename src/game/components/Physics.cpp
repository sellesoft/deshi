#include "Physics.h"

#include "../admin.h"

Physics::Physics() {
	admin = g_admin;
	cpystr(name, "Physics", DESHI_NAME_SIZE);
	layer = SystemLayer_Physics;
	comptype = ComponentType_Physics;
	sender = new Sender();
	
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
	admin = g_admin;
	cpystr(name, "Physics", DESHI_NAME_SIZE);
	layer = SystemLayer_Physics;
	comptype = ComponentType_Physics;
	sender = new Sender();
	
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
	
	admin = g_admin;
	cpystr(name, "Physics", DESHI_NAME_SIZE);
	layer = SystemLayer_Physics;
	comptype = ComponentType_Physics;
	sender = new Sender();
	
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
	admin = g_admin;
	cpystr(name, "Physics", DESHI_NAME_SIZE);
	layer = SystemLayer_Physics;
	comptype = ComponentType_Physics;
	sender = new Sender();
	
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
					"\nvelocity           (",velocity.x,",",velocity.y,",",velocity.z,")"
					"\nacceleration       (",acceleration.x,",",acceleration.y,",",acceleration.z,")"
					"\nrot_velocity       (",rotVelocity.x,",",rotVelocity.y,",",rotVelocity.z,")"
					"\nrot_acceleration   (",rotAcceleration.x,",",rotAcceleration.y,",",rotAcceleration.z,")"
					"\nelasticity         ", elasticity,
					"\nmass               ", mass,
					"\nfriction_kinetic   ", kineticFricCoef,
					"\nfriction_static    ", staticFricCoef,
					"\nstatic_position    ", (staticPosition) ? "true" : "false",
					"\nstatic_rotation    ", (staticRotation) ? "true" : "false",
					"\ntwod               ", (twoDphys) ? "true" : "false",
					"\n");
}

void Physics::LoadDESH(Admin* admin, const char* data, u32& cursor, u32 count){
	u32 entityID = 0xFFFFFFFF, compID = 0xFFFFFFFF, event = 0xFFFFFFFF;
	vec3 position{}, rotation{}, velocity{}, accel{}, rotVel{}, rotAccel{};
	f32 elasticity = 0.f, mass = 0.f, kineticFricCoef = 0.f, staticFricCoef = 0.f;
	b32 staticPos = false, staticRot = false, twoDphys = false;
	forI(count){
		memcpy(&entityID, data+cursor, sizeof(u32)); cursor += sizeof(u32);
		if(entityID >= admin->entities.size()) {
			ERROR("Failed to load physics component at pos '", cursor-sizeof(u32),
				  "' because it has an invalid entity ID: ", entityID); continue;
		}
		memcpy(&compID, data + cursor, sizeof(u32)); cursor += sizeof(u32);
		memcpy(&event, data + cursor, sizeof(u32)); cursor += sizeof(u32);
		
		memcpy(&position,        data+cursor, sizeof(vec3));  cursor += sizeof(vec3);
		memcpy(&rotation,        data+cursor, sizeof(vec3));  cursor += sizeof(vec3);
		memcpy(&velocity,        data+cursor, sizeof(vec3));  cursor += sizeof(vec3);
		memcpy(&accel,           data+cursor, sizeof(vec3));  cursor += sizeof(vec3);
		memcpy(&rotVel,          data+cursor, sizeof(vec3));  cursor += sizeof(vec3);
		memcpy(&rotAccel,        data+cursor, sizeof(vec3));  cursor += sizeof(vec3);
		memcpy(&elasticity,      data+cursor, sizeof(f32));   cursor += sizeof(f32);
		memcpy(&mass,            data+cursor, sizeof(f32));   cursor += sizeof(f32);
		memcpy(&staticPos,       data+cursor, sizeof(b32));   cursor += sizeof(b32);
		memcpy(&staticRot,       data+cursor, sizeof(b32));   cursor += sizeof(b32);
		memcpy(&twoDphys,        data+cursor, sizeof(b32));   cursor += sizeof(b32);
		memcpy(&kineticFricCoef, data+cursor, sizeof(float)); cursor += sizeof(float);
		memcpy(&staticFricCoef,  data+cursor, sizeof(float)); cursor += sizeof(float);
		
		Physics* c = new Physics(position, rotation, velocity, accel, rotVel, rotAccel, elasticity, mass, staticPos, staticRot, twoDphys, kineticFricCoef, staticFricCoef);
		EntityAt(entityID)->AddComponent(c);
		c->SetCompID(compID);
		c->SetEvent(event);
		c->layer_index = admin->freeCompLayers[c->layer].add(c);
	}
}