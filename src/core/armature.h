#pragma once
#ifndef DESHI_ARMATURE_H
#define DESHI_ARMATURE_H

#include "../defines.h"
#include "../math/vector.h"
#include "../math/matrix.h"

struct Bone{
	char name[DESHI_NAME_SIZE];
	
	vec3 head; //relative to armature
	vec3 tail; //vector from head
	
	u32 childrenCount;
	Bone* childrenArray;
	Bone* parent;
};

struct Armature{
	char name[DESHI_NAME_SIZE];
	
	u32 boneCount;
	Bone* boneArray;
};

#endif //DESHI_ARMATURE_H