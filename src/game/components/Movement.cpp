#include "Movement.h"
#include "Physics.h"
#include "../admin.h"
#include "../systems/CanvasSystem.h"

Movement::Movement(Physics* phys) {
	admin = g_admin;
	layer = ComponentLayer_Physics;
	comptype = ComponentType_Movement;
	cpystr(name, "Movement", 63);
	
	this->phys = phys;
	phys->kineticFricCoef = 1;
	phys->fricOverride = true;
}

//for loading
Movement::Movement(Physics* phys, float gndAccel, float airAccel, float maxWalkingSpeed, bool jump) {
	admin = g_admin;
	layer = ComponentLayer_Physics;
	comptype = ComponentType_Movement;
	cpystr(name, "Movement", 63);

	this->phys = phys;
	//phys->kineticFricCoef = 1;
	//phys->fricOverride = true;
	this->gndAccel = gndAccel;
	this->airAccel = airAccel;
	this->maxWalkingSpeed = maxWalkingSpeed;
	this->jump = jump;
}

void Movement::Update() {
	if (phys->contactState == ContactNONE) moveState = InAir;
	else moveState = OnGround;
	
	if (moveState == OnGround) {
		ImGui::DebugDrawText("on ground", DengWindow->dimensions / 2);
	}
	else {
		ImGui::DebugDrawText("in air", DengWindow->dimensions / 2);
	}
	
	
	if (moveState == OnGround) {
		phys->velocity += inputs * gndAccel * DengTime->deltaTime;
		//float projVel = phys->velocity.dot(inputs);
		//
		//if (projVel < maxWalkingSpeed - DengTime->deltaTime * gndAccel) {
		//	phys->velocity += DengTime->fixedDeltaTime * gndAccel * inputs;
		//}
		//else if (maxWalkingSpeed - DengTime->deltaTime * gndAccel <= projVel && projVel < maxWalkingSpeed){
		//	phys->velocity += (maxWalkingSpeed - phys->velocity.mag() * cosf(Math::AngBetweenVectors(phys->velocity, inputs))) * inputs;
		//}
	}
	//else
	//	phys->acceleration += Accelerate(inputs, phys->velocity, airAccel, maxWalkingSpeed);
	
	
	
	
	//TODO(sushi) implement more Source-like speed limiting later
	if (moveState == OnGround && phys->velocity.mag() > maxWalkingSpeed) {
		phys->velocity = phys->velocity.normalized() * maxWalkingSpeed;
	}
	
	if (jump) {
		phys->AddForce(nullptr, Vector3(0, 17500, 0));
		jump = false;
	}
	if (inputs == Vector3::ZERO && moveState == OnGround) {
		if (phys->velocity != Vector3::ZERO) {
			if (phys->velocity.mag() > 0.12){
				phys->AddFrictionForce(nullptr, 5);
				PRINTLN("apply fric");
			}
			else
				phys->velocity = Vector3::ZERO;
		}
	}
	
}

std::vector<char> Movement::Save() {
	return {};
}

void Movement::Load(EntityAdmin* admin, const char* data, u32& cursor, u32 count) {
	u32 entityID = -1;
	Vector3 inputs{};
	float gndAccel{}, airAccel{}, maxWalkingSpeed{};
	bool jump = false;

	for_n(i, count) {
		memcpy(&entityID, data + cursor, sizeof(u32)); cursor += sizeof(u32);
		if (entityID >= admin->entities.size()) {
			ERROR("Failed to load sphere collider component at pos '", cursor - sizeof(u32),
				"' because it has an invalid entity ID: ", entityID); continue;
		}

		memcpy(&inputs, data + cursor, sizeof(Vector3));        cursor += sizeof(Vector3);
		memcpy(&gndAccel, data + cursor, sizeof(float));        cursor += sizeof(float);
		memcpy(&airAccel, data + cursor, sizeof(float));        cursor += sizeof(float);
		memcpy(&maxWalkingSpeed, data + cursor, sizeof(float)); cursor += sizeof(float);
		memcpy(&jump, data + cursor, sizeof(bool));             cursor += sizeof(bool);
		Movement* c = new Movement(admin->entities[entityID].getptr()->GetComponent<Physics>(), gndAccel, airAccel, maxWalkingSpeed, jump);
		admin->entities[entityID].value.AddComponent(c);
		c->layer_index = admin->freeCompLayers[c->layer].add(c);
	}

}

