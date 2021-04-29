#pragma once
#ifndef SYSTEM_H
#define SYSTEM_H

#include "../../EntityAdmin.h"
#include "../../core.h"

struct System {
	EntityAdmin* admin; //reference to owning admin
	
	virtual void Update() = 0;
	//virtual void NotifyComponent(Component*) = 0;
};

#endif //SYSTEM_H