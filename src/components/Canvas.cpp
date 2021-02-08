#include "Canvas.h"

#include "../EntityAdmin.h"


Canvas::Canvas() {
	containers = std::vector<UIContainer*>();
	hideAll = false;

	layer = CL3_RENDCANVAS;
}

Canvas::~Canvas() {
	//delete pge_imgui;
	for (UIContainer* con : containers) delete con;
	containers.clear();

	layer = CL3_RENDCANVAS;
}

void Canvas::Update() {

}
