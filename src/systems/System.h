#pragma once
#include "../dsh_EntityAdmin.h"

struct System {
	EntityAdmin* admin; //reference to owning admin
	double time = 0;

	virtual void Init() {}
	virtual void Update() = 0;
	virtual double Duration() { return time; }
	//virtual void NotifyComponent(Component*) = 0;
};