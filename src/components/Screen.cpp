#include "Screen.h"

#include "../math/Vector3.h"
#include "../math/Vector2.h"

#include "../EntityAdmin.h"

Screen::Screen(){
	//so many members man!
	width = admin->d->window.window_width;
	height = admin->d->window.window_height;
	resolution = width * height;
	dimensions = Vector2(width, height);
	dimensionsV3 = Vector3(width, height);
	mousePos = admin->input->mousepos;
	mousePosV3 = Vector3(mousePos);
	changedResolution = true;
}

void Screen::Update() {

}
