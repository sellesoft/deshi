#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferVk{
	mat4 view;
	mat4 proj;
	vec4 lightPos;
	vec4 viewPos;
} ubo;

layout(push_constant) uniform PushConsts{
	mat4 model;
} primitive;


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec3 outNormal;

void main() {
    gl_Position = ubo.proj * ubo.view * primitive.model * vec4(inPosition.xyz, 1.0);
    outColor = inColor;
	outNormal = mat3(primitive.model) * inNormal;
}