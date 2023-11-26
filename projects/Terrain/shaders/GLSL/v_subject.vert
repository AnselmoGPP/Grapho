#version 450
#extension GL_ARB_separate_shader_objects : enable
#pragma shader_stage(vertex)

#include "..\..\..\projects\Terrain\shaders\GLSL\vertexTools.vert"

//#define MIN_HEIGHT 2014
//#define MAX_HEIGHT 2090
//#define MAX_SLOPE 0.22

layout(set = 0, binding = 0) uniform ubobject {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 normalMatrix;			// mat3
	vec4 camPos;
	//vec4 camPos_time;			// camPos + time
	//vec4 modelPos_gSlope;		// vec3 + float
	LightPD light[NUMLIGHTS];	// n * (2 * vec4)
} ubo;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUVs;

layout(location = 0) out vec3 outPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outUVs;
layout(location = 3) flat out vec3 outCamPos;
//layout(location = 4) flat out vec3 outModelPos;
//layout(location = 5) out float outSqrDist;
layout(location = 6) flat out LightPD outLight[NUMLIGHTS];	// light positions & directions

void main()
{
	//vec3 pos      = inPos;												// position without MVP matrix applied yet
	//vec3 modelPos = ubo.modelPos_gSlope.xyz;
	//float gSlope  = ubo.modelPos_gSlope.a;
	//float sqrDist = getSqrDist(modelPos, ubo.camPos_time.xyz);			// dist modelPos-camPos
	//float height  = getLength(modelPos);
	
	// Others
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPos, 1.0);
	outPos      = (ubo.model * vec4(inPos, 1.0)).xyz;
	outNormal   = mat3(ubo.normalMatrix) * inNormal;
	outUVs      = inUVs;
	outCamPos   = ubo.camPos.xyz;
	//outModelPos = modelPos;
	//outSqrDist  = getSqrDist(ubo.camPos_time.xyz, (ubo.model * vec4(pos, 1.0)).xyz);
	
	for(int i = 0; i < NUMLIGHTS; i++) 
	{
		outLight[i].position.xyz  = ubo.light[i].position.xyz;						// for point & spot light
		outLight[i].direction.xyz = normalize(ubo.light[i].direction.xyz);			// for directional & spot light
	}
}