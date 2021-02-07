#pragma once
//#define DEBUG_P3DPGE
#include "../EntityAdmin.h"
#include "deshi_input.h"
#include "deshi_glfw.h"
#include "deshi_renderer.h"
#include "deshi_imgui.h"
#include "deshi_time.h"

#include <atomic>
#include <thread>

struct DeshiEngine {
	inline static std::atomic<bool> running;
	
	EntityAdmin entityAdmin;
	RenderAPI renderAPI;
	Renderer* renderer;
	Input input;
	Window window;
	deshiImGui* imgui;
	Time time;

	//TODO(,delle) setup loading a config file
	void LoadConfig(){
		
		//render api
		renderAPI = RenderAPI::VULKAN;
		switch(renderAPI){
			case(VULKAN):default:{ 
				renderer = new Renderer_Vulkan; 
				//imgui = new vkImGui;
			}break;
		}
	}
	
	void Start(){
		//init
		LoadConfig();
		time.Init(300);
		window.Init(&input, 1280, 720);
		renderer->Init(&window);
		//imgui->Init(renderer, &input, &window);
		
		//start the engine thread
		DeshiEngine::running = true;
		std::thread t = std::thread(&DeshiEngine::EngineThread, this);
		window.StartLoop();
		
		DeshiEngine::running = false;
		t.join();

		//start entity admin
		entityAdmin.Create(&input, &window);
		
		//cleanup
		//imgui->Cleanup(); delete imgui;
		renderer->Cleanup(); delete renderer;
		window.Cleanup();
	}
	
	void EngineThread(){
		while(DeshiEngine::running){
			if(!Update()){ DeshiEngine::running = false; }
		}
	}
	
	bool Update() {
		time.Update();
		input.Update();
		//imgui->NewFrame();
		entityAdmin.Update();
		//imgui->EndFrame();
		renderer->Draw();
		//entityAdmin.PostRenderUpdate();
		return true;
	}
};
