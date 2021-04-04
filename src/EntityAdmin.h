#pragma once
#ifndef DESHI_ENTITYADMIN_H
#define DESHI_ENTITYADMIN_H

#include "utils/defines.h"
#include "utils/debug.h"
#include "utils/ContainerManager.h"

//deshi Engine data defines; accessible only from inside Components/Systems
#define DengInput admin->input
#define DengWindow admin->window
#define DengTime admin->time
#define DengKeys admin->keybinds

typedef u32 EntityID;
struct Entity;
struct System;
struct Component;
struct Command;

struct PhysicsWorld;

struct PhysicsSystem;
struct RenderCanvasSystem;
struct WorldSystem;
struct SoundSystem;

struct Camera;
struct Keybinds;
struct Controller;
struct Canvas;
struct Console;
struct Scene;
struct Transform;

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
	//are we still doing this?
	//object_pool<Component>* componentsPtr;
	std::vector<Component*> components;
	PhysicsWorld* physicsWorld;
	
	//singletons
	Input*    input;
	Window*   window;
	Time*     time;
	Renderer* renderer;
	Console*  console;
	Scene*    scene; 
	
	Camera*     mainCamera;
	Keybinds*   keybinds;
	Controller* controller;
	Canvas*     tempCanvas;
	
	//systems
	PhysicsSystem* physics;
	RenderCanvasSystem* canvas;
	WorldSystem* world;
	SoundSystem* sound;
	
	//stores the components to be executed in between layers
	std::vector<ContainerManager<Component*>> freeCompLayers;
	
	//pause flags
	bool paused = false;
	bool pause_command = false;
	bool pause_phys = false;
	bool pause_canvas = false;
	bool pause_world = false;
	bool pause_console = false;
	bool pause_sound = false;
	bool pause_last = false;
	
	//imgui capture flags
	bool IMGUI_KEY_CAPTURE = false;
	bool IMGUI_MOUSE_CAPTURE = false;
	
	//console error warn flag and last error
	bool cons_error_warn = false;
	std::string last_error;
	
	void Init(Input* i, Window* w, Time* t, Renderer* r, Console* c);
	void Cleanup();
	
	void Update();
	
	void AddSystem(System* system);
	void RemoveSystem(System* system);
	
	//returns a pointer to a system
	//probably be careful using this cause there could be data races
	//im only implementing it to push data to the console
	//i know i can do it directly but then there would be no color parsing
	//template<class T>
	//	T* GetSystem() {
	//	T* t = nullptr;
	//	for (System* s : systems) { if (T* temp = dynamic_cast<T*>(s)) { t = temp; break; } }
	//	ASSERT(t != nullptr, "attempted to retrieve a system that doesn't exist");
	//	return t;
	//}
	
	void AddComponent(Component* component);
	void RemoveComponent(Component* component);
	
	Command* GetCommand(std::string command);
	bool ExecCommand(std::string command);
	bool ExecCommand(std::string command, std::string args);
};

struct Entity {
	EntityAdmin* admin; //reference to owning admin
	EntityID id;
	std::vector<Component*> components;
	std::string name;
	
	Transform* transform;
	
	//returns a component pointer from the entity of provided type, nullptr otherwise
	template<class T>
		T* GetComponent() {
		T* t = nullptr;
		for (Component* c : components) { 
			if (T* temp = dynamic_cast<T*>(c)) { 
				t = temp; break; 
			} 
		}
		//ASSERT(t != nullptr, "attempted to retrieve a component that doesn't exist");
		if (t == nullptr) {
			//TODO(sushi) make it so stuff like this only shows up when debugging 
			LOG("attempted to retrieve a component that doesn't exist on entity ", name, " with ID ", id);
		}
		
		return t;
	}
	
	//adds a component to the end of the components vector
	//returns the position in the vector
	u32 AddComponent(Component* component);
	u32 AddComponents(std::vector<Component*> components);
	
	Entity();
	~Entity();
};

#endif