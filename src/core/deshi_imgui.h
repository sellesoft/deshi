#pragma once
#ifndef DESHI_IMGUI_H
#define DESHI_IMGUI_H

//keeping this include here so core.h can include this header and get imgui stuff
#include "../external/imgui/imgui.h"

struct Renderer;
struct Renderer_Vulkan;

//thanks: https://github.com/dandistine/olcPGEDearImGui
struct deshiImGui{
	const char* iniFilepath;
	
	virtual void Init(Renderer* renderer);
	virtual void Cleanup() = 0;
	virtual void NewFrame()= 0;
	//ImGui::Render() called in the renderer
};

struct vkImGui : public deshiImGui{
	Renderer_Vulkan* vkr;
	
	void Init(Renderer* renderer) override;
	void Cleanup() override;
	void NewFrame() override;
};

#endif //DESHI_IMGUI_H