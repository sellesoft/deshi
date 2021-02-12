#include "deshi_renderer.h"
#include "deshi_glfw.h"
#include "deshi_import.h"
#include "../components/Model.h"
#include "../math/Math.h"
#include "../geometry/Triangle.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <set>
#include <array>
#include <exception>

const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

//////////////////////////
//// render interface ////
//////////////////////////

void Renderer_Vulkan::Init(Window* window) {
	std::cout << "\n{-} Initializing Vulkan {-}" << std::endl;
	this->glfwWindow = window->window;
	glfwSetWindowUserPointer(window->window, this);
	glfwSetFramebufferSizeCallback(window->window, framebufferResizeCallback);
	
	createInstance();
	setupDebugMessenger();
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();
	createDescriptorPool();
	int w, h;
	glfwGetFramebufferSize(glfwWindow, &w, &h);
	CreateOrResizeWindow(w, h);
	this->window.clearValues = (VkClearValue*)malloc(sizeof(VkClearValue) * 2);
	this->window.clearValues[0].color = {0, 0, 0, 1};
	this->window.clearValues[1].depthStencil = {1.f, 0};
	
	std::cout << "{-} Initializing Rendering {-}" << std::endl;
}

void Renderer_Vulkan::Render() {
	//std::cout << "{-}{-} Drawing Frame {-}{-}" << std::endl;
	if(framebufferResized) {
		int w, h;
		glfwGetFramebufferSize(glfwWindow, &w, &h);
		if(w > 0 && h > 0) {
			CreateOrResizeWindow(w, h);
			window.frameIndex = 0;
			framebufferResized = false;
		}
	}
	
	VkSemaphore image_sema = window.frameSephamores[window.semaphoreIndex].imageAcquiredSemaphore;
	VkSemaphore render_sema = window.frameSephamores[window.semaphoreIndex].renderCompleteSemaphore;
	VkResult result = vkAcquireNextImageKHR(device, window.swapchain, UINT64_MAX, image_sema, VK_NULL_HANDLE, &window.frameIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		framebufferResized = true;
		return;
	} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}
	
	FrameVk* frame = &window.frames[window.frameIndex];
	{ //wait on current fence and reset it after
		vkWaitForFences(device, 1, &frame->fence, VK_TRUE, UINT64_MAX);
		vkResetFences(device, 1, &frame->fence);
	}
	{ //reset command pool and begin command buffer
		vkResetCommandPool(device, frame->commandPool, 0);
        VkCommandBufferBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(frame->commandBuffer, &info);
	}
	{ //begin render pass
		VkRenderPassBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass = window.renderPass;
        info.framebuffer = frame->framebuffer;
        info.renderArea.extent.width = window.width;
        info.renderArea.extent.height = window.height;
        info.clearValueCount = 2;
        info.pClearValues = window.clearValues;
        vkCmdBeginRenderPass(frame->commandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
	}
	
	//draw below here
}

void Renderer_Vulkan::Present() {
	FrameVk* frame = &window.frames[window.frameIndex];
	VkSemaphore image_sema = window.frameSephamores[window.semaphoreIndex].imageAcquiredSemaphore;
	VkSemaphore render_sema = window.frameSephamores[window.semaphoreIndex].renderCompleteSemaphore;
	
	// Submit command buffer
    vkCmdEndRenderPass(frame->commandBuffer);
    {
        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &image_sema;
        info.pWaitDstStageMask = &wait_stage;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &frame->commandBuffer;
        info.signalSemaphoreCount = 1;
        info.pSignalSemaphores = &render_sema;
		
		vkEndCommandBuffer(frame->commandBuffer);
        vkQueueSubmit(graphicsQueue, 1, &info, frame->fence);
    }
	
	if(framebufferResized) { return; }
	
	//present the image
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &render_sema;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &window.swapchain;
	presentInfo.pImageIndices = &window.frameIndex;
	presentInfo.pResults = nullptr;
	VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);
	
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
		int w, h;
		glfwGetFramebufferSize(glfwWindow, &w, &h);
		CreateOrResizeWindow(w, h);
		framebufferResized = false;
	} else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}
	
	//iterate the frame index
	window.frameIndex = (window.frameIndex + 1) % window.imageCount; //loops back to zero after reaching max_frames
}

void Renderer_Vulkan::Cleanup() {
	std::cout << "{-} Initializing Cleanup {-}\n" << std::endl;
	//TODO(r,delle) make rendering cleanup
}

uint32 Renderer_Vulkan::AddTriangle(Triangle* triangle){
	std::cout << "Not implemented yet" << std::endl;
	return 0xFFFFFFFF;
}

void Renderer_Vulkan::RemoveTriangle(uint32 triangleID){
	std::cout << "Not implemented yet" << std::endl;
}

void Renderer_Vulkan::UpdateTriangleColor(uint32 triangleID, Color color){
	std::cout << "Not implemented yet" << std::endl;
}

void Renderer_Vulkan::UpdateTrianglePosition(uint32 triangleID, Vector3 position){
	std::cout << "Not implemented yet" << std::endl;
}

void Renderer_Vulkan::TranslateTriangle(uint32 triangleID, Vector3 translation){
	std::cout << "Not implemented yet" << std::endl;
}

std::vector<uint32> Renderer_Vulkan::AddTriangles(std::vector<Triangle*> triangles){
	std::cout << "Not implemented yet" << std::endl;
	return std::vector<uint32>();
}

void Renderer_Vulkan::RemoveTriangles(std::vector<uint32> triangleIDs){
	std::cout << "Not implemented yet" << std::endl;
}

void Renderer_Vulkan::UpdateTrianglesColor(std::vector<uint32> triangleIDs, Color color){
	std::cout << "Not implemented yet" << std::endl;
}

void Renderer_Vulkan::TranslateTriangles(std::vector<uint32> triangleIDs, Vector3 translation){
	std::cout << "Not implemented yet" << std::endl;
}

uint32 Renderer_Vulkan::LoadMesh(Mesh* mesh){
	std::cout << "Not implemented yet" << std::endl;
	return 0xFFFFFFFF;
}

void Renderer_Vulkan::UnloadMesh(uint32 meshID){
	std::cout << "Not implemented yet" << std::endl;
}

uint32 Renderer_Vulkan::LoadTexture(Texture* texure){
	std::cout << "Not implemented yet" << std::endl;
	return 0xFFFFFFFF;
}

void Renderer_Vulkan::UnloadTexture(uint32 textureID){
	std::cout << "Not implemented yet" << std::endl;
}

void Renderer_Vulkan::ApplyTextureToMesh(uint32 textureID, uint32 meshID){
	std::cout << "Not implemented yet" << std::endl;
}

void Renderer_Vulkan::RemoveTextureFromMesh(uint32 textureID, uint32 meshID){
	std::cout << "Not implemented yet" << std::endl;
}

void Renderer_Vulkan::UpdateMeshMatrix(uint32 meshID, Matrix4 matrix){
	std::cout << "Not implemented yet" << std::endl;
}

void Renderer_Vulkan::UpdateViewMatrix(Matrix4 matrix){
	std::cout << "Not implemented yet" << std::endl;
}

void Renderer_Vulkan::UpdatePerspectiveMatrix(Matrix4 matrix){
	std::cout << "Not implemented yet" << std::endl;
}

//////////////////////////
//// vulkan functions ////
//////////////////////////

void Renderer_Vulkan::RenderPipeline_Default() {
	
}

void Renderer_Vulkan::RenderPipeline_TwoD() {
	
}

void Renderer_Vulkan::RenderPipeline_Metal() {
	
}

void Renderer_Vulkan::RenderPipeline_Wireframe() {
	
}

void Renderer_Vulkan::createInstance() {
	std::cout << "{-}{-} Creating Vulkan Instance {-}{-}" << std::endl;
	if(enableValidationLayers && !checkValidationLayerSupport()) {
		throw std::runtime_error("validation layers requested, but not available!");
	}
	
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "P3DPGE";
	appInfo.applicationVersion = VK_MAKE_VERSION(0,5,0);
	appInfo.pEngineName = "P3DPGE";
	appInfo.engineVersion = VK_MAKE_VERSION(0,5,0);
	appInfo.apiVersion = VK_API_VERSION_1_0;
	
	std::vector<const char*> extensions = getRequiredExtensions();
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
	
	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = static_cast<uint32>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();
	if(enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
		populateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
	} else {
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}
	
	if(vkCreateInstance(&createInfo, allocator, &instance) != VK_SUCCESS) {
		throw std::runtime_error("failed to create instance!");
	}
}

void Renderer_Vulkan::setupDebugMessenger() {
	std::cout << "{-}{-} Setting Up Debug Messenger {-}{-}" << std::endl;
	if(!enableValidationLayers) return;
	
	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	populateDebugMessengerCreateInfo(createInfo);
	
	if (CreateDebugUtilsMessengerEXT(instance, &createInfo, allocator, &debugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug messenger!");
	}
}

void Renderer_Vulkan::createSurface() {
	std::cout << "{-}{-} Creating GLFW Surface {-}{-}" << std::endl;
	if(glfwCreateWindowSurface(instance, glfwWindow, allocator, &window.surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}
}

void Renderer_Vulkan::pickPhysicalDevice() {
	std::cout << "{-}{-} Picking Physical Device {-}{-}" << std::endl;
	uint32 deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
	
	for(auto& device : devices) {
		if(isDeviceSuitable(device)) {
			physicalDevice = device;
			msaaSamples = getMaxUsableSampleCount();
			break;
		}
	}
	
	if(physicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to find a suitable GPU!");
	}
	
	physicalQueueFamilies = findQueueFamilies(physicalDevice);
}

void Renderer_Vulkan::createLogicalDevice() {
	std::cout << "{-}{-} Creating Logical Device {-}{-}" << std::endl;
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32> uniqueQueueFamilies = {
		physicalQueueFamilies.graphicsFamily.value(), physicalQueueFamilies.presentFamily.value()
	};
	
	float queuePriority = 1.0f;
	//queueCreateInfos.reserve(uniqueQueueFamilies.size());
	for(uint32 queueFamily : uniqueQueueFamilies){
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = physicalQueueFamilies.graphicsFamily.value();
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}
	
	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	deviceFeatures.sampleRateShading = VK_TRUE; //enable sample shading feature for the device, VK_FALSE to disable sample shading
	
	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = static_cast<uint32>(queueCreateInfos.size());
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = static_cast<uint32>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();
	if(enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}else{
		createInfo.enabledLayerCount = 0;
	}
	
	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}
	
	vkGetDeviceQueue(device, physicalQueueFamilies.graphicsFamily.value(), 0, &graphicsQueue);
	vkGetDeviceQueue(device, physicalQueueFamilies.presentFamily.value(), 0, &presentQueue);
}

//imgui example way
void Renderer_Vulkan::createDescriptorPool(){
	std::cout << "{-}{-} Creating Descriptor Pool {-}{-}" << std::endl;
	const int types = 11;
	VkDescriptorPoolSize poolSizes[types] = {
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       1000 }
	};
	
	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.maxSets = 1000 * types;
	poolInfo.poolSizeCount = types;
	poolInfo.pPoolSizes = poolSizes;
	
	if (vkCreateDescriptorPool(device, &poolInfo, allocator, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void Renderer_Vulkan::DestroyFrame(FrameVk* frame) {
	vkDestroyFence(device, frame->fence, allocator);
	vkFreeCommandBuffers(device, frame->commandPool, 1, &frame->commandBuffer);
	vkDestroyCommandPool(device, frame->commandPool, allocator);
    frame->fence = VK_NULL_HANDLE;
    frame->commandBuffer = VK_NULL_HANDLE;
    frame->commandPool = VK_NULL_HANDLE;
	
    vkDestroyImageView(device, frame->imageView, allocator);
    vkDestroyFramebuffer(device, frame->framebuffer, allocator);
}

void Renderer_Vulkan::DestroyFrameSemaphore(FrameSemaphoreVk* sema) {
	vkDestroySemaphore(device, sema->imageAcquiredSemaphore, allocator);
	vkDestroySemaphore(device, sema->renderCompleteSemaphore, allocator);
	sema->imageAcquiredSemaphore = sema->renderCompleteSemaphore = VK_NULL_HANDLE;
}

/////////////////////////////////////////////////////////

int Renderer_Vulkan::GetMinImageCountFromPresentMode(VkPresentModeKHR mode) {
	switch(mode) {
		case(VK_PRESENT_MODE_MAILBOX_KHR):      { return 3; }
		case(VK_PRESENT_MODE_FIFO_KHR):         { return 2; }
		case(VK_PRESENT_MODE_FIFO_RELAXED_KHR): { return 2; }
		case(VK_PRESENT_MODE_IMMEDIATE_KHR):    { return 1; }
		default: { return -1; }
	}
	return -1;
}

void Renderer_Vulkan::CreateWindowSwapChain(int w, int h) {
	VkSwapchainKHR oldSwapChain = window.swapchain;
	window.swapchain = NULL;
	vkDeviceWaitIdle(device);
	
	//destroy old window swap chain stuff
	for(uint32 i = 0; i<window.imageCount; ++i) {
		DestroyFrame(&window.frames[i]);
		DestroyFrameSemaphore(&window.frameSephamores[i]);
	}
	free(window.frames);
	free(window.frameSephamores);
	window.frames = NULL;
	window.frameSephamores = NULL;
	window.imageCount = 0;
	if(window.renderPass) { vkDestroyRenderPass(device, window.renderPass, allocator); }
	if(window.pipelines.phong) { vkDestroyPipeline(device, window.pipelines.phong, allocator); }
	
	//query GPUs supported features for the swap chain
	window.supportDetails = querySwapChainSupport(physicalDevice);
	window.surfaceFormat = chooseSwapSurfaceFormat(window.supportDetails.formats);
	window.presentMode = chooseSwapPresentMode(window.supportDetails.presentModes);
	window.extent = chooseSwapExtent(window.supportDetails.capabilities);
	
	//get min image count if not specified
	if(minImageCount == 0) { minImageCount = GetMinImageCountFromPresentMode(window.presentMode); }
	
	uint32 queueFamilyIndices[] = {
		physicalQueueFamilies.graphicsFamily.value(), physicalQueueFamilies.presentFamily.value()
	};
	
	//create swapchain and swap chain images, set windowvk width and height
	{
		VkSwapchainCreateInfoKHR info{};
		info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		info.surface = window.surface;
		info.imageFormat = window.surfaceFormat.format;
		info.imageColorSpace = window.surfaceFormat.colorSpace;
		info.imageArrayLayers = 1;
		info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		if (physicalQueueFamilies.graphicsFamily != physicalQueueFamilies.presentFamily) {
			info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			info.queueFamilyIndexCount = 2;
			info.pQueueFamilyIndices = queueFamilyIndices;
		} else {
			info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			info.queueFamilyIndexCount = 0; // Optional
			info.pQueueFamilyIndices = nullptr; // Optional
		}
		info.preTransform = window.supportDetails.capabilities.currentTransform;
		info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		info.presentMode = window.presentMode;
        info.clipped = VK_TRUE;
        info.oldSwapchain = oldSwapChain;
		info.minImageCount = minImageCount;
		if(window.supportDetails.capabilities.maxImageCount != 0 && info.minImageCount > window.supportDetails.capabilities.maxImageCount) {
			info.minImageCount = window.supportDetails.capabilities.maxImageCount;
		}
		if(window.extent.width == 0xffffffff) {
			info.imageExtent.width = window.width = w;
			info.imageExtent.height = window.height = h;
		} else {
			info.imageExtent.width = window.width = window.extent.width;
			info.imageExtent.height = window.height = window.extent.height;
		}
		
		if (vkCreateSwapchainKHR(device, &info, allocator, &window.swapchain) != VK_SUCCESS) {
			throw std::runtime_error("failed to create swap chain!");
		}
		
		//get swap chain images
		vkGetSwapchainImagesKHR(device, window.swapchain, &window.imageCount, nullptr);
		VkImage images[16] = {};
		ASSERT(window.imageCount >= minImageCount, "the window should always have at least the min image count");
		ASSERT(window.imageCount < 16, "the window should have less than 16 images, around 2-3 is ideal");
		vkGetSwapchainImagesKHR(device, window.swapchain, &window.imageCount, images);
		
		//allocate memory for the window frames and semaphores
		ASSERT(window.frames == NULL, "this was set to null earlier and shouldnt have been changed yet");
		window.frames = (FrameVk*)malloc(sizeof(FrameVk) * window.imageCount);
		window.frameSephamores = (FrameSemaphoreVk*)malloc(sizeof(FrameSemaphoreVk) * window.imageCount);
		memset(window.frames, 0, sizeof(window.frames[0]) * window.imageCount);
		memset(window.frameSephamores, 0, sizeof(window.frameSephamores[0]) * window.imageCount);
		
		//set the frame images to the swap chain images
		for(uint32 i = 0; i<window.imageCount; ++i) {
			window.frames[i].image = images[i];
		}
	}
	
	//delete old swap chain
	if(oldSwapChain) { vkDestroySwapchainKHR(device, oldSwapChain, allocator); }
	
	//create the render pass
	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = window.surfaceFormat.format;
		colorAttachment.samples = msaaSamples;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		
		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = findDepthFormat();
		depthAttachment.samples = msaaSamples;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		
		VkAttachmentDescription colorAttachmentResolve{};
		colorAttachmentResolve.format = window.surfaceFormat.format;
		colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		
		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		VkAttachmentReference colorAttachmentResolveRef{};
		colorAttachmentResolveRef.attachment = 2;
		colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		
		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;
		subpass.pResolveAttachments = &colorAttachmentResolveRef;
		
		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		
		std::array<VkAttachmentDescription, 3> attachments = {colorAttachment, depthAttachment, colorAttachmentResolve};
		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;
		
		if (vkCreateRenderPass(device, &renderPassInfo, allocator, &window.renderPass) != VK_SUCCESS) {
			throw std::runtime_error("failed to create render pass!");
		}
	}
	
	//create the image views
	for(uint32 i = 0; i<window.imageCount; ++i) {
		window.frames[i].imageView = createImageView(window.frames[i].image, window.surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
	}
	
	//create framebuffer attachments
	VkFormat colorFormat = window.surfaceFormat.format;
	createImage(window.width, window.height, 1, msaaSamples, colorFormat, VK_IMAGE_TILING_OPTIMAL, 
				VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, window.attachments.colorImage, window.attachments.colorImageMemory);
	window.attachments.colorImageView = createImageView(window.attachments.colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
	VkFormat depthFormat = findDepthFormat();
	createImage(window.width, window.height, 1, msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL, 
				VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, window.attachments.depthImage, window.attachments.depthImageMemory);
	window.attachments.depthImageView = createImageView(window.attachments.depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
	
	//create the framebuffers
	for (uint32 i = 0; i < window.imageCount; i++) {
		std::array<VkImageView, 3> attachments = {
			window.attachments.colorImageView, window.attachments.depthImageView, window.frames[i].imageView
		};
		
		VkFramebufferCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		info.renderPass = window.renderPass;
		info.attachmentCount = static_cast<uint32>(attachments.size());
		info.pAttachments = attachments.data();
		info.width = window.width;
		info.height = window.height;
		info.layers = 1;
		
		if (vkCreateFramebuffer(device, &info, allocator, &window.frames[i].framebuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}

void Renderer_Vulkan::CreateWindowCommandBuffers() {
	for(uint32 i = 0; i<window.imageCount; ++i) {
		FrameVk* frame = &window.frames[i];
		FrameSemaphoreVk* sema = &window.frameSephamores[i];
		{ //create command pool
			VkCommandPoolCreateInfo poolInfo{};
			poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			poolInfo.queueFamilyIndex = physicalQueueFamilies.graphicsFamily.value();
			poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			if (vkCreateCommandPool(device, &poolInfo, allocator, &frame->commandPool) != VK_SUCCESS) {
				throw std::runtime_error("failed to create command pool!");
			}
		}
		{ //allocate command buffers
			VkCommandBufferAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.commandPool = frame->commandPool;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfo.commandBufferCount = 1;
			if (vkAllocateCommandBuffers(device, &allocInfo, &frame->commandBuffer) != VK_SUCCESS) {
				throw std::runtime_error("failed to allocate command buffers!");
			}
		}
		{ //create sync objects
			VkSemaphoreCreateInfo semaphoreInfo{};
			semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			VkFenceCreateInfo fenceInfo{};
			fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
			if(vkCreateSemaphore(device, &semaphoreInfo, allocator, &sema->imageAcquiredSemaphore) != VK_SUCCESS ||
			   vkCreateSemaphore(device, &semaphoreInfo, allocator, &sema->renderCompleteSemaphore) != VK_SUCCESS ||
			   vkCreateFence(device, &fenceInfo, allocator, &frame->fence) != VK_SUCCESS) {
				throw std::runtime_error("failed to create synchronization objects for a frame!");
			}
		}
	}
}

void Renderer_Vulkan::CreateOrResizeWindow(int w, int h) {
	CreateWindowSwapChain(w, h);
	CreateWindowCommandBuffers();
}

/////////////////////////////
////// utility functions ////
/////////////////////////////

bool Renderer_Vulkan::checkValidationLayerSupport() {
	std::cout << "{-}{-}{-} Checking Validation Layer Support {-}{-}{-}" << std::endl;
	uint32 layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
	
	for(const char* layerName : validationLayers) {
		bool layerFound = false;
		for(const auto& layerProperties : availableLayers) {
			if(strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}
		if(!layerFound) return false;
	}
	return true;
}

std::vector<const char*> Renderer_Vulkan::getRequiredExtensions() {
	std::cout << "{-}{-}{-} Getting Required Extensions {-}{-}{-}" << std::endl;
	uint32 glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
	if(enableValidationLayers) { extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); }
	return extensions;
}

void Renderer_Vulkan::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
	std::cout << "{-}{-}{-} Populating Debug Messenger CreateInfo {-}{-}{-}" << std::endl;
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = /*VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |*/ VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
}

bool Renderer_Vulkan::isDeviceSuitable(VkPhysicalDevice device) {
	std::cout << "{-}{-}{-} Checking Validation Layer Support {-}{-}{-}" << std::endl;
	QueueFamilyIndices indices = findQueueFamilies(device);
	bool extensionsSupported = checkDeviceExtensionSupport(device);
	bool swapChainAdequate = false;
	if(extensionsSupported) {
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}
	
	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
	
	return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

QueueFamilyIndices Renderer_Vulkan::findQueueFamilies(VkPhysicalDevice device) {
	std::cout << "{-}{-}{-} Finding Queue Families {-}{-}{-}" << std::endl;
	QueueFamilyIndices indices;
	
	uint32 queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
	
	int i = 0;
	for(auto& queueFamily : queueFamilies) {
		if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) { indices.graphicsFamily = i; }
		
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, window.surface, &presentSupport);
		if (presentSupport) { indices.presentFamily = i; }
		
		if(indices.isComplete()) { break; }
		i++;
	}
	
	return indices;
}

bool Renderer_Vulkan::checkDeviceExtensionSupport(VkPhysicalDevice device) {
	std::cout << "{-}{-}{-} Checking Device Extension Support {-}{-}{-}" << std::endl;
	uint32 extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
	
	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}
	
	return requiredExtensions.empty();
}

SwapChainSupportDetails Renderer_Vulkan::querySwapChainSupport(VkPhysicalDevice device) {
	std::cout << "{-}{-}{-} Querying SwapChain Support {-}{-}{-}" << std::endl;
	SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, window.surface, &details.capabilities);
	
	uint32 formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, window.surface, &formatCount, nullptr);
	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, window.surface, &formatCount, details.formats.data());
	}
	
	uint32 presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, window.surface, &presentModeCount, nullptr);
	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, window.surface, &presentModeCount, details.presentModes.data());
	}
	
	return details;
}

VkSampleCountFlagBits Renderer_Vulkan::getMaxUsableSampleCount() {
	std::cout << "{-}{-}{-} Getting Max Usable Sample Count {-}{-}{-}" << std::endl;
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
	
	VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
	if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
	if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
	if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
	if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
	if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
	if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }
	
	return VK_SAMPLE_COUNT_1_BIT;
}

VkSurfaceFormatKHR Renderer_Vulkan::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
	std::cout << "{-}{-}{-} Choosing Swap Surface Format {-}{-}{-}" << std::endl;
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}
	return availableFormats[0];
}

VkPresentModeKHR Renderer_Vulkan::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
	std::cout << "{-}{-}{-} Choosing Swap Present Mode {-}{-}{-}" << std::endl;
	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentMode;
		}
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Renderer_Vulkan::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
	std::cout << "{-}{-}{-} Choosing Swap Extent {-}{-}{-}" << std::endl;
	if (capabilities.currentExtent.width != UINT32_MAX) {
		return capabilities.currentExtent;
	} else {
		int width, height;
		glfwGetFramebufferSize(glfwWindow, &width, &height);
		
		VkExtent2D actualExtent = { static_cast<uint32>(width), static_cast<uint32>(height) };
		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
		
		return actualExtent;
	}
}

VkImageView Renderer_Vulkan::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32 mipLevels) {
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;
	
	VkImageView imageView;
	if (vkCreateImageView(device, &viewInfo, allocator, &imageView) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view!");
	}
	
	return imageView;
}

VkFormat Renderer_Vulkan::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
		
		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		} else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}
	
	throw std::runtime_error("failed to find supported format!");
}

VkFormat Renderer_Vulkan::findDepthFormat() {
	return findSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkShaderModule Renderer_Vulkan::createShaderModule(const std::vector<char>& code) {
	//std::cout << "{-}{-}{-} Creating Shader Module {-}{-}{-}" << std::endl;
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32*>(code.data());
	
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, allocator, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module!");
	}
	
	return shaderModule;
}

uint32 Renderer_Vulkan::findMemoryType(uint32 typeFilter, VkMemoryPropertyFlags properties) {
	//std::cout << "{-}{-}{-} Finding Memory Types {-}{-}{-}" << std::endl;
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
	
	for (uint32 i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}
	throw std::runtime_error("failed to find suitable memory type!"); //error out if no suitable memory found
}

void Renderer_Vulkan::createImage(uint32 width, uint32 height, uint32 mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = mipLevels;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = numSamples;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	
	if (vkCreateImage(device, &imageInfo, allocator, &image) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}
	
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, image, &memRequirements);
	
	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
	
	if (vkAllocateMemory(device, &allocInfo, allocator, &imageMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory!");
	}
	
	vkBindImageMemory(device, image, imageMemory, 0);
}

void Renderer_Vulkan::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	
	if (vkCreateBuffer(device, &bufferInfo, allocator, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create buffer!");
	}
	
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);
	
	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
	
	if (vkAllocateMemory(device, &allocInfo, allocator, &bufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate buffer memory!");
	}
	
	vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

VkCommandBuffer Renderer_Vulkan::beginSingleTimeCommands() {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = window.frames[window.frameIndex].commandPool;
	allocInfo.commandBufferCount = 1;
	
	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
	
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	
	vkBeginCommandBuffer(commandBuffer, &beginInfo);
	
	return commandBuffer;
}

void Renderer_Vulkan::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
	vkEndCommandBuffer(commandBuffer);
	
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	
	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);
	
	vkFreeCommandBuffers(device, window.frames[window.frameIndex].commandPool, 1, &commandBuffer);
}

void Renderer_Vulkan::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32 mipLevels) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();
	
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	
	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;
	
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} /*else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	} */else {
		throw std::invalid_argument("unsupported layout transition!");
	}
	
	vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	
	endSingleTimeCommands(commandBuffer);
}

void Renderer_Vulkan::copyBufferToImage(VkBuffer buffer, VkImage image, uint32 width, uint32 height) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();
	
	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	
	region.imageOffset = {0, 0, 0};
	region.imageExtent = {width, height, 1};
	
	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	
	endSingleTimeCommands(commandBuffer);
}

void Renderer_Vulkan::generateMipmaps(VkImage image, VkFormat imageFormat, int32 texWidth, int32 texHeight, uint32 mipLevels) {
	// Check if image format supports linear blitting
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);
	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
		throw std::runtime_error("texture image format does not support linear blitting!");
	}
	
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();
	
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;
	
	int32 mipWidth = texWidth;
	int32 mipHeight = texHeight;
	for (uint32 i = 1; i < mipLevels; i++) {
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
		
		VkImageBlit blit{};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;
		
		vkCmdBlitImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image,  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);
		
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
		
		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;
	}
	
	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	
	endSingleTimeCommands(commandBuffer);
}

void Renderer_Vulkan::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();
	
	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
	
	endSingleTimeCommands(commandBuffer);
}


VKAPI_ATTR VkBool32 VKAPI_CALL Renderer_Vulkan::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
	std::cerr << "/\\  " << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}


VkResult Renderer_Vulkan::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void Renderer_Vulkan::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

void Renderer_Vulkan::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
	auto app = reinterpret_cast<Renderer_Vulkan*>(glfwGetWindowUserPointer(window));
	app->framebufferResized = true;
}

/////////////////////////////////////////
//// vulkan support struct functions ////
/////////////////////////////////////////

VkVertexInputBindingDescription VertexVk::getBindingDescription() {
	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(VertexVk);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 3> VertexVk::getAttributeDescriptions() {
	std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
	
	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(VertexVk, pos);
	
	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(VertexVk, color);
	
	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(VertexVk, texCoord);
	
	return attributeDescriptions;
}
