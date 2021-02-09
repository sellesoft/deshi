#include "Scene.h"

#include "../EntityAdmin.h"

Scene::Scene(EntityAdmin* a) : Component(a) {
	meshes = std::vector<Mesh*>();
	lights = std::vector<Light*>();
	lights = std::vector<Light*>();

	layer = CL2_RENDSCENE;
}