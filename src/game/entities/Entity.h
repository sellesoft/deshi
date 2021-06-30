#pragma once
#ifndef GAME_ENTITY_H
#define GAME_ENTITY_H

#include "../../defines.h"
#include "../../utils/tuple.h" //TODO(delle,Cl) forward declare pair<> somehow
#include "../Transform.h"

#include <string>
#include <vector>
#include <set>
#include <unordered_map>

struct EntityAdmin;
struct Component;

//necessary for loading
#define NUMENTITYTYPES 4

enum EntityType : u32 {
	EntityType_Anonymous,
	EntityType_Player,
	EntityType_StaticMesh,
	EntityType_Trigger
};

static const char* EntityTypeStrings[] = {
	"Anonymous", "Player", "StaticMesh", "Trigger"
};

struct Entity {
	EntityAdmin* admin; //reference to owning admin
	u32 id;
	char name[DESHI_NAME_SIZE];
	EntityType type = EntityType_Anonymous;
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
	
	void SetName(const char* name);
	//adds a component to the end of the components vector
	//returns the position in the vector
	void AddComponent(Component* component);
	void AddComponents(std::vector<Component*> components);
	void RemoveComponent(Component* component);
	void RemoveComponents(std::vector<Component*> components);
	
	virtual std::string SaveTEXT();
	static Entity* LoadTEXT(EntityAdmin* admin, std::string& filepath, std::vector<pair<u32,u32>>& mesh_id_diffs);
	//virtual void LoadDESH(const char* filename);
	
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
	//returns if a component exists on the entitiy
	template<class T>
	bool hasComponent() {
		for (Component* c : components) {
			if (T* temp = dynamic_cast<T*>(c)) {
				return true;
			}
		}
		return false;
	}
};

#endif //GAME_ENTITY_H
