#include "door.h"
#include "../admin.h"
#include "../entities/Entity.h"

Door::Door(b32 isOpen){
	admin = g_admin;
	cpystr(name, "MeshComp", 63);
	comptype = ComponentType_Door;
	
	this->isOpen = isOpen;
	sender = new Sender();
}

void Door::ReceiveEvent(Event event){
	if(event == Event_DoorToggle){
		isOpen = !isOpen;
	}
}

void Door::Load(EntityAdmin* admin, const char* data, u32& cursor, u32 count){
	u32 entityID = 0xFFFFFFFF, compID = 0xFFFFFFFF, event = 0xFFFFFFFF;
	b32 isOpen = 0;
	for_n(i,count){
		memcpy(&entityID, data+cursor, sizeof(u32)); cursor += sizeof(u32);
		if(entityID >= admin->entities.size()) {
			ERROR("Failed to load audio listener component at pos '", cursor-sizeof(u32),
				  "' because it has an invalid entity ID: ", entityID);
			continue;
		}
		memcpy(&compID, data + cursor, sizeof(u32)); cursor += sizeof(u32);
		memcpy(&event, data + cursor, sizeof(u32)); cursor += sizeof(u32);

		memcpy(&isOpen, data+cursor, sizeof(b32)); cursor += sizeof(b32);
		Door* c = new Door(isOpen);
		EntityAt(entityID)->AddComponent(c);
		c->SetCompID(compID);
		c->SetEvent(event);
		c->layer_index = admin->freeCompLayers[c->layer].add(c);
	}
}