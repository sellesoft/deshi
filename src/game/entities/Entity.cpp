#include "Entity.h"
#include "../admin.h"
#include "../components/Component.h"
#include "../components/Light.h"

Entity::Entity() {
	this->admin = 0;
	this->id = -1;
	this->transform = Transform();
	this->name[63] = '\0';
}

Entity::Entity(EntityAdmin* admin, u32 id, Transform transform, const char* name, std::vector<Component*> comps) {
	this->admin = admin;
	this->id = id;
	this->transform = transform;
	if (name) cpystr(this->name, name, 63);
	for (Component* c : comps) {
		if (!c) continue;
		this->components.push_back(c);
		c->entityID = id;
		c->admin = admin;
	}
}

Entity::~Entity() {
	//for (Component* c : components) {
	//	if(c) delete c;
	//}
}

void Entity::operator=(Entity& e) {
	admin = e.admin;
	id = e.id;
	cpystr(name, e.name, 63);
	transform = e.transform;
	//std::copy(e.components.begin(), e.components.end(), components);
	for (Component* c : e.components) {
		if (!c) continue;
		this->components.push_back(c);
	}
	
}

void Entity::Init(){
	for_n(i,components.size()) components[i]->Init();
}

void Entity::CleanUp() {
	for (Component* c : components) {
		if(c) delete c;
	}
}

void Entity::SetName(const char* name) {
	if (name) cpystr(this->name, name, 63);
}

void Entity::AddComponent(Component* c) {
	if (!c) return;
	components.push_back(c);
	c->entityID = id;
	c->entity = this;
	c->admin = this->admin;
	if (c->comptype == ComponentType_Light) {
		DengAdmin->scene.lights.push_back(dyncasta(Light, c));
	}
}

void Entity::AddComponents(std::vector<Component*> comps) {
	u32 value = this->components.size();
	for (Component* c : comps) {
		if (!c) continue;
		this->components.push_back(c);
		c->entityID = id;
		c->admin = this->admin;
	}
}

void Entity::RemoveComponent(Component* c) {
	if (!c) return;
	for_n(i, components.size()) {
		if (components[i] == c) {
			admin->freeCompLayers[c->layer].remove_from(c->layer_index);
			delete c;
			components.erase(components.begin() + i);
			return;
		}
	}
}

void Entity::RemoveComponents(std::vector<Component*> comps) {
	while (comps.size()) {
		for_n(i, components.size()) {
			if (!comps[i]) continue;
			if (components[i] == comps[0]) {
				admin->freeCompLayers[comps[i]->layer].remove_from(comps[i]->layer_index);
				delete comps[i];
				components.erase(components.begin() + i);
				comps.erase(components.begin());
				break;
			}
		}
	}
}
