#version 450

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec4 in_color;
layout(location = 3) in vec3 in_normal;

layout(binding = 0) uniform UBO {
	mat4 proj;
	mat4 view;
	mat4 light_proj;
	mat4 light_view;
	vec4 light_pos;
	float z_near;
	float z_far;
	float time;
} ubo;

layout(push_constant) uniform PC {
	mat4 transformation;
	vec3 color;
} pc;

layout(location = 0) out vec3 out_normal;
layout(location = 1) out vec4 out_color;
layout(location = 2) out vec3 out_view_vector;
layout(location = 3) out vec3 out_light_vector;
layout(location = 4) out vec4 out_shadow_coord;
layout(location = 5) out float out_time;

const mat4 bias_mat = mat4(
		0.5, 0.0, 0.0, 0.0,
		0.0, 0.5, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		0.5, 0.5, 0.0, 1.0);

void main() {
	out_color = vec4(pc.color, 1);
	out_normal = in_normal;
	mat4 light_space = ubo.light_proj * ubo.light_view *mat4(1.f);

	gl_Position = ubo.proj * ubo.view * pc.transformation * vec4(in_pos, 1.0);

	vec4 pos = pc.transformation * vec4(in_pos, 1.0);
	out_normal = mat3(pc.transformation) * in_normal;
	out_light_vector = normalize(ubo.light_pos.xyz - in_pos);
	out_view_vector = -pos.xyz;

	out_shadow_coord = (bias_mat * light_space * pc.transformation) * vec4(in_pos, 1.0);
	out_time = ubo.time;
}
