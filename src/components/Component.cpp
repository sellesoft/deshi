#include "Component.h"

#include "../EntityAdmin.h"

Component::Component(EntityAdmin* a, Entity* e) {
	admin = a;
	entity = e;
}