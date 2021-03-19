#include "AudioListener.h"

#include "../EntityAdmin.h"

AudioListener::AudioListener(Vector3 position, Vector3 velocity, Vector3 orientation) {
	this->position = position;
	this->velocity = velocity;
	this->orientation = orientation;
	
	layer = CL6_SOUND;
}