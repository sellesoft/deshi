#pragma once
#ifndef SYSTEM_CANVAS_H
#define SYSTEM_CANVAS_H
#include "../../defines.h"

struct EntityAdmin;

struct CanvasSystem {
	EntityAdmin* admin;
	
	void Init(EntityAdmin* admin);
	void Update();
};

#endif //SYSTEM_CANVAS_H