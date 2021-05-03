#include "door.h"

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
