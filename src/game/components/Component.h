#pragma once
#ifndef COMPONENT_H
#define COMPONENT_H

#include "../../utils/defines.h"
#include "../Event.h"

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
	ComponentType_ColliderBox       = 1 << 3,
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
}; typedef u32 ComponentType;
static const char* ComponentTypeStrings[] = {
	"None", "MeshComp", "Physics", "Collider", "ColliderBox", "ColliderAABB", "ColliderSphere", "ColliderLandscape", "AudioListener", "AudioSource", "Camera", "Light", "OrbManager", "Door", "Player", "Movement"
};

struct Component : public Receiver {
	EntityAdmin* admin;
	u32 entityID;
	char name[64];
	ComponentType comptype;
	Entity* entity;
	Sender* sender = nullptr; //sender for outputting events to a list of receivers
	ComponentLayer layer = ComponentLayer_NONE;
	int layer_index; //index in the layer for deletion
	
	virtual ~Component() {
		if(sender) sender->RemoveReceiver(this);
	}
	
	void ConnectSend(Component* c) {
		c->sender->AddReceiver(this);
	}
	
	//Init only gets called when this component's entity is spawned thru the world system
	virtual void Init() {};
	virtual void Update() {};
	virtual void ReceiveEvent(Event event) override {};
	virtual std::vector<char> Save() { return std::vector<char>(); };
	virtual void Load() {};
	virtual std::string str(){ return ""; };
};

/* //switch statement on component type for copy/paste
switch(c->comptype){
				case ComponentType_MeshComp:{
					
				}break;
				case ComponentType_Physics:{
					
				}break;
				case ComponentType_Collider:{
					switch(dyncasta(Collider, c)->type){
						case ColliderType_Box:{
							
						}break;
						case ColliderType_AABB:{
							
						}break;
						case ColliderType_Sphere:{
							
						}break;
						case ColliderType_Landscape:{
							
						}break;
					}
				}break;
				case ComponentType_AudioListener:{
					
				}break;
				case ComponentType_AudioSource:{
					
				}break;
				case ComponentType_Camera:{
					
				}break;
				case ComponentType_Light:{
					
				}break;
				case ComponentType_OrbManager:{
					
				}break;
				case ComponentType_Door:{
					
				}break;
				case ComponentType_Player:{
					
				}break;
				case ComponentType_Movement:{
					
				}break;
			}
*/

#endif //COMPONENT_H