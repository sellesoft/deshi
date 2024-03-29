#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec4 in_color;
layout(location = 3) in vec3 in_normal;

layout(binding = 0) uniform UBO {
	mat4 view;
	mat4 proj;
} ubo;

layout(push_constant) uniform PC {
	mat4 transform;
} pc;


layout(location = 0) out vec2 out_uv;

void main() {
	gl_Position = ubo.proj * ubo.view * pc.transform * vec4(in_pos, 1.f);
	out_uv = in_uv;
}
