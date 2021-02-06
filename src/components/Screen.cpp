#pragma once
#include "Input.h"
#include "Component.h"
#include "../math/Vector3.h"
#include "../math/Vector2.h"

struct Screen : public Component {
	float width;
	float height;
	float resolution;
	Vector2 dimensions;
	Vector3 dimensionsV3;
	Vector2 mousePos;
	Vector3 mousePosV3;
	bool changedResolution;

	Screen() {

		//so many members man!
		width = admin->deng->window.window_width;
		height = admin->deng->window.window_height;
		resolution = width * height;
		dimensions = Vector2(width, height);
		dimensionsV3 = Vector3(width, height);
		mousePos = admin->input->mousepos;
		mousePosV3 = Vector3(mousePos);
		changedResolution = true;
	}
};