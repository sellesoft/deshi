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
	ComponentLayer_Sound,
	ComponentLayer_World,
	ComponentLayer_LAST,
	SystemLayer_Physics = 0,
}; typedef u32 ComponentLayer;

struct ComponentTypeHeader{
	u32 type;
	u32 size;
	u32 count;
	u32 arrayOffset;
};

enum ComponentTypeBits : u32{
	ComponentType_NONE              = 0,
	ComponentType_MeshComp          = 1 << 0,
	ComponentType_Physics           = 1 << 1, 
	ComponentType_Collider          = 1 << 2, //TODO(delle,Cl) consolidate these to one collider since we have ColliderType now
	ComponentType_ColliderBox       = 1 << 3, //TODO(delle,Cl) consolidate these to one collider since we have ColliderType now
	ComponentType_ColliderAABB      = 1 << 4,
	ComponentType_ColliderSphere    = 1 << 5,
	ComponentType_ColliderLandscape = 1 << 6,
	ComponentType_AudioListener     = 1 << 7,
	ComponentType_AudioSource       = 1 << 8,
	ComponentType_Camera            = 1 << 9,
	ComponentType_Light             = 1 << 10,
	ComponentType_OrbManager        = 1 << 11,
	ComponentType_Door              = 1 << 12,
	ComponentType_Player            = 1 << 13,
	ComponentType_Movement          = 1 << 14,
	ComponentType_LAST = 0xFFFFFFFF,
}; typedef u32 ComponentType;

struct Component : public Receiver {
	EntityAdmin* admin = 0;
	u32 entityID = -1;
	char name[64];
	ComponentType comptype;

	Entity* entity;

	//sender for outputting events to a list of receivers
	Sender* send = nullptr;
	
	Component(){};
	Component(EntityAdmin* a, u32 entityID);
	virtual ~Component();
	
	//store layer its on and where in that layer it is for deletion
	ComponentLayer layer = ComponentLayer_NONE;
	int layer_index;
	
	//Init only gets called when this component's entity is spawned thru the world system
	virtual void Init(EntityAdmin* admin) {};
	virtual void Update() {};
	void ConnectSend(Component* c);
	virtual void ReceiveEvent(Event event) override {};
	virtual std::vector<char> Save() { return std::vector<char>(); };
	virtual void Load() {};
	virtual std::string str(){ return ""; };
};

#endif //COMPONENT_H