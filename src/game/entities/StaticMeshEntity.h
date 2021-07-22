#pragma once
#ifndef GAME_ENTITY_STATICMESH_H
#define GAME_ENTITY_STATICMESH_H

#include "Entity.h"

typedef u32 ColliderShape;
struct Model;
struct ModelInstance;
struct Physics;
struct Collider;

struct StaticMesh : public Entity{
	ModelInstance* model;
	Physics*       physics;
	Collider*      collider;
	
	StaticMesh(Transform transform = Transform(), const char* name = 0);
	StaticMesh(Model* model, ColliderShape colliderShape, f32 mass = 1.f, Transform transform = Transform(), const char* name = 0);
};

#endif //GAME_ENTITY_STATICMESH_H
