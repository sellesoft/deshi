#pragma once
#ifndef GAME_ENTITY_STATICMESH_H
#define GAME_ENTITY_STATICMESH_H

#include "Entity.h"

struct MeshComp;
struct Physics;
struct Collider;

struct StaticMesh : public Entity{
	MeshComp* mesh;
	Physics*  physics;
	Collider* collider;
	
	StaticMesh(Transform transform = Transform(), const char* name = 0);
	StaticMesh(u32 meshID, u32 colliderType, f32 mass = 1.f, Transform transform = Transform(), const char* name = 0);
};

#endif //GAME_ENTITY_STATICMESH_H
