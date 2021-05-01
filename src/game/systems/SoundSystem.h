#pragma once
#ifndef SYSTEM_SOUND_H
#define SYSTEM_SOUND_H

#include "../../utils/defines.h"
#include <vector>

struct EntityAdmin;
typedef char ALCchar;
typedef struct ALCdevice_struct ALCdevice;
typedef struct ALCcontext_struct ALCcontext;
typedef void ALvoid;
typedef unsigned int ALuint;

struct SoundSystem{
	EntityAdmin* admin;
	const ALCchar* devices;
	const ALCchar* defaultDeviceName;
	int ret;
	char* bufferData;
	ALCdevice* device;
	ALvoid* data;
	ALCcontext* context;
	std::vector<ALuint*> buffers;
	
	void Init(EntityAdmin* admin);
	void Update();
};

#endif //SYSTEM_SOUND_H