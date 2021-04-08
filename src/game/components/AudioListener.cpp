#include "AudioListener.h"

#include "../../EntityAdmin.h"

AudioListener::AudioListener(Vector3 position, Vector3 velocity, Vector3 orientation) {
	this->position = position;
	this->velocity = velocity;
	this->orientation = orientation;
	
	layer = CL3_SOUND;

	sortid = 0;

	strncpy_s(name, "AudioListener", 63);
	this->name[63] = '\0';
	
}