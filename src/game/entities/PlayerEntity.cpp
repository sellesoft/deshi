#include "PlayerEntity.h"
#include "../admin.h"
#include "../components/Player.h"
#include "../components/Physics.h"
#include "../components/MeshComp.h"
#include "../components/Collider.h"
#include "../components/Movement.h"
#include "../components/AudioSource.h"
#include "../components/AudioListener.h"
#include "../../core/model.h"

PlayerEntity::PlayerEntity(Transform _transform) {
	cpystr(name, "player", DESHI_NAME_SIZE);
	type = EntityType_Player;
	transform = _transform;
	physics  = new Physics();
	physics->elasticity = 0;
	movement = new Movement(physics);
	player   = new Player(movement);
	model    = new ModelInstance();
	collider = new AABBCollider(model->mesh, 1);
	listener = new AudioListener();
	source   = 0;
	AddComponents({ physics, movement, player, listener, source, collider, model,  });
}

void PlayerEntity::Init() {
	
}