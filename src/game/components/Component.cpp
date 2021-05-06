#include "component.h"
#include "../admin.h"

Component::Component(EntityAdmin* a, u32 eID) {
	admin = a;
	entityID = eID;
	entity->id;
}

Component::~Component() { 
	if(send) send->RemoveReceiver(this);
};

void Component::ConnectSend(Component* c) {
	c->send->AddReceiver(this);
}
