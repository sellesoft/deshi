#pragma once
#ifndef COMPONENT_COLLIDER_H
#define COMPONENT_COLLIDER_H

#include "Component.h"
#include "../../math/VectorMatrix.h"

struct Command;
struct Mesh;

enum ColliderTypeBits : u32{
	ColliderType_NONE, ColliderType_Box, ColliderType_AABB, ColliderType_Sphere
}; typedef u32 ColliderType;

//TODO(delle,Ph) maybe add offset vec3
struct Collider : public Component {
	ColliderType type;
	u32 collisionLayer;
	Matrix3 inertiaTensor;
	Command* command; //TODO(delle,Ph) implement trigger colliders
};

//rotatable box
struct BoxCollider : public Collider {
	Vector3 halfDims; //half dimensions, entity's position to the bounding box's locally positive corner
	
	BoxCollider(Vector3 halfDimensions, Matrix3& tensor, u32 collisionLayer = 0, Command* command = nullptr);
	BoxCollider(Vector3 halfDimensions, float mass, u32 collisionLayer = 0, Command* command = nullptr);
	
	std::vector<char> Save() override;
	static void Load(std::vector<Entity>& entityArray, const char* fileData, u32& cursor, u32 countToLoad);
};

//axis-aligned bounding box
struct AABBCollider : public Collider {
	Vector3 halfDims; //half dimensions, entity's position to the bounding box's locally positive corner
	
	AABBCollider(Mesh* mesh, float mass, u32 collisionLayer = 0, Command* command = nullptr);
	AABBCollider(Vector3 halfDimensions, Matrix3& tensor, u32 collisionLayer = 0, Command* command = nullptr);
	AABBCollider(Vector3 halfDimensions, float mass, u32 collisionLayer = 0, Command* command = nullptr);
	
	std::vector<char> Save() override;
	static void Load(std::vector<Entity>& entityArray, const char* fileData, u32& cursor, u32 countToLoad);
};

struct SphereCollider : public Collider {
	float radius;
	
	SphereCollider(float radius, Matrix3& tensor, u32 collisionLayer = 0, Command* command = nullptr);
	SphereCollider(float radius, float mass, u32 collisionLayer = 0, Command* command = nullptr);
	
	std::vector<char> Save() override;
	static void Load(std::vector<Entity>& entityArray, const char* fileData, u32& cursor, u32 countToLoad);
};

//TODO(delle,Ph) implement convexPolyCollider
//TODO(delle,Ph) implement capsuleCollider
//TODO(delle,Ph) implement cylinder collider
//TODO(delle,Ph) implement complex collider (collider list)

#endif //COMPONENT_COLLIDER_H