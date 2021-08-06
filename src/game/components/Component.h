#pragma once
#ifndef COMPONENT_H
#define COMPONENT_H

#include "../Event.h"
#include "../../defines.h"
#include "../../math/VectorMatrix.h"

#include <vector>
#include <string>

struct Entity;
struct Admin;

//suffix is the system that comes after the layer
enum ComponentLayer_{
	ComponentLayer_NONE,
	ComponentLayer_Physics,
	ComponentLayer_Canvas,
	ComponentLayer_Sound,
	ComponentLayer_World,
	ComponentLayer_COUNT,
	SystemLayer_Physics = 0,
}; typedef u32 ComponentLayer;

struct ComponentTypeHeader{
	u32 type;
	u32 size;
	u32 count;
	u32 arrayOffset;
};

enum ComponentType_{
	ComponentType_NONE,
	ComponentType_ModelInstance,
	ComponentType_Physics,
	ComponentType_Collider,
	ComponentType_AudioListener,
	ComponentType_AudioSource,
	ComponentType_Camera,
	ComponentType_Light,
	ComponentType_OrbManager,
	ComponentType_Door,
	ComponentType_Player,
	ComponentType_Movement,
	ComponentType_COUNT,
}; typedef u32 ComponentType;
global_ const char* ComponentTypeStrings[] = {
	"None", "ModelInstance", "Physics", "Collider", "AudioListener", "AudioSource", "Camera", "Light", "OrbManager", "Door", "Player", "Movement"
};

struct Component : public Receiver {
	Entity* entity = 0;
	ComponentType type = ComponentType_NONE;
	Sender sender; //sender for outputting events to a list of receivers
	Event event = Event_NONE; //event to be sent TODO(sushi) implement multiple events being able to be sent
	
	ComponentLayer layer = ComponentLayer_NONE;
	int layer_index = -1; //index in the layer for deletion
	u32 compID; //this should ONLY be used for saving/loading, not indexing anykind of array for now
	
	void ConnectSend(Component* c){ c->sender.AddReceiver(this); }
	
	virtual void Init(){}; //Init gets called by admin's creation buffer
	virtual void Start(){}; //Start gets called when the game starts
	virtual void Update(){};
	virtual std::string SaveTEXT(){ return ""; };
	virtual std::string str(){ return ""; };
};

#endif //COMPONENT_H