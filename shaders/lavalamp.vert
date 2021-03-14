#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBufferObject{
	mat4 view;
	mat4 proj;
	vec4 lightPos;
	vec4 viewPos;
	float time;
	float swidth;
	float sheight;
} ubo;

layout(push_constant) uniform PushConsts{
	mat4 model;
} primitive;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec2 outTexCoord;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out float time;
layout(location = 4) out float swidth;
layout(location = 5) out float sheight;


void main() {
	
	gl_Position = ubo.proj * ubo.view * primitive.model * vec4(inPosition.xyz, 1.0);
	
	time = ubo.time;

	//pos.w += (20 * sin(time) + 20)/2;

    //gl_Position = pos + 0.1 * sin(time * gl_VertexIndex / 2) + 0.1 * cos(time * gl_VertexIndex / 2);
    outColor = inColor;
	outTexCoord = inTexCoord;
	outNormal = mat3(primitive.model) * inNormal;
	
	swidth = ubo.swidth;
	sheight = ubo.sheight;
	
}