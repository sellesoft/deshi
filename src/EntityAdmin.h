#pragma once
#include "utils/defines.h"
#include "utils/Debug.h"

typedef uint32 EntityID;
struct Entity;
struct System;
struct Component;
struct Command;

struct PhysicsWorld;
struct World;

struct Camera;
struct Keybinds;
struct MovementState;
struct Canvas;
struct Console;

//DeshiEngine structs
struct Window;
struct Input;
struct Time;
struct Renderer;

//the entity admin is fed down to all systems and components that it controls meaning that
//the core will also be accessible in those places too.
struct EntityAdmin {
	EntityAdmin* admin = this;
	
	std::vector<System*> systems;
	std::map<EntityID, Entity*> entities;
	//object_pool<Component>* componentsPtr;
	std::vector<Component*> components;
	std::map<std::string, Command*> commands;
	PhysicsWorld* physicsWorld;
	
	//singletons
	Input* input;
	Window* window;
	Time* time;
	World* world;
	Renderer* renderer;
	
	Camera* currentCamera;
	Keybinds* currentKeybinds;
	MovementState* tempMovementState;
	Canvas* tempCanvas;
	Console* console;
	
	//stores the components to be executed in between layers
	std::vector<ContainerManager<Component*>> freeCompLayers;
	
	
	bool paused = false;
	bool IMGUI_KEY_CAPTURE = false;
	
	void Create(Input* i, Window* w, Time* t, Renderer* renderer);
	void Cleanup();
	
	void Update();
	
	void AddSystem(System* system);
	void RemoveSystem(System* system);
	
	//returns a pointer to a system
	//probably be careful using this cause there could be data races
	//im only implementing it to push data to the console
	//i know i can do it directly but then there would be no color parsing
	template<class T>
		T* GetSystem() {
		T* t = nullptr;
		for (System* s : systems) { if (T* temp = dynamic_cast<T*>(s)) { t = temp; break; } }
		ASSERT(t != nullptr, "attempted to retrieve a system that doesn't exist");
		return t;
	}
	
	void AddComponent(Component* component);
	void RemoveComponent(Component* component);
	
	Command* GetCommand(std::string command);
	bool ExecCommand(std::string command);
};

struct Entity {
	EntityAdmin* admin; //reference to owning admin
	EntityID id;
	std::vector<Component*> components;
	
	//returns a component pointer from the entity of provided type, nullptr otherwise
	template<class T>
		T* GetComponent() {
		T* t = nullptr;
		for (Component* c : components) { if (T* temp = dynamic_cast<T*>(c)) { t = temp; break; } }
		ASSERT(t != nullptr, "attempted to retrieve a component that doesn't exist");
		return t;
	}
	
	//adds a component to the end of the components vector
	//returns the position in the vector
	uint32 AddComponent(Component* component);
	
	uint32 AddComponents(std::vector<Component*> components);
	
	~Entity();
}; //TODO(,delle) move WorldSystem entity-component functions into Entity