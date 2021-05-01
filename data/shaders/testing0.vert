#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBufferObject{
	mat4 view;
	mat4 proj;
	vec4 lightPos;
	vec4 viewPos;
	vec2 mousePos;
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
layout(location = 9)  out vec3  rayend;
layout(location = 10) out vec3  outPosition;

mat4 translation(vec3 t){
	return mat4(
	1,   0,   0,   0,
	0,   1,   0,   0,
	0,   0,   1,   0,
	t.x, t.y, t.z, 1);
}

void main() {
	
	//cast out ray from mouse
	rayend = vec3(ubo.mousePos, 0);

	rayend.x /= .5 * width;
	rayend.y /= .5 * height;
	rayend.x -= 1.f; rayend.y -= 1.f; rayend.z = -1.f;

	mat4 inv = inverse(ubo.proj * ubo.view);

	vec4 near = vec4((ubo.mousePos.x - width / 2) / width / 2, -1*(ubo.mousePos.y - height / 2) / height / 2, -1, 1.0);
    vec4 far = vec4((ubo.mousePos.x - width / 2) / width / 2, -1*(ubo.mousePos.y - height / 2) / height / 2, 1, 1.0);
    

	vec4 rayend4 = vec4(rayend, 1) * inverse(ubo.proj);
	rayend4.xyz /= rayend4.w;
	rayend = vec3(rayend4);
	vec4 rayend42 = vec4(rayend, 1) * inverse(ubo.view);
	rayend42.xyz /= rayend42.w;
	rayend = vec3(rayend42);

	gl_Position = ubo.proj * ubo.view * primitive.model * vec4(inPosition.xyz, 1.0);
   // gl_Position.x += rayend.x;
	

	rayend = vec3(vec4(rayend, 1) * inverse(translation(camerapos)));
	rayend = normalize(rayend);
	rayend *= 1000;
	rayend *= vec3(vec4(rayend, 1) * translation(camerapos));
	
	//gl_Position.y = floor(gl_Position.y);
	//gl_Position.z = floor(gl_Position.z);
	
	outColor = inColor;
	outTexCoord = inTexCoord;
	outNormal = mat3(primitive.model) * inNormal;
	fragPos = vec3(primitive.model * vec4(inPosition, 1));
	mousePos = ubo.mousePos;
}