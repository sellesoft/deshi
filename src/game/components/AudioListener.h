#pragma once
#ifndef COMPONENT_AUDIOLISTENER_H
#define COMPONENT_AUDIOLISTENER_H

#include "Component.h"

//this is what OpenAL sees as the receiver of sounds in 3D space
//there can only ever be one of them as far as I know.
//this will be implemented further later
struct AudioListener : public Component {
	vec3 position;
	vec3 velocity; //these may not be necessary
	vec3 orientation;
	
	AudioListener(){}
	AudioListener(vec3 position, vec3 velocity = vec3::ZERO, vec3 orientation = vec3::ZERO);
	
	std::string SaveTEXT() override;
	//static void LoadTEXT(Admin* admin, std::string& text);
	static void LoadDESH(Admin* admin, const char* fileData, u32& cursor, u32 countToLoad);
};

#endif //COMPONENT_AUDIOLISTENER_H