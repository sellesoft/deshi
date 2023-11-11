#version 450

#extension GL_ARB_separate_shader_objects : enable

layout(set=0, binding=1) uniform sampler2D tex_sampler;

layout(location = 0) in vec3 in_light_vector_inverse;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec4 in_color;
layout(location = 3) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

void main() {
	out_color = texture(tex_sampler, in_uv);
}
