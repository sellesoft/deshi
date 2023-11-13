#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 1, binding = 0) uniform sampler2D tex;

layout(set = 1, binding = 1) uniform UBO {
	float time;
	vec4 mix;
} ubo;

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

float quant(float v, float res) {
	return floor(v * res) / res;
}

float rand(vec2 uv) {
	return fract(sin(dot(uv, vec2(12.9898,78.233)))*43758.5453123);
}

vec4 dither() {
	float s = ubo.time / 1000.f;

	vec2 tex_size = textureSize(tex, 0);
	vec2 tex_inc = vec2(1/tex_size.x, 1/tex_size.y);

	vec2 tc = in_uv;
	if(tc.x == 0 || tc.y == 0 || tc.x == 1 || tc.y == 1) {
		return texture(tex, tc);
	}

	float seed_float = rand(vec2(s, s/2.f));
	tc = vec2(quant(tc.x, tex_size.x), quant(tc.y, tex_size.y));
	tc.y *= -1;
	vec2 rand_control = vec2(tc.x + quant(s, 5), tc.y + quant(s, 5));
	float random = rand(rand_control);

	vec2 next = tc;
	int len = 2;
	int multiplier = int(rand(tc)*len);
	if(random <= 0.25) {
		next.x += tex_inc.x * multiplier;
	} else if(random <= 0.5) {
		next.y += tex_inc.y * multiplier;
	} else if(random <= 0.75) {
		next.x -= tex_inc.x * multiplier;
	} else {
		next.y -= tex_inc.y * multiplier;
	}

	vec4 curr_col = texture(tex, tc);
	vec4 next_col = texture(tex, next);

	if(next_col != curr_col) {
		return next_col;
	} else {
		return curr_col;
	}
}

void main() {
	out_color = dither() * ubo.mix;
	out_color.a = 1;
}
