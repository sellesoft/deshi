#version 450
#extension GL_ARB_separate_shader_objects : enable
	
layout(location = 0)  in vec3  inColor;
layout(location = 1)  in vec2  inTexCoord;
layout(location = 2)  in vec3  inNormal;
layout(location = 3)  in float time;
layout(location = 4)  in float width;
layout(location = 5)  in float height;
layout(location = 6)  in vec3  cp;
layout(location = 7)  in vec3  fragPos;
layout(location = 8)  in vec2  mousePos;
layout(location = 9)  in vec3  rayend;
layout(location = 10) in vec3  inp;

layout(location = 0) out vec4 outColor;

vec3 quant3(vec3 v, int res){
	return vec3(floor(v.x * res) / res, floor(v.y * res) / res, floor(v.z * res) / res);
}

vec4 quant4(vec4 v, int res){
	return vec4(floor(v.x * res) / res,floor(v.y * res) / res, floor(v.z * res) / res, floor(v.w * res) / res);
}

vec4 fc = gl_FragCoord;
vec2 tc = inTexCoord;

void main() {
	

	float pd = -dot(inNormal, fragPos);
	float ad = dot(cp, inNormal);
	float bd = dot(rayend, inNormal);

	float t = (-pd - ad)/(bd - ad);
	vec3 lste = rayend - cp;
	vec3 lti = lste * t;
	vec3 point = cp + lti;

	float dist = length(point - fragPos);
	
	outColor = vec4(1, rayend.x, 1, 1.0);

}