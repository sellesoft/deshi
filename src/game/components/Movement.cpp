#include "Movement.h"
#include "Physics.h"
#include "../../EntityAdmin.h"


Movement::Movement(Physics* phys) {
	this->phys = phys;
	layer = ComponentLayer_Physics;
	cpystr(name, "Movement", 63);
}

void Movement::Update() {
	phys->acceleration += inputs * airAccel;
}

std::vector<char> Movement::Save() {
	return {};
}