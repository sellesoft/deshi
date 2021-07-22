#pragma once
#ifndef COMPONENT_PHYSICS_H
#define COMPONENT_PHYSICS_H

#include "Component.h"
#include "../../utils/tuple.h"

#include <unordered_map>

typedef u32 ColliderShape;
struct Physics;
struct poly;
struct Collider;
struct Admin;

//only used for 2D currently
struct Manifold2 {
	poly* a = nullptr;
	poly* b = nullptr;
	
	int refID = 0;
	Vector2 colpoints[2];
	float depth[2];
	int nColPoints = 0;
	
	Vector2 norm;
};

struct Manifold3 {
	Collider* a = nullptr;
	Collider* b = nullptr;
	
	ColliderShape coltypea;
	ColliderShape coltypeb;
	
	//bool indicating which normal is the player's
	bool player = 0;
	
	int refID = 0;
	//point then depth
	std::vector<pair<Vector3, float>> colpoints;
	
	Vector3 norm;
};

//temp 2D polygon class 
struct poly {
	std::vector<Vector2> p;
	Vector2 pos;
	Vector2 vel;
	Vector2 acc;
	float rotvel = 0;
	float rotacc = 0;
	float angle = 0;
	float mass;
	float moi = 1;
	std::vector<Vector2> o;
	bool overlap = false;
	
	Physics* ogphys;
	
	bool staticPosition = false;
	
	bool PointInside(Vector2 point) {
		int s = p.size();
		for (int i = 0; i < s; i++) {
			Vector2 a = p[i];
			Vector2 b = p[(i + 1) % s];
			
			Vector2 n = (b - a).perp();
			
			if (n.dot(point - a) > 0) {
				return false;
			}
		}
		return true;
	}
	
	void update() {
		
		//rotate
		float angler = angle * M_PI / 180;
		for (Vector2 v : p) {
			v.x = cosf(angler * v.x) - sinf(angler * v.y);
			v.y = sinf(angler * v.x) + cosf(angler * v.y);
		}
		
		//translate
		for (int i = 0; i < o.size(); i++) {
			p[i] = o[i] + pos;
		}
		
	}
	
};

enum ContactState {
	ContactNONE,
	ContactStationary,
	ContactMoving
};

struct Physics : public Component {
	Vector3 position;
	Vector3 rotation;
	Vector3 scale;
	
	Vector3 velocity;
	Vector3 acceleration;
	Vector3 rotVelocity;
	Vector3 rotAcceleration;
	
	float elasticity; //less than 1 in most cases
	float mass;
	
	std::vector<Vector3> forces;
	Vector3 inputVector = Vector3::ZERO;
	
	bool staticPosition = false;
	bool staticRotation = false;
	//TODO(delle,Ph) separate static movement and rotation
	bool twoDphys = false;
	poly* twoDpolygon = nullptr;
	
	//this is probably temporary, i just need a way to communicate collision normals elsewhere
	std::unordered_map<Physics*, Manifold3> manifolds;
	
	//NOTE these default values are really only meant for debugging
	//and can be removed if I forget to remove them
	//they are the values of wood against wood
	float kineticFricCoef = 0.3;
	float staticFricCoef = 0.42;
	std::unordered_map<Physics*, ContactState> contacts;
	ContactState contactState;
	
	Physics();
	Physics(Vector3 position, Vector3 rotation, Vector3 velocity = Vector3::ZERO, Vector3 acceleration = Vector3::ZERO,
			Vector3 rotVeloctiy = Vector3::ZERO,Vector3 rotAcceleration = Vector3::ZERO, float elasticity = .2f, 
			float mass = 1.f, bool staticPosition = false);
	Physics(Vector3 position, Vector3 rotation, Vector3 velocity, Vector3 acceleration, Vector3 rotVeloctiy, Vector3 rotAcceleration, 
			float elasticity, float mass, bool staticPosition, bool staticRotation, bool twoDphys, 
			float kineticFricCoef, float staticFricCoef);
	Physics(Vector3 position, Vector3 rotation, float mass, float elasticity);
	
	//adds the input vector to the target's input vector
	void AddInput(Vector3 input);
	
	//changes acceleration by adding a force to target, target also applies the impulse to creator
	void AddForce(Physics* creator, Vector3 force);
	
	//if no creator, assume air friction; if creator, assume sliding friction
	//TODO(delle,Ph) change air friction to calculate for shape of object
	void AddFrictionForce(Physics* creator, float frictionCoef, float grav = 9.807);
	
	//changes velocity by adding an impulse to target, target also applies the impulse to creator
	void AddImpulse(Physics* creator, Vector3 impulse);
	void AddImpulseNomass(Physics* creator, Vector3 impulse);
	
	std::string SaveTEXT() override;
	static void LoadDESH(Admin* admin, const char* fileData, u32& cursor, u32 countToLoad);
};

#endif //COMPONENT_PHYSICS_H