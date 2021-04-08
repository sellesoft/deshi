#include "Light.h"

#include "../../EntityAdmin.h"
#include "../../core.h"

Light::Light(const Vector3& position, const Vector3& direction, float strength) {
	this->position = position;
	this->direction = direction;
	this->strength = strength;
	strncpy_s(name, "Light", 63);
	this->name[63] = '\0';
}


void Light::Update() {
	
}
