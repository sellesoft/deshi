#pragma once
#ifndef COMPONENT_LIGHT_H
#define COMPONENT_LIGHT_H

#include "Component.h"
#include "../../math/vector.h"

struct Light : public Component {
	vec3 position;
	vec3 direction;
	float brightness;
	//TODO(delle) add color var
	
	bool active = true;
	
	Light();
	Light(const vec3& position, const vec3& direction, float strength = 1.f);
	
	void Update() override;
	virtual void ReceiveEvent(Event event) override;
	
	std::string SaveTEXT() override;
	static void LoadDESH(Admin* admin, const char* fileData, u32& cursor, u32 countToLoad);
};

#endif //COMPONENT_LIGHT_H