#include "component.h"
#include "../../EntityAdmin.h"

Component::Component(EntityAdmin* a, u32 eID) {
	admin = a;
	entityID = eID;
}

Component::~Component() { 
	if(send) send->RemoveReceiver(this);
};

void Component::ConnectSend(Component* c) {
	c->send->AddReceiver(this);
}
