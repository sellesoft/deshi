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
	mat3 tensor;
	bool noCollide;
	bool sentEvent = false;
	
	std::set<Collider*> collided;
	
	virtual void RecalculateTensor(f32 mass) {};
	static void LoadDESH(Admin* admin, const char* fileData, u32& cursor, u32 countToLoad);
};

//rotatable box
struct BoxCollider : public Collider {
	vec3 halfDims; //half dimensions, entity's position to the bounding box's locally positive corner
	
	BoxCollider(vec3 halfDimensions, mat3& tensor, u32 collisionLayer = 0, Event event = Event_NONE, bool noCollide = 0);
	BoxCollider(vec3 halfDimensions, float mass, u32 collisionLayer = 0, Event event = Event_NONE, bool noCollide = 0);
	
	void RecalculateTensor(f32 mass) override;
	std::string SaveTEXT() override;
};

//axis-aligned bounding box
struct AABBCollider : public Collider {
	vec3 halfDims; //half dimensions, entity's position to the bounding box's locally positive corner
	
	AABBCollider(Mesh* mesh, float mass, u32 collisionLayer = 0, Event event = Event_NONE, bool noCollide = 0);
	AABBCollider(vec3 halfDimensions, mat3& tensor, u32 collisionLayer = 0, Event event = Event_NONE, bool noCollide = 0);
	AABBCollider(vec3 halfDimensions, float mass, u32 collisionLayer = 0, Event event = Event_NONE, bool noCollide = 0);
	
	void RecalculateTensor(f32 mass) override;
	std::string SaveTEXT() override;
};

struct SphereCollider : public Collider {
	float radius;
	
	SphereCollider(float radius, mat3& tensor, u32 collisionLayer = 0, Event event = Event_NONE, bool noCollide = 0);
	SphereCollider(float radius, float mass, u32 collisionLayer = 0, Event event = Event_NONE, bool noCollide = 0);
	
	void RecalculateTensor(f32 mass) override;
	std::string SaveTEXT() override;
};

//collider for terrain
struct LandscapeCollider : public Collider {
	std::vector<pair<AABBCollider, vec3>> aabbcols; //aabb colliders and their local positions
	
	LandscapeCollider(Mesh* mesh, u32 collisionleyer = 0, Event event = Event_NONE, bool noCollide = 0);
	
	std::string SaveTEXT() override;
};



struct ConvexPolyCollider : public Collider {
	
};

//collider defined by arbitrary mesh
struct ComplexCollider : public Collider {
	Mesh* mesh;
	
	ComplexCollider(Mesh* mesh, u32 collisionleyer = 0, Event event = Event_NONE, bool noCollide = 0);
	
	std::string SaveTEXT() override;
};

//TODO(delle,Ph) implement convexPolyCollider
//TODO(delle,Ph) implement capsuleCollider
//TODO(delle,Ph) implement cylinder collider
//TODO(delle,Ph) implement complex collider (collider list)

#endif //COMPONENT_COLLIDER_H