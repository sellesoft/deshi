#include "AudioListener.h"

#include "../admin.h"
#include "../../core/console.h"
#include "../../utils/debug.h"

AudioListener::AudioListener(Vector3 _position, Vector3 _velocity, Vector3 _orientation) {
	position    = _position;
	velocity    = _velocity;
	orientation = _orientation;
	
	type  = ComponentType_AudioListener;
	layer = ComponentLayer_Sound;
}

////////////////////////////
//// saving and loading ////
////////////////////////////

std::string AudioListener::SaveTEXT(){
	return TOSTRING("\n>audio listener"
					"\nposition    (",position.x,",",position.y,",",position.z,")"
					"\nvelocity    (",velocity.x,",",velocity.y,",",velocity.z,")"
					"\norientation (",orientation.x,",",orientation.y,",",orientation.z,")"
					"\n");
}

void AudioListener::LoadDESH(Admin* admin, const char* data, u32& cursor, u32 count){
	u32 entityID = 0xFFFFFFFF;
	forI(count){
		memcpy(&entityID, data+cursor, sizeof(u32)); cursor += sizeof(u32);
		if(entityID >= admin->entities.size()) {
			ERROR("Failed to load audio listener component at pos '", cursor-sizeof(u32),
				  "' because it has an invalid entity ID: ", entityID);
			continue;
		}
		AudioListener* c = new AudioListener();
		c->entity = EntityAt(entityID);
		c->type   = ComponentType_AudioListener;
		c->layer  = ComponentLayer_Sound;
		
		memcpy(&c->compID,      data+cursor, sizeof(u32));   cursor += sizeof(u32);
		memcpy(&c->event,       data+cursor, sizeof(Event)); cursor += sizeof(Event);
		memcpy(&c->position,    data+cursor, sizeof(vec3));  cursor += sizeof(vec3);
		memcpy(&c->velocity,    data+cursor, sizeof(vec3));  cursor += sizeof(vec3);
		memcpy(&c->orientation, data+cursor, sizeof(vec3));  cursor += sizeof(vec3);
		c->entity->AddComponent(c);
		c->layer_index = admin->freeCompLayers[c->layer].add(c);
	}
}