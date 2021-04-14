#include "Scene.h"
#include "../game/Transform.h"

void Scene::Init(){}

void Scene::Reset(){
	models.clear();
	lights.clear();
	lines.clear();
}

std::pair<Vector3, Vector3> Scene::SceneBoundingBox() {
	float inf = std::numeric_limits<float>::max();
	Vector3 max(-inf, -inf, -inf);
	Vector3 min( inf,  inf,  inf);
	
	std::vector<Vector3> vertices;
	
	for (Model mo : models) {
		Mesh* m = &mo.mesh;
		for (Batch b : m->batchArray) {
			for (Vertex ver : b.vertexArray) {
				Vector3 v = Vector3(ver.pos.x, ver.pos.y, ver.pos.z) + m->transform.Translation();
				if      (v.x < min.x) { min.x = v.x; }
				else if (v.x > max.x) { max.x = v.x; }
				if      (v.y < min.y) { min.y = v.y; }
				else if (v.y > max.y) { max.y = v.y; }
				if      (v.z < min.z) { min.z = v.z; }
				else if (v.z > max.z) { max.z = v.z; }
			}
			
		}
		
	}
	
	return std::pair<Vector3, Vector3>(max, min);
}