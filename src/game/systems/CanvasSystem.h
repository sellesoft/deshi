#pragma once
#ifndef SYSTEM_CANVAS_H
#define SYSTEM_CANVAS_H
#include "../../defines.h"

struct Admin;

struct CanvasSystem {
	Admin* admin;
	
	void Init(Admin* admin);
	void Update();
};

#endif //SYSTEM_CANVAS_H