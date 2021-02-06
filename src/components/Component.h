#pragma once
#include <vector>
#include "../utils/defines.h"

struct Entity;
struct EntityAdmin;

struct Component {
	Entity* entity = nullptr; //reference to owning entity
	EntityAdmin* admin = nullptr; //do not want to call admin through entity anymore sorry!
	//virtual void Create(resourceHandle) = 0;
	virtual void Update() {};
	virtual ~Component() {};
};