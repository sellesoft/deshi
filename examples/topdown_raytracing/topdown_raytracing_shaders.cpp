#include "kigu/common.h"
#include "core/baked/shaders.h"

local str8 baked_shader_fullscreen_vert = STR8(
DESHI_BAKED_SHADERS_VERSION
DESHI_BAKED_SHADERS_VERTEX_EXTENSIONS
"\n"
"layout(location = 0) out vec2 out_uv;\n"
"\n"
"void main(){\n"
"	vec2 vertices[3] = vec2[3](vec2(-1,-1), vec2(3,-1), vec2(-1, 3));\n"
"	gl_Position = vec4(vertices["DESHI_BAKED_SHADERS_VERTEX_INDEX"], 0, 1);\n"
"	out_uv = 0.5 * gl_Position.xy + vec2(0.5);\n"
"}\n"
);

// modified version of Star Nest by Pablo Roman Andrioli
// https://www.shadertoy.com/view/XlfGRj
// License: MIT
local str8 baked_shader_stars_fragment = STR8(
DESHI_BAKED_SHADERS_VERSION
DESHI_BAKED_SHADERS_FRAGMENT_EXTENSIONS
"\n"
"#define iterations 17\n"
"#define formuparam 0.53\n"
"\n"
"#define volsteps 20\n"
"#define stepsize 0.1\n"
"\n"
"#define zoom   0.800\n"
"#define tile   0.850\n"
"#define speed  0.0001\n"
"\n"
"#define brightness 0.0015\n"
"#define darkmatter 0.300\n"
"#define distfading 0.730\n"
"#define saturation 0.850\n"
"\n"
"//NOTE: every field is padded to 16 bytes due to std140\n"
DESHI_BAKED_SHADERS_BUFFER_HEADER("UBO", 0, 0)
"  mat4 proj;\n"
"  mat4 view;\n"
"  vec4 resolution_and_time;\n"
DESHI_BAKED_SHADERS_BUFFER_FOOTER("ubo")
"\n"
"layout(location = 0) in vec2 in_uv;\n"
"\n"
"layout(location = 0) out vec4 out_color;\n"
"\n"
"void main(){\n"
"  // get coords and direction \n"
"  vec2 uv = gl_FragCoord.xy / ubo.resolution_and_time.xy - 0.5;\n"
"  uv.y *= ubo.resolution_and_time.y / ubo.resolution_and_time.x;\n"
"  vec3 dir = vec3(uv * zoom, 1.0);\n"
"  float time = ubo.resolution_and_time.z * speed + 0.25;\n"
"  vec3 from = vec3(1.0 + 1.2f*time, 0.5 + 0.2f*time, -1.5);\n"
"\n"
"  // volumetric rendering\n"
"  float s = 0.1;\n"
"  float fade = 1.0;\n"
"  vec3 v = vec3(0.0);\n"
"  for(int r = 0; r < volsteps; r++){\n"
"    vec3 p = from + 0.5 * s * dir;\n"
"    p = abs(vec3(tile) - mod(p, vec3(2.0 * tile))); // tiling fold\n"
"\n"
"    float a = 0.0;\n"
"    float pa = 0.0;\n"
"    for(int i = 0; i < iterations; i++){\n"
"      p = abs(p) / max(dot(p, p), 0.0005) - formuparam; // the magic formula\n"
"      a += abs(length(p) - pa); // absolute sum of average change\n"
"      pa = length(p);\n"
"    }\n"
"\n"
"    float dm = max(0.,darkmatter-a*a*.001); // dark matter\n"
"    a *= a*a; // add contrast\n"
"    if(r > 6) fade *= 1.0 - dm; // dark matter, don't render near\n"
"\n"
"    v += fade;\n"
"    v += vec3(s, s*s, s*s*s*s) * a * brightness * fade; // coloring based on distance\n"
"\n"
"    // distance fading\n"
"    fade *= distfading;\n"
"\n"
"    s += stepsize;\n"
"  }\n"
"\n"
"  // color adjust\n"
"  v = mix(vec3(length(v)), v, saturation);\n"
"\n"
"  out_color = vec4(0.001 * v, 1.0);\n"
"}\n"
);

local str8 baked_shader_tiles_vert = STR8(
DESHI_BAKED_SHADERS_VERSION
DESHI_BAKED_SHADERS_FRAGMENT_EXTENSIONS
"\n"
"//NOTE: every field is padded to 16 bytes due to std140\n"
DESHI_BAKED_SHADERS_BUFFER_HEADER("UBO", 0, 0)
"  mat4 proj;\n"
"  mat4 view;\n"
"  vec4 resolution_and_time;\n"
DESHI_BAKED_SHADERS_BUFFER_FOOTER("ubo")
"\n"
"layout(location = 0) in vec2 in_pos;\n"
"layout(location = 1) in vec2 in_uv;\n"
"layout(location = 2) in uint in_layer;\n"
"\n"
"layout(location = 0) out vec3 out_uv;\n"
"\n"
"void main(){\n"
"  vec4 pos = ubo.proj * ubo.view * vec4(in_pos, 0.0, 1.0);\n"
"  gl_Position = vec4(pos.xy, 0.0, 1.0);\n"
"  out_uv = vec3(in_uv, in_layer);\n"
"}\n"
);

local str8 baked_shader_tiles_frag = STR8(
DESHI_BAKED_SHADERS_VERSION
DESHI_BAKED_SHADERS_FRAGMENT_EXTENSIONS
"\n"
DESHI_BAKED_SHADERS_BINDING("sampler2DArray", "texture_array", 0, 1)
"\n"
"layout(location = 0) in vec3 in_uv;\n"
"\n"
"layout(location = 0) out vec4 out_color;\n"
"\n"
"void main(){\n"
"  out_color = texture(texture_array, in_uv);\n"
"}\n"
);

local str8 baked_shader_light_raycast_vert = STR8(
DESHI_BAKED_SHADERS_VERSION
DESHI_BAKED_SHADERS_FRAGMENT_EXTENSIONS
"\n"
"//NOTE: every field is padded to 16 bytes due to std140\n"
DESHI_BAKED_SHADERS_BUFFER_HEADER("UBO", 0, 0)
"  mat4 proj;\n"
"  mat4 view;\n"
"  vec4 resolution_and_time;\n"
DESHI_BAKED_SHADERS_BUFFER_FOOTER("ubo")
"\n"
"layout(location = 0) in vec2 in_pos;\n"
"layout(location = 1) in vec2 in_uv;\n"
"\n"
"layout(location = 0) out vec2 out_uv;\n"
"\n"
"void main(){\n"
"  vec4 pos = ubo.proj * ubo.view * vec4(in_pos, 0.0, 1.0);\n"
"  gl_Position = vec4(pos.xy, 0.0, 1.0);\n"
"  out_uv = in_uv;\n"
"}\n"
);

local str8 baked_shader_light_raycast_frag = STR8(
DESHI_BAKED_SHADERS_VERSION
DESHI_BAKED_SHADERS_FRAGMENT_EXTENSIONS
"\n"
DESHI_BAKED_SHADERS_BINDING("sampler2DArray", "light_collision_texture", 0, 1)
DESHI_BAKED_SHADERS_PUSH_CONSTANT_HEADER("PushConstant")
"  vec3 light_position;\n"
"  uint light_color;\n"
"  vec4 texture_sizes;\n" //x,y = light collision texture size, z = ray texture size, w = unused
DESHI_BAKED_SHADERS_PUSH_CONSTANT_FOOTER("push")
"\n"
"#define MAX_RADIUS 65535.0\n"
"#define TAU 6.2831853071795864769252867665590\n"
"\n"
"layout(location = 0) in vec2 in_uv;\n"
"\n"
"layout(location = 0) out vec4 out_color;\n"
"\n"
"void main(){\n"

"  out_color = vec4(0.0);\n"
"}\n"
);

local str8 baked_shader_light_sample_frag = STR8(
DESHI_BAKED_SHADERS_VERSION
DESHI_BAKED_SHADERS_FRAGMENT_EXTENSIONS
"\n"
DESHI_BAKED_SHADERS_BINDING("sampler2DArray", "texture_array", 0, 1)
DESHI_BAKED_SHADERS_PUSH_CONSTANT_HEADER("PushConstant")
"  vec3 light_position;\n"
"  uint light_color;\n"
"  vec2 light_center;\n"
"  vec4 texture_sizes;\n" //x,y = light collision texture size, z = ray texture size, w = light texture size
DESHI_BAKED_SHADERS_PUSH_CONSTANT_FOOTER("push")
"\n"
"layout(location = 0) in vec2 in_uv;\n"
"layout(location = 1) in flat int in_layer;\n"
"\n"
"layout(location = 0) out vec4 out_color;\n"
"\n"
"void main(){\n"
"  vec2 coord = in_uv * push.texture_sizes.w;\n"
"  vec2 delta = coord - push.light_center;\n"
"  out_color = texture(texture_array, vec3(in_uv, in_layer));\n"
"}\n"
);