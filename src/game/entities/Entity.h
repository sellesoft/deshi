#pragma once
#ifndef GAME_ENTITY_H
#define GAME_ENTITY_H

#include "../../utils/defines.h"
#include "../Transform.h"

#include <string>
#include <vector>
#include <set>
#include <unordered_map>

struct EntityAdmin;
struct Component;

struct Entity {
	EntityAdmin* admin; //reference to owning admin
	u32 id;
	char name[DESHI_NAME_SIZE];
	Transform transform;
	std::vector<Component*> components;
	//std::unordered_map<Entity*, Entity*> connections; //connections to other entities through components
	
	std::set<Entity*> connections;
	
	Entity();
	Entity(EntityAdmin* admin, u32 id, Transform transform = Transform(),
		   const char* name = 0, std::vector<Component*> components = {});
	~Entity();
	
	void operator =(Entity& e);
	
	virtual void Init();
	
	virtual std::string SaveTEXT();
	//virtual void LoadDESH(const char* filename);
	
	void SetName(const char* name);
	//adds a component to the end of the components vector
	//returns the position in the vector
	void AddComponent(Component* component);
	void AddComponents(std::vector<Component*> components);
	void RemoveComponent(Component* component);
	void RemoveComponents(std::vector<Component*> components);
	
	//returns a component pointer from the entity of provided type, nullptr otherwise
	template<class T>
		T* GetComponent() {
		T* t = nullptr;
		for (Component* c : components) {
			if (T* temp = dynamic_cast<T*>(c)) {
				t = temp; break;
			}
		}
		return t;
	}
};

#endif //GAME_ENTITY_H
