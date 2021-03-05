#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 1, binding = 0) uniform sampler2D albedoSampler;
layout(set = 1, binding = 0) uniform sampler2D normalSampler;
layout(set = 1, binding = 0) uniform sampler2D specularSampler;
layout(set = 1, binding = 0) uniform sampler2D lightSampler;

//layout(constant_id = 0) const bool ALPHA_MASK = false;
//layout(constant_id = 1) const float ALPHA_THRESHOLD = 0.0f;

layout(location = 0) in vec3 inColor;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec4 outColor;

void main() {
	//TODO(r,delle) pbr shading, see pbrtexture in saschawillems' stuff
	outColor = texture(albedoSampler, inTexCoord);
	//outColor = vec4(inColor, 1.0);
}