#pragma once
#ifndef DESHI_IMGUI_VULKAN_H
#define DESHI_IMGUI_VULKAN_H

struct Renderer;

struct deshiImGui{
	const char* iniFilepath;
	Renderer* vkr;
	
	void Init(Renderer* renderer);
	void Cleanup();
	void NewFrame();
	//ImGui::Render() called in the renderer
};

#endif //DESHI_IMGUI_VULKAN_H
