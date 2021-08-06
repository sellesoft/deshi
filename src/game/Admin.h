#pragma once
#ifndef GAME_ADMIN_H
#define GAME_ADMIN_H

#include "Transform.h"
#include "Keybinds.h"
#include "Controller.h"
#include "Editor.h"
#include "systems/PhysicsSystem.h"
#include "systems/CanvasSystem.h"
#include "systems/SoundSystem.h"
#include "entities/Entity.h"
#include "../defines.h"
#include "../utils/ContainerManager.h"

#include <vector>
#include <string>

//struct Entity;
struct System;
struct Component;
struct Command;
struct CameraInstance;

enum GameState_{
	GameState_Play, 
	GameState_Menu, 
	GameState_Debug, 
	GameState_Editor, 
	GameState_COUNT
}; typedef u32 GameState;

struct Admin {
	GameState state;
	
	//systems
	PhysicsSystem physics;
	CanvasSystem  canvas;
	SoundSystem   sound;
	
	//admin singletons
	Keybinds    keybinds;
	Controller  controller;
	Editor      editor;
	
	CameraInstance* mainCamera;
	Entity* player;
	
	std::vector<Entity*> entities;
	std::vector<Entity*> creationBuffer;
	std::vector<Entity*> deletionBuffer;
	
	//stores the components to be executed in between layers
	std::vector<ContainerManager<Component*>> freeCompLayers;
	u32 compIDcount = 0;
	
	//pause flags
	bool skip;
	bool paused;
	bool pause_command, pause_phys, pause_canvas, pause_world, pause_sound;
	
	// debug stuff
	bool debugTimes;
	
	f32 physLyrTime{}, canvasLyrTime{}, sndLyrTime{}, worldLyrTime{};
	f32 physSysTime{}, canvasSysTime{}, sndSysTime{}, worldSysTime{};
	f32 editorTime{};
	
	void Init();
	void Update();
	void PostRenderUpdate();
	void Reset();
	void Cleanup();
	
	inline void SkipUpdate(){ skip = true; };
	void ChangeState(GameState state);
	std::string FormatAdminTime(std::string format);
	
	//// @serialization functions ////
	void SaveTEXT(std::string savename);
	void LoadTEXT(std::string savename);
	void SaveDESH(const char* filename);
	void LoadDESH(const char* filename);
	
	//// @storage functions ////
	//initializes an entity with no components and adds it to the creation buffer
	u32 CreateEntity(const char* name = 0);
	//initializes an entity with a component vector and adds it to the creation buffer
	u32 CreateEntity(std::vector<Component*> components, const char* name = 0, Transform transform = Transform());
	//adds an already initialized entity to the creation buffer
	u32 CreateEntity(Entity* entity);
	//initializes an entity with a component vector and adds it to entities immedietly
	Entity* CreateEntityNow(std::vector<Component*> components, const char* name = 0, Transform transform = Transform());
	
	//adds the entity at ID to the deletion buffer
	void DeleteEntity(u32 id);
	//adds the entity to the deletion buffer
	void DeleteEntity(Entity* entity);
	void RemoveButDontDeleteEntity(Entity* entity);
	
	void AddComponentToLayers(Component* component);
	
	//// @query functions ////
	Entity* EntityRaycast(vec3 origin, vec3 direction, f32 maxDistance);
};


//global_ admin pointer
extern Admin* g_admin;
#define EntityAt(id) g_admin->entities[id]
#define DengAdmin  g_admin
#define DengKeys   g_admin->keybinds
#define DengCamera g_admin->mainCamera

#endif //GAME_ADMIN_H