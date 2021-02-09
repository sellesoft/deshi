#pragma once
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