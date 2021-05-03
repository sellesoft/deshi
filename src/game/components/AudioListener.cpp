#include "AudioListener.h"

#include "../../EntityAdmin.h"

AudioListener::AudioListener(Vector3 position, Vector3 velocity, Vector3 orientation) {
	this->position = position;
	this->velocity = velocity;
	this->orientation = orientation;
	
	layer = ComponentLayer_Sound;
	comptype = ComponentType_AudioListener;
	cpystr(name, "AudioListener", 63);
}

////////////////////////////
//// saving and loading ////
////////////////////////////

std::vector<char> AudioListener::Save() {
	std::vector<char> out;
	return out;
}

/*static std::vector<char> SaveComponentHeader(u32 offset, u32 count){
	ComponentTypeHeader typeHeader;
	typeHeader.type        = ComponentType_AudioListener;
	typeHeader.arrayOffset = offset;
	typeHeader.size        = sizeof(u32) + sizeof(Vector3)*3;
	typeHeader.count       = count;
	
	std::vector<char> out; out.resize(sizeof(ComponentTypeHeader));
	memcpy(&out.data, &typeHeader, sizeof(ComponentTypeHeader));
	
	return out;
}*/

void AudioListener::Load(std::vector<Entity>& entities, const char* data, u32& cursor, u32 count){
	u32 entityID = 0xFFFFFFFF;
	vec3 position{}, velocity{}, orientation{};
	for_n(i,count){
		memcpy(&entityID, data+cursor, sizeof(u32)); 
		cursor += sizeof(u32);
		if(entityID >= entities.size()) {
			ERROR("Failed to load audio listener component at pos '", cursor-sizeof(u32),
				  "' because it has an invalid entity ID: ", entityID);
			continue;
		}
		
		memcpy(&position, data+cursor,    sizeof(vec3)); cursor += sizeof(vec3);
		memcpy(&velocity, data+cursor,    sizeof(vec3)); cursor += sizeof(vec3);
		memcpy(&orientation, data+cursor, sizeof(vec3)); cursor += sizeof(vec3);
		entities[entityID].AddComponent(new AudioListener(position, velocity, orientation));
	}
}