#pragma once
#ifndef DESHI_BAKED_SHADERS_H
#define DESHI_BAKED_SHADERS_H


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// GLSL version
#if DESHI_VULKAN
#  define DESHI_BAKED_SHADERS_VERSION_STRING "#version 450\n"
#elif DESHI_OPENGL
#  define DESHI_BAKED_SHADERS_VERSION_STRING "#version 330 core\n"
#endif


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// GLSL extensions
#if DESHI_VULKAN
#  define DESHI_BAKED_SHADERS_EXTENSIONS_VERTEX_STRING \
"#extension GL_ARB_separate_shader_objects : enable\n"
#elif DESHI_OPENGL
#  define DESHI_BAKED_SHADERS_EXTENSIONS_VERTEX_STRING \
"#extension GL_ARB_separate_shader_objects : enable\n"
#endif

#define DESHI_BAKED_SHADERS_EXTENSIONS_FRAGMENT_STRING "#extension GL_ARB_separate_shader_objects : enable\n"


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// GLSL uniform buffer object
#if DESHI_VULKAN
#  define DESHI_BAKED_SHADERS_UBO_HEADER_STRING(set,binding) \
"layout(set = " set ", binding = " binding ") uniform UniformBufferObject{\n"
#elif DESHI_OPENGL
#  define DESHI_BAKED_SHADERS_UBO_HEADER_STRING(set,binding) \
"uniform UniformBufferObject{\n"
#endif

#if DESHI_VULKAN
#  define DESHI_BAKED_SHADERS_UBO_STRING                              \
"layout(std140, set = 0, binding = 0) uniform UniformBufferObject{\n" \
"	mat4  view;\n"                                                   \
"	mat4  proj;\n"                                                   \
"	vec4  lights[10];\n"                                             \
"	vec4  viewPos;\n"                                                \
"	vec2  screen;\n"                                                 \
"	vec2  mousepos;\n"                                               \
"	vec3  mouseWorld;\n"                                             \
"	float time;\n"                                                   \
"	mat4  depthMVP;\n"                                               \
"	int   enablePCF;\n"                                              \
"}ubo;\n"
#elif DESHI_OPENGL
#  define DESHI_BAKED_SHADERS_UBO_STRING                              \
"layout(std140) uniform UniformBufferObject{\n"                       \
"	mat4  view;\n"                                                   \
"	mat4  proj;\n"                                                   \
"	vec4  lights[10];\n"                                             \
"	vec4  viewPos;\n"                                                \
"	vec2  screen;\n"                                                 \
"	vec2  mousepos;\n"                                               \
"	vec3  mouseWorld;\n"                                             \
"	float time;\n"                                                   \
"	mat4  depthMVP;\n"                                               \
"	int   enablePCF;\n"                                              \
"}ubo;\n"
#endif


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// GLSL push constants
#if DESHI_VULKAN
#  define DESHI_BAKED_SHADERS_PUSHCONSTHEADER_STRING \
"layout(push_constant) uniform PushConsts{\n"
#elif DESHI_OPENGL
#  define DESHI_BAKED_SHADERS_PUSHCONSTHEADER_STRING \
"uniform PushConsts{\n"
#endif

#if DESHI_VULKAN
#  define DESHI_BAKED_SHADERS_PUSHCONST3D_STRING \
"layout(push_constant) uniform PushConsts{\n"  \
"	mat4 model;\n"                            \
"}primitive;\n"
#elif DESHI_OPENGL
#  define DESHI_BAKED_SHADERS_PUSHCONST3D_STRING \
"uniform PushConsts{\n"                        \
"	mat4 model;\n"                            \
"}primitive;\n"
#endif


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// GLSL sampler
#if DESHI_VULKAN
#  define DESHI_BAKED_SHADERS_SAMPLER2D_STRING(set,binding,name) "layout(set = " set ", binding = " binding ") uniform sampler2D " name ";\n"
#elif DESHI_OPENGL
#  define DESHI_BAKED_SHADERS_SAMPLER2D_STRING(set,binding,name) "uniform sampler2D " name ";\n"
#endif

#if DESHI_VULKAN
#  define DESHI_BAKED_SHADERS_SAMPLER2D_NOSET_STRING(binding,name) "layout(binding = " binding ") uniform sampler2D " name ";\n"
#elif DESHI_OPENGL
#  define DESHI_BAKED_SHADERS_SAMPLER2D_NOSET_STRING(binding,name) "uniform sampler2D " name ";\n"
#endif


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// GLSL common shader inputs and outputs
#define DESHI_BAKED_SHADERS_COMMON_VERTEX_INPUT_STRING \
"layout(location = 0) in vec3 inPosition;\n"           \
"layout(location = 1) in vec2 inUV;\n"                 \
"layout(location = 2) in vec4 inColor;\n"              \
"layout(location = 3) in vec3 inNormal;\n"

#define DESHI_BAKED_SHADERS_COMMON_VERTEX_OUTPUT_STRING \
"layout(location = 0) out vec4 outColor;\n"             \
"layout(location = 1) out vec2 outUV;\n"                \
"layout(location = 2) out vec3 outNormal;\n"

#define DESHI_BAKED_SHADERS_COMMON_FRAGMENT_INPUT_STRING \
"layout(location = 0) in vec4 inColor;\n"                \
"layout(location = 1) in vec2 inUV;\n"                   \
"layout(location = 2) in vec3 inNormal;\n"

#define DESHI_BAKED_SHADERS_COMMON_FRAGMENT_OUTPUT_STRING \
"layout(location = 0) out vec4 outColor;\n"


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// GLSL functions
#define DESHI_BAKED_SHADERS_FUNCTION_QUANT_STRING \
"float quant(float v, float res){\n"              \
"	return floor(v * res) / res;\n"              \
"}\n"

#define DESHI_BAKED_SHADERS_FUNCTION_RAND_STRING                        \
"float rand(vec2 uv){\n"                                                \
"	return fract(sin(dot(uv, vec2(12.9898,78.233)))*43758.5453123);\n" \
"}\n"

#define DESHI_BAKED_SHADERS_FUNCTION_LERP_STRING \
"float lerp(float a, float b, float t){\n"       \
"	return (1 - t) * a + t * b;\n"              \
"}\n"


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// GLSL base shaders
local str8 baked_shader_base_vert = STR8(
DESHI_BAKED_SHADERS_VERSION_STRING
DESHI_BAKED_SHADERS_EXTENSIONS_VERTEX_STRING
"\n"
DESHI_BAKED_SHADERS_UBO_STRING
"\n"
DESHI_BAKED_SHADERS_PUSHCONST3D_STRING
"\n"
DESHI_BAKED_SHADERS_COMMON_VERTEX_INPUT_STRING
"\n"
DESHI_BAKED_SHADERS_COMMON_VERTEX_OUTPUT_STRING
"\n"
"void main(){\n"
"	gl_Position = ubo.proj * ubo.view * primitive.model * vec4(inPosition.xyz, 1.0);\n"
"	outColor    = inColor;\n"
"	outUV       = inUV;\n"
"	outNormal   = mat3(primitive.model) * inNormal;\n"
"}\n"
);

local str8 baked_shader_base_frag = STR8(
DESHI_BAKED_SHADERS_VERSION_STRING
DESHI_BAKED_SHADERS_EXTENSIONS_FRAGMENT_STRING
"\n"
DESHI_BAKED_SHADERS_COMMON_FRAGMENT_INPUT_STRING
"\n"
DESHI_BAKED_SHADERS_COMMON_FRAGMENT_OUTPUT_STRING
"\n"
"void main(){\n"
"	outColor = inColor;\n"
"}\n"
);


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// GLSL flat shaders
local str8 baked_shader_flat_vert = STR8(
DESHI_BAKED_SHADERS_VERSION_STRING
DESHI_BAKED_SHADERS_EXTENSIONS_VERTEX_STRING
"\n"
DESHI_BAKED_SHADERS_UBO_STRING
"\n"
DESHI_BAKED_SHADERS_PUSHCONST3D_STRING
"\n"
DESHI_BAKED_SHADERS_COMMON_VERTEX_INPUT_STRING
"\n"
"layout(location = 0) out vec3 outLightVectorInverse;\n"
"layout(location = 1) out vec3 outNormal;\n"
"layout(location = 2) out vec4 outColor;\n"
"\n"
"void main(){\n"
"	gl_Position = ubo.proj * ubo.view * primitive.model * vec4(inPosition.xyz, 1.0);\n"
"	outLightVectorInverse = vec3(ubo.viewPos) - gl_Position.xyz;\n"
"	outNormal = mat3(primitive.model) * inNormal;\n"
"	outColor = inColor;\n"
"}\n"
);

local str8 baked_shader_flat_frag = STR8(
DESHI_BAKED_SHADERS_VERSION_STRING
DESHI_BAKED_SHADERS_EXTENSIONS_FRAGMENT_STRING
"\n"
"layout(location = 0) in vec3 inLightVectorInverse;\n"
"layout(location = 1) in vec3 inNormal;\n"
"layout(location = 2) in vec4 inColor;\n"
"\n"
DESHI_BAKED_SHADERS_COMMON_FRAGMENT_OUTPUT_STRING
"\n"
"void main(){\n"
"	outColor = inColor;\n"
"	outColor.xyz *= clamp(dot(normalize(inLightVectorInverse), normalize(inNormal)) * 0.7, 0.25, 1.0);\n"
"}\n"
);


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// GLSL null shaders
local str8 baked_shader_null_vert = STR8(
DESHI_BAKED_SHADERS_VERSION_STRING
DESHI_BAKED_SHADERS_EXTENSIONS_VERTEX_STRING
"\n"
DESHI_BAKED_SHADERS_UBO_STRING
"\n"
DESHI_BAKED_SHADERS_PUSHCONST3D_STRING
"\n"
DESHI_BAKED_SHADERS_COMMON_VERTEX_INPUT_STRING
"\n"
"layout(location = 0) out vec2 outUV;\n"
"layout(location = 1) out float time;\n"
"\n"
"void main(){\n"
"	gl_Position = ubo.proj * ubo.view * primitive.model * vec4(inPosition.xyz, 1.0);\n"
"	outUV       = inUV;\n"
"	time        = ubo.time;\n"
"}\n"
);

local str8 baked_shader_null_frag = STR8(
DESHI_BAKED_SHADERS_VERSION_STRING
DESHI_BAKED_SHADERS_EXTENSIONS_FRAGMENT_STRING
"\n"
DESHI_BAKED_SHADERS_SAMPLER2D_STRING("1","0","nullSampler")
"\n"
"layout(location = 0) in vec2  inUV;\n"
"layout(location = 1) in float time;\n"
"\n"
DESHI_BAKED_SHADERS_COMMON_FRAGMENT_OUTPUT_STRING
"\n"
DESHI_BAKED_SHADERS_FUNCTION_QUANT_STRING
"\n"
DESHI_BAKED_SHADERS_FUNCTION_RAND_STRING
"\n"
"vec4 dither(){\n"
"	vec2 texSize = textureSize(nullSampler, 0);\n"
"	vec2 texInc = vec2(1 / texSize.x, 1/ texSize.y);\n"
"	\n"
"	vec2 tc = inUV;\n"
"	if(tc.x == 0 || tc.y == 0 || tc.x == 1 || tc.y == 1){\n"
"		return texture(nullSampler, tc);\n"
"	}\n"
"	\n"
"	float seed_float = rand(vec2(time/1000.f,time/1000.f/2.f));\n"
"	tc = vec2(quant(tc.x, texSize.x), quant(tc.y, texSize.y));\n"
"	tc.y *= -1;\n"
"	vec2 randControl = vec2(tc.x + quant(time/1000.f, 5), tc.y + quant(time/1000.f, 5));\n"
"	float random = rand(randControl);\n"
"	\n"
"	vec2 next = tc;\n"
"	int len = 4;\n"
"	if(random <= 0.25){\n"
"		next.x += texInc.x * int(rand(tc)*len);\n"
"	}else if(random > 0.25  && random <= 0.5){\n"
"		next.y += texInc.y * int(rand(tc)*len);\n"
"	}else if(random > 0.5 && random <= 0.75){\n"
"		next.x -= texInc.x * int(rand(tc)*len);\n"
"	}else{\n"
"		next.y -= texInc.y * int(rand(tc)*len);\n"
"	}\n"
"	\n"
"	vec4 currCol = texture(nullSampler, tc);\n"
"	vec4 nextCol = texture(nullSampler, next);\n"
"	\n"
"	if(nextCol != currCol){\n"
"		return nextCol;\n"
"	}else{\n"
"		return currCol;\n"
"	}\n"
"}\n"
"\n"
"void main(){\n"
"	outColor = dither();\n"
"	outColor.a = 1;\n"
"}\n"
);


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// GLSL pbr shaders
local str8 baked_shader_pbr_vert = STR8(
DESHI_BAKED_SHADERS_VERSION_STRING
DESHI_BAKED_SHADERS_EXTENSIONS_VERTEX_STRING
"\n"
DESHI_BAKED_SHADERS_UBO_STRING
"\n"
DESHI_BAKED_SHADERS_PUSHCONST3D_STRING
"\n"
DESHI_BAKED_SHADERS_COMMON_VERTEX_INPUT_STRING
"\n"
DESHI_BAKED_SHADERS_COMMON_VERTEX_OUTPUT_STRING
"layout(location = 3) out vec3 outPosition;\n"
"layout(location = 4) out float time;\n"
"layout(location = 5) out vec3 fragPos;\n"
"layout(location = 6) out vec2 screen;\n"
"\n"
"void main(){\n"
"	gl_Position = ubo.proj * ubo.view * primitive.model * vec4(inPosition.xyz, 1.0);\n"
"	outColor = inColor;\n"
"	outUV = inUV;\n"
"	outNormal = mat3(primitive.model) * inNormal;\n"
"	outPosition = inPosition;\n"
"	time = ubo.time;\n"
"	screen = ubo.screen;\n"
"	fragPos = vec3(primitive.model * vec4(inPosition, 1));\n"
"}\n"
);

local str8 baked_shader_pbr_frag = STR8(
DESHI_BAKED_SHADERS_VERSION_STRING
DESHI_BAKED_SHADERS_EXTENSIONS_FRAGMENT_STRING
"\n"
DESHI_BAKED_SHADERS_SAMPLER2D_STRING("1","0","albedoSampler")
DESHI_BAKED_SHADERS_SAMPLER2D_STRING("1","1","normalSampler")
DESHI_BAKED_SHADERS_SAMPLER2D_STRING("1","2","specularSampler")
DESHI_BAKED_SHADERS_SAMPLER2D_STRING("1","3","lightSampler")
"\n"
"//layout(constant_id = 0) const bool ALPHA_MASK = false;\n"
"//layout(constant_id = 1) const float ALPHA_THRESHOLD = 0.0f;\n"
"\n"
DESHI_BAKED_SHADERS_COMMON_FRAGMENT_INPUT_STRING
"layout(location = 3) in vec3 inPosition;\n"
"layout(location = 4) in float time;\n"
"layout(location = 5) in vec3 fragPos;\n"
"layout(location = 6) in vec2 screen;\n"
"\n"
DESHI_BAKED_SHADERS_COMMON_FRAGMENT_OUTPUT_STRING
"\n"
"void main(){\n"
"	//TODO(delle,r) pbr shading, see pbrtexture in saschawillems' stuff\n"
"	vec2 tc = inUV;\n"
"	vec4 fc = gl_FragCoord;\n"
"	\n"
"	fc.x /= screen.x;\n"
"	fc.y /= screen.y;\n"
"	\n"
"	tc.y *= -1;\n"
"	vec4 normal = texture(normalSampler, tc);\n"
"	vec4 tex = texture(albedoSampler, tc);\n"
"	vec3 light = vec3(-8000, 2 ,2);\n"
"	vec4 lightcolor = vec4(clamp(dot(normalize(light - inPosition), -vec3(normal)), 0, 1),\n"
"						   clamp(dot(normalize(light - inPosition), -vec3(normal)), 0, 1),\n"
"						   clamp(dot(normalize(light - inPosition), -vec3(normal)), 0, 1),\n"
"						   1);\n"
"	\n"
"	outColor = tex;// * lightcolor;// * color;// * color2;\n"
"}\n"
);


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// GLSL phong shaders
local str8 baked_shader_phong_vert = STR8(
DESHI_BAKED_SHADERS_VERSION_STRING
DESHI_BAKED_SHADERS_EXTENSIONS_VERTEX_STRING
"\n"
DESHI_BAKED_SHADERS_UBO_STRING
"\n"
DESHI_BAKED_SHADERS_PUSHCONST3D_STRING
"\n"
DESHI_BAKED_SHADERS_COMMON_VERTEX_INPUT_STRING
"\n"
DESHI_BAKED_SHADERS_COMMON_VERTEX_OUTPUT_STRING
"layout(location = 3) out float outLightBrightness;\n"
"layout(location = 4) out vec3 outWorldPos;\n"
"layout(location = 5) out vec3 viewPosition;\n"
"layout(location = 6) flat out int outEnablePCF;\n"
"layout(location = 7) out vec4 outShadowCoord;\n"
"layout(location = 8) out vec3 outLightVec;\n"
"layout(location = 9) out vec3 outViewVec;\n"
"layout(location = 10) out vec4 outLights[10];\n"
"\n"
"//transformation to shadow map space [-1...1] -> [0...1]: (xy*0.5 + 0.5)\n"
"const mat4 biasMat = mat4(0.5, 0.0, 0.0, 0.0,\n"
"						  0.0, 0.5, 0.0, 0.0,\n"
"						  0.0, 0.0, 1.0, 0.0,\n"
"						  0.5, 0.5, 0.0, 1.0);\n"
"\n"
"void main(){\n"
"	gl_Position = ubo.proj * ubo.view * primitive.model * vec4(inPosition.xyz, 1.0);\n"
"	outColor = inColor;\n"
"	outUV = inUV;\n"
"	outNormal = mat3(primitive.model) * inNormal;\n"
"	//outLightBrightness = ubo.lightPos.w;\n"
"	outWorldPos = vec3(primitive.model * vec4(inPosition.xyz, 1.0));\n"
"	outLights = ubo.lights;\n"
"	viewPosition = (ubo.view * primitive.model * vec4(inPosition.xyz, 1.0)).xyz;\n"
"	\n"
"	if(ubo.enablePCF != 0) outEnablePCF = 1;\n"
"	outShadowCoord = (biasMat * ubo.depthMVP * primitive.model) * vec4(inPosition.xyz, 1.0);\n"
"	outLightVec = normalize(ubo.lights[0].xyz - inPosition);\n"
"	vec4 pos = primitive.model * vec4(inPosition.xyz, 1.0);\n"
"	outViewVec = -pos.xyz;\n"
"}\n"
);

local str8 baked_shader_phong_frag = STR8(
DESHI_BAKED_SHADERS_VERSION_STRING
DESHI_BAKED_SHADERS_EXTENSIONS_FRAGMENT_STRING
"\n"
DESHI_BAKED_SHADERS_SAMPLER2D_NOSET_STRING("1","shadowMap")
"\n"
DESHI_BAKED_SHADERS_COMMON_FRAGMENT_INPUT_STRING
"layout(location = 3) in float lightBrightness;\n"
"layout(location = 4) in vec3 worldPosition;\n"
"layout(location = 5) in vec3 viewPosition;\n"
"layout(location = 6) flat in int inEnablePCF;\n"
"layout(location = 7) in vec4 inShadowCoord;\n"
"layout(location = 8) in vec3 inLightVec;\n"
"layout(location = 9) in vec3 inViewVec;\n"
"layout(location = 10) in vec4 lights[10];\n"
"\n"
DESHI_BAKED_SHADERS_COMMON_FRAGMENT_OUTPUT_STRING
"\n"
"#define ambient 0.1\n"
"\n"
"//ref: https://github.com/SaschaWillems/Vulkan/blob/master/data/shaders/glsl/shadowmapping/scene.frag\n"
"float textureProj(vec4 shadowCoord, vec2 off){\n"
"	float shadow = 1.0;\n"
"	if(shadowCoord.z > -1.0 && shadowCoord.z < 1.0){ //check if coords are within the texture's bounds\n"
"		float dist = texture(shadowMap, shadowCoord.st + off).r; //st == xy\n"
"		if(shadowCoord.w > 0.0 && dist < shadowCoord.z){\n"
"			shadow = ambient;\n"
"		}\n"
"	}\n"
"	return shadow;\n"
"}\n"
"\n"
"float filterPCF(vec4 sc){\n"
"	ivec2 texDim = textureSize(shadowMap, 0);\n"
"	float scale = 1.5;\n"
"	float dx = scale * 1.0 / float(texDim.x);\n"
"	float dy = scale * 1.0 / float(texDim.y);\n"
"	float shadowFactor = 0.0;\n"
"	int count = 0;\n"
"	int range = 1;\n"
"	for(int x = -range; x <= range; x++){\n"
"		for(int y = -range; y <= range; y++){\n"
"			shadowFactor += textureProj(sc, vec2(dx*x, dy*y));\n"
"			count++;\n"
"		}\n"
"	}\n"
"	return shadowFactor / count;\n"
"}\n"
"\n"
"vec4 calcLight(vec3 lightPos, vec3 worldPos, float brightness, vec3 normal){\n"
"	float r = length(lightPos - worldPos);\n"
"	float ang = brightness * dot(normalize(lightPos - worldPos), normal);\n"
"	float light = clamp(ang/(r * r), 0, 1);\n"
"	\n"
"	return  vec4(light, light, light, 1);\n"
"}\n"
"\n"
"vec4 calcLight(vec3 lightPos, vec3 worldPos, float brightness, vec3 normal, vec3 color){\n"
"	float r = length(lightPos - worldPos);\n"
"	float ang = brightness * dot(normalize(lightPos - worldPos), normal);\n"
"	float light = clamp(ang/(r * r), 0, 1);\n"
"	\n"
"	return  vec4(light, light, light, 1) * vec4(color, 1);\n"
"}\n"
"\n"
"void main(){\n"
"	/*outColor = vec4(0,0,0,1);\n"
"	for(int i = 0; i < 10; i++){\n"
"		if(lights[i].w != -1 && lights[i].w != 0){\n"
"			outColor += calcLight(lights[i].xyz, worldPosition, lights[i].w, inNormal);\n"
"			//outColor = clamp(outColor, vec4(0,0,0,0), vec4(1,1,1,1));\n"
"		}\n"
"	}*/\n"
"	//outColor = vec4(outColor.xyz * shadow, 1.0);\n"
"	\n"
"	float shadow = (inEnablePCF == 1) ? filterPCF(inShadowCoord / inShadowCoord.w) : textureProj(inShadowCoord / inShadowCoord.w, vec2(0.0));\n"
"	vec3 N = normalize(inNormal);\n"
"	vec3 L = normalize(inLightVec);\n"
"	vec3 V = normalize(inViewVec);\n"
"	vec3 R = normalize(-reflect(L, N));\n"
"	vec4 diffuse = max(dot(N, L), ambient) * inColor;\n"
"	outColor = shadow * diffuse;\n"
"}\n"
);


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// GLSL twod shaders
local str8 baked_shader_twod_vert = STR8(
DESHI_BAKED_SHADERS_VERSION_STRING
DESHI_BAKED_SHADERS_EXTENSIONS_VERTEX_STRING
"\n"
DESHI_BAKED_SHADERS_PUSHCONSTHEADER_STRING
"	vec2 scale;\n"
"	vec2 translate;\n"
"}push;\n"
"\n"
"layout(location = 0) in vec2 inPos;\n"
"layout(location = 1) in vec2 inUV;\n"
"layout(location = 2) in vec4 inColor;\n"
"\n"
"out gl_PerVertex { vec4 gl_Position; };\n"
"layout(location = 0) out vec2 outUV;\n"
"layout(location = 1) out vec4 outColor;\n"
"\n"
"void main(){\n"
"	gl_Position = vec4(push.scale * inPos + push.translate, 0.0, 1.0);\n"
"	outUV = inUV;\n"
"	outColor = inColor;\n"
"}\n"
);

local str8 baked_shader_twod_frag = STR8(
DESHI_BAKED_SHADERS_VERSION_STRING
DESHI_BAKED_SHADERS_EXTENSIONS_FRAGMENT_STRING
"\n"
DESHI_BAKED_SHADERS_SAMPLER2D_STRING("0","0","fontTexture")
"\n"
"layout(location = 0) in vec2 inUV;\n"
"layout(location = 1) in vec4 inColor;\n"
"\n"
DESHI_BAKED_SHADERS_COMMON_FRAGMENT_OUTPUT_STRING
"\n"
"void main(){\n"
"	outColor = inColor * texture(fontTexture, inUV.st);\n"
"}\n"
);


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// GLSL ui shaders
local str8 baked_shader_ui_vert = STR8(
DESHI_BAKED_SHADERS_VERSION_STRING
DESHI_BAKED_SHADERS_EXTENSIONS_VERTEX_STRING
"\n"
DESHI_BAKED_SHADERS_PUSHCONSTHEADER_STRING
"	vec2 scale;\n"
"	vec2 translate;\n"
"	int  fontIdx;"
"}push;\n"
"\n"
"layout(location = 0) in vec2 inPos;\n"
"layout(location = 1) in vec2 inUV;\n"
"layout(location = 2) in vec4 inColor;\n"
"\n"
"out gl_PerVertex { vec4 gl_Position; };\n"
"layout(location = 0) out vec2 outUV;\n"
"layout(location = 1) out vec4 outColor;\n"
"\n"
"void main(){\n"
"	gl_Position = vec4(push.scale * inPos + push.translate, 0.0, 1.0);\n"
"	outUV = inUV;\n"
"	outColor = inColor;\n"
"}\n"
);

local str8 baked_shader_ui_frag = STR8(
DESHI_BAKED_SHADERS_VERSION_STRING
DESHI_BAKED_SHADERS_EXTENSIONS_FRAGMENT_STRING
"\n"
DESHI_BAKED_SHADERS_SAMPLER2D_STRING("0","0","tex")
"\n"
"layout(location = 0) in vec2 inUV;\n"
"layout(location = 1) in vec4 inColor;\n"
"\n"
DESHI_BAKED_SHADERS_COMMON_FRAGMENT_OUTPUT_STRING
"\n"
"void main(){\n"
"	outColor = inColor * texture(tex, inUV);\n"
"}\n"
);


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// GLSL wireframe shaders
local str8 baked_shader_wireframe_vert = STR8(
DESHI_BAKED_SHADERS_VERSION_STRING
DESHI_BAKED_SHADERS_EXTENSIONS_VERTEX_STRING
"\n"
DESHI_BAKED_SHADERS_UBO_STRING
"\n"
DESHI_BAKED_SHADERS_PUSHCONST3D_STRING
"\n"
DESHI_BAKED_SHADERS_COMMON_VERTEX_INPUT_STRING
"\n"
"layout(location = 0) out vec4 outColor;\n"
"layout(location = 1) out vec3 outNormal;\n"
"\n"
"void main(){\n"
"	gl_Position = ubo.proj * ubo.view * primitive.model * vec4(inPosition.xyz, 1.0);\n"
"	outColor = inColor;\n"
"	outNormal = mat3(primitive.model) * inNormal;\n"
"}\n"
);

local str8 baked_shader_wireframe_frag = STR8(
DESHI_BAKED_SHADERS_VERSION_STRING
DESHI_BAKED_SHADERS_EXTENSIONS_FRAGMENT_STRING
"\n"
"layout(location = 0) in vec4 inColor;\n"
"layout(location = 1) in vec3 inNormal;\n"
"\n"
DESHI_BAKED_SHADERS_COMMON_FRAGMENT_OUTPUT_STRING
"\n"
"void main(){\n"
"	outColor = vec4((inColor.rgb * 1.5), inColor.a);\n"
"}\n"
);


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// GLSL offscreen shader
local str8 baked_shader_offscreen_vert = STR8(
DESHI_BAKED_SHADERS_VERSION_STRING
DESHI_BAKED_SHADERS_EXTENSIONS_VERTEX_STRING
"\n"
DESHI_BAKED_SHADERS_UBO_HEADER_STRING("0","0")
"	mat4  lightVP;\n"
"}ubo;\n"
"\n"
DESHI_BAKED_SHADERS_PUSHCONST3D_STRING
"\n"
DESHI_BAKED_SHADERS_COMMON_VERTEX_INPUT_STRING
"\n"
"void main(){\n"
"    gl_Position = ubo.lightVP * primitive.model * vec4(inPosition.xyz, 1.0);\n"
"}\n"
);


#define L(x) STRINGIZE(x) "\n"

local str8 baked_shader_temp_vert = str8l(
L(#version 450)
L(layout(location = 0) in vec3 in_pos;)
L(layout(location = 1) in vec4 in_color;)
L(layout(binding = 0) uniform UBO {)
L(	mat4 view;)
L(  mat4 proj;)
L(} ubo;)
L(layout(location = 0) out vec4 out_color;)
L(void main() {)
L(	gl_Position = ubo.proj * ubo.view * vec4(in_pos, 1.f);)
L(	out_color = in_color;)
L(})
);

local str8 baked_shader_temp_frag = str8l(
L(#version 450)
L(layout(location = 0) in vec4 in_color;)
L(layout(location = 0) out vec4 out_color;)
L(void main() {)
L(	out_color = in_color;)
L(})
);

local str8 baked_shader_null_vert_2 = str8l(
L(#version 450)
L(#extension GL_ARB_separate_shader_objects : enable)
L()
L(layout(set = 0, binding = 0) uniform UBO {)
L(	mat4 view;)
L(	mat4 proj;)
L(} ubo;)
L()
L(layout(push_constant) uniform PC {)
L(	mat4 transformation;)
L(} primitive;)
L()
L(layout(location = 0) in vec3 in_pos;)
L(layout(location = 1) in vec2 in_uv;)
L(layout(location = 2) in vec4 in_color;)
L(layout(location = 3) in vec3 in_normal;)
L()
L(layout(location = 0) out vec2 out_uv;)
L()
L(void main() {)
L(	gl_Position = ubo.proj * ubo.view * primitive.transformation * vec4(in_pos.xyz, 1.0);)
L(	out_uv = in_uv;)
L(})
);

local str8 baked_shader_null_frag_2 = str8l(
L(#version 450)
L(#extension GL_ARB_separate_shader_objects : enable)
L()
L(layout(location = 0) in vec2 in_uv;)
L()
L(layout(location = 0) out vec4 out_color;)
L()
L(const float square_size = 0.1;)
L()
L(void main() {)
L(	vec4 ws = gl_FragCoord;)
L(	if(mod(ws.x, (2*square_size)) < square_size && mod(ws.y, (2*square_size)) < square_size) {)
L(		out_color = vec4(0);)
L(	} else {)
L(		out_color = vec4(0.8, 0.2, 0.4, 1);)
L(	})
L(})
);

local str8 baked_shader_flat_vert_2 = str8l(
L(#version 450)
L(#extension GL_ARB_separate_shader_objects : enable)
L()
L(layout(set = 0, binding = 0) uniform UBO {)
L(	mat4 view;)
L(	mat4 proj;)
L(} ubo;)
L()
L(layout(push_constant) uniform PC {)
L(	mat4 transformation;)
L(} primitive;)
L()
L(layout(location = 0) in vec3 in_pos;)
L(layout(location = 1) in vec2 in_uv;)
L(layout(location = 2) in vec4 in_color;)
L(layout(location = 3) in vec3 in_normal;)
L()
L(layout(location = 0) out vec4 out_color;)
L(layout(location = 1) out vec2 out_uv;)
L()
L(void main() {)
L(	gl_Position = ubo.proj * ubo.view * primitive.transformation * vec4(in_pos.xyz, 1.0);)
L(	out_color = in_color;)
L(	out_uv = in_uv;)
L(})
);

local str8 baked_shader_flat_frag_2 = str8l (
L(#version 450)
L(#extension GL_ARB_separate_shader_objects : enable)
L()
L(layout(set = 1, binding = 0) uniform sampler2D tex;)
L()
L(layout(location = 0) in vec4 in_color;)
L(layout(location = 1) in vec2 in_uv;)
L(layout(location = 0) out vec4 out_color;)
L()
L(void main() {)
L(	out_color = texture(tex, in_uv) * in_color;)
L(})
);

#undef L

#endif //DESHI_BAKED_SHADERS_H
