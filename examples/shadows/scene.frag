#version 450

layout(binding = 1) uniform  sampler2D shadow_map;

layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec4 in_color;
layout(location = 2) in vec3 in_view_vector;
layout(location = 3) in vec3 in_light_vector;
layout(location = 4) in vec4 in_shadow_coord;
layout(location = 5) in float in_time;

const int enable_pcf = 0;

layout(location = 0) out vec4 out_color;

#define ambient 0.2

float texture_proj(vec4 shadow_coord, vec2 offset) {
	float shadow = 1.0;
	if(shadow_coord.z > -1.0 && shadow_coord.z < 1.0) {
		float dist = texture(shadow_map, shadow_coord.xy + offset).r;
		if(shadow_coord.w > 0.0 && dist < shadow_coord.z) {
			shadow = ambient;
		}
	}
	 return shadow;
}

void main() {
	float shadow = texture_proj(in_shadow_coord/in_shadow_coord.w, vec2(0.0));
	
	vec3 n = normalize(in_normal);
	vec3 l = normalize(in_light_vector);
	vec3 v = normalize(in_view_vector);
	vec3 r = normalize(-reflect(l, n));
	vec3 diffuse = max(dot(n, l), ambient) * in_color.xyz;

	out_color = vec4(in_color.xyz*shadow, 1.0);
}
