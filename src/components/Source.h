#pragma once
#include "Component.h"
#include "Physics.h"
#include "Transform.h"
#include "../math/Vector3.h"

#include "al.h"
#include "alc.h"

//this is what OpenAL sees as the source of sound in 3D space
struct Source : public Component {
	//pointers to either a tranform or physics component
	//physics pointer is necessary if you want to be able to apply the doppler
	//effect to an object's sound. this also allows us to access these elements through
	//them rather than storing them here
	Physics* p = nullptr;
	Transform* t = nullptr;

	ALuint source;
	ALint source_state;

	bool physpoint = false;
	
	float gain = 1;
	float pitch = 1; //ranges from 0.5 to 2

	bool loop = false;
	bool request_play = false;

	//path to sound file
	//this will probably be a collection of sounds eventually, once i figure out how i want that to work
	char* snd_file; 

	void RequestPlay(float gain = 1, float pitch = 1) {
		this->gain = gain; this->pitch = pitch;
		request_play = true;
	}

	Source(char* snd_file, Physics* p, Transform* t = nullptr, bool loop = false, float gain = 1, float pitch = 1) {
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
	}
};