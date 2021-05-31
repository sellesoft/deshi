#pragma once
#ifndef COMPONENT_AUDIOLISTENER_H
#define COMPONENT_AUDIOLISTENER_H

#include "Component.h"
#include "../../math/Vector.h"

//this is what OpenAL sees as the receiver of sounds in 3D space
//there can only ever be one of them as far as I know.
//this will be implemented further later
struct AudioListener : public Component {
	Vector3 position;
	Vector3 velocity; //these may not be necessary
	Vector3 orientation;
	
	AudioListener();
	AudioListener(Vector3 position, Vector3 velocity = Vector3::ZERO, Vector3 orientation = Vector3::ZERO);
	
	std::string SaveTEXT() override;
	//static void LoadTEXT(EntityAdmin* admin, std::string& text);
	static void LoadDESH(EntityAdmin* admin, const char* fileData, u32& cursor, u32 countToLoad);
};

#endif //COMPONENT_AUDIOLISTENER_H