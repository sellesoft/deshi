#pragma once
#ifndef COMPONENT_H
#define COMPONENT_H

#include <vector>
#include "../../utils/defines.h"
#include "../../core/event.h"

struct Entity;
struct EntityAdmin;

//suffix is the system that comes after the layer
enum CompLayer {
	CL0_PHYSICS,
	CL1_RENDCANVAS,
	CL2_WORLD,
	CL3_SOUND,
	CL4_LAST,
	PHYS_LAYER,
	TRANSFORM_LAYER,
	NONE
};


struct Component : public Receiver {
	Entity* entity = nullptr; //reference to owning entity
	EntityAdmin* admin = nullptr; 
	//virtual void Create(resourceHandle) = 0;
	
	Sender* send = nullptr;

	const char* name;

	Component(EntityAdmin* a = nullptr, Entity* e = nullptr);
	
	//store layer its on and where in that layer it is for deletion
	CompLayer layer = NONE;
	int layer_index;
	
	virtual void Update() {};
	virtual ~Component() {};
	virtual void ReceiveEvent(Event event) override {};
};

#endif //COMPONENT_H