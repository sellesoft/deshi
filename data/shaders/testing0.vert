#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBufferObject{
	mat4 view;
	mat4 proj;
	vec4 lights[10];
	vec4 viewPos;
	float time;         
	float width;		
	float height;	  
	vec2 mousepos; 
	vec3 mouseWorld;
} ubo;

layout(push_constant) uniform PushConsts{
	mat4 model;
} primitive;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec3 inNormal;

layout(location = 0)  out vec3  outColor;
layout(location = 1)  out vec2  outTexCoord;
layout(location = 2)  out vec3  outNormal;
layout(location = 3)  out float time;
layout(location = 4)  out float width;
layout(location = 5)  out float height;
layout(location = 6)  out vec3  camerapos;
layout(location = 7)  out vec3  fragPos;
layout(location = 8)  out vec2  mousePos;
layout(location = 9)  out vec3  mouseworld;

mat4 translation(vec3 t){
	return mat4(
	1,   0,   0,   0,
	0,   1,   0,   0,
	0,   0,   1,   0,
	t.x, t.y, t.z, 1);
}

void main() {

	gl_Position = ubo.proj * ubo.view * primitive.model * vec4(inPosition.xyz, 1.0);
	//gl_Position.x = mouseworld.x;
	//gl_Position.y = floor(gl_Position.y);
	//gl_Position.z = floor(gl_Position.z);
	
	outColor = inColor;
	outTexCoord = inTexCoord;
	outNormal = mat3(primitive.model) * inNormal;
	fragPos = vec3(primitive.model * vec4(inPosition, 1));
	mousePos = ubo.mousepos;
	camerapos = ubo.viewPos.xyz;
	mouseworld = ubo.mouseWorld;
	time = ubo.time;
}