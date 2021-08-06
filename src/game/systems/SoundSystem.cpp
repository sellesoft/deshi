#include "SoundSystem.h"
#include "../admin.h"
#include "../components/Camera.h"
#include "../components/AudioSource.h"
#include "../components/Physics.h"
#include "../../core/console.h"

#if defined(_MSC_VER)
#pragma comment(lib,"OpenAL32.lib")
#endif
#include "al.h"
#include "alc.h"

#include <thread>

#define DR_WAV_IMPLEMENTATION
#include "../../external/draudio/dr_wav.h"

#define TEST_ERROR check_al_errors(__FILE__, __LINE__, admin)

//bool thread_play = false;
bool new_sources = false;

std::vector<AudioSource*> sources;

//list all data devices found on the system
static void list_audio_devices(const ALCchar* devices)
{
	const ALCchar* device = devices, * next = devices + 1;
	size_t len = 0;
	
	fprintf(stdout, "Devices list:\n");
	fprintf(stdout, "----------\n");
	while (device && *device != '\0' && next && *next != '\0') {
		fprintf(stdout, "%s\n", device);
		len = strlen(device);
		device += (len + 1);
		next += (len + 2);
	}
	fprintf(stdout, "----------\n");
}

//checking OpenAL Errors. used with TEST_ERROR
bool check_al_errors(const std::string& filename, const std::uint_fast32_t line, Admin* admin){
	ALenum error = alGetError();
	if (error != AL_NO_ERROR){
		ERROR("***ERROR*** (", filename, ": ", line, ")\n");
		switch (error){
			case AL_INVALID_NAME:
			ERROR("AL_INVALID_NAME: a bad name (ID) was passed to an OpenAL function");
			break;
			case AL_INVALID_ENUM:
			ERROR("AL_INVALID_ENUM: an invalid enum value was passed to an OpenAL function");
			break;
			case AL_INVALID_VALUE:
			ERROR("AL_INVALID_VALUE: an invalid value was passed to an OpenAL function");
			break;
			case AL_INVALID_OPERATION:
			ERROR("AL_INVALID_OPERATION: the requested operation is not valid");
			break;
			case AL_OUT_OF_MEMORY:
			ERROR("AL_OUT_OF_MEMORY: the requested operation resulted in OpenAL running out of memory");
			break;
			default:
			ERROR("UNKNOWN AL ERROR: ", error);
		}
		return false;
	}
	return true;
}

ALenum to_al_format(int channels, int bitsPerSample, Admin* admin) {
	if (channels == 1 && bitsPerSample == 8)
		return AL_FORMAT_MONO8;
	else if (channels == 1 && bitsPerSample == 16)
		return AL_FORMAT_MONO16;
	else if (channels == 2 && bitsPerSample == 8)
		return AL_FORMAT_STEREO8;
	else if (channels == 2 && bitsPerSample == 16)
		return AL_FORMAT_STEREO16;
	else {
		Assert(!"unrecognized audio file format");
		return 0;
	}
}

void play_sound(AudioSource* s) {
	float x, y, z;
	
	alSourcePlay(s->source);
	//get source state
	alGetSourcei(s->source, AL_SOURCE_STATE, &s->source_state);
	alGetSource3f(s->source, AL_POSITION, &x, &y, &z);
	
	
	
	while (s->source_state == AL_PLAYING) {
		vec3 pos;
		vec3 vel = vec3::ZERO;
		
		if (s->physpoint) {
			pos = s->p->position;
			vel = s->p->velocity;
		}
		else {
			pos = s->t->position;
		}
		
		alSource3f(s->source, AL_POSITION, pos.x, pos.y, pos.z);
		alSource3f(s->source, AL_VELOCITY, vel.x, vel.y, vel.z);
		
		alGetSourcei(s->source, AL_SOURCE_STATE, &s->source_state);
	}
}

//main sound thread that is constant
void SoundThread(Admin* admin) {
	std::vector<AudioSource*> playingSources;
	while (true) {
		//if new sources are queued we add them
		if (sources.size() != 0) {
			for (AudioSource* s : sources) {
				//make sure source doesn't already exist
				if (!alIsSource(s->source)) {
					vec3 pos;
					vec3 vel = vec3::ZERO;
					if (s->physpoint) {
						pos = s->p->position;
						vel = s->p->velocity;
					}
					else {
						pos = s->t->position;
					}
					
					//generate source and set its data
					alGenSources((ALuint)1, &s->source);                     TEST_ERROR;
					alSourcef (s->source, AL_PITCH, s->pitch);               TEST_ERROR;
					alSourcef (s->source, AL_GAIN, s->gain);                 TEST_ERROR;
					alSource3f(s->source, AL_POSITION, pos.x, pos.y, pos.z); TEST_ERROR;
					alSource3f(s->source, AL_VELOCITY, vel.x, vel.y, vel.z); TEST_ERROR;
					alSourcei (s->source, AL_LOOPING, s->loop);              TEST_ERROR;
					
					ALuint buffer;
					alGenBuffers(1, &buffer); TEST_ERROR;
					
					//initialize file and get it's data
					drwav file;
					drwav_init_file(&file, s->snd_file, NULL);
					
					unsigned int channels = file.channels;
					unsigned int sampleRate = file.sampleRate;
					drwav_uint64 totalPCMFrameCount = file.totalPCMFrameCount;
					std::uint8_t bitsPerSample = file.bitsPerSample;
					
					
					
					drwav_int16* decoded = (drwav_int16*)malloc(file.totalPCMFrameCount * file.channels * sizeof(drwav_int16));
					size_t numberOfSamplesActuallyDecoded = drwav_read_pcm_frames_s16(&file, file.totalPCMFrameCount, decoded);
					
					//put data in buffer
					alBufferData(buffer, to_al_format(channels, bitsPerSample, admin), decoded, totalPCMFrameCount, sampleRate); TEST_ERROR;
					
					//bind buffer to source
					alSourcei(s->source, AL_BUFFER, buffer); TEST_ERROR;
					
					//play source
					alSourcePlay(s->source); TEST_ERROR;
					
					playingSources.push_back(s);
					
					free(decoded);
					
					new_sources = false;
					sources.clear();
				}
			}
		}
		
		//keep track of and manage playing sources
		for (int i = 0; i < playingSources.size(); i++) {
			alGetSourcei(playingSources[i]->source, AL_SOURCE_STATE, &playingSources[i]->source_state); TEST_ERROR;
			//if source is no longer playing we stop keeping track of it
			if (playingSources[i]->source_state != AL_PLAYING) {
				if (playingSources.size() == 1) {
					alDeleteSources(1, &playingSources[i]->source);
					playingSources.clear();
					
					
				}
				else {
					alDeleteSources(1, &playingSources[i]->source);
					playingSources.erase(playingSources.begin() + i);
				}
				
			}
			else {
				//else we update source's info
				vec3 pos;
				vec3 vel = vec3::ZERO;
				
				if (playingSources[i]->physpoint) {
					pos = playingSources[i]->p->position;
					vel = playingSources[i]->p->velocity;
				}
				else {
					pos = playingSources[i]->t->position;
				}
				
				alSource3f(playingSources[i]->source, AL_POSITION, pos.x, pos.y, pos.z); TEST_ERROR;
				alSource3f(playingSources[i]->source, AL_VELOCITY, vel.x, vel.y, vel.z); TEST_ERROR;
			}
		}
	}
}

void SoundSystem::Init(Admin* a) {
	admin = a;
	devices = 0;
	defaultDeviceName = 0;
	bufferData = 0;
	device = 0;
	context = 0;
	
	ALboolean enumeration;
	ALCenum error;
	ALint source_state;
	
	//can we enumerate audio devices?
	enumeration = alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT");
	if (enumeration == AL_FALSE)
		LOG_LOC("OpenAL unable to enumerate devices");
	
	//list audio devices
	//list_audio_devices(alcGetString(NULL, ALC_DEVICE_SPECIFIER));
	
	if (!defaultDeviceName)
		defaultDeviceName = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
	
	//attempt to choose a device
	device = alcOpenDevice(defaultDeviceName);
	TEST_ERROR;
	Assert(device, "unable to open a device with OpenAL");
	
	//display audio device selected in console
	LOG("Audio device selected: ", alcGetString(device, ALC_DEVICE_SPECIFIER));
	
	//attempt to create OpenAL context
	context = alcCreateContext(device, NULL);
	TEST_ERROR;
	Assert(alcMakeContextCurrent(context), "unable to make OpenAL context");
	
	//collect all sources at startup
	//TODO( sushi,So) if its necessary, make a way to collect new sources and to remove them
	//CollectSources(buffers, admin);
	
	//start main sound thread
	//std::thread sndthr(SoundThread, admin);
	//sndthr.detach();
	
}

void SoundSystem::Update() {
	Camera* c = admin->mainCamera;
	
	vec3 ld = c->forward;
	vec3 up = c->up;
	ALfloat listenerOri[] = { ld.x, ld.z, ld.y, up.x, up.z, up.y };
	//set OpenAL's listener to match our camera
	//right now it's only position and orientation
	alListener3f(AL_POSITION, c->position.x, c->position.y, c->position.z);  TEST_ERROR;
	//alListener3f(AL_VELOCITY, 0, 0, 0);
	//TEST_ERROR;
	alListenerfv(AL_ORIENTATION, listenerOri);  TEST_ERROR;
	
	//check if any source is requesting to play audio
	for (auto& e : admin->entities) {
		if (e) {
			for (Component* c : e->components) {
				if (AudioSource* s = dynamic_cast<AudioSource*>(c)) {
					if (s->source_state != AL_PLAYING && s->request_play) {
						sources.push_back(s);
						s->request_play = false;
						new_sources = true;
					}
				}
			}
		}
	}
	
	
	
}