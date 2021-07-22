#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform PushConsts{
	vec2 scale;
	vec2 translate;
} push;

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec4 inColor;

out gl_PerVertex { vec4 gl_Position; };
layout(location = 0) out vec2 outUV;
layout(location = 1) out vec4 outColor;

void main() {
    gl_Position = vec4(push.scale * inPos + push.translate, 0.0, 1.0);
    outUV = inUV;
	outColor = pow(inColor, vec4(2.2f, 2.2f, 2.2f, 2.2f));
}