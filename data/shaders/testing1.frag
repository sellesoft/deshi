#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inColor;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 lightPos;
layout(location = 4) in float lightBrightness;
layout(location = 5) in vec3 worldPos;
layout(location = 6) in vec3 viewPosition;


layout(location = 0) out vec4 outColor;

vec4 calcLight(vec3 lightPos, vec3 worldPos, float brightness, vec3 normal){
	float r = length(lightPos - worldPos) / 20;
	float ang = brightness * dot(normalize(lightPos - worldPos), normal);
	float light = clamp(ang/(r * r),0, 1);

	return  vec4(light, light, light, 1);
}


vec4 calcLight(vec3 lightPos, vec3 worldPos, float brightness, vec3 normal, vec3 color){
	float r = length(lightPos - worldPos) / 20;
	float ang = brightness * dot(normalize(lightPos - worldPos), normal);
	float light = clamp(ang/(r * r),0, 1);

	return  vec4(light, light, light, 1) * vec4(color, 1);
}

void main() {
   



	outColor = calcLight(lightPos, worldPos, lightBrightness, inNormal);

	//outColor = vec4(worldPos - camPos, 1.0);
	

}