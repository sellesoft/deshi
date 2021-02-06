#pragma once
#include "core/deshi.h"
#include "utils/defines.h"
#include "utils/Debug.h"

typedef uint32 EntityID;
struct Entity;
struct System;
struct Component;
struct Command;

struct PhysicsWorld;
struct Input;
struct Screen;
struct Time;
struct World;

struct Camera;
struct Keybinds;
struct Scene;
struct MovementState;
struct Canvas;
struct Console;

struct DeshiEngine;

//the entity admin is fed down to all systems and components that it controls meaning that
//the core will also be accessible in those places too.
struct EntityAdmin {

	//yep
	DeshiEngine* d;

	//reference to itself for doing things from within
	//is this dangerous? 
	EntityAdmin* admin = this;

	std::vector<System*> systems;
	std::map<EntityID, Entity*> entities;
	//object_pool<Component>* componentsPtr;
	std::vector<Component*> components;
	std::map<std::string, Command*> commands;
	PhysicsWorld* physicsWorld;

	//singletons
	Input* input;
	Screen* screen;
	Time* time;
	World* world;

	Camera* currentCamera;
	Keybinds* currentKeybinds;
	Scene* currentScene;
	MovementState* tempMovementState;
	Canvas* tempCanvas;
	Console* console;

	bool paused = false;
	bool IMGUI_KEY_CAPTURE = false;

	void Create(DeshiEngine* d);
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
	bool TriggerCommand(std::string command);
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