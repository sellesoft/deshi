#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform push_consts{
	vec2 scale;
	vec2 translation;
}push;

layout(location = 0) in vec2 in_pos;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec4 in_color;

layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec4 out_color;

void main() {
	gl_Position = vec4(push.scale * in_pos + push.translation, 0.0, 1.0);
	out_uv = in_uv;
	out_color = in_color;
}
