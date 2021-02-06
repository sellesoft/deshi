#pragma once
#include "System.h"
#include "al.h"
#include "alc.h"




struct SoundSystem : public System {

	//storing this here because I don't know where else to put it 
	const ALCchar* devices;
	const ALCchar* defaultDeviceName;
	int ret;
	char* bufferData;
	ALCdevice* device;
	ALvoid* data;
	ALCcontext* context;
	std::vector<ALuint*> buffers;

	void Init() override;
	void Update() override;
};