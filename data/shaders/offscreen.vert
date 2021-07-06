#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject{
	mat4  view;
	mat4  proj;
	vec4  lights[10];
	vec4  viewPos;
	vec2  screen;
	vec2  mousepos;
	vec3  mouseWorld;
	float time;
	int   enablePCF;
	mat4  depthMVP;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec3 inNormal;

out gl_PerVertex{
    vec4 gl_Position;   
};

void main(){
    gl_Position = ubo.depthMVP * vec4(inPosition.xyz, 1.0);
}