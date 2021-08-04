#include "door.h"
#include "../admin.h"
#include "../entities/Entity.h"
#include "../../core/console.h"
#include "../../utils/debug.h"

Door::Door(bool isOpen){
	layer = ComponentLayer_Physics;
	type = ComponentType_Door;
	this->isOpen = isOpen;
}

void Door::ReceiveEvent(Event event){
	if(event == Event_DoorToggle){
		isOpen = !isOpen;
	}
}

std::string Door::SaveTEXT(){
	return TOSTDSTRING("\n>door"
					"\nopen ", (isOpen) ? "true" : "false",
					"\n");
}

void Door::LoadDESH(Admin* admin, const char* data, u32& cursor, u32 count){
	ERROR_LOC("LoadDESH not setup");
}