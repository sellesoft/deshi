#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_KHR_vulkan_glsl : enable

layout(set = 1, binding = 0) uniform sampler2D nullSampler;

layout(location = 0) in vec2  inUV;
layout(location = 1) in float time;

layout(location = 0) out vec4 outColor;

float quant(float v, float res){
	return floor(v * res) / res;
}

float rand(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898,78.233)))*43758.5453123);
}

vec4 dither(){
	
	vec2 texSize = textureSize(nullSampler, 0);
	vec2 texInc = vec2(1 / texSize.x, 1/ texSize.y);

	vec2 tc = inUV;

	if(tc.x == 0 || tc.y == 0 || tc.x == 1 || tc.y == 1){
		return texture(nullSampler, tc);
	}

	float seed_float = rand(vec2(time/1000.f,time/1000.f/2.f));
	tc = vec2(quant(tc.x, texSize.x), quant(tc.y, texSize.y));
	tc.y *= -1;
	vec2 randControl = vec2(tc.x + quant(time/1000.f, 5), tc.y + quant(time/1000.f, 5));
	float random = rand(randControl);
	
	vec2 next = tc;
	int len = 4;
	if(random <= 0.25){
		next.x += texInc.x * int(rand(tc)*len);
	}else if(random > 0.25  && random <= 0.5){
		next.y += texInc.y * int(rand(tc)*len);
	}else if(random > 0.5 && random <= 0.75){
		next.x -= texInc.x * int(rand(tc)*len);
	}else{
		next.y -= texInc.y * int(rand(tc)*len);
	}

	vec4 currCol = texture(nullSampler, tc);
	vec4 nextCol = texture(nullSampler, next);
	
	if(nextCol != currCol){
		return nextCol;
	}else {
		return currCol;
	}
}

void main() {
	outColor = dither();
	outColor.a = 1;
}