#include "Entity.h"

#include "../../EntityAdmin.h"
#include "../../core.h"

#include "../../utils/Command.h"
#include "../../utils/defines.h"
#include "../../scene/Scene.h"

#include "../Keybinds.h"
#include "../Transform.h"
#include "../Controller.h"
#include "../components/Orb.h"
#include "../components/door.h"
#include "../components/Light.h"
#include "../components/Camera.h"
#include "../components/Player.h"
#include "../components/Physics.h"
#include "../components/MeshComp.h"
#include "../components/Collider.h"
#include "../components/Movement.h"
#include "../components/Component.h"
#include "../components/AudioSource.h"
#include "../components/AudioListener.h"

#include <iostream>
#include <fstream>
#include <utility>
#include <stdexcept>

#include <string>

Entity::Entity() {
	this->admin = 0;
	this->id = -1;
	this->transform = Transform();
	this->name[63] = '\0';
}

Entity::Entity(EntityAdmin* admin, u32 id, Transform transform, const char* name, std::vector<Component*> comps) {
	this->admin = admin;
	this->id = id;
	this->transform = transform;
	if (name) cpystr(this->name, name, 63);
	for (Component* c : comps) {
		if (!c) continue;
		this->components.push_back(c);
		c->entityID = id;
		c->admin = admin;
	}
}

Entity::~Entity() {
	for (Component* c : components) {
		if(c) delete c;
	}
}

void Entity::operator=(Entity e) {
	admin = e.admin;
	id = e.id;
	cpystr(name, e.name, 63);
	transform = e.transform;
	//std::copy(e.components.begin(), e.components.end(), components);
	for (Component* c : e.components) {
		if (!c) continue;
		this->components.push_back(c);
	}

}

void Entity::SetName(const char* name) {
	if (name) cpystr(this->name, name, 63);
}

void Entity::AddComponent(Component* c) {
	if (!c) return;
	components.push_back(c);
	c->entityID = id;
	c->admin = this->admin;
}

void Entity::AddComponents(std::vector<Component*> comps) {
	u32 value = this->components.size();
	for (Component* c : comps) {
		if (!c) continue;
		this->components.push_back(c);
		c->entityID = id;
		c->admin = this->admin;
	}
}

void Entity::RemoveComponent(Component* c) {
	if (!c) return;
	for_n(i, components.size()) {
		if (components[i] == c) {
			admin->freeCompLayers[c->layer].remove_from(c->layer_index);
			delete c;
			components.erase(components.begin() + i);
			return;
		}
	}
}

void Entity::RemoveComponents(std::vector<Component*> comps) {
	while (comps.size()) {
		for_n(i, components.size()) {
			if (!comps[i]) continue;
			if (components[i] == comps[0]) {
				admin->freeCompLayers[comps[i]->layer].remove_from(comps[i]->layer_index);
				delete comps[i];
				components.erase(components.begin() + i);
				comps.erase(components.begin());
				break;
			}
		}
	}
}





auto eat_spaces(std::string& str) {
	size_t idx = str.find_first_not_of(' ');
	return (idx != -1) ? str.substr(idx) : "";
}

auto get_kvPair(std::string& str) {
	size_t idx = str.find_first_of(' ');
	if (idx == -1) return std::make_pair(str, std::string(""));

	std::string key = str.substr(0, idx);
	std::string val;
	size_t start = str.find_first_not_of(' ', idx);
	size_t end = -1;
	if (str[start] == '\"') {
		end = str.find_first_of('\"', start + 1);
		val = str.substr(start + 1, end - (start + 1));
	}
	else if (str[start] == '\'') {
		end = str.find_first_of('\'', start + 1);
		val = str.substr(start + 1, end - (start + 1));
	}
	else if (str[start] == '(') {
		end = str.find_first_of(')', start + 1);
		val = str.substr(start + 1, end - (start + 1));
	}
	else {
		end = str.find_first_of(" #\n", start + 1);
		val = str.substr(start, end - start);
	}

	//PRINTLN("key: '"<< key<<"' val: '"<<val<<"'");
	return std::make_pair(key, val);
}

auto get_vec3(std::string& str) {
	size_t sz, off = 0;
	try {
		f32 x = std::stof(str, &sz);             off += sz + 1;
		f32 y = std::stof(str.substr(off), &sz); off += sz + 1;
		f32 z = std::stof(str.substr(off), &sz);
		return vec3(x, y, z);
	}
	catch (std::invalid_argument& ia) {
		ERROR("Failed to parse vector3: (", str, "): ", ia.what());
		return Vector3::ZERO;
	}
	catch (std::out_of_range& oor) {
		try {
			off = 0;
			f64 x = std::stod(str, &sz);             off += sz + 1;
			f64 y = std::stod(str.substr(off), &sz); off += sz + 1;
			f64 z = std::stod(str.substr(off), &sz);
			return vec3(x, y, z);
		}
		catch (...) {
			ERROR("Failed to parse vector3: (", str, ")");
			return Vector3::ZERO;
		}
	}
}

//TODO(delle) move component creation to after collecting all vars
Entity* Entity::CreateEntityFromFile(EntityAdmin* admin, std::string& filename) {
	if (filename.find(".entity") == -1) filename += ".entity";
	std::ifstream file(deshi::dirEntities() + filename, std::ifstream::in);
	if (!file.is_open()) { ERROR("Failed to open file: ", filename.c_str()); return 0; }

	Entity* e = 0;
	AudioListener* al = 0; AudioSource* as = 0; Camera* cam = 0;     Door* door = 0;
	Collider* coll = 0;    Light* light = 0;    MeshComp* mesh = 0;  Physics* phys = 0;

	vec3 pos{}, rot{}, scale = Vector3::ONE;
	vec3 halfDims = Vector3::ONE; f32 radius = 1.f; b32 noCollide = 0;

	//parse file
	std::string line;
	u32 line_number = 0;
	while (std::getline(file, line)) {
		++line_number;
		if (line[0] == ' ') line = eat_spaces(line);
		if (line.empty() || line[0] == '#') continue;

		if (line[0] == '>') {
			if (line.size() < 2) {
				ERROR("Error parsing ", filename, " at line ", line_number, "! Line starting with '>' must have a header following it"); continue;
			}

		find_header:
			if (line.empty()) continue;
			if (line.find(">entity") != -1) {
				e = new Entity;
				while (true) {
					std::getline(file, line); line_number++;
					if (line.empty() || line[0] == '>') goto find_header;
					if (line[0] == ' ') line = eat_spaces(line);
					if (line[0] == '#') continue;

					auto pair = get_kvPair(line);
					if (pair.second == "") {
						ERROR("Error parsing ", filename, " at line ", line_number, "! Unable to extract key-value pair from:\n    ", line);
						continue;
					}

					if (pair.first == "name") {
						cpystr(e->name, pair.second.c_str(), 63);
					}
					else if (pair.first == "position") {
						pos = get_vec3(pair.second);
					}
					else if (pair.first == "rotation") {
						rot = get_vec3(pair.second);
					}
					else if (pair.first == "scale") {
						scale = get_vec3(pair.second);
					}
					else {
						ERROR("Error parsing ", filename, " at line ", line_number, "! Invalid attribute '", pair.first, "' for header 'entity'");
					}
				}
			}
			else if (line.find(">audio listener") != -1) {
				al = new AudioListener(pos);
			}
			else if (line.find(">audio source") != -1) {
				as = new AudioSource;
			}
			else if (line.find(">camera") != -1) {
				//TODO(delle) handle camera component
				cam = new Camera(90.f);
			}
			else if (line.find(">collider") != -1) {
				while (true) {
					std::getline(file, line); line_number++;
					if (line.empty() || line[0] == '>') goto find_header;
					if (line[0] == ' ') line = eat_spaces(line);
					if (line[0] == '#') continue;

					auto pair = get_kvPair(line);
					if (pair.second == "") {
						ERROR("Error parsing ", filename, " at line ", line_number, "! Unable to extract key-value pair from:\n    ", line);
						continue;
					}

					if (pair.first == "type") {
						if (pair.second == "aabb") {
							coll = new AABBCollider(Vector3::ONE, 1);
						}
						else if (pair.second == "box") {
							coll = new BoxCollider(Vector3::ONE, 1);
						}
						else if (pair.second == "sphere") {
							coll = new SphereCollider(1, 1);
						}
						else {
							ERROR("Error parsing ", filename, " at line ", line_number, "! Unhandled collider type '", pair.second, "'"); break;
						}
					}
					else if (pair.first == "halfdims") {
						halfDims = get_vec3(pair.second);
					}
					else if (pair.first == "radius") {
						try { radius = std::stof(pair.second); }
						catch (...) {
							ERROR("Error parsing ", filename, " at line ", line_number, "! Failed to parse float '", pair.second, "'");
						}
					}
					else if (pair.first == "noCollide") {
						if (pair.second == "true" || pair.second == "1") {
							noCollide = 1;
						}
						else if (pair.second == "false" || pair.second == "0") {
							noCollide = 0;
						}
						else {
							ERROR("Error parsing ", filename, " at line ", line_number, "! Failed to parse boolean '", pair.second, "'");
						}
					}
					else if (pair.first == "command") {
						WARNING("Collider command not handled in entity files");
					}
					else {
						ERROR("Error parsing ", filename, " at line ", line_number, "! Invalid attribute '", pair.first, "' for header 'collider'");
					}
				}
			}
			else if (line.find(">light") != -1) {
				//TODO(delle) handle light component
				light = new Light(pos, rot);
			}
			else if (line.find(">mesh") != -1) {
				mesh = new MeshComp;
				while (true) {
					std::getline(file, line); line_number++;
					if (line.empty() || line[0] == '>') goto find_header;
					if (line[0] == ' ') line = eat_spaces(line);
					if (line[0] == '#') continue;

					auto pair = get_kvPair(line);
					if (pair.second == "") {
						ERROR("Error parsing ", filename, " at line ", line_number, "! Unable to extract key-value pair from:\n    ", line);
						continue;
					}

					if (pair.first == "name") {
						mesh->meshID = DengRenderer->CreateMesh(&admin->scene, pair.second.c_str());
					}
					else if (pair.first == "visible") {
						if (pair.second == "true" || pair.second == "1") {
							mesh->mesh_visible = 1;
						}
						else if (pair.second == "false" || pair.second == "0") {
							mesh->mesh_visible = 0;
						}
						else {
							ERROR("Error parsing ", filename, " at line ", line_number, "! Failed to parse boolean '", pair.second, "'");
						}
					}
					else {
						ERROR("Error parsing ", filename, " at line ", line_number, "! Invalid attribute '", pair.first, "' for header 'mesh'");
					}
				}
			}
			else if (line.find(">physics") != -1) {
				phys = new Physics(pos, rot);
				while (true) {
					std::getline(file, line); line_number++;
					if (line.empty() || line[0] == '>') goto find_header;
					if (line[0] == ' ') line = eat_spaces(line);
					if (line[0] == '#') continue;

					auto pair = get_kvPair(line);
					if (pair.second == "") {
						ERROR("Error parsing ", filename, " at line ", line_number, "! Unable to extract key-value pair from:\n    ", line);
						continue;
					}

					if (pair.first == "velocity") {
						phys->velocity = get_vec3(pair.second);
					}
					else if (pair.first == "acceleration") {
						phys->acceleration = get_vec3(pair.second);
					}
					else if (pair.first == "rotVelocity") {
						phys->rotVelocity = get_vec3(pair.second);
					}
					else if (pair.first == "rotAcceleration") {
						phys->rotAcceleration = get_vec3(pair.second);
					}
					else if (pair.first == "elasticity") {
						try { phys->elasticity = std::stof(pair.second); }
						catch (...) {
							ERROR("Error parsing ", filename, " at line ", line_number, "! Failed to parse float '", pair.second, "'");
						}
					}
					else if (pair.first == "mass") {
						try { phys->mass = std::stof(pair.second); }
						catch (...) {
							ERROR("Error parsing ", filename, " at line ", line_number, "! Failed to parse float '", pair.second, "'");
						}
					}
					else if (pair.first == "staticPosition") {
						if (pair.second == "true" || pair.second == "1") {
							phys->isStatic = 1;
						}
						else if (pair.second == "false" || pair.second == "0") {
							phys->isStatic = 0;
						}
						else {
							ERROR("Error parsing ", filename, " at line ", line_number, "! Failed to parse boolean '", pair.second, "'");
						}
					}
					else if (pair.first == "staticRotation") {
						if (pair.second == "true" || pair.second == "1") {
							phys->staticRotation = 1;
						}
						else if (pair.second == "false" || pair.second == "0") {
							phys->staticRotation = 0;
						}
						else {
							ERROR("Error parsing ", filename, " at line ", line_number, "! Failed to parse boolean '", pair.second, "'");
						}
					}
					else {
						ERROR("Error parsing ", filename, " at line ", line_number, "! Invalid attribute '", pair.first, "' for header 'physics'");
					}
				}
			}
			else if (line.find(">door") != -1) {
				door = new Door;
				while (true) {
					std::getline(file, line); line_number++;
					if (line.empty() || line[0] == '>') goto find_header;
					if (line[0] == ' ') line = eat_spaces(line);
					if (line[0] == '#') continue;

					auto pair = get_kvPair(line);
					if (pair.second == "") {
						ERROR("Error parsing ", filename, " at line ", line_number, "! Unable to extract key-value pair from:\n    ", line);
						continue;
					}

					if (pair.first == "isOpen") {
						if (pair.second == "true" || pair.second == "1") {
							door->isOpen = 1;
						}
						else if (pair.second == "false" || pair.second == "0") {
							door->isOpen = 1;
						}
						else {
							ERROR("Error parsing ", filename, " at line ", line_number, "! Failed to parse boolean '", pair.second, "'");
						}
					}
					else {
						ERROR("Error parsing ", filename, " at line ", line_number, "! Invalid attribute '", pair.first, "' for header 'door'");
					}
				}
			}
			else {
				ERROR("Error parsing ", filename, " at line ", line_number, "! Unhandled header"); continue;
			}
		}
	}

	file.close();
	if (e) {
		e->transform.position = pos;
		e->transform.rotation = rot;
		e->transform.scale = scale;
		if (coll) {
			coll->noCollide = noCollide;
			switch (coll->type) {
			case ColliderType_AABB: { ((AABBCollider*)coll)->halfDims = halfDims; }break;
			case ColliderType_Box: { ((BoxCollider*)coll)->halfDims = halfDims;  }break;
			case ColliderType_Sphere: { ((SphereCollider*)coll)->radius = radius;   }break;
			}
		}
		e->AddComponents({ al, as, cam, coll, light, mesh, phys });
		return e;
	}
	else {
		delete al; delete as; delete cam; delete coll; delete light; delete mesh; delete phys;
		return 0;
	}
}

