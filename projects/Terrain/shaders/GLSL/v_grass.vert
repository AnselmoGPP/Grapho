#version 450
#extension GL_ARB_separate_shader_objects : enable
#pragma shader_stage(vertex)

#include "..\..\..\projects\Terrain\shaders\GLSL\vertexTools.vert"

layout(set = 0, binding = 0) uniform ubobject {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 normalMatrix;			// mat3
	vec4 camPos_time;
	vec4 centerPos;				// vec3
	LightPD light[NUMLIGHTS];	// n * (2 * vec4)
} ubo;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUVs;

layout(location = 0) out vec3 outPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outUVs;
layout(location = 3) flat out vec3 outCamPos;
layout(location = 4) flat out vec3 outCenterPos;
layout(location = 5) flat out LightPD outLight[NUMLIGHTS];	// light positions & directions

void main()
{
	vec3 pos = inPos;
	pos += getRatio(inPos.y, -0.5, 0.5) * inNormal * sin(3 * ubo.camPos_time.a) * 0.02;		// speed (2), amplitude (0.01)
	
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(pos, 1.0);
	outPos      = (ubo.model * vec4(pos, 1.0)).xyz;
	outNormal   = mat3(ubo.normalMatrix) * inNormal;
	outUVs      = inUVs;
	outCamPos = ubo.camPos_time.xyz;
	outCenterPos = ubo.centerPos.xyz;
	
	for(int i = 0; i < NUMLIGHTS; i++) 
	{
		outLight[i].position.xyz  = ubo.light[i].position.xyz;						// for point & spot light
		outLight[i].direction.xyz = normalize(ubo.light[i].direction.xyz);			// for directional & spot light
	}
}