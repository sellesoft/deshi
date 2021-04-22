#include "component.h"
#include "../../EntityAdmin.h"


Component::Component(EntityAdmin* a, Entity* e) {
	admin = a;
	entity = e;
}

Component::~Component() { 
	if(send) send->RemoveReceiver(this); 
	//if(admin && layer != ComponentLayer_NONE) admin->freeCompLayers[layer].remove_from(layer_index);
	//TODO(delle,Cl) fix this so we can remove components
};

void Component::ConnectSend(Component* c) {
	c->send->AddReceiver(this);
}
