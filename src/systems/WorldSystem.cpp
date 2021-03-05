#include "WorldSystem.h"
#include "ConsoleSystem.h"
#include "../EntityAdmin.h"
#include "../utils/Debug.h"

#include "../components/World.h"
#include "../components/Transform.h"

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
			DASSERT(false, "No entity at id when there should have been");
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
	AddAComponentToEntity(admin, e, singleton);
	world->creationBuffer.push_back(e);
	return e;
}

Entity* WorldSystem::CreateEntity(EntityAdmin* admin, std::vector<Component*> components) {
	World* world = admin->world;
	Entity* e = new Entity;
	e->components = components;
	AddComponentsToEntity(admin, e, components);
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
		Entity* e = admin->entities.at(entity->id);
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
		admin->freeCompLayers[component->layer].add(component);
		component->entity = e;
		component->admin = admin;
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
			admin->freeCompLayers[c->layer].add(c);
			c->entity = entity;
			c->admin = admin;
		}
		return value;
	} catch(const std::out_of_range& oor) {
		return -1;
	}
}

int32 WorldSystem::AddAComponentToEntity(EntityAdmin* admin, Entity* entity, Component* component) {
	entity->components.push_back(component);
	admin->freeCompLayers[component->layer].add(component);
	component->entity = entity;
	component->admin = entity->admin; // :/
	return entity->components.size()-1;
}

int32 WorldSystem::AddComponentsToEntity(EntityAdmin* admin, Entity* entity, std::vector<Component*> components) {
	int value = entity->components.size();
	for(auto& c : components) {
		entity->components.push_back(c);
		admin->freeCompLayers[c->layer].add(c);
		c->entity = entity;
		c->admin = entity->admin;
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
				admin->freeCompLayers[component->layer].remove_from(component->layer_index);
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
