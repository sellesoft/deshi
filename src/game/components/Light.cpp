#include "Light.h"

#include "../../EntityAdmin.h"

Light::Light(const Vector3& position, const Vector3& direction, float strength) {
	this->position = position;
	this->direction = direction;
	this->strength = strength;
	name = "Light";
}


void Light::Update() {

}
