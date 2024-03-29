#version 450

#extension GL_ARB_separate_shader_objects : enable

layout(std140, set = 0, binding = 0) uniform UBO {
	mat4 view;
	mat4 proj;
	vec4 camera_pos;
	vec2 screen_dims;
} ubo;

layout(push_constant) uniform PC {
	mat4 model;
} primitive;

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec4 in_color;
layout(location = 3) in vec3 in_normal;

layout(location = 0) out vec3 out_light_vector_inverse;
layout(location = 1) flat out vec3 out_normal;
layout(location = 2) out vec4 out_color;
layout(location = 3) out vec2 out_uv;

void main() {
	gl_Position = ubo.proj * ubo.view * primitive.model * vec4(in_pos.xyz, 1.0);
	out_light_vector_inverse =  gl_Position.xyz + vec3(2,2,2);
	out_normal = mat3(primitive.model) * in_normal;
	out_color = in_color;
	out_uv = in_uv;
}
