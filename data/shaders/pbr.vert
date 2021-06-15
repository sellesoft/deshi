#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_debug_printf : enable

layout(set = 0, binding = 0) uniform UniformBufferObject{
	mat4 view;
	mat4 proj;
	vec4 lightPos;
	vec4 viewPos;
	float time;
	float width;
	float height;
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
layout(location = 3) out vec3 outPosition;
layout(location = 4) out float time;
layout(location = 5) out vec3 fragPos;
layout(location = 6) out float width;
layout(location = 7) out float height;




void main() {
    gl_Position = ubo.proj * ubo.view * primitive.model * vec4(inPosition.xyz, 1.0);
    outColor = inColor;
	outTexCoord = inTexCoord;
	outNormal = mat3(primitive.model) * inNormal;
	outPosition = inPosition;
	time = ubo.time;
	width = ubo.width;
	height = ubo.height;
	fragPos = vec3(primitive.model * vec4(inPosition, 1));
}