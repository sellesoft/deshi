#include "Component.h"

#include "../../EntityAdmin.h"

Component::Component(EntityAdmin* a, Entity* e) {
	admin = a;
	entity = e;
}

void Component::ConnectSend(Component* c) {
	c->send->AddReceiver(this);
}