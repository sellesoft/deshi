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
		renderer->Init(&window);
		imgui->Init(renderer, &input, &window, &time);
		
		//start entity admin
		entityAdmin.Create(&input, &window, &time);

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
		imgui->NewFrame();			//place imgui calls after this
		ImGui::ShowDemoWindow();
		entityAdmin.Update();
		renderer->Render();			//place renderer cmd buffer calls after this
		imgui->EndFrame();			//place imgui calls before this
		renderer->Present();			//place renderer cmd buffer calls before this
		//entityAdmin.PostRenderUpdate();
		return true;
	}
};
