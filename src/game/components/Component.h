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
enum ComponentLayerBits : u32{
	ComponentLayer_NONE = 0,
	ComponentLayer_Physics,
	ComponentLayer_Canvas,
	ComponentLayer_World,
	ComponentLayer_Sound,
	ComponentLayer_LAST,
	SystemLayer_Physics,
}; typedef u32 ComponentLayer;

struct ComponentTypeHeader{
	u32 type;
	u32 size;
	u32 count;
	u32 arrayOffset;
};

enum ComponentTypeBits : u32{
	ComponentType_NONE           = 0,
	ComponentType_MeshComp       = 1 << 0,
	ComponentType_Physics        = 1 << 1, 
	ComponentType_ColliderBox    = 1 << 2,
	ComponentType_ColliderAABB   = 1 << 3,
	ComponentType_ColliderSphere = 1 << 4,
	ComponentType_AudioListener  = 1 << 5,
	ComponentType_AudioSource    = 1 << 6,
	ComponentType_Camera         = 1 << 7,
	ComponentType_Light          = 1 << 8,
	ComponentType_OrbManager     = 1 << 9,
	ComponentType_LAST = 0xFFFFFFFF,
}; typedef u32 ComponentType;

struct Component : public Receiver {
	char name[64];
	
	Entity* entity = nullptr; //reference to owning entity
	EntityAdmin* admin = nullptr; 
	
	
	//sender for outputting events to a list of receivers
	Sender* send = nullptr;
	
	Component(EntityAdmin* a = nullptr, Entity* e = nullptr);
	virtual ~Component();
	
	//store layer its on and where in that layer it is for deletion
	ComponentLayer layer = ComponentLayer_NONE;
	int layer_index;
	
	//Init only gets called when this component's entity is spawned thru the world system
	virtual void Init() {};
	virtual void Update() {};
	void ConnectSend(Component* c);
	virtual void ReceiveEvent(Event event) override {};
	virtual std::vector<char> Save() { return std::vector<char>(); };
	virtual void Load() {};
	virtual std::string str(){ return ""; };
};

#endif //COMPONENT_H