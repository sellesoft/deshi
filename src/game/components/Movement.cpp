#include "Movement.h"
#include "Physics.h"
#include "../admin.h"
#include "../systems/CanvasSystem.h"

Movement::Movement(Physics* phys) {
	admin = g_admin;
	layer = ComponentLayer_NONE;
	comptype = ComponentType_Movement;
	cpystr(name, "Movement", 63);
	
	this->phys = phys;
	phys->kineticFricCoef = 4;
}

//for loading
Movement::Movement(Physics* phys, float gndAccel, float airAccel, float maxWalkingSpeed, float maxRunningSpeed, float maxCrouchingSpeed, bool jump, float jumpImpulse) {
	admin = g_admin;
	layer = ComponentLayer_NONE;
	comptype = ComponentType_Movement;
	cpystr(name, "Movement", 63);
	
	this->phys = phys;
	//phys->kineticFricCoef = 1;
	//phys->physOverride = true;
	this->gndAccel = gndAccel;
	this->airAccel = airAccel;
	this->maxWalkingSpeed = maxWalkingSpeed;
	this->maxRunningSpeed = maxRunningSpeed;
	this->maxCrouchingSpeed = maxCrouchingSpeed;
	this->jump = jump;
	this->jumpImpulse = jumpImpulse;
}

void Movement::Update() {


	bool contactMoving = false;
	bool contactStationary = false;
	for (auto c : phys->contacts) {
		if (c.second == ContactMoving) {
			contactMoving = true;
		}
		else if (c.second == ContactStationary) contactStationary = true;
	}
	
	if (contactMoving)          phys->contactState = ContactMoving;
	else if (contactStationary) phys->contactState = ContactStationary;
	else                        phys->contactState = ContactNONE;

	Vector3 norm;
	//check if were on the ground
	if (phys->contactState == ContactNONE) { moveState = InAirNoInput; inAir = true; }
	else {
		bool bonGround = false;
		for (auto& p : phys->manifolds) {
			norm = p.second.norm.normalized();
			if (!p.second.player) norm = -norm;
			float ang = DEGREES(asin(norm.dot(Vector3::UP)));
			if (ang > 45) {
				bonGround = true;
			}
		}
		if (bonGround) {
			if (inputs != Vector3::ZERO) {
				if      (DengInput->KeyDownAnyMod(Key::LCTRL))  moveState = OnGroundCrouching;
				else if (DengInput->KeyDownAnyMod(Key::LSHIFT)) moveState = OnGroundRunning;
				else if (inputs != Vector3::ZERO)               moveState = OnGroundWalking;
				
			} else moveState = OnGroundNoInput;
			inAir = false;
		}
		else { moveState = InAirNoInput; inAir = true; }
	}
	
	//apply gravity
	phys->velocity += Vector3(0, -9.81, 0) * DengTime->fixedDeltaTime;
	
	if (jump) {
		phys->velocity += Vector3(0, 10, 0);
		jump = false;
	}

	auto accel = [&](float max, float accel) {
		//float projvel = phys->velocity.projectOn(inputs);
		//PRINTLN(TOSTRING("---------\n",
		//	"projvel: ", projvel, "\n",
		//	"vel      ", phys->velocity, "\n",
		//	"input    ", inputs));
		//if (projvel < max - accel * DengTime->fixedDeltaTime) 
		//	phys->velocity += DengTime->fixedDeltaTime * accel * inputs;
		//else if (maxWalkingSpeed - fabs(DengTime->fixedDeltaTime * accel) <= projvel && projvel < max)
		//	phys->velocity += (max - phys->velocity.mag() * cosf(Math::AngBetweenVectors(phys->velocity, inputs))) * inputs;
		phys->velocity += accel * DengTime->fixedDeltaTime * inputs;

		if (phys->velocity.mag() > max) phys->velocity.clampMag(0, max);
	
	};

		//float projVel = phys->velocity.dot(inputs);
	//
	//if (projVel < maxWalkingSpeed - DengTime->deltaTime * gndAccel) {
	//	phys->velocity += DengTime->fixedDeltaTime * gndAccel * inputs;
	//}
	//else if (maxWalkingSpeed - DengTime->deltaTime * gndAccel <= projVel && projVel < maxWalkingSpeed){
	//	phys->velocity += (maxWalkingSpeed - phys->velocity.mag() * cosf(Math::AngBetweenVectors(phys->velocity, inputs))) * inputs;
	//}
	//else if (velMag < 0.12 && inputs != Vector3::ZERO) {
	//	phys->velocity = Vector3::ZERO;
	//	phys->acceleration = Vector3::ZERO;
	//}
	
	switch (moveState) {
		case InAirNoInput:
			break;
		case InAirCrouching:
			break;
		case OnGroundNoInput:
			break;
		case OnGroundWalking:
			accel(maxWalkingSpeed, gndAccel);
			break;
		case OnGroundRunning:
			accel(maxRunningSpeed, gndAccel);
			break;
		case OnGroundCrouching:
			accel(maxCrouchingSpeed, gndAccel);
			break;
	}
		
	//apply ground friction
	//TODO(sushi) implement friction scaling with velocity and eventually canceling out new velocity
	if (moveState == OnGroundNoInput) {
		if (phys->velocity != Vector3::ZERO) {
			if (phys->velocity.mag() > 0.12) {
				for (auto& m : phys->manifolds) {
					Vector3 norm = m.second.norm.normalized();
					Vector3 vPerpNorm = phys->velocity - phys->velocity.dot(norm) * norm;
					phys->acceleration += vPerpNorm.normalized() * phys->kineticFricCoef * phys->mass * -9.81 / phys->mass;
					phys->velocity += phys->acceleration * DengTime->fixedDeltaTime;
				}
			} else phys->velocity = Vector3::ZERO;
		}
	}
	
	phys->position += phys->velocity * DengTime->fixedDeltaTime;

	phys->manifolds.clear();
	phys->acceleration = Vector3::ZERO;
}

std::vector<char> Movement::Save() {
	return {};
}

void Movement::Load(EntityAdmin* admin, const char* data, u32& cursor, u32 count) {
	u32 entityID = -1;
	Vector3 inputs{};
	float gndAccel{}, airAccel{}, maxWalkingSpeed{}, maxRunningSpeed{}, maxCrouchingSpeed{}, jumpImpulse{};
	bool jump = false;
	
	for_n(i, count) {
		memcpy(&entityID, data + cursor, sizeof(u32)); cursor += sizeof(u32);
		if (entityID >= admin->entities.size()) {
			ERROR("Failed to load sphere collider component at pos '", cursor - sizeof(u32),
				  "' because it has an invalid entity ID: ", entityID); continue;
		}
		
		memcpy(&inputs,            data + cursor, sizeof(Vector3)); cursor += sizeof(Vector3);
		memcpy(&gndAccel,          data + cursor, sizeof(float));   cursor += sizeof(float);
		memcpy(&airAccel,          data + cursor, sizeof(float));   cursor += sizeof(float);
		memcpy(&maxWalkingSpeed,   data + cursor, sizeof(float));   cursor += sizeof(float);
		memcpy(&maxRunningSpeed,   data + cursor, sizeof(float));   cursor += sizeof(bool);
		memcpy(&maxCrouchingSpeed, data + cursor, sizeof(float));   cursor += sizeof(bool);
		memcpy(&jump,              data + cursor, sizeof(bool));    cursor += sizeof(bool);
		memcpy(&jumpImpulse,       data + cursor, sizeof(float));   cursor += sizeof(bool);
		
		Movement* c = new Movement(EntityAt(entityID)->GetComponent<Physics>(), gndAccel, airAccel, maxWalkingSpeed, maxRunningSpeed, maxCrouchingSpeed, jump, jumpImpulse);
		EntityAt(entityID)->AddComponent(c);
		c->layer_index = admin->freeCompLayers[c->layer].add(c);
	}
	
}

