#pragma once
#ifndef COMPONENT_PLAYER_H
#define COMPONENT_PLAYER_H

#include "Component.h"
#include "../../math/vector.h"

struct Movement;


//NOTE sushi: probably rename this to something more general, like an actor or something, but I don't like the name actor, so think of a better one :)
struct Player : public Component {
	int health;
	
	Movement* movement;
	
	Player();
	Player(Movement* movement);
	Player(Movement* movement, int health);
	
	
	void Update() override;
	
	std::string SaveTEXT() override;
	static void LoadDESH(Admin* admin, const char* fileData, u32& cursor, u32 countToLoad);
};

#endif