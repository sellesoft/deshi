#pragma once
#include "Component.h"
#include "Camera.h"
#include "../math/Vector3.h"

//this is what OpenAL sees as the receiver of sounds in 3D space
//there can only ever be one of them as far as I know.
//this will be implemented further later
struct Listener : public Component {
	Vector3 position;
	Vector3 velocity; //these may not be necessary
	Vector3 orientation;

	Listener(Vector3 position, Vector3 velocity = Vector3::ZERO, Vector3 orientation = Vector3::ZERO);
};