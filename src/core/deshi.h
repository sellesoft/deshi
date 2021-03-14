/* deshi
TODO Tags: + GitIssue, s Severe, u Unimportant, p Physics, r Render, e Entity, i Input,
	  m Math, o Optimization, g General, c Clean Up Code, f Fun

TODO Style: TODO(tags,person,date) description
Rules: tags can be empty but still requires a comma, date can be empty
      eg: TODO(,delle) no tag or date; TODO(ro,sushi,03/12/2021) render,optimization tags for sushi made on that date

Major Ungrouped TODOs
---------------------
add mouse-camera control
add a .str() method to Component.h to print state
remove deshi_ prefix on core files (deshi_imgui may need to be imgui_deshi to not conflict)
either move all commands to Command or move them all to their local files
log console output to a file
settings file(s) [keybinds, video, audio, etc]

Minor Ungrouped TODOs
---------------------
cleanup Triangle and remove unused things
create templated component tuple iterator that loops thru a vector and returns an iterator of components of type
pool/arena components and entities for better performance
implement string returns, better descriptions, and parameter parsing on every command (use spawn_box as reference)
replace/remove external dependencies/includes were possible (glm, boost, sascha, tinyobj)
(maybe) make TODOs script in misc to locally generate TODOs.txt rather than TODOP bot
cleanup compile warnings

Render TODOs
------------
add OBJ loading
add commands for the rendering interface functions
fix crash on minimize/resize
check those vulkan-tutorial links for the suggestions
add the render options back
add instancing [https://learnopengl.com/Advanced-OpenGL/Instancing]
add texture transparency support
add 2D shader and interface functions
add buffer pre-allocation and arenas for vertices/indices/textures/etc
add lighting and shadows
add RenderSettings loading and usage
add dynamic Texture updating on meshes
add the ability to load new textures to the scene
  convert prints to go thru the in-game console rather than other console  
(maybe) remove renderer polymorphism and replace it with defines that are checked on program start

Physics/Atmos TODOs
-------------
redo main physics loop
add physics collision sweeping
add physics based collision resolution for remaining collider primitives
add physics interaction functions

UI TODOs
--------
Fix console not relinquishing input
Restore the spawning/inspector menus
2D shader
add a UI popup when reloading shaders

Math NOTEs: Row-Major matrices, Left-Handed coordinate system (clockwise rotation when looking down axis)
Math TODOs
----------
add quaternions and converions between them and other linear algebra primitives
finish AxisAngleRotationMatrix function
review the projection functions for correctness
(maybe) move conversion functions to thier own file
(maybe) move transform/affine matrix functions to their own file

*/


#pragma once
//#define DEBUG_P3DPGE
#include "../EntityAdmin.h"
#include "deshi_input.h"
#include "deshi_glfw.h"
#include "deshi_renderer.h"
#include "deshi_imgui.h"
#include "deshi_time.h"

#include "../math/Math.h"

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
		renderer->time = &time;
		renderer->Init(&window, imgui); //inits imgui as well
		
		//start entity admin
		entityAdmin.Init(&input, &window, &time, renderer);
		
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
		renderer->Render();           //place imgui calls before this
		renderer->Present();
		//entityAdmin.PostRenderUpdate();
		return true;
	}
};
