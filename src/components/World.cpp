#include "World.h"

#include "../EntityAdmin.h"

World::World() {
	creationBuffer = std::vector<Entity*>();
	deletionBuffer = std::vector<Entity*>();

	//not actually sure where this should go
	layer = CL7_LAST;
}

World::~World() {
	for (Entity* e : creationBuffer) delete e;
	creationBuffer.clear();
	for (Entity* e : deletionBuffer) delete e;
	deletionBuffer.clear();
}

void World::Update() {

}
