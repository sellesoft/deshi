#include "WorldSystem.h"
#include "../../core/console.h"
#include "../../EntityAdmin.h"
#include "../../utils/debug.h"
#include "../components/Component.h"
#include "../Transform.h"

WorldSystem::WorldSystem(EntityAdmin* a){
	admin = a;
}

void WorldSystem::Update() {
	//deletion buffer
	for(Entity* e : deletionBuffer) {
		for(Component* c : e->components){ 
			admin->freeCompLayers[c->layer].remove_from(c->layer_index);
		}
		admin->entities.erase(admin->entities.begin()+e->id);
	}
	deletionBuffer.clear();
	
	//creation buffer
	for(Entity* e : creationBuffer) {
		admin->entities.emplace_back(admin, (u32)admin->entities.size(), e->transform, e->name, e->components);
		for(Component* c : e->components){ 
			c->entity = &admin->entities[admin->entities.size()-1]; 
			c->layer_index = admin->freeCompLayers[c->layer].add(c);
			c->Init();
		}
		operator delete(e); //call this to not delete components, but still delete the staging entity (doesnt call destructor)
	}
	creationBuffer.clear();
}

u32 WorldSystem::CreateEntity(EntityAdmin* admin, const char* name) {
	Entity* e = new Entity;
	e->SetName(name);
	e->admin = admin;
	creationBuffer.push_back(e);
	return admin->entities.size() + creationBuffer.size() - 1;
}

u32 WorldSystem::CreateEntity(EntityAdmin* admin, std::vector<Component*> components, const char* name, Transform transform) {
	Entity* e = new Entity;
	e->SetName(name);
	e->admin = admin;
	e->transform = transform;
	e->AddComponents(components);
	creationBuffer.push_back(e);
	return admin->entities.size() + creationBuffer.size() - 1;
}

u32 WorldSystem::CreateEntity(EntityAdmin* admin, Entity* e) {
	e->admin = admin;
	creationBuffer.push_back(e);
	return admin->entities.size() + creationBuffer.size() - 1;
}

Entity* WorldSystem::CreateEntityNow(EntityAdmin* admin, std::vector<Component*> components, const char* name, Transform transform) {
	Entity* e = new Entity;
	e->SetName(name);
	e->admin = admin;
	e->transform = transform;
	e->AddComponents(components);
	admin->entities.emplace_back(admin, (u32)admin->entities.size(), e->transform, e->name, e->components);
	for (Component* c : e->components) {
		c->entity = &admin->entities[admin->entities.size() - 1];
		c->Init();
	}
	return &admin->entities[admin->entities.size() + creationBuffer.size() - 1];
}

void WorldSystem::DeleteEntity(EntityAdmin* admin, u32 id) {
	if(id < admin->entities.size()){
		deletionBuffer.push_back(&admin->entities[id]);
	}else{
		ERROR("Attempted to add entity '", id, "' to deletion buffer when it doesn't exist on the admin");
	}
}

void WorldSystem::DeleteEntity(EntityAdmin* admin, Entity* entity) {
	if(entity->id < admin->entities.size()){
		deletionBuffer.push_back(entity);
	}else{
		ERROR("Attempted to add entity '", entity->id, "' to deletion buffer when it doesn't exist on the admin");
	}
}
