#include "Light.h"

#include "../../EntityAdmin.h"
#include "../../core.h"

Light::Light(const Vector3& position, const Vector3& direction, float brightness) {
	this->position = position;
	this->direction = direction;
	this->brightness = brightness;
	cpystr(name, "Light", 63);
	comptype = ComponentType_Light;
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

std::vector<char> Light::Save() {
	std::vector<char> out;
	return out;
}

void Light::Load(std::vector<Entity>& entities, const char* data, u32& cursor, u32 count){
	u32 entityID = 0xFFFFFFFF;
	Vector3 position{}, direction{};
	float strength = 0.f;
	for_n(i,count){
		memcpy(&entityID, data+cursor, sizeof(u32)); 
		cursor += sizeof(u32);
		if(entityID >= entities.size()) {
			ERROR("Failed to load light component at pos '", cursor-sizeof(u32),
				  "' because it has an invalid entity ID: ", entityID);
			continue;
		}
		
		memcpy(&position, data+cursor,  sizeof(vec3)); cursor += sizeof(vec3);
		memcpy(&direction, data+cursor, sizeof(vec3)); cursor += sizeof(vec3);
		memcpy(&strength, data+cursor,  sizeof(f32));  cursor += sizeof(f32);
		entities[entityID].AddComponent(new Light(position, direction, strength));
	}
}