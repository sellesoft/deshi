#pragma once
//#define DEBUG_P3DPGE
#include "../EntityAdmin.h"
#include "deshi_input.h"
#include "deshi_glfw.h"
#include "deshi_renderer.h"
#include "deshi_imgui.h"
#include "deshi_time.h"

struct DeshiEngine {
	EntityAdmin entityAdmin;
	RenderAPI renderAPI;
	Renderer* renderer;
	Input input;
	Window window;
	deshiImGui* imgui;
	Time time;
	
	//TODO(,delle) setup loading a config file to a config struct
	//TODO(r,delle) change render API to a define rather than variable so it doesnt have to be a pointer
	void LoadConfig(){
		
		//render api
		renderAPI = RenderAPI::VULKAN;
		switch(renderAPI){
			case(RenderAPI::VULKAN):default:{ 
				renderer = new Renderer_Vulkan;
				imgui = new vkImGui;
			}break;
		}
	}
	
	void Start(){
		//init
		LoadConfig();
		time.Init(300);
		window.Init(&input, 1280, 720);
		renderer->Init(&window, imgui); //inits imgui as well
		
		//start entity admin
		entityAdmin.Create(&input, &window, &time, renderer);
		
		//start main loop
		while(!glfwWindowShouldClose(window.window)){
			glfwPollEvents();
			Update();
		}
		
		//cleanup
		imgui->Cleanup(); delete imgui;
		renderer->Cleanup(); delete renderer;
		window.Cleanup();
	}
	
	bool Update() {
		time.Update();
		input.Update();
		window.Update();
		imgui->NewFrame();            //place imgui calls after this
		ImGui::ShowDemoWindow();
		entityAdmin.Update();
		glm::mat4 temp = ((Renderer_Vulkan*)renderer)->scene.meshes[0].modelMatrix;
		((Renderer_Vulkan*)renderer)->scene.meshes[0].modelMatrix = glm::rotate(temp, time.deltaTime * glm::radians(90.f), glm::vec3(0.f, 1.f, 0.f));
		renderer->Render();           //place imgui calls before this
		renderer->Present();
		//entityAdmin.PostRenderUpdate();
		return true;
	}
};
