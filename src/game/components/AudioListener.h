#pragma once
#ifndef COMPONENT_AUDIOLISTENER_H
#define COMPONENT_AUDIOLISTENER_H

#include "Component.h"
#include "Camera.h"
#include "../../math/Vector.h"

//this is what OpenAL sees as the receiver of sounds in 3D space
//there can only ever be one of them as far as I know.
//this will be implemented further later
struct AudioListener : public Component {
	Vector3 position;
	Vector3 velocity; //these may not be necessary
	Vector3 orientation;
	
	AudioListener(Vector3 position, Vector3 velocity = Vector3::ZERO, Vector3 orientation = Vector3::ZERO);
};

#endif //COMPONENT_AUDIOLISTENER_H