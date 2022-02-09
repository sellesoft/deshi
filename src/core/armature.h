#pragma once
#ifndef DESHI_ARMATURE_H
#define DESHI_ARMATURE_H

#include "kigu/common.h"
#include "math/vector.h"
#include "math/matrix.h"

struct Bone{
	char name[64];
	
	vec3 head; //relative to armature
	vec3 tail; //vector from head
	
	u32 childrenCount;
	Bone* childrenArray;
	Bone* parent;
};

struct Armature{
	char name[64];
	
	u32 boneCount;
	Bone* boneArray;
};

#endif //DESHI_ARMATURE_H