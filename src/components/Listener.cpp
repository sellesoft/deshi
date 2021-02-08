#include "Listener.h"

#include "../EntityAdmin.h"

Listener::Listener(Vector3 position, Vector3 velocity, Vector3 orientation) {
	this->position = position;
	this->velocity = velocity;
	this->orientation = orientation;

	layer = CL6_SOUND;
}