#pragma once
#include "deshi_input.h"
#include "deshi_glfw.h"
#include "deshi_renderer.h"

#include "internal/imgui/imgui.h"
#include "internal/imgui/imgui_impl_glfw.h"
#include "internal/imgui/imgui_impl_vulkan.h"

#if defined(_MSC_VER)
#pragma comment(lib,"vulkan-1.lib")
#pragma comment(lib,"glfw3.lib")
#endif
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <vector>

struct enKeyCharMap{
	Key key;
	char lower;
	char upper;
};

static void check_vk_result(VkResult err){
    if (err == 0)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}

//thanks: https://github.com/dandistine/olcPGEDearImGui
struct deshiImGui{
	Renderer* renderer;
	Input* input;
	Window* window;
	std::vector<Key> controlInputKeys;
	std::vector<enKeyCharMap> enValueInputKeys;
	
	virtual void Init(Renderer* renderer, Input* input, Window* window){
		this->input = input;
		this->window = window;
		this->renderer = renderer;
		
		//Setup Dear ImGui context
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		
		io.KeyMap[ImGuiKey_Tab] = Key::TAB;           io.KeyMap[ImGuiKey_LeftArrow] = Key::LEFT;
		io.KeyMap[ImGuiKey_RightArrow] = Key::RIGHT;  io.KeyMap[ImGuiKey_UpArrow] = Key::UP;
		io.KeyMap[ImGuiKey_DownArrow] = Key::DOWN;    io.KeyMap[ImGuiKey_PageUp] = Key::PAGEUP;
		io.KeyMap[ImGuiKey_PageDown] = Key::PAGEDOWN; io.KeyMap[ImGuiKey_Home] = Key::HOME;
		io.KeyMap[ImGuiKey_End] = Key::END;           io.KeyMap[ImGuiKey_Insert] = Key::INSERT;
		io.KeyMap[ImGuiKey_Delete] = Key::DELETE;     io.KeyMap[ImGuiKey_Backspace] = Key::BACKSPACE;
		io.KeyMap[ImGuiKey_Space] = Key::SPACE;       io.KeyMap[ImGuiKey_Enter] = Key::ENTER;
		io.KeyMap[ImGuiKey_Escape] = Key::ESCAPE;     io.KeyMap[ImGuiKey_KeyPadEnter] = Key::NUMPADENTER;
		io.KeyMap[ImGuiKey_A] = Key::A; io.KeyMap[ImGuiKey_C] = Key::C; io.KeyMap[ImGuiKey_V] = Key::V;
		io.KeyMap[ImGuiKey_X] = Key::X; io.KeyMap[ImGuiKey_Y] = Key::Y; io.KeyMap[ImGuiKey_Z] = Key::Z;
		
		//Create a listing of all the control keys so we can iterate them later
		controlInputKeys = {
			Key::TAB, Key::LEFT, Key::RIGHT, Key::UP, Key::DOWN,Key::PAGEUP, Key::PAGEDOWN, 
			Key::HOME, Key::END, Key::INSERT, Key::DELETE, Key::BACKSPACE, Key::SPACE, Key::ENTER, 
			Key::ESCAPE, Key::NUMPADENTER, Key::A, Key::C, Key::V, Key::X, Key::Y, Key::Z,
		};
		
		//Map keys which input values to input boxes.
		enValueInputKeys = {
			{Key::A, 'a', 'A'}, {Key::B, 'b', 'B'}, {Key::C, 'c', 'C'}, {Key::D, 'd', 'D'},
			{Key::E, 'e', 'E'}, {Key::F, 'f', 'F'}, {Key::G, 'g', 'G'}, {Key::H, 'h', 'H'},
			{Key::I, 'i', 'I'}, {Key::J, 'j', 'J'}, {Key::K, 'k', 'K'}, {Key::L, 'l', 'L'},
			{Key::M, 'm', 'M'}, {Key::N, 'n', 'N'}, {Key::O, 'o', 'O'}, {Key::P, 'p', 'P'},
			{Key::Q, 'q', 'Q'}, {Key::R, 'r', 'R'}, {Key::S, 's', 'S'}, {Key::T, 't', 'T'},
			{Key::U, 'u', 'U'}, {Key::V, 'v', 'V'}, {Key::W, 'w', 'W'}, {Key::X, 'x', 'X'},
			{Key::Y, 'y', 'Y'}, {Key::Z, 'z', 'Z'}, {Key::K0, '0', ')'},{Key::K1, '1', '!'},
			{Key::K2, '2', '@'},{Key::K3, '3', '#'},{Key::K4, '4', '$'},{Key::K5, '5', '%'},
			{Key::K6, '6', '^'},{Key::K7, '7', '&'},{Key::K8, '8', '*'},{Key::K9, '9', '('},
			{Key::NUMPADMULTIPLY, '*', '*'}, {Key::NUMPADDIVIDE, '/', '/'}, {Key::NUMPADPLUS, '+', '+'}, 
			{Key::NUMPADMINUS, '-', '-'}, {Key::NUMPADPERIOD, '.', '.'}, {Key::PERIOD, '.', '>'}, 
			{Key::SPACE, ' ', ' '}, {Key::MINUS, '-', '_'}, {Key::COMMA, ',', '<'}, {Key::EQUALS, '=', '+'},
			{Key::SEMICOLON, ';',':'}, {Key::SLASH, '/','?'}, {Key::TILDE, '~','`'},  
			{Key::LBRACKET, '[', '{'}, {Key::RBRACKET, ']', '}'}, {Key::APOSTROPHE, '\'', '\"'},
		};
		
		//Setup Dear ImGui style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsClassic();
	}
	
	virtual void Cleanup() = 0;
	virtual void NewFrame() = 0;
	virtual void EndFrame() = 0;
};

struct vkImGui : public deshiImGui{
	Renderer_Vulkan* vkr;
	
	void Init(Renderer* renderer, Input* input, Window* window) override{
		deshiImGui::Init(renderer, input, window);
		vkr = (Renderer_Vulkan*)renderer; 
		/*
		//Setup Platform/Renderer backends
		ImGui_ImplGlfw_InitForVulkan(window->window, true);
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = vkr->instance;
		init_info.PhysicalDevice = vkr->physicalDevice;
		init_info.Device = vkr->device;
		init_info.QueueFamily = vkr->physicalQueueFamilies.graphicsFamily;
		init_info.Queue = vkr->graphicsQueue;
		init_info.PipelineCache = vkr->graphicsPipelineCache;
		init_info.DescriptorPool = g_DescriptorPool;
		init_info.Allocator = vkr->allocator;
		init_info.MinImageCount = g_MinImageCount;
		init_info.ImageCount = wd->ImageCount;
		init_info.CheckVkResultFn = check_vk_result;
		ImGui_ImplVulkan_Init(&init_info, vkr->renderPass);
		*/
		
	}
	
	void Cleanup() override{
		/*ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();*/
		ImGui::DestroyContext();
	}
	
	void NewFrame() override{
		
	}
	
	void EndFrame() override{
		
	}
};