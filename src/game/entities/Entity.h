#pragma once
#ifndef GAME_ENTITY_H
#define GAME_ENTITY_H

#include "../../utils/defines.h"
#include "../Transform.h"

#include <vector>

struct EntityAdmin;
struct Component;

struct Entity {
	EntityAdmin* admin; //reference to owning admin
	u32 id;
	char name[64];
	Transform transform;
	std::vector<Component*> components;
	
	Entity();
	Entity(EntityAdmin* admin, u32 id, Transform transform = Transform(),
		   const char* name = 0, std::vector<Component*> components = {});
	~Entity();
	
	void operator =(Entity& e);
	
	virtual void Init();
	void CleanUp();
	//virtual std::vector<char> Save();
	//virtual void Load(const char* filename);
	
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
