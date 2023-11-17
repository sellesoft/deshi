#version 450

layout(location = 0) in vec3 in_pos;

layout(binding = 0) uniform UBO {
	mat4 proj;
	mat4 view;
} ubo;

layout(push_constant) uniform PC {
	mat4 model;
} pc;

void main() {
	gl_Position = ubo.proj * ubo.view * pc.model * vec4(in_pos, 1.0);
}
