#include "Light.h"

#include "../admin.h"


Light::Light(){
	type = ComponentType_Light;
	layer = ComponentLayer_Physics;
}

Light::Light(const Vector3& position, const Vector3& direction, float brightness) {
	this->position = position;
	this->direction = direction;
	this->brightness = brightness;
	type = ComponentType_Light;
	layer = ComponentLayer_Physics;
}


void Light::Update() {
	position = entity->transform.position;
}

void Light::ReceiveEvent(Event event) {
	switch (event) {
		case Event_LightToggle:
		active = !active;
		break;
	}
}

std::string Light::SaveTEXT(){
	return TOSTRING("\n>light"
					"\nposition   (",position.x,",",position.y,",",position.z,")"
					"\ndirection  (",direction.x,",",direction.y,",",direction.z,")"
					"\nbrightness ", brightness,
					"\nactive     ", (active) ? "true" : "false",
					"\n");
}

void Light::LoadDESH(Admin* admin, const char* data, u32& cursor, u32 count){
	ERROR("Light::LoadDESH not setup");
}