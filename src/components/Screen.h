#pragma once
#include "Component.h"


struct Screen : public Component {
	float width;
	float height;
	float resolution;
	Vector2 dimensions;
	Vector3 dimensionsV3;
	Vector2 mousePos;
	Vector3 mousePosV3;
	bool changedResolution;

	Screen();

	
	void Update() override;
};