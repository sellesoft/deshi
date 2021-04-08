#include "AudioSource.h"
#include "../../EntityAdmin.h"
#include "Physics.h"
#include "../transform.h"
#include "../../math/Vector.h"

AudioSource::AudioSource() {
	//empty version for adding component through command
	strncpy_s(name, "AudioSource", 63);
	this->name[63] = '\0';
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

	strncpy_s(name, "AudioSource", 63);
	this->name[63] = '\0';
	sortid = 1;
	layer = CL3_SOUND;
}

void AudioSource::RequestPlay(float gain, float pitch) {
	this->gain = gain; this->pitch = pitch;
	request_play = true;
}

