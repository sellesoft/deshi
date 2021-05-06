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
	PRINTLN(TOSTRING(inputs));
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