#pragma once
#ifndef SYSTEM_SOUND_H
#define SYSTEM_SOUND_H

#include "System.h"

#if defined(_MSC_VER)
#pragma comment(lib,"OpenAL32.lib")
#endif
#include "al.h"
#include "alc.h"

struct SoundSystem : public System {
	
	//storing this here because I don't know where else to put it 
	const ALCchar* devices = 0;
	const ALCchar* defaultDeviceName = 0;
	int ret;
	char* bufferData = 0;
	ALCdevice* device = 0;
	ALvoid* data = 0;
	ALCcontext* context = 0;
	std::vector<ALuint*> buffers;
	
	SoundSystem(EntityAdmin* admin);
	
	void Init();
	void Update() override;
};

#endif //SYSTEM_SOUND_H