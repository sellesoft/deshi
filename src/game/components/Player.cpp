#include "Player.h"
#include "Physics.h"
#include "Movement.h"
#include "../admin.h"

Player::Player(Movement* movement) {
	admin = g_admin;
	layer = ComponentLayer_Physics;
	comptype = ComponentType_Player;
	cpystr(name, "Player", 63);
	
	this->movement = movement;
}

void Player::Update() {
	
}

std::vector<char> Player::Save() {
	return {};
}

void Player::Load(EntityAdmin* admin, const char* data, u32& cursor, u32 count){
	ERROR("Player::Load not setup");
}