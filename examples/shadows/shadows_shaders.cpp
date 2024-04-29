/* shadows example shaders
Index:
@offscreen
@scene
@debug_quad

Ref:
https://github.com/SaschaWillems/Vulkan/tree/master/shaders/glsl/shadowmapping  (Sascha Willems)
*/


#include "core/baked/shaders.h"


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @offscreen


local str8 baked_shader_offscreen_vert = STR8(
DESHI_BAKED_SHADERS_VERSION
DESHI_BAKED_SHADERS_VERTEX_EXTENSIONS
"\n"
DESHI_BAKED_SHADERS_BUFFER_HEADER("OffscreenUBO", 0, 0)
"	mat4 proj;\n"
"	mat4 view;\n"
DESHI_BAKED_SHADERS_BUFFER_FOOTER("ubo")
"\n"
DESHI_BAKED_SHADERS_PUSH_CONSTANT_HEADER("PushConstant")
"	mat4 model;\n"
"	vec3 color;\n"
DESHI_BAKED_SHADERS_PUSH_CONSTANT_FOOTER("push")
"\n"
"layout(location = 0) in vec3 in_pos;\n"
"\n"
"void main(){\n"
"	gl_Position = ubo.proj * ubo.view * push.model * vec4(in_pos, 1.0);\n"
"}\n"
);


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @scene


local str8 baked_shader_scene_vert = STR8(
DESHI_BAKED_SHADERS_VERSION
DESHI_BAKED_SHADERS_VERTEX_EXTENSIONS
"\n"
DESHI_BAKED_SHADERS_BUFFER_HEADER("SceneUBO", 0, 0)
"	mat4 proj;\n"
"	mat4 view;\n"
"	mat4 light_proj;\n"
"	mat4 light_view;\n"
"	vec3 light_pos;\n"
"	float time;\n"
DESHI_BAKED_SHADERS_BUFFER_FOOTER("ubo")
"\n"
DESHI_BAKED_SHADERS_PUSH_CONSTANT_HEADER("PushConstant")
"	mat4 transformation;\n"
"	vec3 color;\n"
DESHI_BAKED_SHADERS_PUSH_CONSTANT_FOOTER("push")
"\n"
"layout(location = 0) in vec3 in_pos;\n"
"layout(location = 1) in vec2 in_uv;\n"
"layout(location = 2) in vec4 in_color;\n"
"layout(location = 3) in vec3 in_normal;\n"
"\n"
"layout(location = 0) out vec3 out_normal;\n"
"layout(location = 1) out vec4 out_color;\n"
"layout(location = 2) out vec3 out_view_vector;\n"
"layout(location = 3) out vec3 out_light_vector;\n"
"layout(location = 4) out vec4 out_shadow_coord;\n"
"layout(location = 5) out float out_time;\n"
"\n"
"const mat4 bias_mat = mat4(\n"
"		0.5, 0.0, 0.0, 0.0,\n"
"		0.0, 0.5, 0.0, 0.0,\n"
"		0.0, 0.0, 1.0, 0.0,\n"
"		0.5, 0.5, 0.0, 1.0);\n"
"\n"
"void main(){\n"
"	out_color = vec4(push.color, 1);\n"
"	out_normal = in_normal;\n"
"	mat4 light_space = ubo.light_proj * ubo.light_view * mat4(1.0f);\n"
"	\n"
"	gl_Position = ubo.proj * ubo.view * push.transformation * vec4(in_pos, 1.0);\n"
"	\n"
"	vec4 pos = push.transformation * vec4(in_pos, 1.0);\n"
"	out_normal = mat3(push.transformation) * in_normal;\n"
"	out_light_vector = normalize(ubo.light_pos.xyz - in_pos);\n"
"	out_view_vector = -pos.xyz;\n"
"	\n"
"	out_shadow_coord = (bias_mat * light_space * push.transformation) * vec4(in_pos, 1.0);\n"
"	out_time = ubo.time;\n"
"}\n"
);

local str8 baked_shader_scene_frag = STR8(
DESHI_BAKED_SHADERS_VERSION
DESHI_BAKED_SHADERS_FRAGMENT_EXTENSIONS
"\n"
DESHI_BAKED_SHADERS_BINDING("sampler2D", "shadow_map", 0, 1)
"\n"
"layout(location = 0) in vec3 in_normal;\n"
"layout(location = 1) in vec4 in_color;\n"
"layout(location = 2) in vec3 in_view_vector;\n"
"layout(location = 3) in vec3 in_light_vector;\n"
"layout(location = 4) in vec4 in_shadow_coord;\n"
"layout(location = 5) in float in_time;\n"
"\n"
"layout(location = 0) out vec4 out_color;\n"
"\n"
"#define ambient 0.2\n"
"\n"
"float texture_proj(vec4 shadow_coord, vec2 offset){\n"
"	float shadow = 1.0;\n"
"	if(shadow_coord.z > -1.0 && shadow_coord.z < 1.0){\n"
"		float dist = texture(shadow_map, shadow_coord.xy + offset).r;\n"
"		if(shadow_coord.w > 0.0 && dist < shadow_coord.z){\n"
"			shadow = ambient;\n"
"		}\n"
"	}\n"
"	return shadow;\n"
"}\n"
"\n"
"void main(){\n"
"	float shadow = texture_proj(in_shadow_coord / in_shadow_coord.w, vec2(0.0));\n"
"	\n"
"	vec3 n = normalize(in_normal);\n"
"	vec3 l = normalize(in_light_vector);\n"
"	vec3 v = normalize(in_view_vector);\n"
"	vec3 r = normalize(-reflect(l, n));\n"
"	vec3 diffuse = max(dot(n, l), ambient) * in_color.xyz;\n"
"	\n"
"	out_color = vec4(in_color.xyz*shadow, 1.0);\n"
"}\n"
);


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @debug_quad


local str8 baked_shader_debug_quad_vert = STR8(
DESHI_BAKED_SHADERS_VERSION
DESHI_BAKED_SHADERS_VERTEX_EXTENSIONS
"\n"
"layout(location = 0) out vec2 out_uv;\n"
"\n"
"void main(){"
"	out_uv = vec2(("DESHI_BAKED_SHADERS_VERTEX_INDEX" << 1) & 2, "DESHI_BAKED_SHADERS_VERTEX_INDEX" & 2);"
"	gl_Position = vec4(out_uv * 2.f - 1.f, 0.f, 1.f);"
"}"
);

local str8 baked_shader_debug_quad_frag = STR8(
DESHI_BAKED_SHADERS_VERSION
DESHI_BAKED_SHADERS_FRAGMENT_EXTENSIONS
"\n"
DESHI_BAKED_SHADERS_BUFFER_HEADER("DebugQuadUBO", 0, 0)
"	float near_z;\n"
"	float far_z;\n"
DESHI_BAKED_SHADERS_BUFFER_FOOTER("ubo")
"\n"
DESHI_BAKED_SHADERS_BINDING("sampler2D", "shadow_map", 0, 1)
"\n"
"layout(location = 0) in vec2 in_uv;\n"
"\n"
"layout(location = 0) out vec4 out_color;\n"
"\n"
"float linearize_depth(float depth){\n"
"	float n = ubo.near_z;\n"
"	float f = ubo.far_z;\n"
"	float z = depth;\n"
"	return (2.0 * n) / (f + n - z * (f - n));\n"
"}\n"
"\n"
"void main(){\n"
"	float depth = texture(shadow_map, in_uv).r;\n"
"	out_color = vec4(vec3(1.0 - linearize_depth(depth)), 1.0);\n"
"}\n"
);
