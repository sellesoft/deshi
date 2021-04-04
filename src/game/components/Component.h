#pragma once
#ifndef COMPONENT_H
#define COMPONENT_H

#include "../../utils/defines.h"
#include "../../core/event.h"

#include <vector>
#include <string>

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
	
	const char* name;
	
	Component(EntityAdmin* a = nullptr, Entity* e = nullptr);
	
	//store layer its on and where in that layer it is for deletion
	CompLayer layer = NONE;
	int layer_index;
	
	void ConnectSend(Component* c);
	virtual ~Component() {};
	virtual void Update() {};
	virtual void ReceiveEvent(Event event) override {};
	virtual std::string str(){ return ""; };
};

#endif //COMPONENT_H