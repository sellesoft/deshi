#pragma once
#ifndef COMPONENT_COLLIDER_H
#define COMPONENT_COLLIDER_H

#include "Component.h"
#include "../../math/VectorMatrix.h"

struct Command;
struct Mesh;

enum ColliderTypeBits : u32{
	ColliderType_NONE, ColliderType_Box, ColliderType_AABB, ColliderType_Sphere, ColliderType_Landscape
}; typedef u32 ColliderType;

//TODO(delle,Ph) maybe add offset vec3
struct Collider : public Component {
	ColliderType type;
	u32 collisionLayer;
	Matrix3 inertiaTensor;
	b32 noCollide;
	Command* command;
	
	virtual void RecalculateTensor(f32 mass) {};
};

//rotatable box
struct BoxCollider : public Collider {
	Vector3 halfDims; //half dimensions, entity's position to the bounding box's locally positive corner
	
	BoxCollider(Vector3 halfDimensions, Matrix3& tensor, u32 collisionLayer = 0, Command* command = nullptr, b32 noCollide = 0);
	BoxCollider(Vector3 halfDimensions, float mass, u32 collisionLayer = 0, Command* command = nullptr, b32 noCollide = 0);
	
	void RecalculateTensor(f32 mass) override;
	std::vector<char> Save() override;
	static void Load(EntityAdmin* admin, const char* fileData, u32& cursor, u32 countToLoad);
};

//axis-aligned bounding box
struct AABBCollider : public Collider {
	Vector3 halfDims; //half dimensions, entity's position to the bounding box's locally positive corner
	
	AABBCollider(Mesh* mesh, float mass, u32 collisionLayer = 0, Command* command = nullptr, b32 noCollide = 0);
	AABBCollider(Vector3 halfDimensions, Matrix3& tensor, u32 collisionLayer = 0, Command* command = nullptr, b32 noCollide = 0);
	AABBCollider(Vector3 halfDimensions, float mass, u32 collisionLayer = 0, Command* command = nullptr, b32 noCollide = 0);
	
	void RecalculateTensor(f32 mass) override;
	std::vector<char> Save() override;
	static void Load(EntityAdmin* admin, const char* fileData, u32& cursor, u32 countToLoad);
};

struct SphereCollider : public Collider {
	float radius;
	
	SphereCollider(float radius, Matrix3& tensor, u32 collisionLayer = 0, Command* command = nullptr, b32 noCollide = 0);
	SphereCollider(float radius, float mass, u32 collisionLayer = 0, Command* command = nullptr, b32 noCollide = 0);
	
	void RecalculateTensor(f32 mass) override;
	std::vector<char> Save() override;
	static void Load(EntityAdmin* admin, const char* fileData, u32& cursor, u32 countToLoad);
};

//collider for terrain
struct LandscapeCollider : public Collider {
	std::vector<std::pair<AABBCollider, Vector3>> aabbcols; //aabb colliders and their local positions
	
	LandscapeCollider(Mesh* mesh, u32 collisionleyer = 0, Command* command = nullptr, b32 noCollide = 0);
	
	static void Load(EntityAdmin* admin, const char* fileData, u32& cursor, u32 countToLoad);
};



struct ConvexPolyCollider : public Collider {
	
};

//collider defined by arbitrary mesh
struct ComplexCollider : public Collider {
	Mesh* mesh;
	
	ComplexCollider(Mesh* mesh, u32 collisionleyer = 0, Command* command = nullptr, b32 noCollide = 0);
	
	static void Load(EntityAdmin* admin, const char* fileData, u32& cursor, u32 countToLoad);
};

//TODO(delle,Ph) implement convexPolyCollider
//TODO(delle,Ph) implement capsuleCollider
//TODO(delle,Ph) implement cylinder collider
//TODO(delle,Ph) implement complex collider (collider list)

#endif //COMPONENT_COLLIDER_H