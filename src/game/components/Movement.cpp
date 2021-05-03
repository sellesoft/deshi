#include "Movement.h"
#include "Physics.h"
#include "../../EntityAdmin.h"

#include "../systems/CanvasSystem.h"

Movement::Movement(Physics* phys) {
	this->phys = phys;
	phys->kineticFricCoef = 1;
	phys->fricOverride = true;
	layer = ComponentLayer_Physics;
	comptype = ComponentType_Movement;
	cpystr(name, "Movement", 63);
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

	if(moveState != InAir)
		phys->acceleration += inputs * gndAccel;

	//TODO(sushi) implement more Source-like speed limiting later
	if (moveState == OnGround && phys->velocity.mag() > 12) {
		phys->velocity = phys->velocity.normalized() * 12;
	}

	if (jump) {
		phys->AddForce(nullptr, Vector3(0, 17500, 0));
		jump = false;
	}

	if (inputs == Vector3::ZERO && moveState == OnGround) {
		if (phys->velocity != Vector3::ZERO) {
			PRINTLN(phys->velocity.mag());
			if (phys->velocity.mag() > 0.12)
				phys->AddFrictionForce(nullptr, 5);
			else
				phys->velocity = Vector3::ZERO;
		}
	}

}

std::vector<char> Movement::Save() {
	return {};
}