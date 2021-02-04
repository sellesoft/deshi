#pragma once
//#define DEBUG_P3DPGE
//#include "EntityAdmin.h"
#include "deshi_input.h"
#include "deshi_glfw.h"
#include "deshi_renderer.h"

#include <atomic>
#include <thread>

struct Deshi {
	inline static std::atomic<bool> running;
	
	//EntityAdmin entityAdmin;
	RenderAPI renderAPI;
	Renderer* renderer;
	Input input;
	Window window;
	
	//TODO(,delle) setup loading a config file
	void LoadConfig(){
		
		//render api
		renderAPI = RenderAPI::VULKAN;
		switch(renderAPI){
			//case(VULKAN):{ renderer = Renderer_Vulkan(window); }break;
			default:{ renderer = new Renderer_Vulkan; }break;
		}
	}
	
	void Start(){
		//init
		LoadConfig();
		window.Init(&input, 1280, 720);
		renderer->Init(&window);
		
		//start the engine thread
		Deshi::running = true;
		std::thread t = std::thread(&Deshi::EngineThread, this);
		
		window.StartLoop();
		
		Deshi::running = false;
		t.join();
		
		//cleanup
		renderer->Cleanup();
		window.Cleanup();
	}
	
	void EngineThread(){
		while(Deshi::running){
			if(!Update()){ Deshi::running = false; }
		}
	}
	
	bool Update() {
		//entityAdmin.PreRenderUpdate();
		renderer->Draw();
		//entityAdmin.PostRenderUpdate();
		return true;
	}
};
