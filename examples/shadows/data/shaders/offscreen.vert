#version 450

layout(location = 0) in vec3 in_pos;

layout(binding = 0) uniform UBO {
	mat4 depth_mvp;
} ubo;

void main() {
	gl_Position = ubo.depth_mvp * vec4(in_pos, 1.0);
}
