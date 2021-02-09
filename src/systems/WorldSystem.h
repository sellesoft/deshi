#pragma once
#include "System.h"

/*
	Note on how this system is meant to work:

	Rather than dealing with objects being created/deleted in the middle of the frame and
	the consequence that might occur from that, I opted to use a creation/deletion buffer
	that only acts at the end of the frame (after rendering).

	Advantages:
	1. Objects wont get removed/inserted in the middle of the frame, possibly causing
	unexpected behaviour and affecting some systems while avoiding others.
	2. If the frame is going too long, we can opt to delay the creation or deletion of
	an entity or component, though I would say to be careful with delayed creation as players
	might notice even if the creation of something is one or two frames delayed.
	3. You can always add/remove an Entity manually into the middle of the frame through
	the EntityAdmin if necessary

	Disadvantages:
	1. More noticeable delays when on low framerates.
	2. An entity queued for deletion will still affect systems until the end of the frame.
	3. The buffer has potential to grow too large if delays keep getting stacked
*/

struct Entity;
struct Component;
struct EntityAdmin;
typedef uint32 EntityID;

struct WorldSystem : public System {
	void Init() override;
	void Update() override;

//// Entity Lifetime Functions ////

	//initializes an entity with no components and adds it to the creation buffer
	//returns the entity that was created
	static Entity* CreateEntity(EntityAdmin* admin);

	//initializes an entity with a single component and adds it to the creation buffer
	//returns the entity that was created
	static Entity* CreateEntity(EntityAdmin* admin, Component* singleton);

	//initializes an entity with a component vector and adds it to the creation buffer
	//returns the entity that was created
	static Entity* CreateEntity(EntityAdmin* admin, std::vector<Component*> components);

	//adds an already initialized entity to the creation buffer
	//returns its place in the queue
	static int32 AddEntityToCreationBuffer(EntityAdmin* admin, Entity* entity);

	//adds an already initialized entity to the deletion buffer
	//returns its place in the queue, -1 if the entity could not be found
	static int32 AddEntityToDeletionBuffer(EntityAdmin* admin, Entity* entity);

//// Entity Component Functions ////

	//adds a component to the end of an entity's components vector
	//returns the position in the vector
	static int32 AddAComponentToEntity(EntityAdmin* admin, Entity* entity, Component* component);

	//adds components to the end of an entity's components vector
	//returns the position of the first added component in the entity's vector
	static int32 AddComponentsToEntity(EntityAdmin* admin, Entity* entity, std::vector<Component*> components);

	//adds a component to the end of the components vector of an entity that already exists in the world
	//returns the position in the vector, or -1 if the entity could not be found
	static int32 AddAComponentToWorldEntity(EntityAdmin* admin, Entity* entity, Component* component);

	//adds components to the end of the components vector of an entity that already exists in the world
	//returns the position of the first added component in the entity's vector, or -1 if the entity could not be found
	static int32 AddComponentsToWorldEntity(EntityAdmin* admin, Entity* entity, std::vector<Component*> components);

	//returns an entity's component vector
	//returns 0 if the entity could not be found
	static std::vector<Component*>* GetComponentsOnWorldEntity(EntityAdmin* admin, Entity* entity);

	//returns an entity's component vector
	//returns 0 if the entity could not be found
	static inline std::vector<Component*>* GetComponentsOnEntity(Entity* entity);

	//removes and deletes a component from an entity's components vector
	//returns true if successful removal, false otherwise
	static bool RemoveAComponentFromEntity(EntityAdmin* admin, Entity* entity, Component* component);

	//removes and deletes a component from an entity's components vector
	//returns true if successful removal, false otherwise
	static bool RemoveComponentsFromEntity(EntityAdmin* admin, Entity* entity, std::vector<Component*> components);
};