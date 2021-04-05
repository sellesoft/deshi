/*
Vulkan Spec [https://renderdoc.org/vkspec_chunked/index.html]
In real implementation: 
https://vulkan-tutorial.com/en/Vertex_buffers/Staging_buffer#page_Using-a-staging-buffer:~:text=You%20may
https://vulkan-tutorial.com/en/Vertex_buffers/Staging_buffer#page_Conclusion:~:text=It%20should
https://vulkan-tutorial.com/en/Vertex_buffers/Index_buffer#page_Using-an-index-buffer:~:text=The%20previous
https://vulkan-tutorial.com/en/Uniform_buffers/Descriptor_layout_and_buffer#page_Updating-uniform-data:~:text=Using%20a%20UBO
https://vulkan-tutorial.com/en/Texture_mapping/Combined_image_sampler#page_Updating-the-descriptors:~:text=determined.-,It%20is
https://vulkan-tutorial.com/en/Generating_Mipmaps#page_Generating-Mipmaps:~:text=Beware%20if%20you
https://vulkan-tutorial.com/en/Generating_Mipmaps#page_Linear-filtering-support:~:text=There%20are%20two
https://vulkan-tutorial.com/en/Multisampling#page_Conclusion:~:text=features%2C-,like
*/

#include "renderer_vulkan.h"
#include "../core.h"
#include "../scene/Scene.h"
#include "../math/Math.h"
#include "../geometry/Triangle.h"

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

#define LOGGING_LEVEL 3
#if LOGGING_LEVEL == 0
#define PRINTVK(level, message) (void)0
#else
#define PRINTVK(level, ...) if(LOGGING_LEVEL >= level){ LOG(__VA_ARGS__); }
#endif

//////////////////////////
//// render interface ////
//////////////////////////

void Renderer::
Init(Time* time, Window* window, deshiImGui* imgui) {
	PRINTVK(1, "\nInitializing Vulkan");
	this->time = time;
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
	SetupPipelineCreation();
	CreatePipelines();
	CreateSyncObjects();
	
	LoadDefaultAssets();
	CreateSceneBuffers();
	
	PRINTVK(2, "  Initializing ImGui");
	imgui->Init(this);
	//BuildCommandBuffers();
	initialized = true;
	
	PRINTVK(1, "Initializing Rendering");
}

void Renderer::
Render() {
	//std::cout << "  Rendering Frame     " << std::endl;
	if(remakeWindow){
		int w, h;
		glfwGetFramebufferSize(window, &w, &h);
		if(w <= 0 || h <= 0){  ImGui::EndFrame(); return;  }
		ResizeWindow(w, h);
		frameIndex = 0;
		remakeWindow = false;
	}
	
	//reset stats
	stats = {};
	
	vkWaitForFences(device, 1, &fencesInFlight[frameIndex], VK_TRUE, UINT64_MAX);
	VkSemaphore image_sema  = semaphores[frameIndex].imageAcquired;
	VkSemaphore render_sema = semaphores[frameIndex].renderComplete;
	
	//get next image from surface
	u32 imageIndex;
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
	
	//render stuff
	ImGui::Render(); ImDrawData* drawData = ImGui::GetDrawData();
	if(drawData){
		stats.drawnIndices += drawData->TotalIdxCount;
		stats.totalVertices += drawData->TotalVtxCount;
	}
	BuildCommandBuffers();
	
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
	
	//update stats
	stats.drawnTriangles += stats.drawnIndices / 3;
	stats.totalVertices += u32(vertexBuffer.size());
	stats.totalIndices += u32(indexBuffer.size());
	stats.totalTriangles += stats.totalIndices / 3;
	
	if(remakePipelines){ 
		CreatePipelines(); 
		UpdateMaterialPipelines();
		remakePipelines = false; 
	}
}

void Renderer::
Present() {}

void Renderer::
Cleanup() {
	PRINTVK(1, "Initializing Cleanup\n");
	
	//save pipeline cache to disk
	if(pipelineCache != VK_NULL_HANDLE){
		
		/* Get size of pipeline cache */
		size_t size{};
		ASSERTVK(vkGetPipelineCacheData(device, pipelineCache, &size, nullptr), "failed to get pipeline cache data size");
		
		/* Get data of pipeline cache */
		std::vector<char> data(size);
		ASSERTVK(vkGetPipelineCacheData(device, pipelineCache, &size, data.data()), "faile to get pipeline cache data");
		
		/* Write pipeline cache data to a file in binary format */
		deshi::writeFileBinary(deshi::dirData() + "pipeline_cache.dat", data);
	}
	
	//write pre-pipeline data
	
	vkDeviceWaitIdle(device);
}
/*
u32 Renderer::
AddTriangle(Triangle* triangle){
	PRINT("Not implemented yet");
	return 0xFFFFFFFF;
}

void Renderer::
RemoveTriangle(u32 triangleID){
	PRINT("Not implemented yet");
}

void Renderer::
UpdateTriangleColor(u32 triangleID, Color color){
	PRINT("Not implemented yet");
}

void Renderer::
UpdateTrianglePosition(u32 triangleID, Vector3 position){
	PRINT("Not implemented yet");
}

void Renderer::
TranslateTriangle(u32 triangleID, Vector3 translation){
	PRINT("Not implemented yet");
}

std::vector<u32> Renderer::
AddTriangles(std::vector<Triangle*> triangles){
	PRINT("Not implemented yet");
	return std::vector<u32>();
}

void Renderer::
RemoveTriangles(std::vector<u32> triangleIDs){
	PRINT("Not implemented yet");
}

void Renderer::
UpdateTrianglesColor(std::vector<u32> triangleIDs, Color color){
	PRINT("Not implemented yet");
}

void Renderer::
TranslateTriangles(std::vector<u32> triangleIDs, Vector3 translation){
	PRINT("Not implemented");
}
*/
u32 Renderer::
LoadBaseMesh(Mesh* m){
	PRINTVK(3, "    Loading Mesh: ", m->name);
	MeshVk mesh;  mesh.base = true; 
	mesh.ptr = m; mesh.visible = false;
	mesh.primitives.reserve(m->batchCount);
	mesh.name = m->name;

	//resize scene vectors
	vertexBuffer.reserve(vertexBuffer.size() + m->vertexCount);
	indexBuffer.reserve(indexBuffer.size() + m->indexCount);
	textures.reserve(textures.size() + m->textureCount);
	materials.reserve(materials.size() + m->batchCount);
	
	u32 batchVertexStart;
	u32 batchIndexStart;
	for(Batch& batch : m->batchArray){
		batchVertexStart = u32(vertexBuffer.size());
		batchIndexStart = u32(indexBuffer.size());
		
		//vertices
		for(int i=0; i<batch.vertexArray.size(); ++i){ 
			VertexVk vert;
			vert.pos      = glm::make_vec3(&batch.vertexArray[i].pos.x);
			vert.texCoord = glm::make_vec2(&batch.vertexArray[i].uv.x);
			vert.color    = glm::make_vec3(&batch.vertexArray[i].color.x);
			vert.normal   = glm::make_vec3(&batch.vertexArray[i].normal.x);
			vertexBuffer.push_back(vert);
		}
		
		//indices
		for(u32 i : batch.indexArray){
			indexBuffer.push_back(batchVertexStart+i);
		}
		//scene.indexBuffer.insert(scene.indexBuffer.end(), batch.indexArray.begin(), batch.indexArray.end());
		
		//TODO(delle,ReOp) not every material uses textures
		//material
		MaterialVk mat; mat.shader = u32(batch.shader);
		{
			//material textures
			for(int i=0; i<batch.textureArray.size(); ++i){ 
				u32 idx = LoadTexture(batch.textureArray[i]);
				switch(textures[idx].type){
					case(TEXTURE_ALBEDO):  { mat.albedoID   = idx; }break;
					case(TEXTURE_NORMAL):  { mat.normalID   = idx; }break;
					case(TEXTURE_LIGHT):   { mat.lightID    = idx; }break;
					case(TEXTURE_SPECULAR):{ mat.specularID = idx; }break;
				}
			}
			
			mat.pipeline = GetPipelineFromShader(batch.shader);
			
			//allocate and write descriptor set for material
			VkDescriptorSetAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = descriptorPool;
			allocInfo.pSetLayouts = &descriptorSetLayouts.textures;
			allocInfo.descriptorSetCount = 1;
			ASSERTVK(vkAllocateDescriptorSets(device, &allocInfo, &mat.descriptorSet), "failed to allocate materials descriptor sets");
			
			std::array<VkWriteDescriptorSet, 4> writeDescriptorSets{};
			writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSets[0].dstSet = mat.descriptorSet;
			writeDescriptorSets[0].dstArrayElement = 0;
			writeDescriptorSets[0].descriptorCount = 1;
			writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeDescriptorSets[0].pImageInfo = &textures[mat.albedoID].imageInfo;
			writeDescriptorSets[0].dstBinding = 0;
			
			memcpy(&writeDescriptorSets[1], &writeDescriptorSets[0], sizeof(VkWriteDescriptorSet));
			writeDescriptorSets[1].pImageInfo = &textures[mat.normalID].imageInfo;
			writeDescriptorSets[1].dstBinding = 1;
			
			memcpy(&writeDescriptorSets[2], &writeDescriptorSets[0], sizeof(VkWriteDescriptorSet));
			writeDescriptorSets[2].pImageInfo = &textures[mat.specularID].imageInfo;
			writeDescriptorSets[2].dstBinding = 2;
			
			memcpy(&writeDescriptorSets[3], &writeDescriptorSets[0], sizeof(VkWriteDescriptorSet));
			writeDescriptorSets[3].pImageInfo = &textures[mat.lightID].imageInfo;
			writeDescriptorSets[3].dstBinding = 3;
			
			vkUpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
			
			//TODO(delle,ReVu) specialization constants for materials here or in pipeline
			//see gltfscenerendering.cpp:575
			
			mat.id = u32(materials.size());
			materials.push_back(mat);
		}
		
		//primitive
		PrimitiveVk primitive;
		primitive.firstIndex = batchIndexStart;
		primitive.indexCount = batch.indexArray.size();
		primitive.materialIndex = mat.id;
		mesh.primitives.push_back(primitive);
	}
	
	//add mesh to scene
	mesh.id = u32(meshes.size());
	meshes.push_back(mesh);
	if(initialized){ CreateSceneBuffers(); }
	return mesh.id;
}

u32 Renderer::
CreateMesh(u32 meshID, Matrix4 matrix){
	if(meshID < meshes.size()){
		MeshVk mesh; mesh.base = false; 
		mesh.ptr = meshes[meshID].ptr; mesh.visible = true;
		mesh.primitives = std::vector<PrimitiveVk>(meshes[meshID].primitives);
		mesh.modelMatrix = glm::make_mat4(matrix.data);
		mesh.name = meshes[meshID].name;
		mesh.id = u32(meshes.size());
		meshes.push_back(mesh);
		meshes[meshID].children.push_back(mesh.id);
		return mesh.id;
	}
	return 0xFFFFFFFF;
}

void Renderer::
UnloadBaseMesh(u32 meshID){
	if(meshID < meshes.size()){
		if(meshes[meshID].base){
			//loop thru children and remove them
			//remove verts and indices
			ERROR("UnloadMesh: Not implemented yet");
		}else{
			ERROR("Only a base mesh can be unloaded");
		}
	}
}

void Renderer::
RemoveMesh(u32 meshID){
	if(meshID < meshes.size()){
		if(!meshes[meshID].base){
			//TODO(delle,ReOp) optimize this with mesh pooling
			for(int i=meshID; i<meshes.size(); ++i) { --meshes[i].id;  } 
			meshes.erase(meshes.begin() + meshID);
		}else{
			ERROR("Only a child/non-base mesh can be removed");
		}
	}
}

Matrix4 Renderer::
GetMeshMatrix(u32 meshID){
	if(meshID < meshes.size()){
		return Matrix4((float*)glm::value_ptr(meshes[meshID].modelMatrix));
	}
	return Matrix4(0.f);
}

Mesh* Renderer::
GetMeshPtr(u32 meshID){
	if(meshID < meshes.size()){
		return meshes[meshID].ptr;
	}
	return nullptr;
}

void Renderer::
UpdateMeshMatrix(u32 meshID, Matrix4 matrix){
	if(meshID < meshes.size()){
		meshes[meshID].modelMatrix = glm::make_mat4(matrix.data);
	}
}

void Renderer::
TransformMeshMatrix(u32 meshID, Matrix4 transform){
	if(meshID < meshes.size()){
		meshes[meshID].modelMatrix = glm::make_mat4(transform.data) * meshes[meshID].modelMatrix;
	}
}

void Renderer::
UpdateMeshBatchMaterial(u32 meshID, u32 batchIndex, u32 matID){
	if(meshID < meshes.size() && batchIndex < meshes[meshID].primitives.size() && matID < materials.size()){
		meshes[meshID].primitives[batchIndex].materialIndex = matID;
	}
}

void Renderer::
UpdateMeshVisibility(u32 meshID, bool visible){
	if(meshID == -1){
		for(auto& mesh : meshes){ mesh.visible = visible; }
	}else if(meshID < meshes.size()){
		meshes[meshID].visible = visible;
	}
}
/*
u32 Renderer::
MakeInstance(u32 meshID, Matrix4 matrix) {
	ERROR("MakeInstance: Not implemented yet");
	return 0xFFFFFFFF;
}

void Renderer::
RemoveInstance(u32 instanceID) {
	ERROR("RemoveInstance: Not implemented yet");
}

void Renderer::
UpdateInstanceMatrix(u32 instanceID, Matrix4 matrix) {
	ERROR("UpdateInstanceMatrix: Not implemented yet");
}

void Renderer::
TransformInstanceMatrix(u32 instanceID, Matrix4 transform) {
	ERROR("TransformInstanceMatrix: Not implemented yet");
}

void Renderer::
UpdateInstanceVisibility(u32 instanceID, bool visible) {
	ERROR("UpdateInstanceVisibility: Not implemented yet");
}
*/
u32 Renderer::
LoadTexture(Texture texture){
	PRINTVK(3, "    Loading Texture: ", texture.filename);
	//TODO(delle,OpReVu) optimize checking if a texture was already loaded
	for(auto& tex : textures){ if(strcmp(tex.filename, texture.filename) == 0){ return tex.id; } }
	
	TextureVk tex; 
	strncpy_s(tex.filename, texture.filename, 63);
	
	std::string imagePath = deshi::getTexture(texture.filename);
	if(imagePath == ""){ return 0; }
	tex.pixels = stbi_load(imagePath.c_str(), &tex.width, &tex.height, &tex.channels, STBI_rgb_alpha);
	ASSERT(tex.pixels, "stb failed to load an image");
	
	tex.type = u32(texture.type);
	tex.mipLevels = u32(std::floor(std::log2(std::max(tex.width, tex.height)))) + 1;
	tex.imageSize = tex.width * tex.height * 4;
	
	//copy the memory to a staging buffer
	StagingBufferVk staging{};
	CreateAndMapBuffer(staging.buffer, staging.memory, tex.imageSize, static_cast<size_t>(tex.imageSize), tex.pixels, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	
	//copy the staging buffer to the image and generate its mipmaps
	createImage(tex.width, tex.height, tex.mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, tex.image, tex.imageMemory);
	transitionImageLayout(tex.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, tex.mipLevels);
	copyBufferToImage(staging.buffer, tex.image, u32(tex.width), u32(tex.height));
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
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR; //TODO(delle,ReVu) VK_SAMPLER_MIPMAP_MODE_NEAREST for more performance
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
	samplerInfo.maxLod = f32(tex.mipLevels);
	ASSERTVK(vkCreateSampler(device, &samplerInfo, nullptr, &tex.sampler), "failed to create texture sampler");
	
	//create image view
	tex.view = createImageView(tex.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, tex.mipLevels);
	
	//fill descriptor image info
	tex.imageInfo.imageView = tex.view;
	tex.imageInfo.sampler = tex.sampler;
	tex.imageInfo.imageLayout = tex.layout;
	
	//add the texture to the scene and return its index
	u32 idx = u32(textures.size());
	tex.id = idx;
	textures.push_back(tex);
	return idx;
}
/*
void Renderer::
UnloadTexture(u32 textureID){
	PRINT("Not implemented yet");
}
*/
std::string Renderer::
ListTextures(){
	std::string out = "[c:yellow]ID  Filename  Width  Height  Depth  Type[c]\n";
	for(auto& tex : textures){
		if(tex.id < 10){
			out += TOSTRING(" ", tex.id, "  ", tex.filename, "  ", tex.width, "  ", tex.height, "  ", tex.channels, "  ", tex.type, "\n");
		}else{
			out += TOSTRING(tex.id, "  ", tex.filename, "  ", tex.width, "  ", tex.height, "  ", tex.channels, "  ", tex.type, "\n");
		}
	}
	return out;
}

u32 Renderer::
CreateMaterial(u32 shader, u32 albedoTextureID, u32 normalTextureID, u32 specTextureID, u32 lightTextureID){
	PRINTVK(3, "    Creating material");
	MaterialVk mat; mat.id = u32(meshes.size());
	mat.shader = shader; mat.pipeline = GetPipelineFromShader(shader);
	mat.albedoID = albedoTextureID; mat.normalID = normalTextureID;
	mat.specularID = specTextureID; mat.lightID = lightTextureID;
	
	//allocate and write descriptor set for material
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.pSetLayouts = &descriptorSetLayouts.textures;
	allocInfo.descriptorSetCount = 1;
	ASSERTVK(vkAllocateDescriptorSets(device, &allocInfo, &mat.descriptorSet), "failed to allocate materials descriptor sets");
	
	std::array<VkWriteDescriptorSet, 4> writeDescriptorSets{};
	writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSets[0].dstSet = mat.descriptorSet;
	writeDescriptorSets[0].dstArrayElement = 0;
	writeDescriptorSets[0].descriptorCount = 1;
	writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeDescriptorSets[0].pImageInfo = &textures[mat.albedoID].imageInfo;
	writeDescriptorSets[0].dstBinding = 0;
	
	memcpy(&writeDescriptorSets[1], &writeDescriptorSets[0], sizeof(VkWriteDescriptorSet));
	writeDescriptorSets[1].pImageInfo = &textures[mat.normalID].imageInfo;
	writeDescriptorSets[1].dstBinding = 1;
	
	memcpy(&writeDescriptorSets[2], &writeDescriptorSets[0], sizeof(VkWriteDescriptorSet));
	writeDescriptorSets[2].pImageInfo = &textures[mat.specularID].imageInfo;
	writeDescriptorSets[2].dstBinding = 2;
	
	memcpy(&writeDescriptorSets[3], &writeDescriptorSets[0], sizeof(VkWriteDescriptorSet));
	writeDescriptorSets[3].pImageInfo = &textures[mat.lightID].imageInfo;
	writeDescriptorSets[3].dstBinding = 3;
	
	vkUpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
	
	//add to scene
	materials.push_back(mat);
	return mat.id;
}

void Renderer::
UpdateMaterialTexture(u32 matID, u32 texSlot, u32 texID){
	if(matID < materials.size() && texID < textures.size()){
		VkWriteDescriptorSet writeDescriptorSet{};
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.dstSet = materials[matID].descriptorSet;
		writeDescriptorSet.dstArrayElement = 0;
		writeDescriptorSet.descriptorCount = 1;
		writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		
		switch(texSlot){
			case(0):{ 
				materials[matID].albedoID = texID; 
				writeDescriptorSet.pImageInfo = &textures[materials[matID].albedoID].imageInfo;
				writeDescriptorSet.dstBinding = 0;
			} break;
			case(1):{ 
				materials[matID].normalID = texID; 
				writeDescriptorSet.pImageInfo = &textures[materials[matID].normalID].imageInfo;
				writeDescriptorSet.dstBinding = 1;
			} break;
			case(2):{ 
				materials[matID].specularID = texID; 
				writeDescriptorSet.pImageInfo = &textures[materials[matID].specularID].imageInfo;
				writeDescriptorSet.dstBinding = 2;
			}  break;
			case(3):{ 
				materials[matID].lightID = texID; 
				writeDescriptorSet.pImageInfo = &textures[materials[matID].lightID].imageInfo;
				writeDescriptorSet.dstBinding = 3;
			}  break;
			default:{ return; }
		}
		
		vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
	}
	
}

void Renderer::
UpdateMaterialShader(u32 matID, u32 shader){
	if(matID == 0xFFFFFFFF){
		for(auto& mat : materials){ mat.pipeline = GetPipelineFromShader(shader); }
	}else if(matID < materials.size()){
		materials[matID].pipeline = GetPipelineFromShader(shader);
	}
}

std::vector<u32> Renderer::
GetMaterialIDs(u32 MeshID) {
	if (MeshID < meshes.size()) {
		MeshVk* m = &meshes[MeshID];
		std::vector<u32> out; out.resize(m->primitives.size());
		for (auto& a : m->primitives) {
			out.push_back(a.materialIndex);
		}
		return out;
	}
	return std::vector<u32>();
}

void Renderer::
LoadDefaultAssets(){
	PRINTVK(2, "  Loading default assets");
	//load default textures
	textures.reserve(8);
	Texture nullTex   ("null128.png");     LoadTexture(nullTex);
	Texture defaultTex("default1024.png"); LoadTexture(defaultTex);
	Texture blackTex  ("black1024.png");   LoadTexture(blackTex);
	Texture whiteTex  ("white1024.png");   LoadTexture(whiteTex);
	
	materials.reserve(8);
	//default default materials
	CreateMaterial(0); //flat
	CreateMaterial(1); //phong
	
	//load default meshes
	Mesh* default_box = new Mesh; *default_box = Mesh::CreateBox(Vector3(1,1,1)); LoadBaseMesh(default_box);
	Mesh* planar_box = new Mesh; *planar_box = Mesh::CreatePlanarBox(Vector3(1,1,1)); LoadBaseMesh(planar_box);
	Mesh* textured_box = new Mesh; *textured_box = Mesh::CreatePlanarBox(Vector3(1,1,1), Texture("UV_Grid_Sm.jpg")); LoadBaseMesh(textured_box);
	
	//TODO(delle,ReVu) add local axis, global axis, and grid 
}

//ref: gltfscenerendering.cpp:350
void Renderer::
LoadScene(Scene* sc){
	PRINTVK(2, "  Loading Scene");
	//load meshes, materials, and textures
	for(Model& model : sc->models){ LoadBaseMesh(&model.mesh); }
	
	CreateSceneBuffers();
}

void Renderer::
CreateSceneBuffers(){
	PRINTVK(3, "    Creating Scene Buffers");
	StagingBufferVk vertexStaging{}, indexStaging{};
	size_t vertexBufferSize = vertexBuffer.size() * sizeof(VertexVk);
	size_t indexBufferSize  = indexBuffer.size() * sizeof(u32);
	
	//create host visible vertex and index buffers (CPU/RAM)
	CreateAndMapBuffer(vertexStaging.buffer, vertexStaging.memory, vertices.bufferSize, vertexBufferSize, vertexBuffer.data(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	
	CreateAndMapBuffer(indexStaging.buffer, indexStaging.memory, indices.bufferSize, indexBufferSize, indexBuffer.data(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	
	//create device local buffers (GPU)
	CreateAndMapBuffer(vertices.buffer, vertices.bufferMemory, vertices.bufferSize, vertexBufferSize, nullptr, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	
	CreateAndMapBuffer(indices.buffer, indices.bufferMemory, indices.bufferSize, indexBufferSize, nullptr, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	
	//copy data from staging buffers to device local buffers
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();{
		VkBufferCopy copyRegion{};
		
		copyRegion.size = vertexBufferSize;
		vkCmdCopyBuffer(commandBuffer, vertexStaging.buffer, vertices.buffer, 1, &copyRegion);
		
		copyRegion.size = indexBufferSize;
		vkCmdCopyBuffer(commandBuffer, indexStaging.buffer, indices.buffer, 1, &copyRegion);
		
	}endSingleTimeCommands(commandBuffer);
	
	//free staging resources
	vkDestroyBuffer(device, vertexStaging.buffer, nullptr);
	vkFreeMemory(device, vertexStaging.memory, nullptr);
	vkDestroyBuffer(device, indexStaging.buffer, nullptr);
	vkFreeMemory(device, indexStaging.memory, nullptr);
}

void Renderer::
UpdateVertexBuffer(){
	PRINTVK(3, "    Updating vertex buffer");
	StagingBufferVk vertexStaging{};
	size_t vertexBufferSize = vertexBuffer.size() * sizeof(VertexVk);
	
	//create host visible vertex buffer (CPU/RAM)
	CreateAndMapBuffer(vertexStaging.buffer, vertexStaging.memory, vertices.bufferSize, vertexBufferSize, vertexBuffer.data(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	
	//create device local buffers (GPU)
	CreateAndMapBuffer(vertices.buffer, vertices.bufferMemory, vertices.bufferSize, vertexBufferSize, nullptr, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	
	//copy data from staging buffers to device local buffers
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();{
		VkBufferCopy copyRegion{};
		copyRegion.size = vertexBufferSize;
		vkCmdCopyBuffer(commandBuffer, vertexStaging.buffer, vertices.buffer, 1, &copyRegion);
	}endSingleTimeCommands(commandBuffer);
	
	//free staging resources
	vkDestroyBuffer(device, vertexStaging.buffer, nullptr);
	vkFreeMemory(device, vertexStaging.memory, nullptr);
}

void Renderer::
UpdateIndexBuffer(){
	PRINTVK(3, "    Updating index buffer");
	StagingBufferVk indexStaging{};
	size_t indexBufferSize  = indexBuffer.size() * sizeof(u32);
	
	//create host visible index buffer (CPU/RAM)
	CreateAndMapBuffer(indexStaging.buffer, indexStaging.memory, indices.bufferSize, indexBufferSize, indexBuffer.data(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	
	//create device local buffers (GPU)
	CreateAndMapBuffer(indices.buffer, indices.bufferMemory, indices.bufferSize, indexBufferSize, nullptr, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	
	//copy data from staging buffers to device local buffers
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();{
		VkBufferCopy copyRegion{};
		copyRegion.size = indexBufferSize;
		vkCmdCopyBuffer(commandBuffer, indexStaging.buffer, indices.buffer, 1, &copyRegion);
	}endSingleTimeCommands(commandBuffer);
	
	//free staging resources
	vkDestroyBuffer(device, indexStaging.buffer, nullptr);
	vkFreeMemory(device, indexStaging.memory, nullptr);
}

void Renderer::
UpdateCameraPosition(Vector3 position){
	shaderData.values.viewPos = glm::vec4(glm::make_vec3(&position.x), 1.f);
}

void Renderer::
UpdateCameraViewMatrix(Matrix4 m){
	shaderData.values.view = glm::make_mat4(m.data);
}

void Renderer::
UpdateCameraProjectionMatrix(Matrix4 m){
	shaderData.values.proj = glm::make_mat4(m.data);
}

void Renderer::
ReloadShader(u32 shader) {
	switch(shader){
		case(Shader::FLAT):default: { 
			pipelineCreateInfo.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
			pipelineCreateInfo.basePipelineHandle  = VK_NULL_HANDLE;
			vkDestroyPipeline(device, pipelines.FLAT, nullptr);
			shaderStages[0] = CompileAndLoadShader("flat.vert", VK_SHADER_STAGE_VERTEX_BIT);
			shaderStages[1] = CompileAndLoadShader("flat.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
			ASSERTVK(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.FLAT), "failed to create flat graphics pipeline");
			pipelineCreateInfo.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;
			pipelineCreateInfo.basePipelineHandle = pipelines.FLAT;
		} break;
		case(Shader::WIREFRAME):    {
			if(deviceFeatures.fillModeNonSolid){
				vkDestroyPipeline(device, pipelines.WIREFRAME, nullptr);
				rasterizationState.polygonMode = VK_POLYGON_MODE_LINE;
				rasterizationState.cullMode = VK_CULL_MODE_NONE;
				depthStencilState.depthTestEnable = VK_FALSE;
				shaderStages[0] = CompileAndLoadShader("wireframe.vert", VK_SHADER_STAGE_VERTEX_BIT);
				shaderStages[1] = CompileAndLoadShader("wireframe.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
				ASSERTVK(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.WIREFRAME), "failed to create wireframe graphics pipeline");
				rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
				rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
				depthStencilState.depthTestEnable = VK_TRUE;
			}
		} break;
		case(Shader::PHONG):        {
			vkDestroyPipeline(device, pipelines.PHONG, nullptr);
			shaderStages[0] = CompileAndLoadShader("phong.vert", VK_SHADER_STAGE_VERTEX_BIT);
			shaderStages[1] = CompileAndLoadShader("phong.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
			ASSERTVK(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.PHONG), "failed to create phong graphics pipeline");
		} break;
		case(Shader::TWOD):         {
			vkDestroyPipeline(device, pipelines.TWOD, nullptr);
			shaderStages[0] = CompileAndLoadShader("twod.vert", VK_SHADER_STAGE_VERTEX_BIT);
			shaderStages[1] = CompileAndLoadShader("twod.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
			ASSERTVK(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.TWOD), "failed to create twod graphics pipeline");
		} break;
		case(Shader::PBR):          { 
			vkDestroyPipeline(device, pipelines.PBR, nullptr);
			shaderStages[0] = CompileAndLoadShader("pbr.vert", VK_SHADER_STAGE_VERTEX_BIT);
			shaderStages[1] = CompileAndLoadShader("pbr.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
			ASSERTVK(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.PBR), "failed to create pbr graphics pipeline");
		} break;
		case(Shader::LAVALAMP):     { 
			vkDestroyPipeline(device, pipelines.LAVALAMP, nullptr);
			shaderStages[0] = CompileAndLoadShader("lavalamp.vert", VK_SHADER_STAGE_VERTEX_BIT);
			shaderStages[1] = CompileAndLoadShader("lavalamp.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
			ASSERTVK(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.LAVALAMP), "failed to create lavalamp graphics pipeline");
		} break;
		case(Shader::TESTING0):     { 
			vkDestroyPipeline(device, pipelines.TESTING0, nullptr);
			shaderStages[0] = CompileAndLoadShader("testing0.vert", VK_SHADER_STAGE_VERTEX_BIT);
			shaderStages[1] = CompileAndLoadShader("testing0.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
			ASSERTVK(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.TESTING0), "failed to create testing0 graphics pipeline");
		} break;
		case(Shader::TESTING1):     { 
			vkDestroyPipeline(device, pipelines.TESTING1, nullptr);
			shaderStages[0] = CompileAndLoadShader("testing1.vert", VK_SHADER_STAGE_VERTEX_BIT);
			shaderStages[1] = CompileAndLoadShader("testing1.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
			ASSERTVK(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.TESTING1), "failed to create testing1 graphics pipeline");
		} break;
	}
	UpdateMaterialPipelines();
}

void Renderer::
ReloadAllShaders() {
	CompileAllShaders();
	remakePipelines = true;
}

void Renderer::
UpdateDebugOptions(bool wireframe, bool globalAxis, bool wireframeOnly) {
	settings.wireframe = wireframe; settings.globalAxis = globalAxis;
	settings.wireframeOnly = wireframeOnly;
};

//////////////////////////////////
//// initialization functions ////
//////////////////////////////////

void Renderer::
CreateInstance() {
	PRINTVK(2, "  Creating Vulkan Instance");
	if(enableValidationLayers && !checkValidationLayerSupport()) {
		ASSERT(false, "validation layers requested, but not available");
	}
	
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "deshi";
	appInfo.applicationVersion = VK_MAKE_VERSION(0,5,0);
	appInfo.pEngineName = "deshi";
	appInfo.engineVersion = VK_MAKE_VERSION(0,5,0);
	appInfo.apiVersion = VK_API_VERSION_1_0;
	
	std::vector<const char*> extensions = getRequiredExtensions();
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
	
	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = static_cast<u32>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();
	if(enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<u32>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
		populateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
	} else {
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}
	
	ASSERTVK(vkCreateInstance(&createInfo, allocator, &instance), "failed to create instance");
}

void Renderer::
SetupDebugMessenger() {
	PRINTVK(2, "  Setting Up Debug Messenger");
	if(!enableValidationLayers) return;
	
	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	populateDebugMessengerCreateInfo(createInfo);
	
	ASSERTVK(CreateDebugUtilsMessengerEXT(instance, &createInfo, allocator, &debugMessenger), "failed to set up debug messenger");
}

void Renderer::
CreateSurface() {
	PRINTVK(2, "  Creating GLFW Surface");
	ASSERTVK(glfwCreateWindowSurface(instance, window, allocator, &surface), "failed to create window surface");
}

void Renderer::
PickPhysicalDevice() {
	PRINTVK(2, "  Picking Physical Device");
	u32 deviceCount = 0;
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

void Renderer::
CreateLogicalDevice() {
	PRINTVK(2, "  Creating Logical Device");
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<u32> uniqueQueueFamilies = {
		physicalQueueFamilies.graphicsFamily.value, physicalQueueFamilies.presentFamily.value
	};
	
	float queuePriority = 1.0f;
	//queueCreateInfos.reserve(uniqueQueueFamilies.size());
	for(u32 queueFamily : uniqueQueueFamilies){
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = physicalQueueFamilies.graphicsFamily.value;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}
	
	//TODO(delle,ReVu) add rendering options here
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
	createInfo.queueCreateInfoCount = static_cast<u32>(queueCreateInfos.size());
	createInfo.pEnabledFeatures = &enabledFeatures;
	createInfo.enabledExtensionCount = static_cast<u32>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();
	if(enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<u32>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}else{
		createInfo.enabledLayerCount = 0;
	}
	
	ASSERTVK(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device), "failed to create logical device");
	
	vkGetDeviceQueue(device, physicalQueueFamilies.graphicsFamily.value, 0, &graphicsQueue);
	vkGetDeviceQueue(device, physicalQueueFamilies.presentFamily.value, 0, &presentQueue);
}

//TODO(delle,ReVu) find a better/more accurate way to do this, see gltfloading.cpp, line:592
void Renderer::
CreateDescriptorPool(){
	PRINTVK(2, "  Creating Descriptor Pool");
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

void Renderer::
CreatePipelineCache(){
	PRINTVK(2, "  Creating Pipeline Cache");
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo{};
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	pipelineCacheCreateInfo.flags = 0;
	
	/* Try to read pipeline cache file if exists */ //NOTE(delle) this saves ~2000ms on my system
	std::vector<char> data = deshi::readFileBinary(deshi::dirData()+"pipeline_cache.dat");
	if(data.size() > 0){
		pipelineCacheCreateInfo.initialDataSize = data.size();
		pipelineCacheCreateInfo.pInitialData = data.data();
	}
	
	ASSERTVK(vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &pipelineCache), "failed to create pipeline cache");
}

void Renderer::
CreateUniformBuffer(){
	PRINTVK(2, "  Creating Uniform Buffer");
	CreateOrResizeBuffer(shaderData.uniformBuffer, shaderData.uniformBufferMemory, shaderData.uniformBufferSize, sizeof(shaderData.values) , VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	UpdateUniformBuffer();
}

void Renderer::
CreateCommandPool(){
	PRINTVK(2, "  Creating Command Pool");
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = physicalQueueFamilies.graphicsFamily.value;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	
	ASSERTVK(vkCreateCommandPool(device, &poolInfo, allocator, &commandPool), "failed to create command pool");
}

void Renderer::
CreateClearValues(){
	PRINTVK(2, "  Creating Clear Values");
	clearValues[0].color = {0, 0, 0, 1};
	clearValues[1].depthStencil = {1.f, 0};
}

void Renderer::
CreateSyncObjects(){
	PRINTVK(2, "  Creating Sync Objects");
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


void Renderer::
CreateLayouts(){
	PRINTVK(2, "  Creating Layouts");
	//// uniform camera matrices binding ////
	std::array<VkDescriptorSetLayoutBinding, 4> setLayoutBindings{};
	setLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	setLayoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	setLayoutBindings[0].binding = 0;
	setLayoutBindings[0].descriptorCount = 1;
	
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{};
	descriptorSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCI.pBindings = setLayoutBindings.data();
	descriptorSetLayoutCI.bindingCount = 1;
	
	ASSERTVK(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, nullptr, &descriptorSetLayouts.matrices), "failed to create ubo descriptor set layout");
	
	//// material textures bindings ////
	// Color/albedo map
	setLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	setLayoutBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	setLayoutBindings[0].binding = 0;
	setLayoutBindings[0].descriptorCount = 1;
	
	// Normal map
	memcpy(&setLayoutBindings[1], &setLayoutBindings[0], sizeof(VkDescriptorSetLayoutBinding));
	setLayoutBindings[1].binding = 1;
	
	// Specular/reflective map
	memcpy(&setLayoutBindings[2], &setLayoutBindings[0], sizeof(VkDescriptorSetLayoutBinding));
	setLayoutBindings[2].binding = 2;
	
	// Light/emissive map
	memcpy(&setLayoutBindings[3], &setLayoutBindings[0], sizeof(VkDescriptorSetLayoutBinding));
	setLayoutBindings[3].binding = 3;
	
	
	descriptorSetLayoutCI.bindingCount = 4;
	ASSERTVK(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, nullptr, &descriptorSetLayouts.textures), "failed to create textures descriptor set layout");;
	
	std::array<VkDescriptorSetLayout, 2> setLayouts = { 
		descriptorSetLayouts.matrices, descriptorSetLayouts.textures
	};
	
	//push constants for passing global instance id
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(glm::mat4);
	
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = setLayouts.size();
	pipelineLayoutInfo.pSetLayouts = setLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
	
	ASSERTVK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout), "failed to create pipeline layout");
	
	//allocate and write descriptor set for matrices/uniform buffer
	{
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.pSetLayouts = &descriptorSetLayouts.matrices;
		allocInfo.descriptorSetCount = 1;
		ASSERTVK(vkAllocateDescriptorSets(device, &allocInfo, &sceneDescriptorSet), "failed to allocate matrices descriptor sets");
		
		VkDescriptorBufferInfo descBufferInfo{};
		descBufferInfo.buffer = shaderData.uniformBuffer;
		descBufferInfo.offset = 0;
		descBufferInfo.range  = sizeof(shaderData.values);
		
		VkWriteDescriptorSet writeDescriptorSet {};
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.dstSet = sceneDescriptorSet;
		writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writeDescriptorSet.dstBinding = 0;
		writeDescriptorSet.pBufferInfo = &descBufferInfo;
		writeDescriptorSet.descriptorCount = 1;
		vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
	}
}

void Renderer::
ResizeWindow(int w, int h) {
	PRINTVK(1, "Creating Window");
	// Ensure all operations on the device have been finished before destroying resources
	vkDeviceWaitIdle(device);
	
	width = w; height = h;
	CreateSwapChain();
	CreateRenderPass();
	CreateFrames(); //image views, color/depth resources, framebuffers, commandbuffers
}

void Renderer::
CreateSwapChain() {
	PRINTVK(2, "  Creating Swapchain");
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
	
	u32 queueFamilyIndices[] = {
		physicalQueueFamilies.graphicsFamily.value, physicalQueueFamilies.presentFamily.value
	};
	
	//create swapchain and swap chain images, set width and height
	VkSwapchainCreateInfoKHR info{};
	info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	info.surface = surface;
	info.imageFormat = surfaceFormat.format;
	info.imageColorSpace = surfaceFormat.colorSpace;
	info.imageArrayLayers = 1;
	info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	if (physicalQueueFamilies.graphicsFamily.value != physicalQueueFamilies.presentFamily.value) {
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

void Renderer::
CreateRenderPass(){
	PRINTVK(2, "  Creating Render Pass");
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
	renderPassInfo.attachmentCount = static_cast<u32>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;
	
	ASSERTVK(vkCreateRenderPass(device, &renderPassInfo, allocator, &renderPass), "failed to create render pass");
}

void Renderer::
CreateFrames(){
	PRINTVK(2, "  Creating Frames");
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
	for (u32 i = 0; i < imageCount; ++i) {
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
		info.attachmentCount = (u32)frameBufferAttachments.size();
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

void Renderer::
SetupPipelineCreation(){
	PRINTVK(2, "  Setting up pipeline creation");
	
	//determines how to group vertices together
	//https://renderdoc.org/vkspec_chunked/chap22.html#VkPipelineInputAssemblyStateCreateInfo
	inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyState.flags = 0;
	inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; 
	inputAssemblyState.primitiveRestartEnable = VK_FALSE;
	
	//how to draw/cull/depth things
	//https://renderdoc.org/vkspec_chunked/chap28.html#VkPipelineRasterizationStateCreateInfo
	rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationState.flags = 0;
	rasterizationState.depthClampEnable = VK_FALSE; //look into for shadowmapping
	rasterizationState.rasterizerDiscardEnable = VK_FALSE;
	rasterizationState.polygonMode = VK_POLYGON_MODE_FILL; //draw mode: fill, wireframe, vertices
	rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE; //VK_FRONT_FACE_COUNTER_CLOCKWISE
	rasterizationState.depthBiasEnable = VK_FALSE;
	rasterizationState.depthBiasConstantFactor = 0.0f;
	rasterizationState.depthBiasClamp = 0.0f;
	rasterizationState.depthBiasSlopeFactor = 0.0f;
	rasterizationState.lineWidth = 1.0f;
	
	//how to combine colors; alpha: options to allow alpha blending
	//https://renderdoc.org/vkspec_chunked/chap30.html#VkPipelineColorBlendAttachmentState
	colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachmentState.blendEnable = VK_FALSE; //alpha: VK_TRUE
	colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; //aplha: VK_BLEND_FACTOR_SRC_ALPHA
	colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; //alpha: VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA
	colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
	
	//container struct for color blend attachments with overall blending constants
	//https://renderdoc.org/vkspec_chunked/chap30.html#VkPipelineColorBlendStateCreateInfo
	colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendState.flags = 0;
	colorBlendState.logicOpEnable = VK_FALSE;
	colorBlendState.logicOp = VK_LOGIC_OP_COPY;
	colorBlendState.attachmentCount = 1;
	colorBlendState.pAttachments = &colorBlendAttachmentState;
	colorBlendState.blendConstants[0] = 0.0f;
	colorBlendState.blendConstants[1] = 0.0f;
	colorBlendState.blendConstants[2] = 0.0f;
	colorBlendState.blendConstants[3] = 0.0f;
	
	//depth testing and discarding
	//https://renderdoc.org/vkspec_chunked/chap29.html#VkPipelineDepthStencilStateCreateInfo
	depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilState.flags = 0;
	depthStencilState.depthTestEnable = VK_TRUE;
	depthStencilState.depthWriteEnable = VK_TRUE;
	depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencilState.depthBoundsTestEnable = VK_FALSE;
	depthStencilState.stencilTestEnable = VK_FALSE;
	depthStencilState.minDepthBounds = 0.0f;
	depthStencilState.maxDepthBounds = 1.0f;
	depthStencilState.stencilTestEnable = VK_FALSE;
	depthStencilState.front = {};
	depthStencilState.back = {};
	
	//container for viewports and scissors
	//https://renderdoc.org/vkspec_chunked/chap27.html#VkPipelineViewportStateCreateInfo
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.flags = 0;
	viewportState.viewportCount = 1;
	viewportState.pViewports = nullptr;
	viewportState.scissorCount = 1;
	viewportState.pScissors = nullptr;
	
	//useful for multisample anti-aliasing (MSAA)
	//https://renderdoc.org/vkspec_chunked/chap28.html#VkPipelineMultisampleStateCreateInfo
	multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleState.flags = 0;
	multisampleState.rasterizationSamples = msaaSamples; //VK_SAMPLE_COUNT_1_BIT to disable anti-aliasing
	multisampleState.sampleShadingEnable = VK_TRUE; //enable sample shading in the pipeline, VK_FALSE to disable
	multisampleState.minSampleShading = .2f; //min fraction for sample shading; closer to one is smoother
	multisampleState.pSampleMask = nullptr;
	multisampleState.alphaToCoverageEnable = VK_FALSE;
	multisampleState.alphaToOneEnable = VK_FALSE;
	
	//dynamic states that can vary in the command buffer
	dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT, 
		VK_DYNAMIC_STATE_SCISSOR, 
		/*VK_DYNAMIC_STATE_LINE_WIDTH*/ 
	};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = u32(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();
	
	//// vertex input flow control ////
	//https://renderdoc.org/vkspec_chunked/chap23.html#VkPipelineVertexInputStateCreateInfo
	//vertex binding and attributes
	VkVertexInputBindingDescription vertexBindingDesc{};
	vertexBindingDesc.binding = 0;
	vertexBindingDesc.stride = sizeof(VertexVk);
	vertexBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	
	VkVertexInputAttributeDescription posDesc{};
	posDesc.binding     = 0;
	posDesc.location    = 0;
	posDesc.format      = VK_FORMAT_R32G32B32_SFLOAT;
	posDesc.offset      = offsetof(VertexVk, pos);
	VkVertexInputAttributeDescription uvDesc{};
	uvDesc.binding      = 0;
	uvDesc.location     = 1;
	uvDesc.format       = VK_FORMAT_R32G32_SFLOAT;
	uvDesc.offset       = offsetof(VertexVk, texCoord);
	VkVertexInputAttributeDescription colorDesc{};
	colorDesc.binding   = 0;
	colorDesc.location  = 2;
	colorDesc.format    = VK_FORMAT_R32G32B32_SFLOAT;
	colorDesc.offset    = offsetof(VertexVk, color);
	VkVertexInputAttributeDescription normalDesc{};
	normalDesc.binding  = 0;
	normalDesc.location = 3;
	normalDesc.format   = VK_FORMAT_R32G32B32_SFLOAT;
	normalDesc.offset   = offsetof(VertexVk, normal);
	
	vertexInputBindings = {
		vertexBindingDesc
	};
	vertexInputAttributes = {
		posDesc, uvDesc, colorDesc, normalDesc
	};
	
	vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputState.flags = 0;
	vertexInputState.vertexBindingDescriptionCount = u32(vertexInputBindings.size());
	vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
	vertexInputState.vertexAttributeDescriptionCount = u32(vertexInputAttributes.size());
	vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();
	
	//base pipeline info and options
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.layout              = pipelineLayout;
	pipelineCreateInfo.renderPass          = renderPass;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineCreateInfo.pRasterizationState = &rasterizationState;
	pipelineCreateInfo.pColorBlendState    = &colorBlendState;
	pipelineCreateInfo.pMultisampleState   = &multisampleState;
	pipelineCreateInfo.pViewportState      = &viewportState;
	pipelineCreateInfo.pDepthStencilState  = &depthStencilState;
	pipelineCreateInfo.pDynamicState       = &dynamicState;
	pipelineCreateInfo.pVertexInputState   = &vertexInputState;
	pipelineCreateInfo.stageCount          = u32(shaderStages.size());
	pipelineCreateInfo.pStages             = shaderStages.data();
}

void Renderer::
CreatePipelines(){
	PRINTVK(2, "  Creating Pipelines");
	TIMER_START(t_p);
	
	//destroy previous pipelines
	if(pipelines.FLAT){ vkDestroyPipeline(device, pipelines.FLAT, nullptr); }
	if(pipelines.PHONG){ vkDestroyPipeline(device, pipelines.PHONG, nullptr); }
	if(pipelines.TWOD){ vkDestroyPipeline(device, pipelines.TWOD, nullptr); }
	if(pipelines.PBR){ vkDestroyPipeline(device, pipelines.PBR, nullptr); }
	if(pipelines.WIREFRAME){ vkDestroyPipeline(device, pipelines.WIREFRAME, nullptr); }
	if(pipelines.LAVALAMP){ vkDestroyPipeline(device, pipelines.LAVALAMP, nullptr); }
	if(pipelines.TESTING0){ vkDestroyPipeline(device, pipelines.TESTING0, nullptr); }
	if(pipelines.TESTING1){ vkDestroyPipeline(device, pipelines.TESTING1, nullptr); }
	
	//destroy previous shader modules
	size_t oldCount = shaderModules.size();
	for(auto& pair : shaderModules){
		vkDestroyShaderModule(device, pair.second, allocator);
	}
	shaderModules.clear(); shaderModules.reserve(oldCount);
	
	//compile uncompiled shaders
	PRINTVK(3, "    Compiling shaders");
	TIMER_START(t_s);
	for(auto& s : GetUncompiledShaders()){ CompileShader(s, false); }
	PRINTVK(3, "    Finished compiling shaders in ", TIMER_END(t_s), "ms");
	
	//flag that this pipelineCreateInfo will be used as a base
	pipelineCreateInfo.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
	pipelineCreateInfo.basePipelineHandle  = VK_NULL_HANDLE;
	pipelineCreateInfo.basePipelineIndex   = -1;
	
	//flat/default pipeline
	//shaderStages[0] = loadShader("base.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[0] = loadShader("flat.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = loadShader("flat.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	ASSERTVK(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.FLAT), "failed to create flat graphics pipeline");
	
	//all other pipelines are derivatives
	pipelineCreateInfo.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;
	pipelineCreateInfo.basePipelineHandle = pipelines.FLAT;
	pipelineCreateInfo.basePipelineIndex = -1; //can either use handle or index, not both (section 9.5 of vulkan spec)
	
	//phong
	shaderStages[0] = loadShader("phong.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = loadShader("phong.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	ASSERTVK(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.PHONG), "failed to create phong graphics pipeline");
	
	//2d
	shaderStages[0] = loadShader("twod.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = loadShader("twod.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	ASSERTVK(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.TWOD), "failed to create twod graphics pipeline");
	
	//pbr
	shaderStages[0] = loadShader("pbr.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = loadShader("pbr.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	ASSERTVK(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.PBR), "failed to create pbr graphics pipeline");
	
	//testing shaders //NOTE(delle) testing shaders should be removed on release
	shaderStages[0] = loadShader("testing0.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = loadShader("testing0.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	ASSERTVK(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.TESTING0), "failed to create testing0 graphics pipeline");
	
	shaderStages[0] = loadShader("testing1.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = loadShader("testing1.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	ASSERTVK(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.TESTING1), "failed to create testing1 graphics pipeline");
	
	//wireframe
	if(deviceFeatures.fillModeNonSolid){
		rasterizationState.polygonMode = VK_POLYGON_MODE_LINE;
		rasterizationState.cullMode = VK_CULL_MODE_NONE;
		depthStencilState.depthTestEnable = VK_FALSE;
		
		shaderStages[0] = loadShader("wireframe.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader("wireframe.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		ASSERTVK(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.WIREFRAME), "failed to create wireframe graphics pipeline");
		
		rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
		depthStencilState.depthTestEnable = VK_TRUE;
	}
	
	//lavalamp
	shaderStages[0] = loadShader("lavalamp.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = loadShader("lavalamp.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	ASSERTVK(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.LAVALAMP), "failed to create lavalamp graphics pipeline");
	
	PRINTVK(2, "  Finished creating pipelines in ", TIMER_END(t_p), "ms");
}

void Renderer::
BuildCommandBuffers() {
	//PRINTVK(2, "  Building Command Buffers");
	VkCommandBufferBeginInfo cmdBufferInfo{};
	cmdBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufferInfo.flags = 0;
	cmdBufferInfo.pInheritanceInfo = nullptr;
	
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = extent;
	renderPassInfo.clearValueCount = (u32)clearValues.size();
	renderPassInfo.pClearValues = clearValues.data();
	
	VkViewport viewport{};
	viewport.width    = (float)width;
	viewport.height   = (float)height;
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;
	
	VkRect2D scissor{}; //TODO(delle,Re) letterboxing settings here
	scissor.extent.width  = width;
	scissor.extent.height = height;
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	
	//TODO(delle,ReOpVu) figure out why we are doing it for all frames
	for(int i = 0; i < imageCount; ++i){
		renderPassInfo.framebuffer = frames[i].framebuffer;
		ASSERTVK(vkBeginCommandBuffer(frames[i].commandBuffer, &cmdBufferInfo), "failed to begin recording command buffer");
		vkCmdBeginRenderPass(frames[i].commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdSetViewport(frames[i].commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(frames[i].commandBuffer, 0, 1, &scissor);
		//bind scene matrices descriptor to set 0
		vkCmdBindDescriptorSets(frames[i].commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &sceneDescriptorSet, 0, nullptr);
		//// draw stuff below here ////
		
		VkDeviceSize offsets[1] = { 0 };
		//index buffer
		vkCmdBindVertexBuffers(frames[i].commandBuffer, 0, 1, &vertices.buffer, offsets);
		//index buffer
		vkCmdBindIndexBuffer(frames[i].commandBuffer, indices.buffer, 0, VK_INDEX_TYPE_UINT32);
		
		if(settings.wireframeOnly){ //draw all with wireframe shader
			vkCmdBindPipeline(frames[i].commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.WIREFRAME);
			for(MeshVk& mesh : meshes){
				if(mesh.visible && mesh.primitives.size() > 0){
					//push the mesh's model matrix to the vertex shader
					vkCmdPushConstants(frames[i].commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &mesh.modelMatrix);
					
					for (PrimitiveVk& primitive : mesh.primitives) {
						if (primitive.indexCount > 0) {
							vkCmdDrawIndexed(frames[i].commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
							stats.drawnIndices += primitive.indexCount;
						}
					}
				}
			}
		}else{
			for(MeshVk& mesh : meshes){
				if(mesh.visible && mesh.primitives.size() > 0){
					//push the mesh's model matrix to the vertex shader
					vkCmdPushConstants(frames[i].commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &mesh.modelMatrix);
					
					for (PrimitiveVk& primitive : mesh.primitives) {
						if (primitive.indexCount > 0) {
							MaterialVk& material = materials[primitive.materialIndex];
							// Bind the pipeline for the primitive's material
							vkCmdBindPipeline(frames[i].commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material.pipeline);
							vkCmdBindDescriptorSets(frames[i].commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &material.descriptorSet, 0, nullptr);
							vkCmdDrawIndexed(frames[i].commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
							stats.drawnIndices += primitive.indexCount;
							
							//TODO(delle,OpVu) this might slow the loop down with ifs
							if(settings.wireframe && material.pipeline != pipelines.WIREFRAME){
								vkCmdBindPipeline(frames[i].commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.WIREFRAME);
								vkCmdDrawIndexed(frames[i].commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
								stats.drawnIndices += primitive.indexCount;
							}
						}
					}
				}
			}
		}
		
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


//TODO(delle,ReOpVu) maybe only do one mapping at buffer creation, see: gltfscenerendering.cpp, line:600
void Renderer::
UpdateUniformBuffer(){
	if(!shaderData.freeze){
		//PRINTVK(2, "  Updating Uniform Buffer     \n");
		shaderData.values.time = time->totalTime;
		shaderData.values.swidth = (glm::f32)extent.width;
		shaderData.values.sheight = (glm::f32)extent.height;
		shaderData.values.lightPos = glm::vec4(1.0f, -3.f, -1.0f, 1.0f);
		
		//map shader data to uniform buffer
		void* data;
		vkMapMemory(device, shaderData.uniformBufferMemory, 0, sizeof(shaderData.values), 0, &data);{
			memcpy(data, &shaderData.values, sizeof(shaderData.values));
		}vkUnmapMemory(device, shaderData.uniformBufferMemory);
	}
}

///////////////////////////
//// utility functions ////
///////////////////////////

bool Renderer::
checkValidationLayerSupport() {
	PRINTVK(3, "    Checking Validation Layer Support");
	u32 layerCount;
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

std::vector<const char*> Renderer::
getRequiredExtensions() {
	PRINTVK(3, "    Getting Required Extensions");
	u32 glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
	if(enableValidationLayers) { extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); }
	return extensions;
}

void Renderer::
populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
	PRINTVK(3, "    Populating Debug Messenger CreateInfo");
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = /*VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |*/ VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
}

bool Renderer::
isDeviceSuitable(VkPhysicalDevice device) {
	PRINTVK(3, "    Checking Validation Layer Support");
	QueueFamilyIndices indices = findQueueFamilies(device);
	bool extensionsSupported = checkDeviceExtensionSupport(device);
	bool swapChainAdequate = false;
	if(extensionsSupported) {
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}
	
	return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

QueueFamilyIndices Renderer::
findQueueFamilies(VkPhysicalDevice device) {
	PRINTVK(3, "    Finding Queue Families");
	QueueFamilyIndices indices;
	
	u32 queueFamilyCount = 0;
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

bool Renderer::
checkDeviceExtensionSupport(VkPhysicalDevice device) {
	PRINTVK(3, "    Checking Device Extension Support");
	u32 extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
	
	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}
	
	return requiredExtensions.empty();
}

SwapChainSupportDetails Renderer::
querySwapChainSupport(VkPhysicalDevice device) {
	PRINTVK(3, "    Querying SwapChain Support");
	SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
	
	u32 formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
	}
	
	u32 presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
	}
	
	return details;
}

VkSampleCountFlagBits Renderer::
getMaxUsableSampleCount() {
	PRINTVK(3, "    Getting Max Usable Sample Count");
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

VkSurfaceFormatKHR Renderer::
chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
	PRINTVK(3, "    Choosing Swap Surface Format");
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}
	return availableFormats[0];
}

VkPresentModeKHR Renderer::
chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
	PRINTVK(3, "    Choosing Swap Present Mode");
	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentMode;
		}
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Renderer::
chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
	PRINTVK(3, "    Choosing Swap Extent");
	if (capabilities.currentExtent.width != UINT32_MAX) {
		return capabilities.currentExtent;
	} else {
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		
		VkExtent2D actualExtent = { static_cast<u32>(width), static_cast<u32>(height) };
		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
		
		return actualExtent;
	}
}

VkImageView Renderer::
createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, u32 mipLevels) {
	PRINTVK(4, "      Creating Image View");
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

VkFormat Renderer::
findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
	PRINTVK(4, "      Finding supported image formats");
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

VkFormat Renderer::
findDepthFormat() {
	return findSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

u32 Renderer::
findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties) {
	PRINTVK(4, "      Finding Memory Types");
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
	
	for (u32 i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}
	ASSERT(false, "failed to find suitable memory type"); //error out if no suitable memory found
}

void Renderer::
createImage(u32 width, u32 height, u32 mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
	PRINTVK(4, "      Creating Image");
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

void Renderer::
CreateOrResizeBuffer(VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkDeviceSize& bufferSize, size_t newSize, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties){
	PRINTVK(4, "      Creating or Resizing Buffer");
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

void Renderer::
CreateAndMapBuffer(VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkDeviceSize& bufferSize, size_t newSize, void* data, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties){
	PRINTVK(4, "      Creating and Mapping Buffer");
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
				VkMappedMemoryRange mappedRange{};
				mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
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

VkCommandBuffer Renderer::
beginSingleTimeCommands() {
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

void Renderer::
endSingleTimeCommands(VkCommandBuffer commandBuffer) {
	vkEndCommandBuffer(commandBuffer);
	
	//TODO(delle,ReOpVu) maybe add a fence to ensure the buffer has finished executing
	//instead of waiting for queue to be idle, see: sascha/VulkanDevice.cpp:508
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	
	vkQueueWaitIdle(graphicsQueue);
	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void Renderer::
transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, u32 mipLevels) {
	PRINTVK(4, "      Transitioning Image Layout");
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
	}else {
		ASSERT(false, "unsupported layout transition");
	}
	
	vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	
	endSingleTimeCommands(commandBuffer);
}

void Renderer::
copyBufferToImage(VkBuffer buffer, VkImage image, u32 width, u32 height) {
	PRINTVK(4, "      Copying Buffer To Image");
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

void Renderer::
generateMipmaps(VkImage image, VkFormat imageFormat, i32 texWidth, i32 texHeight, u32 mipLevels) {
	PRINTVK(4, "      Creating Image Mipmaps");
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
	
	i32 mipWidth = texWidth;
	i32 mipHeight = texHeight;
	for (u32 i = 1; i < mipLevels; i++) {
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

void Renderer::
copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();
	
	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
	
	endSingleTimeCommands(commandBuffer);
}

void RemakePipeline(VkPipeline pipeline);

//TODO(delle,ReOp) maybe optimize this by simply doing: &pipelines + shader*sizeof(pipelines.FLAT)
VkPipeline Renderer::
GetPipelineFromShader(u32 shader){
	switch(shader){
		case(Shader::FLAT):default: { return pipelines.FLAT;      };
		case(Shader::PHONG):        { return pipelines.PHONG;     };
		case(Shader::TWOD):         { return pipelines.TWOD;      };
		case(Shader::PBR):          { return pipelines.PBR;       };
		case(Shader::WIREFRAME):    { return pipelines.WIREFRAME; };
		case(Shader::LAVALAMP):     { return pipelines.LAVALAMP;  };
		case(Shader::TESTING0):     { return pipelines.TESTING0;  };
		case(Shader::TESTING1):     { return pipelines.TESTING1;  };
	}
}


VkPipelineShaderStageCreateInfo Renderer::
loadShader(std::string fileName, VkShaderStageFlagBits stage) {
	PRINTVK(3, "    Loading shader: ", fileName);
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
	std::vector<char> code = deshi::readFileBinary(deshi::dirShaders() + fileName);
	VkShaderModuleCreateInfo moduleInfo{};
	moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	moduleInfo.codeSize = code.size();
	moduleInfo.pCode = (u32*)code.data();
	
	VkShaderModule shaderModule;
	ASSERTVK(vkCreateShaderModule(device, &moduleInfo, allocator, &shaderModule), "failed to create shader module");
	shaderStage.module = shaderModule;
	
	shaderModules.push_back(std::make_pair(shaderName, shaderStage.module));
	return shaderStage;
}

VkPipelineShaderStageCreateInfo Renderer::
CompileAndLoadShader(std::string filename, VkShaderStageFlagBits stage, bool optimize) {
	PRINTVK(3, "    Compiling and loading shader: ", filename);
	//check if file exists
	std::filesystem::path entry(deshi::dirShaders() + filename);
	if(std::filesystem::exists(entry)){
		std::string ext = entry.extension().string();
		std::string filename = entry.filename().string();
		
		//setup shader compiler
		shaderc_compiler_t compiler = shaderc_compiler_initialize();
		shaderc_compile_options_t options = shaderc_compile_options_initialize();
		if(optimize) shaderc_compile_options_set_optimization_level(options, shaderc_optimization_level_performance);
		
		std::vector<char> code = deshi::readFileBinary(deshi::dirShaders() + filename); //read shader code
		
		//try compile from GLSL to SPIR-V binary
		shaderc_compilation_result_t result;
		if(ext.compare(".vert") == 0){
			result = shaderc_compile_into_spv(compiler, code.data(), code.size(), shaderc_glsl_vertex_shader, 
											  filename.c_str(), "main", options);
		}else if(ext.compare(".frag") == 0){
			result = shaderc_compile_into_spv(compiler, code.data(), code.size(), shaderc_glsl_fragment_shader, 
											  filename.c_str(), "main", options);
		}else{ ASSERT(false, "unsupported shader"); }
		
		//check for errors
		if(!result){ PRINT("[ERROR]"<< filename <<": Shader compiler returned a null result"); ASSERT(false, ""); }
		if(shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success){
			PRINT("[ERROR]"<< filename <<": "<< shaderc_result_get_error_message(result)); ASSERT(false, "");
		}
		
		//create shader module
		VkShaderModuleCreateInfo moduleInfo{};
		moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleInfo.codeSize = shaderc_result_get_length(result);
		moduleInfo.pCode = (u32*)shaderc_result_get_bytes(result);
		
		VkShaderModule shaderModule;
		ASSERTVK(vkCreateShaderModule(device, &moduleInfo, allocator, &shaderModule), "failed to create shader module");
		
		//setup shader stage create info
		VkPipelineShaderStageCreateInfo shaderStage{};
		shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStage.stage = stage;
		shaderStage.pName = "main";
		shaderStage.module = shaderModule;
		
		//cleanup and return
		shaderc_result_release(result);
		return shaderStage;
	}else{
		ASSERT(false, "failed to load shader module b/c file does not exist");
	}
}


void Renderer::
CompileAllShaders(bool optimize){
	//setup shader compiler
	shaderc_compiler_t compiler = shaderc_compiler_initialize();
	shaderc_compile_options_t options = shaderc_compile_options_initialize();
	if(optimize) shaderc_compile_options_set_optimization_level(options, shaderc_optimization_level_performance);
	
	//loop thru all files in the shaders dir, compile the shaders, write them to .spv files
	for(auto& entry : std::filesystem::directory_iterator(deshi::dirShaders())){
		std::string ext = entry.path().extension().string();
		std::string filename = entry.path().filename().string();
		
		if(ext.compare(".spv") == 0) continue; //early out if .spv
		std::vector<char> code = deshi::readFileBinary(entry.path().string()); //read shader code
		PRINTVK(4, "      Compiling shader: ", filename);
		
		//try compile from GLSL to SPIR-V binary
		shaderc_compilation_result_t result;
		if(ext.compare(".vert") == 0){
			result = shaderc_compile_into_spv(compiler, code.data(), code.size(), shaderc_glsl_vertex_shader, 
											  filename.c_str(), "main", options);
		}else if(ext.compare(".frag") == 0){
			result = shaderc_compile_into_spv(compiler, code.data(), code.size(), shaderc_glsl_fragment_shader, 
											  filename.c_str(), "main", options);
		}else{ continue; }
		
		//check for errors
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

//TODO(delle,ReCl) clean this up
std::vector<std::string> Renderer::
GetUncompiledShaders(){
	std::vector<std::string> compiled;
	for(auto& entry : std::filesystem::directory_iterator(deshi::dirShaders())){
		if(entry.path().extension() == ".spv"){
			compiled.push_back(entry.path().stem().string());
		}
	}
	
	std::vector<std::string> files;
	for(auto& entry : std::filesystem::directory_iterator(deshi::dirShaders())){
		if(entry.path().extension() == ".spv"){ continue; }
		bool good = true;
		for(auto& s : compiled){
			if(entry.path().filename().string().compare(s) == 0){ good = false; break; }
		}
		if(good) files.push_back(entry.path().filename().string());
	}
	return files;
}

void Renderer::
CompileShader(std::string& filename, bool optimize){
	PRINTVK(3, "    Compiling shader: ", filename);
	std::filesystem::path entry(deshi::dirShaders() + filename);
	if(std::filesystem::exists(entry)){
		std::string ext = entry.extension().string();
		std::string filename = entry.filename().string();
		
		//setup shader compiler
		shaderc_compiler_t compiler = shaderc_compiler_initialize();
		shaderc_compile_options_t options = shaderc_compile_options_initialize();
		if(optimize) shaderc_compile_options_set_optimization_level(options, shaderc_optimization_level_performance);
		
		std::vector<char> code = deshi::readFileBinary(deshi::dirShaders() + filename); //read shader code
		
		//try compile from GLSL to SPIR-V binary
		shaderc_compilation_result_t result;
		if(ext.compare(".vert") == 0){
			result = shaderc_compile_into_spv(compiler, code.data(), code.size(), shaderc_glsl_vertex_shader, 
											  filename.c_str(), "main", options);
		}else if(ext.compare(".frag") == 0){
			result = shaderc_compile_into_spv(compiler, code.data(), code.size(), shaderc_glsl_fragment_shader, 
											  filename.c_str(), "main", options);
		}else{ return; }
		
		//check for errors
		if(!result){ PRINT("[ERROR]"<< filename <<": Shader compiler returned a null result"); return; }
		if(shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success){
			PRINT("[ERROR]"<< filename <<": "<< shaderc_result_get_error_message(result)); return;
		}
		
		//create or overwrite .spv files
		std::ofstream outFile(entry.string() + ".spv", std::ios::out | std::ios::binary | std::ios::trunc);
		ASSERT(outFile.is_open(), "failed to open file");
		outFile.write(shaderc_result_get_bytes(result), shaderc_result_get_length(result));
		
		//cleanup file and compiler results
		outFile.close();
		shaderc_result_release(result);
	}else{
		PRINT("[ERROR] failed to open file: " << filename);
	}
}

void Renderer::
UpdateMaterialPipelines(){
	PRINTVK(4, "      Updating material pipelines");
	for(auto& mat : materials){
		mat.pipeline = GetPipelineFromShader(mat.shader);
	}
}

VKAPI_ATTR VkBool32 VKAPI_CALL Renderer::
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
	//TODO(sushi, Con) fix console color formatting for this case
	g_console->PushConsole(TOSTRING("[c:error]", pCallbackData->pMessage, "[c]"));
	PRINT(pCallbackData->pMessage);
	return VK_FALSE;
}


VkResult Renderer::
CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

int Renderer::
GetMinImageCountFromPresentMode(VkPresentModeKHR mode) {
	switch(mode) {
		case(VK_PRESENT_MODE_MAILBOX_KHR):      { return 3; }
		case(VK_PRESENT_MODE_FIFO_KHR):         { return 2; }
		case(VK_PRESENT_MODE_FIFO_RELAXED_KHR): { return 2; }
		case(VK_PRESENT_MODE_IMMEDIATE_KHR):    { return 1; }
		default: { return -1; }
	}
	return -1;
}

void Renderer::
framebufferResizeCallback(GLFWwindow* window, int width, int height) {
	auto app = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
	app->remakeWindow = true;
}