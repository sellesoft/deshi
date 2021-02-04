//#define DEBUG_P3DPGE
//#include "EntityAdmin.h"
#include "deshi_input.h"
#include "deshi_glfw.h"

struct Deshi {
	//EntityAdmin entityAdmin;
	//Renderer renderer;
	Input input;
	Window window;
	
	bool Init() {
		window.Init(&input, 1280, 720);
		//renderer.Init();
		return true;
	}
	
	bool Update(float deltaTime) {
		return true;
	}
	
	void Cleanup(){
		//renderer.Cleanup();
		window.Cleanup();
	}
	
	void Start(){
		window.StartLoop();
	}
};

int main() {
	Deshi engine;
	if (engine.Init()) { engine.Start(); }
}