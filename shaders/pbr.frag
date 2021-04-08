#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 1, binding = 0) uniform sampler2D albedoSampler;
layout(set = 1, binding = 1) uniform sampler2D normalSampler;
layout(set = 1, binding = 2) uniform sampler2D specularSampler;
layout(set = 1, binding = 3) uniform sampler2D lightSampler;

//layout(constant_id = 0) const bool ALPHA_MASK = false;
//layout(constant_id = 1) const float ALPHA_THRESHOLD = 0.0f;

layout(location = 0) in vec3 inColor;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inPosition;


layout(location = 0) out vec4 outColor;

void main() {
	//TODO(delle,r) pbr shading, see pbrtexture in saschawillems' stuff
	vec2 tc = inTexCoord;
	tc.y *= -1;
	vec4 tex = texture(albedoSampler, tc);
	vec3 light = vec3(2,2,2);
	vec4 lightcolor = vec4(
					  clamp(dot(normalize(light - inPosition), -inNormal), .1f, 1),
			bhbhbi		  clamp(dot(normalize(light - inPosition), -inNormal), .1f, 1),
					  clamp(dot(normalize(light - inPosition), -inNormal), .1f, 1), 1);
	
	outColor = tex * lightcolor;


	//outColor = vec4(inColor, 1.0);
}