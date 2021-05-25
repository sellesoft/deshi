#include "AudioListener.h"

#include "../admin.h"

AudioListener::AudioListener() {
	layer = ComponentLayer_Sound;
	comptype = ComponentType_AudioListener;
	cpystr(name, "AudioListener", 63);
	sender = new Sender();
}

AudioListener::AudioListener(Vector3 position, Vector3 velocity, Vector3 orientation) {
	this->position = position;
	this->velocity = velocity;
	this->orientation = orientation;
	
	layer = ComponentLayer_Sound;
	comptype = ComponentType_AudioListener;
	cpystr(name, "AudioListener", 63);
	sender = new Sender();
}

////////////////////////////
//// saving and loading ////
////////////////////////////

std::vector<char> AudioListener::SaveTEXT(){
	return std::vector<char>();
}

void AudioListener::LoadDESH(EntityAdmin* admin, const char* data, u32& cursor, u32 count){
	u32 entityID = 0xFFFFFFFF, compID = 0xFFFFFFFF, event = 0xFFFFFFFF;
	for_n(i,count){
		memcpy(&entityID, data+cursor, sizeof(u32)); cursor += sizeof(u32);
		if(entityID >= admin->entities.size()) {
			ERROR("Failed to load audio listener component at pos '", cursor-sizeof(u32),
				  "' because it has an invalid entity ID: ", entityID);
			continue;
		}
		memcpy(&compID, data + cursor, sizeof(u32)); cursor += sizeof(u32);
		memcpy(&event, data + cursor, sizeof(u32)); cursor += sizeof(u32);
		
		AudioListener* c = new AudioListener();
		memcpy(&c->position,    data+cursor, sizeof(vec3)); cursor += sizeof(vec3);
		memcpy(&c->velocity,    data+cursor, sizeof(vec3)); cursor += sizeof(vec3);
		memcpy(&c->orientation, data+cursor, sizeof(vec3)); cursor += sizeof(vec3);
		EntityAt(entityID)->AddComponent(c);
		c->SetCompID(compID);
		c->SetEvent(event);
		c->layer_index = admin->freeCompLayers[c->layer].add(c);
	}
}