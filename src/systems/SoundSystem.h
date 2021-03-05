#pragma once
#include "System.h"

#if defined(_MSC_VER)
#pragma comment(lib,"OpenAL32.lib")
#endif
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