#version 450

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec4 in_color;

layout(binding = 0) uniform UBO {
	mat4 view;
	mat4 proj;
} ubo;

layout(location = 0) out vec4 out_color;

void main() {
	gl_Position = ubo.proj * ubo.view * vec4(in_pos, 1.f);
	out_color = in_color;
}
