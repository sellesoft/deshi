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
	switch (admin->state) {
	case GameState::EDITOR:
		movement->phys->isStatic = true;
		break;
	case GameState::PLAY:
		movement->phys->isStatic = false;
		break;
	}
}

std::vector<char> Player::Save() {
	return {};
}