#include "dsh_WorldSystem.h"
#include "../dsh_EntityAdmin.h"
#include "../utils/dsh_Debug.h"

#include "../components/dsh_World.h"
#include "../components/dsh_Transform.h"
#include "../components/dsh_Mesh.h"

void WorldSystem::Init() {
	
}

void WorldSystem::Update() {
	World* world = admin->world;

	//deletion buffer
	for(Entity* entity : world->deletionBuffer) {
		uint32 id = entity->id;
		try {
			delete admin->entities.at(id);
		} catch(const std::out_of_range& oor) {
			ASSERT(false, "No entity at id when there should have been");
		}
		admin->entities.erase(id);
	}
	world->deletionBuffer.clear();

	//creation buffer
	for(Entity* entity : world->creationBuffer) {
		entity->id = !admin->entities.empty() ? admin->entities.rbegin()->second->id + 1 : 1; //set id to be one greater than the last
		admin->entities.insert(admin->entities.end(), {entity->id, entity}); //TODO(o,delle) see if this is worth it vs regular insert
		entity->admin = admin;
	}
	world->creationBuffer.clear();
}

Entity* WorldSystem::CreateEntity(EntityAdmin* admin) {
	World* world = admin->world;
	Entity* e = new Entity;
	world->creationBuffer.push_back(e);
	return e;
}

Entity* WorldSystem::CreateEntity(EntityAdmin* admin, Component* singleton) {
	World* world = admin->world;
	Entity* e = new Entity;
	AddAComponentToEntity(e, singleton);
	world->creationBuffer.push_back(e);
	return e;
}

Entity* WorldSystem::CreateEntity(EntityAdmin* admin, std::vector<Component*> components) {
	World* world = admin->world;
	Entity* e = new Entity;
	e->components = components;
	AddComponentsToEntity(e, components);
	world->creationBuffer.push_back(e);
	return e;
}

int32 WorldSystem::AddEntityToCreationBuffer(EntityAdmin* admin, Entity* entity) {
	World* world = admin->world;
	world->creationBuffer.push_back(entity);
	return world->deletionBuffer.size()-1;
}

int32 WorldSystem::AddEntityToDeletionBuffer(EntityAdmin* admin, Entity* entity) {
	World* world = admin->world;
	try {
		admin->entities.at(entity->id);
		world->deletionBuffer.push_back(entity);
		return world->deletionBuffer.size()-1;
	} catch(const std::out_of_range& oor) {
		return -1;
	}
}

int32 WorldSystem::AddAComponentToWorldEntity(EntityAdmin* admin, Entity* entity, Component* component) {
	try {
		Entity* e = admin->entities.at(entity->id);
		e->components.push_back(component);
		component->entity = e;
		return e->components.size()-1;
	} catch(const std::out_of_range& oor) {
		return -1;
	}
}

int32 WorldSystem::AddComponentsToWorldEntity(EntityAdmin* admin, Entity* entity, std::vector<Component*> components) {
	try {
		Entity* e = admin->entities.at(entity->id);
		int value = e->components.size();
		for(auto& c : components) {
			e->components.push_back(c);
			c->entity = entity;
		}
		return value;
	} catch(const std::out_of_range& oor) {
		return -1;
	}
}

int32 WorldSystem::AddAComponentToEntity(Entity* entity, Component* component) {
	entity->components.push_back(component);
	component->entity = entity;
	return entity->components.size()-1;
}

int32 WorldSystem::AddComponentsToEntity(Entity* entity, std::vector<Component*> components) {
	int value = entity->components.size();
	for(auto& c : components) {
		entity->components.push_back(c);
		c->entity = entity;
	}
	return value;
}

std::vector<Component*>* WorldSystem::GetComponentsOnWorldEntity(EntityAdmin* admin, Entity* entity) {
	try {
		return &admin->entities.at(entity->id)->components;
	} catch(const std::out_of_range& oor) {
		return 0;
	}
}

inline std::vector<Component*>* WorldSystem::GetComponentsOnEntity(Entity* entity) {
	return &entity->components;
}

bool WorldSystem::RemoveAComponentFromEntity(EntityAdmin* admin, Entity* entity, Component* component) {
	try {
		std::vector<Component*>* components = &admin->entities.at(entity->id)->components;
		for(int i = 0; i < components->size(); ++i) {
			if(components->at(i) == component) {
				delete components->at(i);
				components->erase(components->begin()+i);
				return true;
			}
		}
		return false;
	} catch(const std::out_of_range& oor) {
		return false;
	}
}

bool WorldSystem::RemoveComponentsFromEntity(EntityAdmin* admin, Entity* entity, std::vector<Component*> components) { //TODO test this
	try {
		std::vector<Component*>* coms = &admin->entities.at(entity->id)->components;
		bool value = false;
		for(int i = 0; i < components.size(); ++i) {
			for(int j = 0; j < coms->size(); ++j) {
				if(components.at(i) == coms->at(j)) {
					delete coms->at(j);
					coms->erase(coms->begin()+j);
					value = true;
				}
			}
			if(!value) {
				return false;
			} else {
				value = false;
			}
		}
		return value;
	} catch(const std::out_of_range& oor) {
		return false;
	}
}
