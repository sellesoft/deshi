#version 450

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec4 in_color;
layout(location = 3) in vec3 in_normal;

layout(binding = 0) uniform UBO {
	mat4 proj;
	mat4 view;
	mat4 model;
	mat4 light_space;
	vec4 light_pos;
	float z_near;
	float z_far;
} ubo;

layout(location = 0) out vec3 out_normal;
layout(location = 1) out vec4 out_color;
layout(location = 2) out vec3 out_view_vector;
layout(location = 3) out vec3 out_light_vector;
layout(location = 4) out vec4 out_shadow_coord;

const mat4 bias_mat = mat4(
		0.5, 0.0, 0.0, 0.0,
		0.0, 0.5, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		0.5, 0.5, 0.0, 1.0);

void main() {
	out_color = in_color;
	out_normal = in_normal;

	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(in_pos.xyz, 1.0);

	vec4 pos = ubo.model * vec4(in_pos, 1.0);
	out_normal = mat3(ubo.model) * in_normal;
	out_light_vector = normalize(ubo.light_pos.xyz - in_pos);
	out_view_vector = -pos.xyz;

	out_shadow_coord = (bias_mat * ubo.light_space * ubo.model) * vec4(in_pos, 1.0);
}
