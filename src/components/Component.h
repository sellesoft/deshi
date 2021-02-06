#pragma once
#include <vector>
#include "../utils/dsh_defines.h"

struct Entity;

struct Component {
	Entity* entity = nullptr; //reference to owning entity
	//virtual void Create(resourceHandle) = 0;
	EntityAdmin* admin = nullptr;
	virtual void Create() {};
	virtual ~Component() {};
};