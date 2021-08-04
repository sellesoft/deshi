#include "Player.h"
#include "Physics.h"
#include "Movement.h"
#include "../admin.h"
#include "../../core/console.h"
#include "../../utils/debug.h"

Player::Player() {
	layer = ComponentLayer_Physics;
	type  = ComponentType_Player;
}

Player::Player(Movement* movement) {
	layer = ComponentLayer_Physics;
	type  = ComponentType_Player;
	this->movement = movement;
}

Player::Player(Movement* movement, int health) {
	layer = ComponentLayer_Physics;
	type  = ComponentType_Player;
	this->movement = movement;
	this->health = health;
	
}

void Player::Update() {
	
}

std::string Player::SaveTEXT(){
	return TOSTDSTRING("\n>player"
					"\nhealth ", health,
					"\n");
}

void Player::LoadDESH(Admin* admin, const char* data, u32& cursor, u32 count){
	ERROR_LOC("LoadDESH not setup");
}