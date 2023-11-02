#version 450

#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_light_vector_inverse;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec4 in_color;

layout(location = 0) out vec4 out_color;

void main() {
	out_color = in_color;
	out_color.xyz *= clamp(dot(normalize(in_light_vector_inverse), normalize(in_normal)) * 0.7, 0.25, 1.0);
}
