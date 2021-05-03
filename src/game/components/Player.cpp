#include "Player.h"
#include "Physics.h"
#include "Movement.h"
#include "../../EntityAdmin.h"

Player::Player(Movement* movement) {
	layer = ComponentLayer_Physics;
	this->movement = movement;
	cpystr(name, "Player", 63);
	
}

void Player::Update() {
	
}

std::vector<char> Player::Save() {
	return {};
}

void Player::Load(std::vector<Entity>& entities, const char* data, u32& cursor, u32 count){
	ERROR("Player loading not setup");
}