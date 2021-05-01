#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inColor;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 viewPosition;
layout(location = 4) in vec3 position;
layout(location = 5) in vec3 camPos;




layout(location = 0) out vec4 outColor;

void main() {

    vec3 light = vec3(camPos);

	vec3 xTangent = dFdx( viewPosition );
    vec3 yTangent = dFdy( viewPosition );
    vec3 faceNormal = normalize( cross( xTangent, yTangent ) );

	float n = clamp(dot(normalize(light - position), faceNormal) * 0.9, .1f, 1);

	outColor = vec4(n, n, n, 1);
}