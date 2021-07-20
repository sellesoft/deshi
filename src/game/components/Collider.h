#pragma once
#ifndef COMPONENT_COLLIDER_H
#define COMPONENT_COLLIDER_H

#include "Component.h"
#include "../../math/VectorMatrix.h"
#include "../../utils/tuple.h"

#include <set>

struct Command;
struct Mesh;
struct Admin;

enum ColliderShapeBits{
	ColliderShape_NONE, 
	ColliderShape_Box, 
	ColliderShape_AABB,
	ColliderShape_Sphere, 
	ColliderShape_Landscape, 
	ColliderShape_Complex,
	ColliderShape_COUNT,
}; typedef u32 ColliderShape;
global_ const char* ColliderShapeStrings[] = {
	"None", "Box", "AABB", "Sphere", "Landscape", "Complex"
};

//TODO(delle,Ph) maybe add offset vec3
struct Collider : public Component {
	ColliderShape shape;
	u32 collLayer;
	Matrix3 tensor;
	b32 noCollide;
	b32 sentEvent = false;
	
	std::set<Collider*> collided;
	
	virtual void RecalculateTensor(f32 mass) {};
	static void LoadDESH(Admin* admin, const char* fileData, u32& cursor, u32 countToLoad);
};

//rotatable box
struct BoxCollider : public Collider {
	Vector3 halfDims; //half dimensions, entity's position to the bounding box's locally positive corner
	
	BoxCollider(Vector3 halfDimensions, Matrix3& tensor, u32 collisionLayer = 0, Event event = Event_NONE, b32 noCollide = 0);
	BoxCollider(Vector3 halfDimensions, float mass, u32 collisionLayer = 0, Event event = Event_NONE, b32 noCollide = 0);
	
	void RecalculateTensor(f32 mass) override;
	std::string SaveTEXT() override;
};

//axis-aligned bounding box
struct AABBCollider : public Collider {
	Vector3 halfDims; //half dimensions, entity's position to the bounding box's locally positive corner
	
	AABBCollider(Mesh* mesh, float mass, u32 collisionLayer = 0, Event event = Event_NONE, b32 noCollide = 0);
	AABBCollider(Vector3 halfDimensions, Matrix3& tensor, u32 collisionLayer = 0, Event event = Event_NONE, b32 noCollide = 0);
	AABBCollider(Vector3 halfDimensions, float mass, u32 collisionLayer = 0, Event event = Event_NONE, b32 noCollide = 0);
	
	void RecalculateTensor(f32 mass) override;
	std::string SaveTEXT() override;
};

struct SphereCollider : public Collider {
	float radius;
	
	SphereCollider(float radius, Matrix3& tensor, u32 collisionLayer = 0, Event event = Event_NONE, b32 noCollide = 0);
	SphereCollider(float radius, float mass, u32 collisionLayer = 0, Event event = Event_NONE, b32 noCollide = 0);
	
	void RecalculateTensor(f32 mass) override;
	std::string SaveTEXT() override;
};

//collider for terrain
struct LandscapeCollider : public Collider {
	std::vector<pair<AABBCollider, Vector3>> aabbcols; //aabb colliders and their local positions
	
	LandscapeCollider(Mesh* mesh, u32 collisionleyer = 0, Event event = Event_NONE, b32 noCollide = 0);
	
	std::string SaveTEXT() override;
};



struct ConvexPolyCollider : public Collider {
	
};

//collider defined by arbitrary mesh
struct ComplexCollider : public Collider {
	Mesh* mesh;
	
	ComplexCollider(Mesh* mesh, u32 collisionleyer = 0, Event event = Event_NONE, b32 noCollide = 0);
	
	std::string SaveTEXT() override;
};

//TODO(delle,Ph) implement convexPolyCollider
//TODO(delle,Ph) implement capsuleCollider
//TODO(delle,Ph) implement cylinder collider
//TODO(delle,Ph) implement complex collider (collider list)

#endif //COMPONENT_COLLIDER_H