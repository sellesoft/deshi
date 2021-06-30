#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBufferObject{
	mat4  view;
	mat4  proj;
	vec4  lights[10];
	vec4  viewPos;
	vec2  screen;
	vec2  mousepos;
	vec3  mouseWorld;
	float time;
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
layout(location = 3) out float outLightBrightness;
layout(location = 4) out vec3 outWorldPos;
layout(location = 5) out vec3 viewPosition;
layout(location = 6) out vec4 outLights[10];

void main() {
	
	viewPosition = (ubo.view * primitive.model * vec4(inPosition.xyz, 1.0)).xyz;
    gl_Position = ubo.proj * ubo.view * primitive.model * vec4(inPosition.xyz, 1.0);
    outColor = inColor;
	outTexCoord = inTexCoord;
	outNormal = mat3(primitive.model) * inNormal;
	//outLightPos = ubo.lightPos.xyz;
	//outLightBrightness = ubo.lightPos.w;
	outLights = ubo.lights;
	
	outWorldPos = vec3(primitive.model * vec4(inPosition.xyz, 1.0));
}