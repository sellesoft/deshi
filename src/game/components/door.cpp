#include "door.h"
#include "../../EntityAdmin.h"
#include "../entities/Entity.h"

Door::Door(b32 isOpen){
	this->isOpen = isOpen;
	comptype = ComponentType_Door;
}

void Door::ReceiveEvent(Event event){
	switch(event){
		case Event_DoorOpen:{
			isOpen = 1;
		}break;
		case Event_DoorClose:{
			isOpen = 0;
		}break;
	}
}

void Door::Load(EntityAdmin* admin, const char* data, u32& cursor, u32 count){
	u32 entityID = 0xFFFFFFFF;
	b32 isOpen = 0;
	for_n(i,count){
		memcpy(&entityID, data+cursor, sizeof(u32)); cursor += sizeof(u32);
		if(entityID >= admin->entities.size()) {
			ERROR("Failed to load audio listener component at pos '", cursor-sizeof(u32),
				  "' because it has an invalid entity ID: ", entityID);
			continue;
		}
		
		memcpy(&isOpen, data+cursor, sizeof(b32)); cursor += sizeof(b32);
		Door* c = new Door(isOpen);
		admin->entities[entityID].value.AddComponent(c);
		c->layer_index = admin->freeCompLayers[c->layer].add(c);
		c->Init(admin);
	}
}