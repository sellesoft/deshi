#pragma once
#include "../math/dsh_Math.h"

struct Armature;

struct Bone {
	Vector3 head; //relative to armature
	Vector3 tail; //vector from head

	Armature* armature;
	Bone* parent;
	std::vector<Bone*> children;

	std::string name;
};