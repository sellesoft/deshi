#include "AudioListener.h"

#include "../../EntityAdmin.h"

AudioListener::AudioListener(Vector3 position, Vector3 velocity, Vector3 orientation) {
	this->position = position;
	this->velocity = velocity;
	this->orientation = orientation;
	
	layer = CL3_SOUND;
	sortid = 0;
	cpystr(name, "AudioListener", 63);
}

void AudioListener::Load(std::vector<Entity>& entities, const char* data, u32& cursor, u32 count){
	u32 entityID = 0xFFFFFFFF;
	Vector3 position, velocity, orientation;
	for_n(i,count){
		memcpy(&entityID, data+cursor, sizeof(u32)); 
		cursor += sizeof(u32);
		if(entityID >= entities.size()) {
			ERROR("Failed to load audio listener component at pos '", cursor-sizeof(u32),
				  "' because it has an invalid entity ID: ", entityID);
			continue;
		}
		
		memcpy(&position, data+cursor, sizeof(Vector3)*3); 
		cursor += sizeof(Vector3)*3;
		entities[entityID].AddComponent(new AudioListener(position, velocity, orientation));
	}
}