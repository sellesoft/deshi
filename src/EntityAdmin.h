#pragma once
#ifndef DESHI_ENTITYADMIN_H
#define DESHI_ENTITYADMIN_H

#include "utils/defines.h"
#include "utils/debug.h"
#include "utils/ContainerManager.h"
#include "scene/scene.h"
#include "game/Transform.h"
#include "game/Keybinds.h"
#include "game/Controller.h"
#include "game/UndoManager.h"

#include <map>

//deshi Engine data defines; accessible only from inside Components/Systems
#define DengInput admin->input
#define DengWindow admin->window
#define DengTime admin->time
#define DengKeys admin->keybinds

struct Entity;
struct System;
struct Component;
struct Command;

struct PhysicsWorld;
struct PhysicsSystem;
struct CanvasSystem;
struct WorldSystem;
struct SoundSystem;

struct Camera;

//core structs
struct Window;
struct Input;
struct Time;
struct Renderer;
struct Console;

enum struct GameState{
	NONE, PLAY, PLAY_DEBUG, EDITOR, MENU
};

//the entity admin is fed down to all systems and components that it controls meaning that
//the core will also be accessible in those places too.
struct EntityAdmin {
	EntityAdmin* admin = this;
	
	GameState state = GameState::NONE;
	
	std::vector<Entity> entities;
	//are we still doing this?  reply: eventually, not necessary for prototype
	//object_pool<Component>* componentsPtr;
	
	//core
	Input*    input;
	Window*   window;
	Time*     time;
	Renderer* renderer;
	Console*  console;
	
	//systems
	PhysicsWorld*  physicsWorld; //TODO(delle) move these vars to a global vars file
	PhysicsSystem* physics;
	CanvasSystem*  canvas;
	WorldSystem*   world;
	SoundSystem*   sound;
	
	//admin singletons
	Scene       scene;
	Keybinds    keybinds;
	Controller  controller;
	UndoManager undoManager;
	
	Camera* mainCamera;
	
	//stores the components to be executed in between layers
	std::vector<ContainerManager<Component*>> freeCompLayers;
	
	//pause flags
	b32  skip = false;
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
	
	void Save();
	void Load(const char* filename);
	
	Command* GetCommand(std::string command);
	bool     ExecCommand(std::string command);
	bool     ExecCommand(std::string command, std::string args);
};

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
	
	std::string Save();
	void Load(const char* filename);
	
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

#endif