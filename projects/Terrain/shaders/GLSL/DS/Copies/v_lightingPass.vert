#version 450
#extension GL_ARB_separate_shader_objects : enable
#pragma shader_stage(vertex)

#include "..\..\..\projects\Terrain\shaders\GLSL\vertexTools.vert"

layout(set = 0, binding = 0) uniform ubobject {
	LightPD light[NUMLIGHTS];	// n * (2 * vec4)
} ubo;

layout (location = 0) in vec3 inPos;				// NDC position. Since it's in NDCs, no MVP transformation is required-
layout (location = 1) in vec2 inUVs;

layout(location = 0) out vec2 outUVs;				// UVs
layout(location = 1) flat out LightPD outLight[NUMLIGHTS];

void main()
{
	//gl_Position.x = gl_Position.x * ubo.aspRatio.x;
	gl_Position = vec4(inPos, 1.0f);
    outUVs = inUVs;
	
	for(int i = 0; i < NUMLIGHTS; i++) 
	{
		outLight[i].position.xyz  = ubo.light[i].position.xyz;							// for point & spot light
		outLight[i].direction.xyz = normalize(ubo.light[i].direction.xyz);				// for directional & spot light
	}
}