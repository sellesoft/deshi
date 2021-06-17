#version 450

layout (triangles) in;
layout (line_strip, max_vertices = 8) out;

layout(set = 0, binding = 1) uniform UBO {
	mat4 view;
	mat4 proj;
} ubo;

layout(push_constant) uniform PushConsts{
	mat4 model;
} primitive;

layout (location = 0) in vec3 inColor[];
layout (location = 1) in vec2 inTexCoord[];
layout (location = 2) in vec3 inNormal[];

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec2 outTexCoord;
layout(location = 2) out vec3 outNormal;

void main(void){	
	float normalLength = 0.25;
	vec3 face_pos = vec3(0.0, 0.0, 0.0);
	vec3 face_normal = vec3(0.0, 0.0, 0.0);

	for(int i=0; i<gl_in.length(); i++){
		vec3 pos = gl_in[i].gl_Position.xyz;
		vec3 normal = inNormal[i].xyz;

		face_pos += pos;
		face_normal += normal;

		gl_Position = ubo.proj * ubo.view * primitive.model * vec4(pos, 1.0);
		outColor = vec3(0.0, 1.0, 0.0);
		EmitVertex();

		gl_Position = ubo.proj * ubo.view * primitive.model * vec4(pos + normal * normalLength, 1.0);
		outColor = vec3(0.0, 1.0, 0.0);
		EmitVertex();

		EndPrimitive();
	}

	face_pos /= 3.0;
	normalize(face_normal);

	gl_Position = ubo.proj * ubo.view * primitive.model * vec4(face_pos, 1.0);
	outColor = vec3(1.0, 0.0, 0.0);
	EmitVertex();

	gl_Position = ubo.proj * ubo.view * primitive.model * vec4(face_pos + face_normal * normalLength, 1.0);
	outColor = vec3(1.0, 0.0, 0.0);
	EmitVertex();

	EndPrimitive();
}