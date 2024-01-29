/* deshi Baked Shaders
Index:
@abstractions
@functions
@null
@twod
@wireframe
@temp
@flat
@voxel

TODO:
- directx abstractions
*/
#ifndef DESHI_BAKED_SHADERS_H
#define DESHI_BAKED_SHADERS_H


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @abstractions


#if DESHI_VULKAN
#  define DESHI_BAKED_SHADERS_VERSION "#version 450\n"
#elif DESHI_OPENGL //#if DESHI_VULKAN
#  define DESHI_BAKED_SHADERS_VERSION "#version 330 core\n"
#else //#elif DESHI_OPENGL //#if DESHI_VULKAN
#  error "unhandled graphics backend"
#endif //#else //#elif DESHI_OPENGL //#if DESHI_VULKAN

#if DESHI_VULKAN || DESHI_OPENGL
#define DESHI_BAKED_SHADERS_VERTEX_EXTENSIONS "#extension GL_ARB_separate_shader_objects : enable\n"
#else //#if DESHI_VULKAN || DESHI_OPENGL
#  error "unhandled graphics backend"
#endif //#else //#if DESHI_VULKAN || DESHI_OPENGL

#if DESHI_VULKAN || DESHI_OPENGL
#define DESHI_BAKED_SHADERS_FRAGMENT_EXTENSIONS "#extension GL_ARB_separate_shader_objects : enable\n"
#else //#if DESHI_VULKAN || DESHI_OPENGL
#  error "unhandled graphics backend"
#endif //#else //#if DESHI_VULKAN || DESHI_OPENGL

#if DESHI_VULKAN
#  define DESHI_BAKED_SHADERS_BUFFER_HEADER(name,set,binding) "layout(std140, set = " #set ", binding = " #binding ") uniform " name "{\n"
#elif DESHI_OPENGL //#if DESHI_VULKAN
#  define DESHI_BAKED_SHADERS_BUFFER_HEADER(name,set,binding) "layout(std140) uniform " name "{\n"
#else //#elif DESHI_OPENGL //#if DESHI_VULKAN
#  error "unhandled graphics backend"
#endif //#else //#elif DESHI_OPENGL //#if DESHI_VULKAN

#define DESHI_BAKED_SHADERS_BUFFER_FOOTER(name) "} " name ";\n"

#if DESHI_VULKAN
#  define DESHI_BAKED_SHADERS_PUSH_CONSTANT_HEADER(name) "layout(push_constant) uniform " name "{\n"
#elif DESHI_OPENGL //#if DESHI_VULKAN
#  define DESHI_BAKED_SHADERS_PUSH_CONSTANT_HEADER(name) "layout(std140) uniform " name "{\n"
#else //#elif DESHI_OPENGL //#if DESHI_VULKAN
#  error "unhandled graphics backend"
#endif //#else //#elif DESHI_OPENGL //#if DESHI_VULKAN

#define DESHI_BAKED_SHADERS_PUSH_CONSTANT_FOOTER(name) "} " name ";\n"

#if DESHI_VULKAN
#  define DESHI_BAKED_SHADERS_BINDING(type,name,set,binding) "layout(set = " #set ", binding = " #binding ") uniform " type " " name ";\n"
#elif DESHI_OPENGL //#if DESHI_VULKAN
#  define DESHI_BAKED_SHADERS_BINDING(type,name,set,binding) "uniform " type " " name ";\n"
#else //#elif DESHI_OPENGL //#if DESHI_VULKAN
#  error "unhandled graphics backend"
#endif //#else //#elif DESHI_OPENGL //#if DESHI_VULKAN


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @functions


#define DESHI_BAKED_SHADERS_FUNCTION_QUANT \
"float quant(float v, float res){\n"       \
"	return floor(v * res) / res;\n"        \
"}\n"

#define DESHI_BAKED_SHADERS_FUNCTION_RAND                              \
"float rand(vec2 uv){\n"                                               \
"	return fract(sin(dot(uv, vec2(12.9898,78.233)))*43758.5453123);\n" \
"}\n"

#define DESHI_BAKED_SHADERS_FUNCTION_LERP  \
"float lerp(float a, float b, float t){\n" \
"	return (1 - t) * a + t * b;\n"         \
"}\n"

#define DESHI_BAKED_SHADERS_FUNCTION_DITHER \
"vec4 dither(sampler2D sampler, vec2 uv){\n" \
"	vec2 texSize = textureSize(sampler, 0);\n" \
"	vec2 texInc = vec2(1 / texSize.x, 1/ texSize.y);\n" \
"	\n" \
"	vec2 tc = uv;\n" \
"	if(tc.x == 0 || tc.y == 0 || tc.x == 1 || tc.y == 1){\n" \
"		return texture(sampler, tc);\n" \
"	}\n" \
"	\n" \
"	float seed_float = rand(vec2(time/1000.f,time/1000.f/2.f));\n" \
"	tc = vec2(quant(tc.x, texSize.x), quant(tc.y, texSize.y));\n" \
"	tc.y *= -1;\n" \
"	vec2 randControl = vec2(tc.x + quant(time/1000.f, 5), tc.y + quant(time/1000.f, 5));\n" \
"	float random = rand(randControl);\n" \
"	\n" \
"	vec2 next = tc;\n" \
"	int len = 4;\n" \
"	if(random <= 0.25){\n" \
"		next.x += texInc.x * int(rand(tc)*len);\n" \
"	}else if(random > 0.25  && random <= 0.5){\n" \
"		next.y += texInc.y * int(rand(tc)*len);\n" \
"	}else if(random > 0.5 && random <= 0.75){\n" \
"		next.x -= texInc.x * int(rand(tc)*len);\n" \
"	}else{\n" \
"		next.y -= texInc.y * int(rand(tc)*len);\n" \
"	}\n" \
"	\n" \
"	vec4 currCol = texture(sampler, tc);\n" \
"	vec4 nextCol = texture(sampler, next);\n" \
"	\n" \
"	if(nextCol != currCol){\n" \
"		return nextCol;\n" \
"	}else{\n" \
"		return currCol;\n" \
"	}\n" \
"}\n"


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @null


local str8 baked_shader_null_vert = STR8(
DESHI_BAKED_SHADERS_VERSION
DESHI_BAKED_SHADERS_VERTEX_EXTENSIONS
"\n"
DESHI_BAKED_SHADERS_BUFFER_HEADER("CameraInfo", 0, 0)
"	mat4 view;\n"
"	mat4 proj;\n"
"	vec4 position;\n"
DESHI_BAKED_SHADERS_BUFFER_FOOTER("cam")
"\n"
DESHI_BAKED_SHADERS_PUSH_CONSTANT_HEADER("PushConstant")
"	mat4 model;\n"
DESHI_BAKED_SHADERS_PUSH_CONSTANT_FOOTER("push")
"\n"
"layout(location = 0) in vec3 in_pos;\n"
"layout(location = 1) in vec2 in_uv;\n"
"layout(location = 2) in vec4 in_color;\n"
"layout(location = 3) in vec3 in_normal;\n"
"\n"
"layout(location = 0) out vec2 out_uv;\n"
"layout(location = 1) flat out vec3 out_normal;\n"
"\n"
"void main(){\n"
"	gl_Position = cam.proj * cam.view * push.model * vec4(in_pos.xyz, 1.0);\n"
"	out_uv = in_uv;\n"
"	out_normal = in_normal;\n"
"}\n"
);

local str8 baked_shader_null_frag = STR8(
DESHI_BAKED_SHADERS_VERSION
DESHI_BAKED_SHADERS_FRAGMENT_EXTENSIONS
"\n"
"layout(location = 0) in vec2 in_uv;\n"
"layout(location = 1) flat in vec3 in_normal;\n"
"\n"
"layout(location = 0) out vec4 out_color;\n"
"\n"
"void main(){\n"
"	vec2 ws = in_uv;\n"
"	int qwsx = int(floor(ws.x * 10.0));\n"
"	int qwsy = int(floor(ws.y * 10.0));\n"
"	if(mod(qwsx, 2) == 0) {\n"
"		if(mod(qwsy, 2) == 1){\n"
"			out_color = vec4(0);\n"
"		}else{\n"
"			out_color = vec4(0.8, 0.2, 0.4, 1.0);\n"
"		}\n"
"	}else{\n"
"		if(mod(qwsy, 2) == 0){\n"
"			out_color = vec4(0.0);\n"
"		}else{\n"
"			out_color = vec4(0.8, 0.2, 0.4, 1.0);\n"
"		}\n"
"	}\n"
"}\n"
);


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @twod


local str8 baked_shader_twod_vert = STR8(
DESHI_BAKED_SHADERS_VERSION
DESHI_BAKED_SHADERS_VERTEX_EXTENSIONS
"\n"
DESHI_BAKED_SHADERS_PUSH_CONSTANT_HEADER("PushConstant")
"	vec2 scale;\n"
"	vec2 translate;\n"
DESHI_BAKED_SHADERS_PUSH_CONSTANT_FOOTER("push")
"\n"
"layout(location = 0) in vec2 in_pos;\n"
"layout(location = 1) in vec2 in_uv;\n"
"layout(location = 2) in vec4 in_color;\n"
"\n"
"layout(location = 0) out vec2 out_uv;\n"
"layout(location = 1) out vec4 out_color;\n"
"\n"
"void main(){\n"
"	gl_Position = vec4(push.scale * in_pos + push.translate, 0.0, 1.0);\n"
"	out_uv = in_uv;\n"
"	out_color = in_color;\n"
"}\n"
);

local str8 baked_shader_twod_frag = STR8(
DESHI_BAKED_SHADERS_VERSION
DESHI_BAKED_SHADERS_FRAGMENT_EXTENSIONS
"\n"
DESHI_BAKED_SHADERS_BINDING("sampler2D", "font_texture", 0, 0)
"\n"
"layout(location = 0) in vec2 in_uv;\n"
"layout(location = 1) in vec4 in_color;\n"
"\n"
"layout(location = 0) out vec4 out_color;\n"
"\n"
"void main(){\n"
"	out_color = in_color * texture(font_texture, in_uv.st);\n"
"}\n"
);


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @wireframe


local str8 baked_shader_wireframe_vert = STR8(
DESHI_BAKED_SHADERS_VERSION
DESHI_BAKED_SHADERS_VERTEX_EXTENSIONS
"\n"
DESHI_BAKED_SHADERS_BUFFER_HEADER("CameraInfo", 0, 0)
"	mat4 view;\n"
"	mat4 proj;\n"
"	vec4 position;\n"
DESHI_BAKED_SHADERS_BUFFER_FOOTER("cam")
"\n"
DESHI_BAKED_SHADERS_PUSH_CONSTANT_HEADER("PushConstant")
"	mat4 model;\n"
DESHI_BAKED_SHADERS_PUSH_CONSTANT_FOOTER("push")
"\n"
"layout(location = 0) in vec3 in_pos;\n"
"layout(location = 1) in vec4 in_color;\n"
"\n"
"layout(location = 0) out vec4 out_color;\n"
"\n"
"void main(){\n"
"	gl_Position = cam.proj * cam.view * push.model * vec4(in_pos.xyz, 1.0);\n"
"	out_color = in_color;\n"
"}\n"
);

local str8 baked_shader_wireframe_frag = STR8(
DESHI_BAKED_SHADERS_VERSION
DESHI_BAKED_SHADERS_FRAGMENT_EXTENSIONS
"\n"
"layout(location = 0) in vec4 in_color;\n"
"\n"
"layout(location = 0) out vec4 out_color;\n"
"\n"
"void main(){\n"
"	out_color = vec4((1.5 * in_color.rgb), in_color.a);\n"
"}\n"
);


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @temp


local str8 baked_shader_temp_vert = STR8(
DESHI_BAKED_SHADERS_VERSION
DESHI_BAKED_SHADERS_VERTEX_EXTENSIONS
"\n"
DESHI_BAKED_SHADERS_BUFFER_HEADER("CameraInfo", 0, 0)
"	mat4 view;\n"
"	mat4 proj;\n"
"	vec4 position;\n"
DESHI_BAKED_SHADERS_BUFFER_FOOTER("cam")
"\n"
"layout(location = 0) in vec3 in_pos;\n"
"layout(location = 1) in vec4 in_color;\n"
"\n"
"layout(location = 0) out vec4 out_color;\n"
"void main(){\n"
"	gl_Position = cam.proj * cam.view * vec4(in_pos, 1.0);\n"
"	out_color = in_color;\n"
"}\n"
);

local str8 baked_shader_temp_frag = STR8(
DESHI_BAKED_SHADERS_VERSION
DESHI_BAKED_SHADERS_FRAGMENT_EXTENSIONS
"\n"
"layout(location = 0) in vec4 in_color;\n"
"\n"
"layout(location = 0) out vec4 out_color;\n"
"\n"
"void main(){\n"
"	out_color = in_color;\n"
"}\n"
);


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @flat


local str8 baked_shader_flat_vert = STR8(
DESHI_BAKED_SHADERS_VERSION
DESHI_BAKED_SHADERS_VERTEX_EXTENSIONS
"\n"
DESHI_BAKED_SHADERS_BUFFER_HEADER("CameraInfo", 0, 0)
"	mat4 view;\n"
"	mat4 proj;\n"
"	vec4 position;\n"
DESHI_BAKED_SHADERS_BUFFER_FOOTER("cam")
"\n"
DESHI_BAKED_SHADERS_PUSH_CONSTANT_HEADER("PushConstant")
"	mat4 model;\n"
DESHI_BAKED_SHADERS_PUSH_CONSTANT_FOOTER("push")
"\n"
"layout(location = 0) in vec3 in_pos;\n"
"layout(location = 1) in vec2 in_uv;\n"
"layout(location = 2) in vec4 in_color;\n"
"layout(location = 3) in vec3 in_normal;\n"
"\n"
"layout(location = 0) out vec3 out_light_vector_inverse;\n"
"layout(location = 1) out vec3 out_normal;\n"
"layout(location = 2) out vec2 out_uv;\n"
"layout(location = 3) out vec4 out_color;\n"
"\n"
"void main(){\n"
"	gl_Position = cam.proj * cam.view * push.model * vec4(in_pos.xyz, 1.0);\n"
"	out_light_vector_inverse = vec3(cam.position) - gl_Position.xyz;\n"
"	out_normal = mat3(push.model) * in_normal;\n"
"	out_uv = in_uv;\n"
"	out_color = in_color;\n"
"}\n"
);

local str8 baked_shader_flat_frag = STR8(
DESHI_BAKED_SHADERS_VERSION
DESHI_BAKED_SHADERS_FRAGMENT_EXTENSIONS
"\n"
"layout(location = 0) in vec3 in_light_vector_inverse;\n"
"layout(location = 1) in vec3 in_normal;\n"
"layout(location = 2) in vec4 in_color;\n"
"\n"
"layout(location = 0) out vec4 out_color;\n"
"\n"
"void main(){\n"
"	out_color = in_color;\n"
"	out_color.xyz *= clamp(dot(normalize(in_light_vector_inverse), normalize(in_normal)) * 0.7, 0.25, 1.0);\n"
"}\n"
);

local str8 baked_shader_flat_textured_frag = STR8(
DESHI_BAKED_SHADERS_VERSION
DESHI_BAKED_SHADERS_FRAGMENT_EXTENSIONS
"\n"
DESHI_BAKED_SHADERS_BINDING("sampler2D", "tex", 1, 0)
"\n"
"layout(location = 0) in vec3 in_light_vector_inverse;\n"
"layout(location = 1) in vec3 in_normal;\n"
"layout(location = 2) in vec2 in_uv;\n"
"layout(location = 3) in vec4 in_color;\n"
"\n"
"layout(location = 0) out vec4 out_color;\n"
"\n"
"void main(){\n"
"	out_color = texture(tex, in_uv) * in_color;\n"
"	out_color.xyz *= clamp(dot(normalize(in_light_vector_inverse), normalize(in_normal)) * 0.7, 0.25, 1.0);\n"
"}\n"
);


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @voxel


local str8 baked_shader_voxel_vert = STR8(
DESHI_BAKED_SHADERS_VERSION
DESHI_BAKED_SHADERS_VERTEX_EXTENSIONS
"\n"
DESHI_BAKED_SHADERS_BUFFER_HEADER("CameraInfo", 0, 0)
"	mat4 view;\n"
"	mat4 proj;\n"
"	vec4 position;\n"
DESHI_BAKED_SHADERS_BUFFER_FOOTER("cam")
"\n"
DESHI_BAKED_SHADERS_PUSH_CONSTANT_HEADER("PushConstant")
"	mat4 model;\n"
DESHI_BAKED_SHADERS_PUSH_CONSTANT_FOOTER("push")
"\n"
"layout(location = 0) in vec3 in_pos;\n"
"layout(location = 1) in vec2 in_uv;\n"
"layout(location = 2) in vec4 in_color;\n"
"layout(location = 3) in vec3 in_normal;\n"
"\n"
"layout(location = 0) out vec4 out_color;\n"
"\n"
"void main(){\n"
"	gl_Position = cam.proj * cam.view * push.model * vec4(in_pos.xyz, 1.0);\n"
"	out_color = in_color;\n"
"}\n"
);

local str8 baked_shader_voxel_frag = STR8(
DESHI_BAKED_SHADERS_VERSION
DESHI_BAKED_SHADERS_FRAGMENT_EXTENSIONS
"\n"
"layout(location = 0) in vec4 in_color;\n"
"\n"
"layout(location = 0) out vec4 out_color;\n"
"\n"
"void main(){\n"
"	out_color = in_color;\n"
"}\n"
);


#endif //#ifndef DESHI_BAKED_SHADERS_H