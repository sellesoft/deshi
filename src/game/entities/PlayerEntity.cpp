#include "PlayerEntity.h"
#include "../admin.h"
#include "../components/Player.h"
#include "../components/Physics.h"
#include "../components/MeshComp.h"
#include "../components/Collider.h"
#include "../components/Movement.h"
#include "../components/AudioSource.h"
#include "../components/AudioListener.h"


PlayerEntity::PlayerEntity(Transform transform) {
	cpystr(name, "player", DESHI_NAME_SIZE);
	this->transform = transform;
	physics = new Physics();
	physics->elasticity = 0;
	movement = new Movement(physics);
	player = new Player(movement);
	mesh = new MeshComp(0);
	collider = new AABBCollider(mesh->mesh, 1);
	listener = new AudioListener();
	//source = new AudioSource()
	AddComponents({ physics, movement, player, mesh, collider, listener });
}



void PlayerEntity::Init() {
	
}