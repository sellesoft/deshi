#pragma once
#ifndef COMPONENT_H
#define COMPONENT_H

#include <vector>
#include "../utils/defines.h"

struct Entity;
struct EntityAdmin;

//suffix is the system that comes after the layer
enum CompLayer {
	CL0_COMMAND,
	CL1_PHYSICS,
	CL2_RENDSCENE,
	CL3_RENDCANVAS,
	CL4_WORLD,
	CL5_CONSOLE,
	CL6_SOUND,
	CL7_LAST,
	PHYS_LAYER,
	TRANSFORM_LAYER
};


struct Component {
	Entity* entity = nullptr; //reference to owning entity
	EntityAdmin* admin = nullptr; //do not want to call admin through entity anymore sorry!
	//virtual void Create(resourceHandle) = 0;
	
	Component(EntityAdmin* a = nullptr, Entity* e = nullptr);
	
	//store layer its on and where in that layer it is for deletion
	CompLayer layer;
	int layer_index;
	
	virtual void Update() {};
	virtual ~Component() {};
};

#endif //COMPONENT_H