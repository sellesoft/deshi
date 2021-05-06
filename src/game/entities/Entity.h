
#ifndef DESHI_ENTITY_H
#define DESHI_ENTITY_H

#include "../Transform.h"

#include "../../utils/defines.h"
#include "../../utils/debug.h"
#include "../../utils/ContainerManager.h"

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

	void operator =(Entity e);

	std::vector<char> Save();
	void Load(const char* filename);

	void SetName(const char* name);
	//adds a component to the end of the components vector
	//returns the position in the vector
	void AddComponent(Component* component);
	void AddComponents(std::vector<Component*> components);
	void RemoveComponent(Component* component);
	void RemoveComponents(std::vector<Component*> components);

	//returns an entity loaded from the entities folder
	static Entity* CreateEntityFromFile(EntityAdmin* admin, std::string& filename);

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

#endif
