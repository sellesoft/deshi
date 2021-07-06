#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 1, binding = 0) uniform sampler2D albedoSampler;
layout(set = 1, binding = 1) uniform sampler2D normalSampler;
layout(set = 1, binding = 2) uniform sampler2D specularSampler;
layout(set = 1, binding = 3) uniform sampler2D lightSampler;

layout(location = 0)  in vec3  inColor;
layout(location = 1)  in vec2  inTexCoord;
layout(location = 2)  in vec3  inNormal;
layout(location = 3)  in float time;
layout(location = 4)  in vec2  screen;
layout(location = 5)  in vec3  cp;
layout(location = 6)  in vec3  fragPos;
layout(location = 7)  in vec2  mousePos;
layout(location = 8)  in vec3  mouseworld;

layout(location = 0) out vec4 outColor;

float pi = 3.1415926535;


float quant(float v, float res){
	return floor(v * res) / res;
}
vec2 quant2(vec2 v, float res){
	return vec2(floor(v.x * res) / res, floor(v.y * res) / res);
}
vec3 quant3(vec3 v, float res){
	return vec3(floor(v.x * res) / res, floor(v.y * res) / res, floor(v.z * res) / res);
}
vec4 quant4(vec4 v, float res){
	return vec4(floor(v.x * res) / res,floor(v.y * res) / res, floor(v.z * res) / res, floor(v.w * res) / res);
}
float interp(float a, float b, float t){
	return (1 - t) * a + t * b;
}


vec3 VectorPlaneIntersect(vec3 planep, vec3 planen, vec3 ls, vec3 le){
	float plane_d = dot(-planen, planep);
	float ad = dot(ls, planen);
	float bd = dot(le, planen);
	float t = (-plane_d - ad) / (bd - ad);
	vec3 line_start_to_end = le - ls;
	vec3 line_to_intersect = line_start_to_end * t;
	return ls + line_to_intersect;
	
}


/*

Misc. Shader Functions

*/


vec4 facewrap(){
	vec2 tc = inTexCoord;
	
	tc.y *= -1;
	return texture(albedoSampler, vec2(tc.x + sin(time), tc.y + cos(time)));
}


vec4 mouseWaterDrop(){
	vec3 mi = VectorPlaneIntersect(fragPos, inNormal, cp, mouseworld);
	
	mi.x += 3 * sin(length(fragPos - mi) * time / 400);
	//mi.y += 3 * cos(length(fragPos - mi) * time / 300);
	
	float dist = length(fragPos - mi);
	
	return vec4(dist / 30 , dist / 30, dist / 30, 1);
}


vec4 waves(vec3 pos, float radius){
	vec3 mi = VectorPlaneIntersect(fragPos, inNormal, cp, mouseworld);
	
	vec2 tc = inTexCoord;
	
	
	tc.x -= 0.5;
	tc.y -= 0.5;
	tc.x *= 2;
	tc.y *= 2;
	
	
	mi = pos;
	//float ang = acos(dot(fragPos, vec3(0,0,1)) / (length(fragPos)));
	//mi.x = fragPos.x / 2;
	//mi.z = fragPos.z / 2;
	
	//mi.x += fragPos.x + sin(fragPos.x * ang);
	//mi.z += fragPos.z + cos(fragPos.z * ang);
	
	
	//mi.x = 10 * cos(fragPos.x);
	
	float ogdist = length(fragPos - mi);
	
	//fragPos += normalize(fragPos) * ang;
	
	vec3 mip = mi + normalize(fragPos - mi) * radius/0.2 * sin(2 * length(fragPos - mi) - 2 * time);// + cos(2 * time);//sin(time / 4));
	
	//mi.xz += 20;
	
	//mi.xz += 300 * cos(1000 * sin(time) *  length(quant3(fragPos, 3) - mi) - 5 * time);// + cos(2 * time);
	//mi.x += 3 * sin(length(fragPos - mi) * time / 400);
	//mi.z += mod2 * sin(10 * length(fragPos - mi) * cos(time / 3));
	//mi.xz += 50;
	float dist = length(mip - mi);
	
	
	if(ogdist / radius < 2){
		//return vec4(ang/180, ang/180, ang/180, 1) * 3;
		
		return (clamp(vec4(0, 1 - dist / radius, 1 - dist / radius, 1),
					  vec4(-1,-1,-1,1), vec4(1,1,1,1))) / (0.16 * ogdist);
	}else{
		return vec4(0,0,0,1);
	}
	
	
}

vec4 fc = gl_FragCoord;
vec2 tc = inTexCoord;

vec4 fun(){

	vec4 samp = texture(albedoSampler, tc);
	vec4 sampr = vec4(samp.r, 0, 0, 1);
	vec4 sampg = vec4(0, samp.g, 0, 1);
	vec4 sampb = vec4(0, 0, samp.b, 1);

	vec4 black = vec4(0,0,0,1);

	float sot = (sin(time) + 1) / 2;

	float res = 1000;

	float ftcx = floor(tc.x * res) / res;
	float ftcy = floor(tc.y * res) / res;

	float ctcx = ceil(tc.x * res) / res;
	float ctcy = ceil(tc.y * res) / res;

	float xlen = ctcx - ftcx;
	float ylen = ctcy - ftcy;

	float tcmfy = tc.y - ftcy;
	float tcmfx = tc.x - ftcx;

	float hfay = tcmfy / ylen;
	float hfax = tcmfx / xlen;

	float border = 0.05;

	if(hfay < border || hfax > 1 - border || hfay < border || hfax > 1 - border){
		return black;
	}
	else if(hfax < 0.333){
		return sampr;
	}
	else if(hfax > 0.333 && hfax < 0.666){
		return sampg;
	}
	else if(hfax > 0.666){
		return sampb;
	}
	else{
		return black;
	}

	if(hfay < 0.01){
		return vec4(0,0,0,1);
	}
	else if(hfay < 0.333){
		return vec4(samp.r, 0, 0, 1);
	}
	else if(hfay > 0.333 && hfay < 0.666){
		return vec4(0, samp.g, 0, 1);
	}
	else if(hfay > 0.99){
		return vec4(0,0,0,1);
	}
	else{
		return vec4(0, 0, samp.b, 1);
	}

	return texture(albedoSampler, tc);
}





void main() {
	//outColor = mouseColor(vec3(0,-5,0), 500 * ((-sin(time + cos(time)) + 1.1)/2), 500 * ((-sin(time + cos(time)) + 1.1)/2));
	
	//tc.x -= 0.5;
	//tc.y -= 0.5;
	//tc.x *= 2;
	//tc.y *= 2;
	//
	//fc.x /= screen.x;
	//fc.y /= screen.y;
	
	outColor = vec4(0,0,0,1);
	
	outColor = mouseWaterDrop();
	
	outColor = fun();

	//outColor = waves(vec3(0,0,0), 100) + waves(vec3(10, 0, 10), 100);
	
	
	
}