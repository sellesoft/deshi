#pragma once
#include "deshi_input.h"
#include "deshi_glfw.h"
#include "deshi_renderer.h"
#include "deshi_time.h"

#include "../external/imgui/imgui.h"
#include "../external/imgui/imgui_impl_glfw.h"
#include "../external/imgui/imgui_impl_vulkan.h"

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
	Key::Key key;
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
	Input* input;
	Window* window;
	Time* time;
	
	virtual void Init(Renderer* renderer, Input* input, Window* window, Time* time){
		this->input = input;
		this->window = window;
		this->time = time;
		
		//Setup Dear ImGui context
		ImGui::CreateContext();
		
		//Setup Dear ImGui style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsClassic();
	}
	
	virtual void Cleanup() = 0;
	virtual void NewFrame()= 0;
	virtual void EndFrame() = 0;
};

struct vkImGui : public deshiImGui{
	Renderer_Vulkan* vkr;
	
	void Init(Renderer* renderer, Input* input, Window* window, Time* time) override{
		deshiImGui::Init(renderer, input, window, time);
		vkr = (Renderer_Vulkan*)renderer; 
		VkResult err;
		
		//Setup Platform/Renderer backends
		ImGui_ImplGlfw_InitForVulkan(vkr->window, true);
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = vkr->instance;
		init_info.PhysicalDevice = vkr->physicalDevice;
		init_info.Device = vkr->device;
		init_info.QueueFamily = vkr->physicalQueueFamilies.graphicsFamily.get();
		init_info.Queue = vkr->graphicsQueue;
		init_info.PipelineCache = vkr->graphicsPipelineCache;
		init_info.DescriptorPool = vkr->descriptorPool;
		init_info.Allocator = vkr->allocator;
		init_info.MinImageCount = vkr->minImageCount;
		init_info.ImageCount = vkr->imageCount;
		init_info.CheckVkResultFn = check_vk_result;
		init_info.MSAASamples = vkr->msaaSamples;
		ImGui_ImplVulkan_Init(&init_info, vkr->renderPass);
		
		// Upload Fonts
		{
			VkCommandPool command_pool = vkr->frames[vkr->frameIndex].commandPool;
			VkCommandBuffer command_buffer = vkr->frames[vkr->frameIndex].commandBuffer;
			
			err = vkResetCommandPool(vkr->device, command_pool, 0);
			check_vk_result(err);
			VkCommandBufferBeginInfo begin_info = {};
			begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			err = vkBeginCommandBuffer(command_buffer, &begin_info);
			check_vk_result(err);
			
			ImGui_ImplVulkan_CreateFontsTexture(command_buffer);
			
			VkSubmitInfo end_info = {};
			end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			end_info.commandBufferCount = 1;
			end_info.pCommandBuffers = &command_buffer;
			err = vkEndCommandBuffer(command_buffer);
			check_vk_result(err);
			err = vkQueueSubmit(vkr->graphicsQueue, 1, &end_info, VK_NULL_HANDLE);
			check_vk_result(err);
			
			err = vkDeviceWaitIdle(vkr->device);
			check_vk_result(err);
			ImGui_ImplVulkan_DestroyFontUploadObjects();
		}
	}
	
	void Cleanup() override{
		VkResult err;
		err = vkDeviceWaitIdle(vkr->device);
		check_vk_result(err);
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
	
	void NewFrame() override{
		//TODO(r,delle) find out if this is actually needed
		if(vkr->framebufferResized){
			int w, h;
			glfwGetFramebufferSize(vkr->window, &w, &h);
			if(w > 0 && h > 0){
				ImGui_ImplVulkan_SetMinImageCount(vkr->minImageCount);
			}
		}
		
		// Start the Dear ImGui frame
		ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
	}
	
	void EndFrame() override{
		ImGui::Render();
		// Record dear imgui primitives into command buffer
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), vkr->frames[vkr->frameIndex].commandBuffer);
	}
};