#include "window.h"
#include "input.h"

#if defined(_MSC_VER)
#pragma comment(lib,"glfw3.lib")
#endif
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

void glfwError(int id, const char* description){
	std::cout << description << std::endl;
}

void Window::Init(Input* input, int32 width, int32 height, int32 x, int32 y, DisplayMode displayMode){
	glfwSetErrorCallback(&glfwError);
	if (!glfwInit()){ return; }
	Window::input = input;
	
	//TODO(r,delle) maybe we should not allow the window to be resizable in-game, but in-engine is fine
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE); 
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(width, height, "Deshi", NULL, NULL);
	monitor = glfwGetPrimaryMonitor();
	if(!window) { glfwTerminate(); return; }
	if(!monitor) { glfwTerminate(); return; }
	
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);
	int xpos, ypos;
	glfwGetWindowPos(window, &xpos, &ypos);
	this->x = xpos; this->y = ypos;
	this->width = width; this->height = height;
	this->screenWidth = mode->width; this->screenHeight = mode->height;
	this->screenRefreshRate = mode->refreshRate;
	this->displayMode = displayMode;
	this->dimensions = Vector2(width, height);
	
	UpdateDisplayMode(displayMode);
	
	//keyboard mappings
	input->mapKeys[0x00] = Key::NONE;
	input->mapKeys['A'] = Key::A; input->mapKeys['B'] = Key::B; input->mapKeys['C'] = Key::C;
	input->mapKeys['D'] = Key::D; input->mapKeys['E'] = Key::E; input->mapKeys['F'] = Key::F;
	input->mapKeys['G'] = Key::G; input->mapKeys['H'] = Key::H; input->mapKeys['I'] = Key::I;
	input->mapKeys['J'] = Key::J; input->mapKeys['K'] = Key::K; input->mapKeys['L'] = Key::L; 
	input->mapKeys['M'] = Key::M; input->mapKeys['N'] = Key::N; input->mapKeys['O'] = Key::O;
	input->mapKeys['P'] = Key::P; input->mapKeys['Q'] = Key::Q; input->mapKeys['R'] = Key::R;
	input->mapKeys['S'] = Key::S; input->mapKeys['T'] = Key::T; input->mapKeys['U'] = Key::U;
	input->mapKeys['V'] = Key::V; input->mapKeys['W'] = Key::W; input->mapKeys['X'] = Key::X;
	input->mapKeys['Y'] = Key::Y; input->mapKeys['Z'] = Key::Z;
	
	input->mapKeys[GLFW_KEY_0] = Key::K0; input->mapKeys[GLFW_KEY_1] = Key::K1;
	input->mapKeys[GLFW_KEY_2] = Key::K2; input->mapKeys[GLFW_KEY_3] = Key::K3; 
	input->mapKeys[GLFW_KEY_4] = Key::K4; input->mapKeys[GLFW_KEY_5] = Key::K5;
	input->mapKeys[GLFW_KEY_6] = Key::K6; input->mapKeys[GLFW_KEY_7] = Key::K7; 
	input->mapKeys[GLFW_KEY_8] = Key::K8; input->mapKeys[GLFW_KEY_9] = Key::K9;
	
	input->mapKeys[GLFW_KEY_F1]  = Key::F1;  input->mapKeys[GLFW_KEY_F2]  = Key::F2; 
	input->mapKeys[GLFW_KEY_F3]  = Key::F3;  input->mapKeys[GLFW_KEY_F4]  = Key::F4; 
	input->mapKeys[GLFW_KEY_F5]  = Key::F5;  input->mapKeys[GLFW_KEY_F6]  = Key::F6;
	input->mapKeys[GLFW_KEY_F7]  = Key::F7;  input->mapKeys[GLFW_KEY_F8]  = Key::F8; 
	input->mapKeys[GLFW_KEY_F9]  = Key::F9;  input->mapKeys[GLFW_KEY_F10] = Key::F10;
	input->mapKeys[GLFW_KEY_F11] = Key::F11; input->mapKeys[GLFW_KEY_F12] = Key::F12;
	
	input->mapKeys[GLFW_KEY_DOWN] = Key::DOWN; input->mapKeys[GLFW_KEY_UP]    = Key::UP;
	input->mapKeys[GLFW_KEY_LEFT] = Key::LEFT; input->mapKeys[GLFW_KEY_RIGHT] = Key::RIGHT;
	
	input->mapKeys[GLFW_KEY_ESCAPE]        = Key::ESCAPE; 
	input->mapKeys[GLFW_KEY_GRAVE_ACCENT]  = Key::TILDE;
	input->mapKeys[GLFW_KEY_TAB]           = Key::TAB; 
	input->mapKeys[GLFW_KEY_CAPS_LOCK]     = Key::CAPSLOCK;
	input->mapKeys[GLFW_KEY_LEFT_SHIFT]    = Key::LSHIFT; 
	input->mapKeys[GLFW_KEY_RIGHT_SHIFT]   = Key::RSHIFT;
	input->mapKeys[GLFW_KEY_LEFT_CONTROL]  = Key::LCONTROL; 
	input->mapKeys[GLFW_KEY_RIGHT_CONTROL] = Key::RCONTROL;
	input->mapKeys[GLFW_KEY_LEFT_ALT]      = Key::LALT; 
	input->mapKeys[GLFW_KEY_RIGHT_ALT]     = Key::RALT; 
	input->mapKeys[GLFW_KEY_BACKSPACE]     = Key::BACKSPACE; 
	input->mapKeys[GLFW_KEY_ENTER]         = Key::ENTER;
	input->mapKeys[GLFW_KEY_MINUS]         = Key::MINUS; 
	input->mapKeys[GLFW_KEY_EQUAL]         = Key::EQUALS;
	input->mapKeys[GLFW_KEY_LEFT_BRACKET]  = Key::LBRACKET; 
	input->mapKeys[GLFW_KEY_RIGHT_BRACKET] = Key::RBRACKET;
	input->mapKeys[GLFW_KEY_SLASH]         = Key::SLASH; 
	input->mapKeys[GLFW_KEY_SEMICOLON]     = Key::SEMICOLON;
	input->mapKeys[GLFW_KEY_APOSTROPHE]    = Key::APOSTROPHE; 
	input->mapKeys[GLFW_KEY_COMMA]         = Key::COMMA;
	input->mapKeys[GLFW_KEY_PERIOD]        = Key::PERIOD;
	input->mapKeys[GLFW_KEY_BACKSLASH]     = Key::BACKSLASH;
	input->mapKeys[GLFW_KEY_SPACE]         = Key::SPACE;
	
	input->mapKeys[GLFW_KEY_INSERT]  = Key::INSERT; input->mapKeys[GLFW_KEY_DELETE]      = Key::DELETE;
	input->mapKeys[GLFW_KEY_HOME]    = Key::HOME;   input->mapKeys[GLFW_KEY_END]         = Key::END;
	input->mapKeys[GLFW_KEY_PAGE_UP] = Key::PAGEUP; input->mapKeys[GLFW_KEY_PAGE_DOWN]   = Key::PAGEDOWN;
	input->mapKeys[GLFW_KEY_PAUSE]   = Key::PAUSE;  input->mapKeys[GLFW_KEY_SCROLL_LOCK] = Key::SCROLL;
	
	input->mapKeys[GLFW_KEY_KP_0] = Key::NUMPAD0; input->mapKeys[GLFW_KEY_KP_1] = Key::NUMPAD1;
	input->mapKeys[GLFW_KEY_KP_2] = Key::NUMPAD2; input->mapKeys[GLFW_KEY_KP_3] = Key::NUMPAD3;
	input->mapKeys[GLFW_KEY_KP_4] = Key::NUMPAD4; input->mapKeys[GLFW_KEY_KP_5] = Key::NUMPAD5;
	input->mapKeys[GLFW_KEY_KP_6] = Key::NUMPAD6; input->mapKeys[GLFW_KEY_KP_7] = Key::NUMPAD7;
	input->mapKeys[GLFW_KEY_KP_8] = Key::NUMPAD8; input->mapKeys[GLFW_KEY_KP_9] = Key::NUMPAD9;
	
	input->mapKeys[GLFW_KEY_KP_MULTIPLY] = Key::NUMPADMULTIPLY;
	input->mapKeys[GLFW_KEY_KP_DIVIDE]   = Key::NUMPADDIVIDE;
	input->mapKeys[GLFW_KEY_KP_ADD]      = Key::NUMPADPLUS; 
	input->mapKeys[GLFW_KEY_KP_SUBTRACT] = Key::NUMPADMINUS;
	input->mapKeys[GLFW_KEY_KP_DECIMAL]  = Key::NUMPADPERIOD; 
	input->mapKeys[GLFW_KEY_KP_ENTER]    = Key::NUMPADENTER;
	input->mapKeys[GLFW_KEY_NUM_LOCK]    = Key::NUMLOCK;
	
	//mouse mappings
	input->mapMouse[GLFW_MOUSE_BUTTON_1] = MouseButton::MB_LEFT;
	input->mapMouse[GLFW_MOUSE_BUTTON_2] = MouseButton::MB_RIGHT;
	input->mapMouse[GLFW_MOUSE_BUTTON_3] = MouseButton::MB_MIDDLE;
	input->mapMouse[GLFW_MOUSE_BUTTON_4] = MouseButton::MB_FOUR;
	input->mapMouse[GLFW_MOUSE_BUTTON_5] = MouseButton::MB_FIVE;
	
	//event callbacks
	//void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
	glfwSetMouseButtonCallback(window, 
							   [](GLFWwindow* w, int button, int action, int mods)->void{
								   std::map<size_t, uint8>::iterator it = Window::input->mapMouse.find(button);
								   if(it != Window::input->mapMouse.end()){
									   if(action == GLFW_PRESS){
										   Window::input->realMouseState[it->second] = true;
									   }else if(action == GLFW_RELEASE){
										   Window::input->realMouseState[it->second] = false;
									   }
								   }
							   });
	
	//static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
	glfwSetCursorPosCallback(window,
							 [](GLFWwindow* window, double xpos, double ypos)->void{
								 //mouse coords come in screen space, must convert to window space
								 GLFWmonitor* monitor = glfwGetPrimaryMonitor();
								 int x, y, width, height;
								 glfwGetMonitorWorkarea(monitor, &x, &y, &width, &height);
								 Window::input->mouseFocus = true;
								 Window::input->realMouseX = xpos - (float)x;
								 Window::input->realMouseY = ypos - (float)y;
								 Window::input->realScreenMouseX = xpos;
								 Window::input->realScreenMouseY = ypos;
							 });
	
	//void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
	glfwSetKeyCallback(window,
					   [](GLFWwindow* window, int key, int scancode, int action, int mods)->void{
						   std::map<size_t, uint8>::iterator it = Window::input->mapKeys.find(key);
						   if(it != Window::input->mapKeys.end()){
							   if(action == GLFW_PRESS){
								   Window::input->realKeyState[it->second] = true;
							   }else if(action == GLFW_RELEASE){
								   Window::input->realKeyState[it->second] = false;
							   }
						   }
					   });
	
	//void cursor_enter_callback(GLFWwindow* window, int entered)
	glfwSetCursorEnterCallback(window, 
							   [](GLFWwindow* w, int entered)->void{
								   if (entered){
									   Window::input->keyFocus = true;
								   }else{
									   Window::input->keyFocus = false;
								   }
							   });
	
	//void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
	glfwSetScrollCallback(window, 
						  [](GLFWwindow* window, double xoffset, double yoffset)->void{
							  Window::input->realScreenMouseX = xoffset;
							  Window::input->realScreenMouseY = yoffset;
						  });
	
}//Init

void Window::Update() {
	int xpos, ypos;
	glfwGetWindowPos(window, &xpos, &ypos);
	this->x = xpos; this->y = ypos;
	
	int width, height;
	glfwGetWindowSize(window, &width, &height);
	this->width = width; this->height = height;
	this->minimized = width <= 0 || height <= 0 ? true : false;
	
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);
	this->screenWidth = mode->width; this->screenHeight = mode->height;
	this->screenRefreshRate = mode->refreshRate; 
	
	this->dimensions = Vector2(width, height);
}

void Window::Cleanup(){
	glfwTerminate();
}

void Window::UpdateDisplayMode(DisplayMode displayMode){
	if(displayMode == this->displayMode){return;}
	this->displayMode = displayMode;
	
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);
	switch(displayMode){
		case(DisplayMode::FULLSCREEN):{
			glfwSetWindowMonitor(this->window, this->monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
		}break;
		case(DisplayMode::BORDERLESS_WINDOWED):{
			glfwSetWindowAttrib(this->window, GLFW_DECORATED, GLFW_FALSE);
			glfwSetWindowMonitor(this->window, 0, 0, 0, mode->width, mode->height, mode->refreshRate);
		}break;
		default:{ //DisplayMode::WINDOWED
			glfwSetWindowMonitor(this->window, 0, this->x, this->y, this->width, this->height, GLFW_DONT_CARE);
		}break;
	}
}