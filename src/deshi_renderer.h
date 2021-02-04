#ifndef DESHI_RENDERER_H
#define DESHI_RENDERER_H

#include "deshi_defines.h"
#include "deshi_glfw.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <boost/optional.hpp>

struct Renderer{
	void Init(Window* window) = 0;
	void Draw() = 0;
	void Cleanup() = 0;
};

struct Renderer_Vulkan : public Renderer{
	
	
	void Init(Window* window) override{
		
	}
	
	void Draw() override{
		
	}
	
	void Cleanup() override{
		
	}
};

#endif //DESHI_RENDERER_H
