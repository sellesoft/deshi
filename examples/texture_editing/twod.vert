#version 450

layout(location = 0) in vec2 in_pos;
layout(location = 1) in vec2 in_uv;

layout(location = 0) out vec2 out_uv;

layout(push_constant) uniform push_consts{
	vec2 scale;
	vec2 translation;
}push;

void main() {
	gl_Position = vec4(push.scale*in_pos+push.translation, 0, 1);
	out_uv = in_uv;
}
