#include "deshi_renderer.h"
#include "deshi_glfw.h"
#include "deshi_imgui.h"
#include "deshi_assets.h"
#include "deshi_time.h"
#include "../animation/Model.h"
#include "../animation/Scene.h"
#include "../math/Math.h"
#include "../geometry/Triangle.h"

#include "../external/saschawillems/VulkanInitializers.hpp"
#include "../external/imgui/imgui.h"
#include "../external/imgui/imgui_impl_glfw.h"
#include "../external/imgui/imgui_impl_vulkan.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../external/stb/stb_image.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_access.hpp>

#if defined(_MSC_VER)
#pragma comment(lib,"shaderc_combined.lib")
#endif
#include <shaderc/shaderc.h>

#include <set>
#include <array>
#include <exception>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <fstream>

const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

#define ASSERTVK(func, message) ASSERT((func) == VK_SUCCESS, message);

//////////////////////////
//// render interface ////
//////////////////////////

void Renderer_Vulkan::Init(Window* window, deshiImGui* imgui) {
	PRINT("\n{-} Initializing Vulkan");
	this->window = window->window;
	glfwGetFramebufferSize(window->window, &width, &height);
	glfwSetWindowUserPointer(window->window, this);
	glfwSetFramebufferSizeCallback(window->window, framebufferResizeCallback);
	
	CreateInstance();
	SetupDebugMessenger();
	CreateSurface();
	PickPhysicalDevice();
	CreateLogicalDevice();
	CreateClearValues();
	CreateCommandPool();
	CreateDescriptorPool();
	CreatePipelineCache();
	CreateUniformBuffer();
	
	CreateSwapChain();
	CreateRenderPass();
	CreateFrames(); //image views, color/depth resources, framebuffers, commandbuffers
	CreateLayouts();
	CreatePipelines();
	CreateSyncObjects();
	
	LoadDefaultAssets();
	
	//debug scene
	Scene* test = new Scene;
	Model box = Model::CreatePlanarBox(Vector3(1, 1, 1));
	Texture tex("UV_Grid_Sm.jpg");
	box.mesh.batchArray[0].textureArray.push_back(tex);
	box.mesh.batchArray[0].textureCount = 1;
	
	Model whaleShip; whaleShip.mesh = Mesh::CreateMeshFromOBJ("whale_ship.obj", "ship", Matrix4::TranslationMatrix(5, 0, 0));
	
	test->models = {box, whaleShip};
	LoadScene(test);
	delete test;
	
	imgui->Init(this);
	BuildCommandBuffers();
	initialized = true;
	
	PRINT("{-} Initializing Rendering");
}

void Renderer_Vulkan::Render() {
	//std::cout << "{-}{-} Rendering Frame {-}{-}" << std::endl;
	if(remakeWindow){
		int w, h;
		glfwGetFramebufferSize(window, &w, &h);
		if(w <= 0 || h <= 0){  ImGui::EndFrame(); return;  }
		ResizeWindow(w, h);
		frameIndex = 0;
		remakeWindow = false;
	}
	
	vkWaitForFences(device, 1, &fencesInFlight[frameIndex], VK_TRUE, UINT64_MAX);
	VkSemaphore image_sema  = semaphores[frameIndex].imageAcquired;
	VkSemaphore render_sema = semaphores[frameIndex].renderComplete;
	
	//get next image from surface
	uint32 imageIndex;
	VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, image_sema, VK_NULL_HANDLE, &imageIndex);
	if(result == VK_ERROR_OUT_OF_DATE_KHR){
		remakeWindow = true;
		return;
	}else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR){
		ASSERT(false, "failed to acquire swap chain image");
	}
	
	// Check if a previous frame is using this image (i.e. there is its fence to wait on)
	if(imagesInFlight[imageIndex] != VK_NULL_HANDLE){
		vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}
	// Mark the image as now being in use by this frame
	imagesInFlight[imageIndex] = fencesInFlight[frameIndex];
	vkResetFences(device, 1, &fencesInFlight[frameIndex]);
	
	//render imgui stuff
	ImGui::Render();
	ImDrawData* imDrawData = ImGui::GetDrawData();
	if(imDrawData){
		BuildCommandBuffers();
	}
	
	//update uniform buffers
	UpdateUniformBuffer();
	
	//submit the command buffer to the queue
	VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &image_sema;
	submitInfo.pWaitDstStageMask = &wait_stage;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &frames[imageIndex].commandBuffer;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &render_sema;
	
	ASSERTVK(vkQueueSubmit(graphicsQueue, 1, &submitInfo, fencesInFlight[frameIndex]), "failed to submit draw command buffer");
	
	if(remakeWindow){ return; }
	
	//present the image
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &render_sema;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchain;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;
	result = vkQueuePresentKHR(presentQueue, &presentInfo);
	
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || remakeWindow) {
		int w, h;
		glfwGetFramebufferSize(window, &w, &h);
		remakeWindow = false;
		ResizeWindow(w, h);
	} else if (result != VK_SUCCESS) {
		ASSERT(false, "failed to present swap chain image");
	}
	
	//iterate the frame index
	frameIndex = (frameIndex + 1) % MAX_FRAMES; //loops back to zero after reaching max_frames
	ASSERTVK(vkQueueWaitIdle(graphicsQueue), "graphics queue failed to wait");
	
	if(remakePipelines){ 
		CreatePipelines(); 
		UpdateMaterialPipelines();
		remakePipelines = false; 
	}
	
	//PRINT("--------------" << frameIndex << "---------------");
}

void Renderer_Vulkan::Present() {}

void Renderer_Vulkan::Cleanup() {
	PRINT("{-} Initializing Cleanup\n");
	vkDeviceWaitIdle(device);
	
	//TODO(r,delle) maybe add rendering cleanup, but maybe not
	//because OS will cleanup on program close and be faster at it
	//so maybe only save changes to user settings
	//NOTE but then again, it might allow for dynamically swapping renderers
	
	//TODO(ro,delle) save pipeline cache to disk on exit, and load on start (~20x creation speed)
}

uint32 Renderer_Vulkan::AddTriangle(Triangle* triangle){
	PRINT("Not implemented yet");
	return 0xFFFFFFFF;
}

void Renderer_Vulkan::RemoveTriangle(uint32 triangleID){
	PRINT("Not implemented yet");
}

void Renderer_Vulkan::UpdateTriangleColor(uint32 triangleID, Color color){
	PRINT("Not implemented yet");
}

void Renderer_Vulkan::UpdateTrianglePosition(uint32 triangleID, Vector3 position){
	PRINT("Not implemented yet");
}

void Renderer_Vulkan::TranslateTriangle(uint32 triangleID, Vector3 translation){
	PRINT("Not implemented yet");
}

std::vector<uint32> Renderer_Vulkan::AddTriangles(std::vector<Triangle*> triangles){
	PRINT("Not implemented yet");
	return std::vector<uint32>();
}

void Renderer_Vulkan::RemoveTriangles(std::vector<uint32> triangleIDs){
	PRINT("Not implemented yet");
}

void Renderer_Vulkan::UpdateTrianglesColor(std::vector<uint32> triangleIDs, Color color){
	PRINT("Not implemented yet");
}

void Renderer_Vulkan::TranslateTriangles(std::vector<uint32> triangleIDs, Vector3 translation){
	PRINT("Not implemented");
}

uint32 Renderer_Vulkan::LoadMesh(Mesh* m){
	PRINT("{-}{-}{-} Loading Mesh: " << m->name);
	MeshVk mesh{}; mesh.visible = true;
	mesh.modelMatrix = glm::make_mat4(m->transform.data);
	mesh.primitives.reserve(m->batchCount);
	
	//resize scene vectors
	scene.vertexBuffer.reserve(scene.vertexBuffer.size() + m->vertexCount);
	scene.indexBuffer.reserve(scene.indexBuffer.size() + m->indexCount);
	scene.textures.reserve(scene.textures.size() + m->textureCount);
	scene.materials.reserve(scene.materials.size() + m->batchCount);
	
	uint32 batchVertexStart;
	uint32 batchIndexStart;
	for(Batch& batch : m->batchArray){
		batchVertexStart = uint32(scene.vertexBuffer.size());
		batchIndexStart = uint32(scene.indexBuffer.size());
		
		//vertices
		for(int i=0; i<batch.vertexArray.size(); ++i){ 
			VertexVk vert;
			vert.pos      = glm::make_vec3(&batch.vertexArray[i].pos.x);
			vert.texCoord = glm::make_vec2(&batch.vertexArray[i].uv.x);
			vert.color    = glm::make_vec3(&batch.vertexArray[i].color.x);
			vert.normal   = glm::make_vec3(&batch.vertexArray[i].normal.x);
			scene.vertexBuffer.push_back(vert);
		}
		
		//indices
		for(uint32 i : batch.indexArray){
			scene.indexBuffer.push_back(batchVertexStart+i);
		}
		//scene.indexBuffer.insert(scene.indexBuffer.end(), batch.indexArray.begin(), batch.indexArray.end());
		
		//material
		MaterialVk mat; mat.shader = uint32(batch.shader);
		{
			//material textures
			for(int i=0; i<batch.textureArray.size(); ++i){ 
				uint32 idx = LoadTexture(batch.textureArray[i]);
				switch(scene.textures[idx].type){
					case(TEXTURE_ALBEDO):  { mat.albedoTextureIndex   = idx; }break;
					case(TEXTURE_NORMAL):  { mat.normalTextureIndex   = idx; }break;
					case(TEXTURE_LIGHT):   { mat.lightTextureIndex    = idx; }break;
					case(TEXTURE_SPECULAR):{ mat.specularTextureIndex = idx; }break;
				}
			}
			
			mat.pipeline = GetPipelineFromShader(batch.shader);
			
			//allocate and write descriptor set for material
			VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayouts.textures, 1);
			ASSERTVK(vkAllocateDescriptorSets(device, &allocInfo, &mat.descriptorSet), "failed to allocate materials descriptor sets");
			
			std::vector<VkWriteDescriptorSet> writeDescriptorSet = {
				vks::initializers::writeDescriptorSet(mat.descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &scene.getTextureDescriptorInfo(mat.albedoTextureIndex)),
				vks::initializers::writeDescriptorSet(mat.descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &scene.getTextureDescriptorInfo(mat.normalTextureIndex)),
				vks::initializers::writeDescriptorSet(mat.descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &scene.getTextureDescriptorInfo(mat.specularTextureIndex)),
				vks::initializers::writeDescriptorSet(mat.descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, &scene.getTextureDescriptorInfo(mat.lightTextureIndex)),
			};
			vkUpdateDescriptorSets(device, writeDescriptorSet.size(), writeDescriptorSet.data(), 0, nullptr);
			
			//TODO(r,delle) specialization constants for materials here or in pipeline
			//see gltfscenerendering.cpp:575
			
			mat.id = uint32(scene.materials.size());
			scene.materials.push_back(mat);
		}
		
		//primitive
		PrimitiveVk primitive{};
		primitive.firstIndex = batchIndexStart;
		primitive.indexCount = batch.indexArray.size();
		primitive.materialIndex = mat.id;
		mesh.primitives.push_back(primitive);
	}
	
	//add mesh to scene
	mesh.id = uint32(scene.meshes.size());
	scene.meshes.push_back(mesh);
	if(initialized){ CreateSceneBuffers(); }
	return mesh.id;
}

void Renderer_Vulkan::UnloadMesh(uint32 meshID){
	PRINT("Not implemented yet");
}

void Renderer_Vulkan::ApplyTextureToMesh(uint32 textureID, uint32 meshID){
	PRINT("Not implemented yet");
}

void Renderer_Vulkan::RemoveTextureFromMesh(uint32 textureID, uint32 meshID){
	PRINT("Not implemented yet");
}

void Renderer_Vulkan::UpdateMeshMatrix(uint32 meshID, Matrix4 matrix){
	scene.meshes[meshID].modelMatrix = glm::make_mat4(matrix.data);
}

void Renderer_Vulkan::TransformMeshMatrix(uint32 meshID, Matrix4 transform){
	scene.meshes[meshID].modelMatrix = glm::make_mat4(transform.data) * scene.meshes[meshID].modelMatrix;
}

void Renderer_Vulkan::UpdateMeshBatchShader(uint32 meshID, uint32 batchIndex, uint32 shader){
	uint32 matID = scene.meshes[meshID].primitives[batchIndex].materialIndex;
	scene.materials[matID].pipeline = GetPipelineFromShader(shader);
}

uint32 Renderer_Vulkan::LoadTexture(Texture texture){
	PRINT("{-}{-}{-} Loading Texture: " << texture.filename);
	TextureVk tex; 
	tex.pixels = stbi_load((deshi::getTexturesPath() + texture.filename).c_str(), 
						   &tex.width, &tex.height, &tex.channels, STBI_rgb_alpha);
	ASSERT(tex.pixels, "stb failed to load image");
	
	tex.type = uint32(texture.type);
	tex.mipLevels = uint32(std::floor(std::log2(std::max(tex.width, tex.height)))) + 1;
	tex.imageSize = tex.width * tex.height * 4;
	
	//copy the memory to a staging buffer
	StagingBufferVk staging{};
	CreateAndMapBuffer(staging.buffer, staging.memory, tex.imageSize, static_cast<size_t>(tex.imageSize), tex.pixels, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	
	//copy the staging buffer to the image and generate its mipmaps
	createImage(tex.width, tex.height, tex.mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, tex.image, tex.imageMemory);
	transitionImageLayout(tex.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, tex.mipLevels);
	copyBufferToImage(staging.buffer, tex.image, uint32(tex.width), uint32(tex.height));
	generateMipmaps(tex.image, VK_FORMAT_R8G8B8A8_SRGB, tex.width, tex.height, tex.mipLevels);
	//image layout set to SHADER_READ_ONLY when generating mipmaps
	tex.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	
	//cleanup staging memory
	vkDestroyBuffer(device, staging.buffer, nullptr);
	vkFreeMemory(device, staging.memory, nullptr);
	stbi_image_free(tex.pixels); tex.pixels = nullptr;
	
	//create sampler
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR; //VK_SAMPLER_MIPMAP_MODE_NEAREST for more performance
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = enabledFeatures.samplerAnisotropy;
	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(physicalDevice, &properties);
	samplerInfo.maxAnisotropy = enabledFeatures.samplerAnisotropy ?  properties.limits.maxSamplerAnisotropy : 1.0f;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = static_cast<float>(tex.mipLevels);
	ASSERTVK(vkCreateSampler(device, &samplerInfo, nullptr, &tex.sampler), "failed to create texture sampler");
	
	//create image view
	tex.view = createImageView(tex.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, tex.mipLevels);
	
	//fill descriptor image info
	tex.imageInfo.imageView = tex.view;
	tex.imageInfo.sampler = tex.sampler;
	tex.imageInfo.imageLayout = tex.layout;
	
	//add the texture to the scene and return its index
	uint32 idx = uint32(scene.textures.size());
	tex.id = idx;
	scene.textures.push_back(tex);
	return idx;
}

void Renderer_Vulkan::UnloadTexture(uint32 textureID){
	PRINT("Not implemented yet");
}

void Renderer_Vulkan::LoadDefaultAssets(){
	//load default textures
	scene.textures.reserve(8);
	Texture nullTexture   ("null128.png");     LoadTexture(nullTexture);
	Texture defaultTexture("default1024.png"); LoadTexture(defaultTexture);
	Texture blackTexture  ("black1024.png");   LoadTexture(blackTexture);
	Texture whiteTexture  ("white1024.png");   LoadTexture(whiteTexture);
	
	//TODO(r,delle) add local axis 
	//TODO(r,delle) add global axis 
	//TODO(r,delle) add grid
}

//ref: gltfscenerendering.cpp:350
void Renderer_Vulkan::LoadScene(Scene* sc){
	PRINT("{-}{-} Loading Scene");
	//load meshes, materials, and textures
	for(Model& model : sc->models){ LoadMesh(&model.mesh); }
	
	CreateSceneBuffers();
}

void Renderer_Vulkan::CreateSceneBuffers(){
	PRINT("{-}{-}{-} Creating Scene Buffers");
	StagingBufferVk vertexStaging{}, indexStaging{};
	size_t vertexBufferSize = scene.vertexBuffer.size() * sizeof(VertexVk);
	size_t indexBufferSize  = scene.indexBuffer.size()  * sizeof(uint32);
	
	//create host visible vertex and index buffers (CPU/RAM)
	CreateAndMapBuffer(vertexStaging.buffer, vertexStaging.memory, scene.vertices.bufferSize, vertexBufferSize, scene.vertexBuffer.data(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	
	CreateAndMapBuffer(indexStaging.buffer, indexStaging.memory, scene.indices.bufferSize, indexBufferSize, scene.indexBuffer.data(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	
	//create device local buffers (GPU)
	CreateAndMapBuffer(scene.vertices.buffer, scene.vertices.bufferMemory, scene.vertices.bufferSize, vertexBufferSize, nullptr, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	
	CreateAndMapBuffer(scene.indices.buffer, scene.indices.bufferMemory, scene.indices.bufferSize, indexBufferSize, nullptr, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	
	//copy data from staging buffers to device local buffers
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();{
		VkBufferCopy copyRegion{};
		
		copyRegion.size = vertexBufferSize;
		vkCmdCopyBuffer(commandBuffer, vertexStaging.buffer, scene.vertices.buffer, 1, &copyRegion);
		
		copyRegion.size = indexBufferSize;
		vkCmdCopyBuffer(commandBuffer, indexStaging.buffer, scene.indices.buffer, 1, &copyRegion);
		
	}endSingleTimeCommands(commandBuffer);
	
	//free staging resources
	vkDestroyBuffer(device, vertexStaging.buffer, nullptr);
	vkFreeMemory(device, vertexStaging.memory, nullptr);
	vkDestroyBuffer(device, indexStaging.buffer, nullptr);
	vkFreeMemory(device, indexStaging.memory, nullptr);
}

void Renderer_Vulkan::UpdateCameraPosition(Vector3 position){
	camera.position = glm::make_vec3(&position.x);
}

void Renderer_Vulkan::UpdateCameraRotation(Vector3 rotation){
	camera.rotation = glm::make_vec3(&rotation.x);
}

void Renderer_Vulkan::UpdateCameraViewMatrix(Matrix4 m){
	if(camera.precalcMatrices){
		shaderData.values.view = glm::make_mat4(m.data);
	}else{
		glm::mat4 rotM = glm::mat4(1.f);
		rotM = glm::rotate(rotM, glm::radians(camera.rotation.y), glm::vec3(0.f, 1.f, 0.f));
		rotM = glm::rotate(rotM, glm::radians(camera.rotation.x), glm::vec3(1.f, 0.f, 0.f));
		glm::vec4 target(0.f, 0.f, 1.f, 0.f);
		target = rotM * target; target = glm::normalize(target);
		
		shaderData.values.view = glm::lookAt(camera.position, camera.position + glm::vec3(target), glm::vec3(0.f, 1.f, 0.f));
	}
}

void Renderer_Vulkan::UpdateCameraProjectionMatrix(Matrix4 m){
	if(camera.precalcMatrices){
		shaderData.values.proj = glm::make_mat4(m.data);
	}else{
		float aspectRatio = extent.width / (float) extent.height;
		shaderData.values.proj = glm::perspective(glm::radians(camera.fovX / aspectRatio), aspectRatio, camera.nearZ, camera.farZ);
		shaderData.values.proj[1][1] *= -1;
	}
}

void Renderer_Vulkan::UpdateCameraProjectionProperties(float fovX, float nearZ, float farZ, bool precalc){
	camera.fovX = fovX;
	camera.nearZ = nearZ;
	camera.farZ = farZ;
	camera.precalcMatrices = precalc;
}

void Renderer_Vulkan::ReloadShaders() {
	remakePipelines = true;
}

//////////////////////////////////
//// initialization functions ////
//////////////////////////////////

void Renderer_Vulkan::CreateInstance() {
	PRINT("{-}{-} Creating Vulkan Instance");
	if(enableValidationLayers && !checkValidationLayerSupport()) {
		ASSERT(false, "validation layers requested, but not available");
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
	
	ASSERTVK(vkCreateInstance(&createInfo, allocator, &instance), "failed to create instance");
}

void Renderer_Vulkan::SetupDebugMessenger() {
	PRINT("{-}{-} Setting Up Debug Messenger");
	if(!enableValidationLayers) return;
	
	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	populateDebugMessengerCreateInfo(createInfo);
	
	ASSERTVK(CreateDebugUtilsMessengerEXT(instance, &createInfo, allocator, &debugMessenger), "failed to set up debug messenger");
}

void Renderer_Vulkan::CreateSurface() {
	PRINT("{-}{-} Creating GLFW Surface");
	ASSERTVK(glfwCreateWindowSurface(instance, window, allocator, &surface), "failed to create window surface");
}

void Renderer_Vulkan::PickPhysicalDevice() {
	PRINT("{-}{-} Picking Physical Device");
	uint32 deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
	
	for(auto& device : devices) {
		if(isDeviceSuitable(device)) {
			physicalDevice = device;
			break;
		}
	}
	
	if(physicalDevice == VK_NULL_HANDLE) {
		ASSERT(false, "failed to find a suitable GPU");
	}
	
	//get physical device capabilities
	msaaSamples = getMaxUsableSampleCount();
	vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
	physicalQueueFamilies = findQueueFamilies(physicalDevice);
}

void Renderer_Vulkan::CreateLogicalDevice() {
	PRINT("{-}{-} Creating Logical Device");
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
	
	//TODO(r,delle) add rendering options here
	//enable poosible features
	if(deviceFeatures.samplerAnisotropy){
		enabledFeatures.samplerAnisotropy = VK_TRUE; //enable anistropic filtering
		enabledFeatures.sampleRateShading = VK_TRUE; //enable sample shading feature for the device, VK_FALSE to disable sample shading
	}
	if(deviceFeatures.fillModeNonSolid){
		enabledFeatures.fillModeNonSolid = VK_TRUE; //wireframe
		if(deviceFeatures.wideLines){
			//enabledFeatures.wideLines = VK_TRUE; //wide lines (anime/toon style)
		}
	}
	
	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = static_cast<uint32>(queueCreateInfos.size());
	createInfo.pEnabledFeatures = &enabledFeatures;
	createInfo.enabledExtensionCount = static_cast<uint32>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();
	if(enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}else{
		createInfo.enabledLayerCount = 0;
	}
	
	ASSERTVK(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device), "failed to create logical device");
	
	vkGetDeviceQueue(device, physicalQueueFamilies.graphicsFamily.value(), 0, &graphicsQueue);
	vkGetDeviceQueue(device, physicalQueueFamilies.presentFamily.value(), 0, &presentQueue);
}

//TODO(r,delle) find a better/more accurate way to do this, see gltfloading.cpp, line:592
void Renderer_Vulkan::CreateDescriptorPool(){
	PRINT("{-}{-} Creating Descriptor Pool");
	const int types = 11;
	VkDescriptorPoolSize poolSizes[types] = {
		{ VK_DESCRIPTOR_TYPE_SAMPLER,                1000 },
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
	
	ASSERTVK(vkCreateDescriptorPool(device, &poolInfo, allocator, &descriptorPool), "failed to create descriptor pool");
}

void Renderer_Vulkan::CreatePipelineCache(){
	PRINT("{-}{-} Creating Pipeline Cache");
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	ASSERTVK(vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &pipelineCache), "failed to create pipeline cache");
}

void Renderer_Vulkan::CreateUniformBuffer(){
	PRINT("{-}{-} Creating Uniform Buffer");
	CreateOrResizeBuffer(shaderData.uniformBuffer, shaderData.uniformBufferMemory, shaderData.uniformBufferSize, sizeof(shaderData.values) , VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	UpdateUniformBuffer();
}

void Renderer_Vulkan::CreateCommandPool(){
	PRINT("{-}{-} Creating Command Pool");
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = physicalQueueFamilies.graphicsFamily.value();
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	
	ASSERTVK(vkCreateCommandPool(device, &poolInfo, allocator, &commandPool), "failed to create command pool");
}

void Renderer_Vulkan::CreateClearValues(){
	clearValues[0].color = {0, 0, 0, 1};
	clearValues[1].depthStencil = {1.f, 0};
}

void Renderer_Vulkan::CreateSyncObjects(){
	PRINT("{-}{-} Creating Sync Objects");
	semaphores.resize(MAX_FRAMES);
	fencesInFlight.resize(MAX_FRAMES);
	imagesInFlight.resize(imageCount);
	
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	
	for(int i = 0; i < MAX_FRAMES; ++i){
		if(vkCreateSemaphore(device, &semaphoreInfo, allocator, &semaphores[i].imageAcquired) ||
		   vkCreateSemaphore(device, &semaphoreInfo, allocator, &semaphores[i].renderComplete) || vkCreateFence(device, &fenceInfo, nullptr, &fencesInFlight[i])){
			ASSERT(false, "failed to create sync objects");
		}
	}
}


void Renderer_Vulkan::CreateLayouts(){
	PRINT("{-}{-} Creating Layouts");
	//create set layout for passing matrices
	std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
		vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0)
	};
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings.data(), (uint32)setLayoutBindings.size());
	ASSERTVK(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, nullptr, &descriptorSetLayouts.matrices), "failed to create ubo descriptor set layout");
	
	//create set layout for passing textures (4 types of textures)
	setLayoutBindings = {
		// Color/albedo map
		vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
		// Normal map
		vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
		// Specular/reflective map
		vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
		// Light/emissive map
		vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3),
	};
	descriptorSetLayoutCI = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings.data(), (uint32)setLayoutBindings.size());
	ASSERTVK(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, nullptr, &descriptorSetLayouts.textures), "failed to create textures descriptor set layout");
	
	//create pipeline layout wth push constant to push model matrix with every mesh
	std::array<VkDescriptorSetLayout, 2> setLayouts = { descriptorSetLayouts.matrices, descriptorSetLayouts.textures };
	VkPushConstantRange pushConstantRange = vks::initializers::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0);
	
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = setLayouts.size();
	pipelineLayoutInfo.pSetLayouts = setLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
	
	ASSERTVK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout), "failed to create pipeline layout");
	
	//allocate and write descriptor set for matrices/uniform buffers
	{
		VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayouts.matrices, 1);
		ASSERTVK(vkAllocateDescriptorSets(device, &allocInfo, &scene.descriptorSet), "failed to allocate matrices descriptor sets");
		
		VkDescriptorBufferInfo descBufferInfo{};
		descBufferInfo.buffer = shaderData.uniformBuffer;
		descBufferInfo.offset = 0;
		descBufferInfo.range  = sizeof(shaderData.values);
		VkWriteDescriptorSet writeDescriptorSet = vks::initializers::writeDescriptorSet(scene.descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &descBufferInfo);
		vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
	}
	
	
}

//////////////////////////////////
//// window resized functions ////
//////////////////////////////////

void Renderer_Vulkan::ResizeWindow(int w, int h) {
	PRINT("{-} Creating Window");
	// Ensure all operations on the device have been finished before destroying resources
	vkDeviceWaitIdle(device);
	
	width = w; height = h;
	CreateSwapChain();
	CreateRenderPass();
	CreateFrames(); //image views, color/depth resources, framebuffers, commandbuffers
	CreatePipelines();
	UpdateMaterialPipelines();
}

void Renderer_Vulkan::CreateSwapChain() {
	PRINT("{-}{-} Creating Swapchain");
	VkSwapchainKHR oldSwapChain = swapchain;
	swapchain = NULL;
	vkDeviceWaitIdle(device);
	
	//query GPUs supported features for the swap chain
	supportDetails = querySwapChainSupport(physicalDevice);
	surfaceFormat = chooseSwapSurfaceFormat(supportDetails.formats);
	presentMode = chooseSwapPresentMode(supportDetails.presentModes);
	extent = chooseSwapExtent(supportDetails.capabilities);
	
	//get min image count if not specified
	if(minImageCount == 0) { minImageCount = GetMinImageCountFromPresentMode(presentMode); }
	
	uint32 queueFamilyIndices[] = {
		physicalQueueFamilies.graphicsFamily.value(), physicalQueueFamilies.presentFamily.value()
	};
	
	//create swapchain and swap chain images, set width and height
	VkSwapchainCreateInfoKHR info{};
	info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	info.surface = surface;
	info.imageFormat = surfaceFormat.format;
	info.imageColorSpace = surfaceFormat.colorSpace;
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
	info.preTransform = supportDetails.capabilities.currentTransform;
	info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	info.presentMode = presentMode;
	info.clipped = VK_TRUE;
	info.oldSwapchain = oldSwapChain;
	info.minImageCount = minImageCount;
	if(supportDetails.capabilities.maxImageCount != 0 && info.minImageCount > supportDetails.capabilities.maxImageCount) {
		info.minImageCount = supportDetails.capabilities.maxImageCount;
	}
	if(extent.width == 0xffffffff) {
		info.imageExtent.width = width;
		info.imageExtent.height = height;
	} else {
		info.imageExtent.width = width = extent.width;
		info.imageExtent.height = height = extent.height;
	}
	
	ASSERTVK(vkCreateSwapchainKHR(device, &info, allocator, &swapchain), "failed to create swap chain");
	
	//delete old swap chain
	if(oldSwapChain != VK_NULL_HANDLE) { vkDestroySwapchainKHR(device, oldSwapChain, allocator); }
}

void Renderer_Vulkan::CreateRenderPass(){
	PRINT("{-}{-} Creating Render Pass");
	if(renderPass) { vkDestroyRenderPass(device, renderPass, allocator); }
	
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = surfaceFormat.format;
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
	colorAttachmentResolve.format = surfaceFormat.format;
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
	
	ASSERTVK(vkCreateRenderPass(device, &renderPassInfo, allocator, &renderPass), "failed to create render pass");
}

void Renderer_Vulkan::CreateFrames(){
	PRINT("{-}{-} Creating Frames");
	//get swap chain images
	vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr); //gets the image count
	VkImage images[16] = {};
	ASSERT(imageCount >= minImageCount, "the window should always have at least the min image count");
	ASSERT(imageCount < 16, "the window should have less than 16 images, around 2-3 is ideal");
	vkGetSwapchainImagesKHR(device, swapchain, &imageCount, images); //assigns to images
	
	//color framebuffer attachment
	if(attachments.colorImage){
		vkDestroyImageView(device, attachments.colorImageView, nullptr);
		vkDestroyImage(device, attachments.colorImage, nullptr);
		vkFreeMemory(device, attachments.colorImageMemory, nullptr);
	}
	VkFormat colorFormat = surfaceFormat.format;
	createImage(width, height, 1, msaaSamples, colorFormat, VK_IMAGE_TILING_OPTIMAL, 
				VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, attachments.colorImage, attachments.colorImageMemory);
	attachments.colorImageView = createImageView(attachments.colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
	
	//depth framebuffer attachments
	if(attachments.depthImage){
		vkDestroyImageView(device, attachments.depthImageView, nullptr);
		vkDestroyImage(device, attachments.depthImage, nullptr);
		vkFreeMemory(device, attachments.depthImageMemory, nullptr);
	}
	VkFormat depthFormat = findDepthFormat();
	createImage(width, height, 1, msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL, 
				VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, attachments.depthImage, attachments.depthImageMemory);
	attachments.depthImageView = createImageView(attachments.depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
	
	
	frames.resize(imageCount);
	for (uint32 i = 0; i < imageCount; ++i) {
		//set the frame images to the swap chain images
		//NOTE the previous image and its memory gets freed when the swapchain gets destroyed
		frames[i].image = images[i];
		
		//create the image views
		if(frames[i].imageView){ vkDestroyImageView(device, frames[i].imageView, nullptr); };
		frames[i].imageView = createImageView(frames[i].image, surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
		
		//create the framebuffers
		if(frames[i].framebuffer){ vkDestroyFramebuffer(device, frames[i].framebuffer, nullptr); }
		std::array<VkImageView, 3> frameBufferAttachments = { attachments.colorImageView, attachments.depthImageView, frames[i].imageView };
		VkFramebufferCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		info.renderPass = renderPass;
		info.attachmentCount = (uint32)frameBufferAttachments.size();
		info.pAttachments = frameBufferAttachments.data();
		info.width = width;
		info.height = height;
		info.layers = 1;
		ASSERTVK(vkCreateFramebuffer(device, &info, allocator, &frames[i].framebuffer), "failed to create framebuffer");
		
		//allocate command buffers
		if(frames[i].commandBuffer) { vkFreeCommandBuffers(device, commandPool, 1, &frames[i].commandBuffer); }
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;
		ASSERTVK(vkAllocateCommandBuffers(device, &allocInfo, &frames[i].commandBuffer), "failed to allocate command buffer");
	}
}

void Renderer_Vulkan::CreatePipelines(){
	PRINT("{-}{-} Creating Pipelines");
	
	//destroy previous pipelines
	if(pipelines.DEFAULT){ vkDestroyPipeline(device, pipelines.DEFAULT, nullptr); }
	if(pipelines.TWOD){ vkDestroyPipeline(device, pipelines.TWOD, nullptr); }
	if(pipelines.PBR){ vkDestroyPipeline(device, pipelines.PBR, nullptr); }
	if(pipelines.WIREFRAME){ vkDestroyPipeline(device, pipelines.WIREFRAME, nullptr); }
	
	//destroy previous shader modules
	size_t oldCount = shaderModules.size();
	for(auto& pair : shaderModules){
		vkDestroyShaderModule(device, pair.second, allocator);
	}
	shaderModules.clear(); shaderModules.reserve(oldCount);
	
	//determines how to group vertices together
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
	//how to draw/cull/depth things
	VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE, 0);
	//VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
	
	//how to combine colors
	VkPipelineColorBlendAttachmentState colorBlendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);
	//options to allow for blending based on alpha
	//colorBlendAttachmentState.blendEnable = VK_TRUE; 
	//colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	//colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	
	VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &colorBlendAttachmentState);;
	
	//depth testing and discarding
	VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
	
	//how many viewports there are
	VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
	
	//useful for multisample anti-aliasing (MSAA)
	VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
	multisampleState.rasterizationSamples = msaaSamples; //use VK_SAMPLE_COUNT_1_BIT to disable anti-aliasing
	multisampleState.sampleShadingEnable = VK_TRUE; //enable sample shading in the pipeline, VK_FALSE to enable
	multisampleState.minSampleShading = .2f; //min fraction for sample shading; closer to one is smoother
	
	std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR/*, VK_DYNAMIC_STATE_LINE_WIDTH*/ };
	VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables.data(), (uint32)dynamicStateEnables.size(), 0);
	
	//vertex input flow control
	std::vector<VkVertexInputBindingDescription> vertexInputBindings = VertexVk::getBindingDescriptions();
	std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = VertexVk::getAttributeDescriptions();
	VkPipelineVertexInputStateCreateInfo vertexInputState = vks::initializers::pipelineVertexInputStateCreateInfo(vertexInputBindings, vertexInputAttributes);
	
	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;
	
	//base pipeline info and options
	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.layout              = pipelineLayout;
	pipelineInfo.renderPass          = renderPass;
	pipelineInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineInfo.pRasterizationState = &rasterizationState;
	pipelineInfo.pColorBlendState    = &colorBlendState;
	pipelineInfo.pMultisampleState   = &multisampleState;
	pipelineInfo.pViewportState      = &viewportState;
	pipelineInfo.pDepthStencilState  = &depthStencilState;
	pipelineInfo.pDynamicState       = &dynamicState;
	pipelineInfo.pVertexInputState   = &vertexInputState;
	pipelineInfo.stageCount          = (uint32)shaderStages.size();
	pipelineInfo.pStages             = shaderStages.data();
	pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex   = -1;
	
	//flag that this pipelineInfo will be used as a base
	pipelineInfo.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
	
	//compile the shaders (boolean param is optimization)
	CompileShaders(false);
	
	//textured/default pipeline //TODO(r,delle) change this to phong and create a pbf shader for 4 textures
	shaderStages[0] = loadShader("default.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = loadShader("default.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	ASSERTVK(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineInfo, nullptr, &pipelines.DEFAULT), "failed to create default graphics pipeline");
	
	//all other pipelines are derivatives
	pipelineInfo.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;
	pipelineInfo.basePipelineHandle = pipelines.DEFAULT;
	pipelineInfo.basePipelineIndex = -1; //can either use handle or index, not both (section 9.5 of vulkan spec)
	
	//wireframe pipeline
	if(deviceFeatures.fillModeNonSolid){
		rasterizationState.polygonMode = VK_POLYGON_MODE_LINE;
		
		shaderStages[0] = loadShader("wireframe.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader("wireframe.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		ASSERTVK(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineInfo, nullptr, &pipelines.WIREFRAME), "failed to create wireframe graphics pipeline");
	}
	
	//TODO(r,delle) add more pipelines/shader
}

void Renderer_Vulkan::BuildCommandBuffers() {
	//PRINT("{-}{-} Building Command Buffers");
	VkCommandBufferBeginInfo cmdBufferInfo{};
	cmdBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufferInfo.flags = 0;
	cmdBufferInfo.pInheritanceInfo = nullptr;
	
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = extent;
	renderPassInfo.clearValueCount = (uint32)clearValues.size();
	renderPassInfo.pClearValues = clearValues.data();
	
	const VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
	const VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
	
	for(int i = 0; i < imageCount; ++i){
		renderPassInfo.framebuffer = frames[i].framebuffer;
		ASSERTVK(vkBeginCommandBuffer(frames[i].commandBuffer, &cmdBufferInfo), "failed to begin recording command buffer");
		vkCmdBeginRenderPass(frames[i].commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdSetViewport(frames[i].commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(frames[i].commandBuffer, 0, 1, &scissor);
		// Bind scene matrices descriptor to set 0
		vkCmdBindDescriptorSets(frames[i].commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &scene.descriptorSet, 0, nullptr);
		////draw stuff below here////
		
		scene.Draw(frames[i].commandBuffer, pipelineLayout);
		
		//draw imgui stuff
		ImDrawData* imDrawData = ImGui::GetDrawData();
		if(imDrawData){
			ImGui_ImplVulkan_RenderDrawData(imDrawData, frames[i].commandBuffer);
		}
		
		////draw stuff above here////
		vkCmdEndRenderPass(frames[i].commandBuffer);
		ASSERTVK(vkEndCommandBuffer(frames[i].commandBuffer), "failed to end recording command buffer");
	}
}


//TODO(ro,delle) maybe only do one mapping at buffer creation, see: gltfscenerendering.cpp, line:600
void Renderer_Vulkan::UpdateUniformBuffer(){
	//PRINT("{-}{-} Updating Uniform Buffer {-}{-}\n");
	shaderData.values.time = time->totalTime;
	shaderData.values.swidth = (glm::f32)extent.width;
	shaderData.values.sheight = (glm::f32)extent.height;
	shaderData.values.viewPos = glm::vec4(camera.position, 0.f) * glm::vec4(-1.f, 0.f, -1.f, 0.f);
	shaderData.values.lightPos = glm::vec4(0.0f, 2.5f, 0.0f, 1.0f);
	
	//map shader data to uniform buffer
	void* data;
	vkMapMemory(device, shaderData.uniformBufferMemory, 0, sizeof(shaderData.values), 0, &data);{
		memcpy(data, &shaderData.values, sizeof(shaderData.values));
	}vkUnmapMemory(device, shaderData.uniformBufferMemory);
}

/////////////////////////////
////// utility functions ////
/////////////////////////////

bool Renderer_Vulkan::checkValidationLayerSupport() {
	PRINT("{-}{-}{-} Checking Validation Layer Support");
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
	PRINT("{-}{-}{-} Getting Required Extensions");
	uint32 glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
	if(enableValidationLayers) { extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); }
	return extensions;
}

void Renderer_Vulkan::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
	PRINT("{-}{-}{-} Populating Debug Messenger CreateInfo");
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = /*VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |*/ VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
}

bool Renderer_Vulkan::isDeviceSuitable(VkPhysicalDevice device) {
	PRINT("{-}{-}{-} Checking Validation Layer Support");
	QueueFamilyIndices indices = findQueueFamilies(device);
	bool extensionsSupported = checkDeviceExtensionSupport(device);
	bool swapChainAdequate = false;
	if(extensionsSupported) {
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}
	
	return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

QueueFamilyIndices Renderer_Vulkan::findQueueFamilies(VkPhysicalDevice device) {
	PRINT("{-}{-}{-} Finding Queue Families");
	QueueFamilyIndices indices;
	
	uint32 queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
	
	int i = 0;
	for(auto& queueFamily : queueFamilies) {
		if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) { indices.graphicsFamily = i; }
		
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
		if (presentSupport) { indices.presentFamily = i; }
		
		if(indices.isComplete()) { break; }
		i++;
	}
	
	return indices;
}

bool Renderer_Vulkan::checkDeviceExtensionSupport(VkPhysicalDevice device) {
	PRINT("{-}{-}{-} Checking Device Extension Support");
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
	PRINT("{-}{-}{-} Querying SwapChain Support");
	SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
	
	uint32 formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
	}
	
	uint32 presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
	}
	
	return details;
}

VkSampleCountFlagBits Renderer_Vulkan::getMaxUsableSampleCount() {
	PRINT("{-}{-}{-} Getting Max Usable Sample Count");
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
	PRINT("{-}{-}{-} Choosing Swap Surface Format");
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}
	return availableFormats[0];
}

VkPresentModeKHR Renderer_Vulkan::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
	PRINT("{-}{-}{-} Choosing Swap Present Mode");
	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentMode;
		}
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Renderer_Vulkan::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
	PRINT("{-}{-}{-} Choosing Swap Extent");
	if (capabilities.currentExtent.width != UINT32_MAX) {
		return capabilities.currentExtent;
	} else {
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		
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
	ASSERTVK(vkCreateImageView(device, &viewInfo, allocator, &imageView), "failed to create texture image view");
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
	
	ASSERT(false, "failed to find supported format");
}

VkFormat Renderer_Vulkan::findDepthFormat() {
	return findSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkPipelineShaderStageCreateInfo Renderer_Vulkan::loadShader(std::string fileName, VkShaderStageFlagBits stage) {
	PRINT("{-}{-}{-} Loading Shader " << fileName);
	//setup shader stage create info
	VkPipelineShaderStageCreateInfo shaderStage{};
	shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStage.stage = stage;
	shaderStage.pName = "main";
	
	//get shader name
	char shaderName[16];
	strncpy_s(shaderName, fileName.c_str(), 15);
	shaderName[15] = '\0'; //null-character
	
	//check if shader has already been created
	for(auto& module : shaderModules){
		if(strcmp(shaderName, module.first) == 0){
			shaderStage.module = module.second;
			break;
		}
	}
	
	//create shader module
	std::vector<char> code = deshi::readFile(deshi::getShadersPath() + fileName);
	VkShaderModuleCreateInfo moduleInfo{};
	moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	moduleInfo.codeSize = code.size();
	moduleInfo.pCode = (uint32*)code.data();
	
	VkShaderModule shaderModule;
	ASSERTVK(vkCreateShaderModule(device, &moduleInfo, allocator, &shaderModule), "failed to create shader module");
	shaderStage.module = shaderModule;
	
	shaderModules.push_back(std::make_pair(shaderName, shaderStage.module));
	return shaderStage;
}

uint32 Renderer_Vulkan::findMemoryType(uint32 typeFilter, VkMemoryPropertyFlags properties) {
	//PRINT("{-}{-}{-} Finding Memory Types {-}{-}{-}");
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
	
	for (uint32 i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}
	ASSERT(false, "failed to find suitable memory type"); //error out if no suitable memory found
}

void Renderer_Vulkan::createImage(uint32 width, uint32 height, uint32 mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
	PRINT("{-}{-}{-}{-} Creating Image");
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
	ASSERTVK(vkCreateImage(device, &imageInfo, allocator, &image), "failed to create image");
	
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, image, &memRequirements);
	
	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
	ASSERTVK(vkAllocateMemory(device, &allocInfo, allocator, &imageMemory), "failed to allocate image memory");
	
	vkBindImageMemory(device, image, imageMemory, 0);
}

void Renderer_Vulkan::CreateOrResizeBuffer(VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkDeviceSize& bufferSize, size_t newSize, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties){
	PRINT("{-}{-}{-}{-} Creating or Resizing Buffer");
	//delete old buffer
	if(buffer != VK_NULL_HANDLE){ vkDestroyBuffer(device, buffer, allocator); }
	if (bufferMemory != VK_NULL_HANDLE){ vkFreeMemory(device, bufferMemory, allocator); }
	
	VkDeviceSize alignedBufferSize = ((newSize-1) / bufferMemoryAlignment + 1) * bufferMemoryAlignment;
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = alignedBufferSize;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	ASSERTVK(vkCreateBuffer(device, &bufferInfo, allocator, &buffer), "failed to create buffer");
	
	VkMemoryRequirements req;
	vkGetBufferMemoryRequirements(device, buffer, &req);
	bufferMemoryAlignment = (bufferMemoryAlignment > req.alignment) ? bufferMemoryAlignment : req.alignment;
	
	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = req.size;
	allocInfo.memoryTypeIndex = findMemoryType(req.memoryTypeBits, properties);
	
	ASSERTVK(vkAllocateMemory(device, &allocInfo, allocator, &bufferMemory), "failed to allocate buffer memory");
	vkBindBufferMemory(device, buffer, bufferMemory, 0);
	bufferSize = newSize;
}

void Renderer_Vulkan::CreateAndMapBuffer(VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkDeviceSize& bufferSize, size_t newSize, void* data, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties){
	PRINT("{-}{-}{-}{-} Creating and Mapping Buffer");
	//delete old buffer
	if(buffer != VK_NULL_HANDLE){ vkDestroyBuffer(device, buffer, allocator); }
	if (bufferMemory != VK_NULL_HANDLE){ vkFreeMemory(device, bufferMemory, allocator); }
	
	//create buffer
	VkDeviceSize alignedBufferSize = ((newSize-1) / bufferMemoryAlignment + 1) * bufferMemoryAlignment;
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = alignedBufferSize;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	ASSERTVK(vkCreateBuffer(device, &bufferInfo, allocator, &buffer), "failed to create buffer");
	
	VkMemoryRequirements req;
	vkGetBufferMemoryRequirements(device, buffer, &req);
	bufferMemoryAlignment = (bufferMemoryAlignment > req.alignment) ? bufferMemoryAlignment : req.alignment;
	
	//allocate buffer
	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = req.size;
	allocInfo.memoryTypeIndex = findMemoryType(req.memoryTypeBits, properties);
	
	ASSERTVK(vkAllocateMemory(device, &allocInfo, allocator, &bufferMemory), "failed to allocate buffer memory");
	
	//if data pointer, map buffer and copy data
	if(data != nullptr){
		void* mapped;
		ASSERTVK(vkMapMemory(device, bufferMemory, 0, newSize, 0, &mapped), "couldnt map memory");{
			memcpy(mapped, data, newSize);
			// If host coherency hasn't been requested, do a manual flush to make writes visible
			if ((properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0){
				VkMappedMemoryRange mappedRange = vks::initializers::mappedMemoryRange();
				mappedRange.memory = bufferMemory;
				mappedRange.offset = 0;
				mappedRange.size = newSize;
				vkFlushMappedMemoryRanges(device, 1, &mappedRange);
			}
		}vkUnmapMemory(device, bufferMemory);
	}
	
	vkBindBufferMemory(device, buffer, bufferMemory, 0);
	bufferSize = newSize;
}

VkCommandBuffer Renderer_Vulkan::beginSingleTimeCommands() {
	VkCommandBuffer commandBuffer;
	
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
	
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(commandBuffer, &beginInfo);
	
	return commandBuffer;
}

void Renderer_Vulkan::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
	vkEndCommandBuffer(commandBuffer);
	
	//TODO(r,delle) maybe add a fence to ensure the buffer has finished executing
	//instead of waiting for queue to be idle, see: sascha/VulkanDevice.cpp:508
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	
	vkQueueWaitIdle(graphicsQueue);
	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void Renderer_Vulkan::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32 mipLevels) {
	PRINT("{-}{-}{-}{-} Transitioning Image Layout");
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
		ASSERT(false, "unsupported layout transition");
	}
	
	vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	
	endSingleTimeCommands(commandBuffer);
}

void Renderer_Vulkan::copyBufferToImage(VkBuffer buffer, VkImage image, uint32 width, uint32 height) {
	PRINT("{-}{-}{-}{-} Copying Buffer To Image");
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();
	
	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageOffset = {0, 0, 0};
	region.imageExtent = {width, height, 1};
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	
	endSingleTimeCommands(commandBuffer);
}

void Renderer_Vulkan::generateMipmaps(VkImage image, VkFormat imageFormat, int32 texWidth, int32 texHeight, uint32 mipLevels) {
	PRINT("{-}{-}{-}{-} Creating Image Mipmaps");
	// Check if image format supports linear blitting
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);
	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
		ASSERT(false, "texture image format does not support linear blitting");
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

//TODO(ro,delle) maybe optimize this by simply doing: &pipelines + shader*sizeof(pipelines.DEFAULT)
VkPipeline Renderer_Vulkan::GetPipelineFromShader(uint32 shader){
	switch(shader){
		case(Shader::DEFAULT):default:{ return pipelines.DEFAULT;   };
		case(Shader::TWOD):           { return pipelines.TWOD;      };
		case(Shader::PBR):            { return pipelines.PBR;       };
		case(Shader::WIREFRAME):      { return pipelines.WIREFRAME; };
	}
}

void Renderer_Vulkan::CompileShaders(bool optimize){
	PRINT("{-}{-}{-} Compiling Shaders");
	//setup shader compiler
	shaderc_compiler_t compiler = shaderc_compiler_initialize();
	shaderc_compile_options_t options = shaderc_compile_options_initialize();
	if(optimize) shaderc_compile_options_set_optimization_level(options, shaderc_optimization_level_performance);
	
	//loop thru all files in the shaders dir, compile the shaders, write them to .spv files
	for(auto& entry : std::filesystem::directory_iterator(deshi::getShadersPath())){
		std::string ext = entry.path().extension().string();
		std::string filename = entry.path().filename().string();
		
		if(ext.compare(".spv") == 0) continue; //early out
		
		//read in ascii shader code
		std::vector<char> code = deshi::readFile(entry.path().string());
		
		//try compile from GLSL to SPIR-V binary
		shaderc_compilation_result_t result;
		if(ext.compare(".vert") == 0){
			result = shaderc_compile_into_spv(compiler, code.data(), code.size(), shaderc_glsl_vertex_shader, 
											  filename.c_str(), "main", options);
		}else if(ext.compare(".frag") == 0){
			result = shaderc_compile_into_spv(compiler, code.data(), code.size(), shaderc_glsl_fragment_shader, 
											  filename.c_str(), "main", options);
		}else{ continue; }
		
		if(!result){ PRINT("[ERROR]"<< filename <<": Shader compiler returned a null result"); continue; }
		if(shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success){
			PRINT("[ERROR]"<< filename <<": "<< shaderc_result_get_error_message(result)); continue;
		}
		
		//create or overwrite .spv files
		std::ofstream outFile(entry.path().string() + ".spv", std::ios::out | std::ios::binary | std::ios::trunc);
		ASSERT(outFile.is_open(), "failed to open file");
		outFile.write(shaderc_result_get_bytes(result), shaderc_result_get_length(result));
		
		//cleanup file and compiler results
		outFile.close();
		shaderc_result_release(result);
	}
	
	//cleanup shader compiler and options
	shaderc_compile_options_release(options);
	shaderc_compiler_release(compiler);
}

void Renderer_Vulkan::UpdateMaterialPipelines(){
	for(auto& mat : scene.materials){
		mat.pipeline = GetPipelineFromShader(mat.shader);
	}
}

VKAPI_ATTR VkBool32 VKAPI_CALL Renderer_Vulkan::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
	PRINT("/\\  " << pCallbackData->pMessage);
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

void Renderer_Vulkan::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
	auto app = reinterpret_cast<Renderer_Vulkan*>(glfwGetWindowUserPointer(window));
	app->remakeWindow = true;
}

/////////////////////////////////////////
//// vulkan support struct functions ////
/////////////////////////////////////////

std::vector<VkVertexInputBindingDescription> VertexVk::getBindingDescriptions() {
	std::vector<VkVertexInputBindingDescription> bindingDescriptions = {
		vks::initializers::vertexInputBindingDescription(0, sizeof(VertexVk), VK_VERTEX_INPUT_RATE_VERTEX)
	};
	return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription> VertexVk::getAttributeDescriptions() {
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions = {
		vks::initializers::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexVk, pos)),
		vks::initializers::vertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexVk, texCoord)),
		vks::initializers::vertexInputAttributeDescription(0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexVk, color)),
		vks::initializers::vertexInputAttributeDescription(0, 3, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexVk, normal))
	};
	return attributeDescriptions;
}

inline VkDescriptorImageInfo SceneVk::getTextureDescriptorInfo(size_t index){
	return textures[index].imageInfo;
}

void SceneVk::Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout){
	// All vertices and indices are stored in single buffers, so we only need to bind once
	VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices.buffer, offsets);
	vkCmdBindIndexBuffer(commandBuffer, indices.buffer, 0, VK_INDEX_TYPE_UINT32);
	
	for(MeshVk& mesh : meshes){
		if(mesh.visible && mesh.primitives.size() > 0){
			//push the mesh's model matrix to the vertex shader
			vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &mesh.modelMatrix);
			
			for (PrimitiveVk& primitive : mesh.primitives) {
				if (primitive.indexCount > 0) {
					MaterialVk& material = materials[primitive.materialIndex];
					// Bind the pipeline for the primitive's material
					vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material.pipeline);
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &material.descriptorSet, 0, nullptr);
					vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
				}
			}
		}
	}
}