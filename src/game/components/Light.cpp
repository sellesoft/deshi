#include "Light.h"

#include "../admin.h"
#include "../../core.h"

Light::Light(const Vector3& position, const Vector3& direction, float brightness) {
	admin = g_admin;
	this->position = position;
	this->direction = direction;
	this->brightness = brightness;
	cpystr(name, "Light", DESHI_NAME_SIZE);
	comptype = ComponentType_Light;
	layer = ComponentLayer_Physics;
	sender = new Sender();
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
					"\n");
}

void Light::LoadDESH(EntityAdmin* admin, const char* data, u32& cursor, u32 count){
	u32 entityID = 0xFFFFFFFF, compID = 0xFFFFFFFF, event = 0xFFFFFFFF;
	Vector3 position{}, direction{};
	float strength = 0.f;
	for_n(i,count){
		memcpy(&entityID, data+cursor, sizeof(u32)); 
		cursor += sizeof(u32);
		if(entityID >= admin->entities.size()) {
			ERROR("Failed to load light component at pos '", cursor-sizeof(u32),
				  "' because it has an invalid entity ID: ", entityID); continue;
		}
		memcpy(&compID, data + cursor, sizeof(u32)); cursor += sizeof(u32);
		memcpy(&event, data + cursor, sizeof(u32)); cursor += sizeof(u32);
		
		memcpy(&position,  data+cursor, sizeof(vec3)); cursor += sizeof(vec3);
		memcpy(&direction, data+cursor, sizeof(vec3)); cursor += sizeof(vec3);
		memcpy(&strength,  data+cursor, sizeof(f32));  cursor += sizeof(f32);
		Light* c = new Light(position, direction, strength);
		EntityAt(entityID)->AddComponent(c);
		c->SetCompID(compID);
		c->SetEvent(event);
		c->layer_index = admin->freeCompLayers[c->layer].add(c);
	}
}