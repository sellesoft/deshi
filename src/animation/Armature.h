#pragma once
#ifndef DESHI_ARMATURE_H
#define DESHI_ARMATURE_H

#include "Bone.h"

struct Armature {
	std::vector<Bone> bones;
	
	std::string name;
};

#endif //DESHI_ARMATURE_H