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
	phys->kineticFricCoef = 0.32;
	phys->physOverride = true;
}

//for loading
Movement::Movement(Physics* phys, float gndAccel, float airAccel, float maxWalkingSpeed, bool jump) {
	admin = g_admin;
	layer = ComponentLayer_Physics;
	comptype = ComponentType_Movement;
	cpystr(name, "Movement", 63);
	
	this->phys = phys;
	//phys->kineticFricCoef = 1;
	//phys->physOverride = true;
	this->gndAccel = gndAccel;
	this->airAccel = airAccel;
	this->maxWalkingSpeed = maxWalkingSpeed;
	this->jump = jump;
}

void Movement::Update() {

	//check if were on the ground
	if (phys->contactState == ContactNONE) moveState = InAir;
	else {
		bool onGround = false;
		for (auto& p : phys->manifolds) {
			Vector3 norm = -p.second.norm.normalized();
			float ang = DEGREES(asin(norm.dot(Vector3::UP)));
			PRINTLN(ang);
			if (ang > 45) {
				onGround = true;
			}
		}
		if (onGround) moveState = OnGround;
		else          moveState = InAir;
	}
	if (moveState == OnGround) {
		ImGui::DebugDrawText("on ground", DengWindow->dimensions / 2);
	}
	else {
		ImGui::DebugDrawText("in air", DengWindow->dimensions / 2);
	}
	
	
	if (moveState == OnGround) {

		if (jump) {
			phys->acceleration += Vector3(0, 9999, 0);
			phys->velocity += phys->acceleration * inputs * DengTime->deltaTime;
			jump = false;
		}
		else {
			phys->velocity += inputs * gndAccel * DengTime->deltaTime;
		}

		
		//float projVel = phys->velocity.dot(inputs);
		//
		//if (projVel < maxWalkingSpeed - DengTime->deltaTime * gndAccel) {
		//	phys->velocity += DengTime->fixedDeltaTime * gndAccel * inputs;
		//}
		//else if (maxWalkingSpeed - DengTime->deltaTime * gndAccel <= projVel && projVel < maxWalkingSpeed){
		//	phys->velocity += (maxWalkingSpeed - phys->velocity.mag() * cosf(Math::AngBetweenVectors(phys->velocity, inputs))) * inputs;
		//}

		float velMag = phys->velocity.mag();
		if (velMag > maxWalkingSpeed) {
			phys->velocity /= velMag;
			phys->velocity *= maxWalkingSpeed;
		}
		else if (velMag < 0.12) {
			phys->velocity = Vector3::ZERO;
			phys->acceleration = Vector3::ZERO;
		}

		phys->position += phys->velocity * DengTime->deltaTime;
	}
	else {
		phys->acceleration += Vector3(0, -9.81, 0);
		//phys->velocity += inputs * gndAccel * DengTime->deltaTime;
		phys->velocity += phys->acceleration * DengTime->deltaTime;
		phys->position += phys->velocity * DengTime->deltaTime;

	}

	phys->acceleration = Vector3::ZERO;


	for (auto& m : phys->manifolds) {
		Vector3 norm = m.second.norm.normalized();

		Vector3 vPerpNorm = phys->velocity - phys->velocity.dot(norm) * norm;

		//PRINTLN(TOSTRING("------------------",
		//	"norm:     ", norm, "\n",
		//	"vel:      ", phys->velocity, "\n",
		//	"vel perp: ", vPerpNorm));


	}
	
	//TODO(sushi) implement more Source-like speed limiting later
	if (moveState == OnGround && phys->velocity.mag() > maxWalkingSpeed) {
		phys->velocity = phys->velocity.normalized() * maxWalkingSpeed;
	}
	
	
	if (inputs == Vector3::ZERO && moveState == OnGround) {
		if (phys->velocity != Vector3::ZERO) {
			if (phys->velocity.mag() > 0.12) {
			
				for (auto& m : phys->manifolds) {
					Vector3 norm = m.second.norm.normalized();

					Vector3 vPerpNorm = phys->velocity - phys->velocity.dot(norm) * norm;

					//PRINTLN(TOSTRING("------------------",
					//	"norm:     ", norm, "\n",
					//	"vel:      ", phys->velocity, "\n",
					//	"vel perp: ", vPerpNorm));
					//PRINTLN(TOSTRING("vel perp: ", vPerpNorm));


					phys->forces.push_back(-vPerpNorm.normalized() * phys->kineticFricCoef * phys->mass * -9.81);


				}
				
				//phys->AddFrictionForce(nullptr, 0.34);
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
		Movement* c = new Movement(EntityAt(entityID)->GetComponent<Physics>(), gndAccel, airAccel, maxWalkingSpeed, jump);
		EntityAt(entityID)->AddComponent(c);
		c->layer_index = admin->freeCompLayers[c->layer].add(c);
	}
	
}

