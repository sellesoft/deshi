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
layout(location = 4)  in float width;
layout(location = 5)  in float height;
layout(location = 6)  in vec3  cp;
layout(location = 7)  in vec3  fragPos;
layout(location = 8)  in vec2  mousePos;
layout(location = 9)  in vec3  mouseworld;

layout(location = 0) out vec4 outColor;

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


vec4 waves(vec3 pos, float radius, float tcm){
	vec3 mi = VectorPlaneIntersect(fragPos, inNormal, cp, mouseworld);

	vec2 tc = inTexCoord;


	tc.x -= 0.5;
	tc.y -= 0.5;
	tc.x *= 2;
	tc.y *= 2;


	mi = pos;

	float ogdist = length(fragPos - mi);

	mi += normalize(fragPos - mi) * radius/2 * sin(1 * length(fragPos - mi) - 5 * time);// + cos(2 * time);//sin(time / 4));
	
	//mi.xz += 20;
	
	//mi.xz += 300 * cos(1000 * sin(time) *  length(quant3(fragPos, 3) - mi) - 5 * time);// + cos(2 * time);
	//mi.x += 3 * sin(length(fragPos - mi) * time / 400);
	//mi.z += mod2 * sin(10 * length(fragPos - mi) * cos(time / 3));
	//mi.xz += 50;
	float dist = length(fragPos - mi);

	if(ogdist / radius < 2){
		//return vec4(ogdist/50, ogdist/50, ogdist/50, 1);

		return (clamp(vec4(0, 1 - dist / radius, 1 - dist / radius, 1),
			vec4(-1,-1,-1,1), vec4(1,1,1,1)));
	}else{
		return vec4(0,0,0,1);
	}

	
}



vec4 fc = gl_FragCoord;
vec2 tc = inTexCoord;

void main() {
	//outColor = mouseColor(vec3(0,-5,0), 500 * ((-sin(time + cos(time)) + 1.1)/2), 500 * ((-sin(time + cos(time)) + 1.1)/2));

	tc.x -= 0.5;
	tc.y -= 0.5;
	tc.x *= 2;
	tc.y *= 2;

	fc.x /= width;
	fc.y /= height;

	outColor = 
	quant4(waves(vec3(-100 ,-5,-100), 100, 150), 22); //+ waves(vec3(0, -5, 0), 1000, 0), 2);// + mouseColor(vec3(75 * sin(time), -5, -75 * sin(time)));

	//outColor = vec4(1, tc.y, tc.x, 1);

	//outColor = mouseColor(vec3( 30 * sin(time), -5,  30 * cos(time * 2))) + 
		       //mouseColor(vec3(-30 * sin(time * 2), 0, -30 * cos(time)));//+
			   //mouseColor(vec3(30, -5, -30)) + mouseColor(vec3(-30, 0,  30));
}