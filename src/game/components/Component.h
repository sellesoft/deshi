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
	char name[64];
	
	Entity* entity = nullptr; //reference to owning entity
	EntityAdmin* admin = nullptr; 
	
	
	//sender for outputting events to a list of receivers
	Sender* send = nullptr;
	
	Component(EntityAdmin* a = nullptr, Entity* e = nullptr);
	
	//sorting id for when we save because mmm yeah
	int sortid;

	//store layer its on and where in that layer it is for deletion
	CompLayer layer = NONE;
	int layer_index;
	
	void ConnectSend(Component* c);
	virtual ~Component() { if(send) send->RemoveReceiver(this); };
	virtual void Update() {};
	virtual std::string Save();
	virtual void Load();
	virtual void ReceiveEvent(Event event) override {};
	virtual std::string str(){ return ""; };
};

#endif //COMPONENT_H