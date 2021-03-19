#pragma once
#ifndef COMPONENT_CANVAS_H
#define COMPONENT_CANVAS_H

#include "Component.h"

struct UIContainer;

struct Canvas : public Component {
	
	//fix when ready
	//olc::imgui::PGE_ImGUI* pge_imgui;
	std::vector<UIContainer*> containers;
	bool hideAll;
	
	bool SHOW_FPS_GRAPH = false;
	
	Canvas();
	~Canvas();
	
	void Update() override;
};

#endif //COMPONENT_CANVAS_H