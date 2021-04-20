#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBufferObject{
	mat4 view;
	mat4 proj;
	vec4 lightPos;
	vec4 viewPos;
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


void main() {
    vec3 light = vec3(ubo.viewPos);

	vec3 normal = mat3(primitive.model) * inNormal;

    gl_Position = ubo.proj * ubo.view * primitive.model * vec4(inPosition.xyz, 1.0);
	outColor = vec3(clamp(dot(normalize(light - inPosition), normal), .1f, 1),
					clamp(dot(normalize(light - inPosition), normal), .1f, 1),
					clamp(dot(normalize(light - inPosition), normal), .1f, 1));
    //outColor = inColor;
	outTexCoord = inTexCoord;
	outNormal = normal;
}