#include "component.h"
#include "../../EntityAdmin.h"


Component::Component(EntityAdmin* a, Entity* e) {
	admin = a;
	entity = e;
}

Component::~Component() { 
	if(send) send->RemoveReceiver(this); 
};

void Component::ConnectSend(Component* c) {
	c->send->AddReceiver(this);
}
