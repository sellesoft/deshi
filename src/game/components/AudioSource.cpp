#include "AudioSource.h"
#include "../admin.h"

#include "Physics.h"
#include "../transform.h"
#include "../../math/Vector.h"

AudioSource::AudioSource() {
	//empty version for adding component through command
	cpystr(name, "AudioSource", 63);
	layer = ComponentLayer_Sound;
	comptype = ComponentType_AudioSource;
	sender = new Sender();
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
	layer = ComponentLayer_Sound;
	comptype = ComponentType_AudioSource;
	sender = new Sender();
}

void AudioSource::RequestPlay(float gain, float pitch) {
	this->gain = gain; this->pitch = pitch;
	request_play = true;
}

std::vector<char> AudioSource::Save() {
	std::vector<char> out;
	return out;
}

void AudioSource::Load(EntityAdmin* admin, const char* data, u32& cursor, u32 count){
	u32 entityID = 0xFFFFFFFF, compID = 0xFFFFFFFF, event = 0xFFFFFFFF;
	for_n(i,count){
		memcpy(&entityID, data+cursor, sizeof(u32)); 
		cursor += sizeof(u32);
		if(entityID >= admin->entities.size()) {
			ERROR("Failed to load audio source component at pos '", cursor-sizeof(u32),
				  "' because it has an invalid entity ID: ", entityID);
			continue;
		}
		memcpy(&compID, data + cursor, sizeof(u32)); cursor += sizeof(u32);
		memcpy(&event, data + cursor, sizeof(u32)); cursor += sizeof(u32);
		//c->SetCompID(compID);
		//c->SetEvent(event);
		ERROR("AudioSource::Load not setup");
	}
}