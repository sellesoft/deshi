#include "AudioSource.h"
#include "../../EntityAdmin.h"
#include "Physics.h"
#include "../transform.h"
#include "../../math/Vector.h"

AudioSource::AudioSource() {
	//empty version for adding component through command
	cpystr(name, "AudioSource", 63);
	sortid = 1;
	layer = CL3_SOUND;
}

AudioSource::AudioSource(char* snd_file, Physics* p, Transform* t, bool loop, float gain, float pitch) {
	this->snd_file = snd_file;
	this->loop = loop;
	this->gain = gain;
	this->pitch = pitch;
	if (p != nullptr) {
		physpoint = true;
		this->p = p;
	}
	else {
		ASSERT(t != nullptr, "if no physics pointer is specified, tranform must be");
		this->t = t;
	}
	
	cpystr(name, "AudioSource", 63);
	sortid = 1;
	layer = CL3_SOUND;
}

void AudioSource::RequestPlay(float gain, float pitch) {
	this->gain = gain; this->pitch = pitch;
	request_play = true;
}

void AudioSource::Load(std::vector<Entity>& entities, const char* data, u32& cursor, u32 count){
	u32 entityID = 0xFFFFFFFF;
	
	for_n(i,count){
		memcpy(&entityID, data+cursor, sizeof(u32)); 
		cursor += sizeof(u32);
		if(entityID >= entities.size()) {
			ERROR("Failed to load audio source component at pos '", cursor-sizeof(u32),
				  "' because it has an invalid entity ID: ", entityID);
			continue;
		}
		
		entities[entityID].AddComponent(new AudioSource());
	}
}