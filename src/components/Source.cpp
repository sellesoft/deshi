#include "Source.h"

#include "../EntityAdmin.h"

Source::Source(char* snd_file, Physics* p, Transform* t, bool loop, float gain, float pitch) {
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

	layer = CL6_SOUND;
}

void Source::RequestPlay(float gain, float pitch) {
	this->gain = gain; this->pitch = pitch;
	request_play = true;
}

