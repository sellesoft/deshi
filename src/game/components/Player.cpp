#include "Player.h"
#include "Physics.h"
#include "Movement.h"
#include "../admin.h"

Player::Player() {
	admin = g_admin;
	layer = ComponentLayer_Physics;
	comptype = ComponentType_Player;
	cpystr(name, "Player", DESHI_NAME_SIZE);
	sender = new Sender();
}

Player::Player(Movement* movement) {
	admin = g_admin;
	layer = ComponentLayer_Physics;
	comptype = ComponentType_Player;
	cpystr(name, "Player", DESHI_NAME_SIZE);
	sender = new Sender();
	
	this->movement = movement;
}

Player::Player(Movement* movement, int health) {
	admin = g_admin;
	layer = ComponentLayer_Physics;
	comptype = ComponentType_Player;
	cpystr(name, "Player", DESHI_NAME_SIZE);
	sender = new Sender();
	
	this->movement = movement;
	this->health = health;
	
}

void Player::Update() {
	
}

std::string Player::SaveTEXT(){
	return TOSTRING("\n>player"
					"\nhealth ", health,
					"\n");
}

void Player::LoadDESH(EntityAdmin* admin, const char* data, u32& cursor, u32 count){
	u32 entityID = -1, compID = -1, event = -1;
	int health = 0;
	forI(count) {
		memcpy(&entityID, data + cursor, sizeof(u32)); cursor += sizeof(u32);
		if (entityID >= admin->entities.size()) {
			ERROR("Failed to load sphere collider component at pos '", cursor - sizeof(u32),
				  "' because it has an invalid entity ID: ", entityID); continue;
		}
		memcpy(&compID, data + cursor, sizeof(u32)); cursor += sizeof(u32);
		memcpy(&event, data + cursor, sizeof(u32)); cursor += sizeof(u32);
		
		memcpy(&health, data + cursor, sizeof(int));  cursor += sizeof(int);
		Player* c = new Player(EntityAt(entityID)->GetComponent<Movement>(), health);
		EntityAt(entityID)->AddComponent(c);
		c->SetCompID(compID);
		c->SetEvent(event);
		c->layer_index = admin->freeCompLayers[c->layer].add(c);
	}
	if(count > 0)
		admin->player = EntityAt(entityID);
}