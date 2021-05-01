#pragma once
#ifndef DESHI_IMGUI_VULKAN_H
#define DESHI_IMGUI_VULKAN_H

#include <string>

struct Renderer;

struct deshiImGui{
	std::string iniFilepath;
	Renderer* vkr;
	
	void Init(Renderer* renderer);
	void Cleanup();
	void NewFrame();
	//ImGui::Render() called in the renderer
};

#endif //DESHI_IMGUI_VULKAN_H
