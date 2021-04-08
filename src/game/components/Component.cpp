#include "Component.h"

#include "../../EntityAdmin.h"

Component::Component(EntityAdmin* a, Entity* e) {
	admin = a;
	entity = e;
}

void Component::ConnectSend(Component* c) {
	c->send->AddReceiver(this);
}

std::string Component::Save() {
	
	//TODO(sushi) implement a good way to save event connections
	//std::string s = "";
	//
	//for (Receiver* r : send->receivers) {
	//	s += 
	//}
	return "";
}

void Component::Load() {

}
