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
#include "game/systems/PhysicsSystem.h"
#include "game/systems/CanvasSystem.h"
#include "game/systems/SoundSystem.h"
#include "game/entities/Entity.h"


#define DengKeys admin->keybinds
#define DengAdmin g_admin
#define DengCamera g_admin->mainCamera

//struct Entity;
struct System;
struct Component;
struct Command;
struct Camera;

enum GameStateBits : u32{
	GameState_Play, GameState_Menu, GameState_Debug, GameState_Editor, GameState_LAST
}; typedef u32 GameState;

struct EntityAdmin {
	GameState state;
	
	//systems
	PhysicsSystem physics;
	CanvasSystem  canvas;
	SoundSystem   sound;
	
	//admin singletons
	Scene       scene;
	Keybinds    keybinds;
	Controller  controller;
	UndoManager undoManager;
	
	Camera* editorCamera;
	Camera* mainCamera;
	Entity* player;
	Entity* selectedEntity;
	
	std::vector<Entity> entities;
	std::vector<Entity*> creationBuffer;
	std::vector<Entity*> deletionBuffer;
	
	//object_pool<Component>* components;
	//stores the components to be executed in between layers
	std::vector<ContainerManager<Component*>> freeCompLayers;
	
	//pause flags
	b32  skip;
	bool paused;
	bool pause_command, pause_phys, pause_canvas, pause_world, pause_sound;
	
	//timer related
	bool debugTimes;
	TIMER_START(t_a);
	
	//debug stuff
	bool find_triangle_neighbors;
	b32  fast_outline;
	
	void Init();
	void Update();
	void PostRenderUpdate();
	void Cleanup();
	
	inline void SkipUpdate(){ skip = true; };
	void ChangeState(GameState state);
	
	void Reset();
	void Save(const char* filename);
	void Load(const char* filename);
	
	//// Entity Storage Functions ////
	
	//initializes an entity with no components and adds it to the creation buffer
	//returns the eventual position in the admin's entities array
	u32 CreateEntity(const char* name = 0);
	
	//initializes an entity with a component vector and adds it to the creation buffer
	//returns the eventual position in the admin's entities array
	u32 CreateEntity(std::vector<Component*> components, const char* name = 0, Transform transform = Transform());
	
	//adds an already initialized entity to the creation buffer
	u32 CreateEntity(Entity* entity);
	
	//initializes an entity with a component vector and adds it to entities immedietly
	//returns a pointer to the entitiy
	Entity* CreateEntityNow(std::vector<Component*> components, const char* name = 0, Transform transform = Transform());
	
	//adds the entity at ID to the deletion buffer
	void DeleteEntity(u32 id);
	
	//adds an already initialized entity to the deletion buffer
	void DeleteEntity(Entity* entity);
};


//global admin pointer
extern EntityAdmin* g_admin;
#define EntityAt(id) g_admin->entities[id]

#endif