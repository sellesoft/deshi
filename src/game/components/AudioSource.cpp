#include "AudioSource.h"
#include "Physics.h"
#include "../admin.h"
#include "../transform.h"
#include "../../math/Vector.h"
#include "../../core/console.h"

AudioSource::AudioSource() {
	layer = ComponentLayer_Sound;
	type  = ComponentType_AudioSource;
}

AudioSource::AudioSource(const char* snd_file, Physics* p, Transform* t, bool loop, float gain, float pitch) {
	this->snd_file = (char*)snd_file;
	this->loop = loop;
	this->gain = gain;
	this->pitch = pitch;
	if (p != nullptr) {
		physpoint = true;
		this->p = p;
	}
	else {
		Assert(t != nullptr, "if no physics pointer is specified, tranform must be");
		this->t = t;
	}
	
	layer = ComponentLayer_Sound;
	type  = ComponentType_AudioSource;
}

void AudioSource::RequestPlay(float gain, float pitch) {
	this->gain = gain; this->pitch = pitch;
	request_play = true;
}

////////////////////////////
//// saving and loading ////
////////////////////////////

std::string AudioSource::SaveTEXT(){
	return TOSTDSTRING("\n>audio source"
					"\n");
}

void AudioSource::LoadDESH(Admin* admin, const char* data, u32& cursor, u32 count){
	ERROR("AudioSource::LoadDESH not setup");
}