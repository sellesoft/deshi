#pragma once
#ifndef COMPONENT_LIGHT_H
#define COMPONENT_LIGHT_H

#include "Component.h"
#include "../../math/Vector.h"

struct Light : public Component {
	Vector3 position;
	Vector3 direction;
	float brightness;

	bool active = true;

	Light(const Vector3& position, const Vector3& direction, float strength = 1.f);
	
	void Update() override;
	virtual void ReceiveEvent(Event event) override;

	std::vector<char> Save() override;
	static void Load(std::vector<Entity>& entityArray, const char* fileData, u32& cursor, u32 countToLoad);
};

#endif //COMPONENT_LIGHT_H