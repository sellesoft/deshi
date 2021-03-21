#include "Canvas.h"

#include "../EntityAdmin.h"


Canvas::Canvas() {
	containers = std::vector<UIContainer*>();
	hideAll = false;

	layer = CL2_RENDCANVAS;
}

Canvas::~Canvas() {
	//delete pge_imgui;
	for (UIContainer* con : containers) delete con;
	containers.clear();
}

void Canvas::Update() {

}
