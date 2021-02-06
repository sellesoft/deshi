#pragma once
#include "Component.h"

struct UIContainer;
namespace olc { namespace imgui { struct PGE_ImGUI; } }

struct Canvas : public Component {
	
	//fix when ready
	//olc::imgui::PGE_ImGUI* pge_imgui;
	std::vector<UIContainer*> containers;
	bool hideAll;

	bool SHOW_FPS_GRAPH = false;

	Canvas() {
		containers = std::vector<UIContainer*>();
		hideAll = false;
	}

	~Canvas() {
		//delete pge_imgui;
		for(UIContainer* con : containers) delete con;
		containers.clear();
	}
	
	void Update() override;
};