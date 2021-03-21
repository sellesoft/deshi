#pragma once
#ifndef DESHI_BONE_H
#define DESHI_BONE_H

#include "../math/Math.h"

struct Armature;

struct Bone {
	Vector3 head; //relative to armature
	Vector3 tail; //vector from head
	
	Armature* armature;
	Bone* parent;
	std::vector<Bone*> children;
	
	std::string name;
};

#endif //DESHI_BONE_H