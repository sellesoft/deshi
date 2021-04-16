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
	virtual ~Component() { if(send) send->RemoveReceiver(this); };
	
	//sorting id for when we save because mmm yeah
	int sortid;
	
	//store layer its on and where in that layer it is for deletion
	CompLayer layer = NONE;
	int layer_index;
	
	//Init only gets called when this component's entity is spawned thru the world system
	virtual void Init() {};
	virtual void Update() {};
	void ConnectSend(Component* c);
	virtual void ReceiveEvent(Event event) override {};
	virtual std::string Save();
	virtual void Load();
	virtual std::string str(){ return ""; };
};

#endif //COMPONENT_H