#include "PlayerEntity.h"
#include "../admin.h"
#include "../components/Player.h"
#include "../components/Physics.h"
#include "../components/MeshComp.h"
#include "../components/Collider.h"
#include "../components/Movement.h"
#include "../components/AudioSource.h"
#include "../components/AudioListener.h"


PlayerEntity::PlayerEntity(EntityAdmin* admin, u32 id, Transform t) : Entity(admin, id, t, "player") {
	physics = new Physics();
	movement = new Movement(physics);
	player = new Player(movement);
	mesh = new MeshComp(Mesh::CreateMeshFromOBJ("box.obj"));
	collider = new AABBCollider(mesh->mesh, 1);
	listener = new AudioListener();
	//source = new AudioSource()
	AddComponents({ physics, movement, player, mesh, collider, listener });
	
}

void PlayerEntity::Init() {

}