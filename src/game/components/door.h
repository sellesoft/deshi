#pragma once
#ifndef COMPONENT_DOOR_H
#define COMPONENT_DOOR_H

#include "Component.h"

struct Door : public Component{
	b32 isOpen;
	
	Door(b32 isOpen = 0);
	
	void ReceiveEvent(Event event) override;
	static void Load(EntityAdmin* admin, const char* data, u32& cursor, u32 count);
};

#endif //COMPONENT_DOOR_H
