#version 450

layout(binding = 1) uniform sampler2D sampler_color;

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform UBO {
	mat4 proj;
	mat4 view;
	mat4 light_proj;
	mat4 light_view;
	mat4 light_pos;
	float z_near;
	float z_far;
} ubo;

layout(push_constant) uniform PC {
	mat4 transformation;
	vec3 color;
} pc;

float linearize_depth(float depth) {
	float n = ubo.z_near;
	float f = ubo.z_far;
	float z = depth;
	return (2.0 * n) / (f + n - z * (f - n));
}

void main() {
	float depth = texture(sampler_color, in_uv).r;
	out_color = vec4(vec3(1.0-linearize_depth(depth)), 1.0);
}
