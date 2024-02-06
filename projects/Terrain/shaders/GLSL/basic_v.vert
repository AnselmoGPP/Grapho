#version 450
#extension GL_ARB_separate_shader_objects : enable
#pragma shader_stage(vertex)

#include "..\..\..\projects\Terrain\shaders\GLSL\vertexTools.vert"

layout(set = 0, binding = 0) uniform globalUbo {
    mat4 view;
    mat4 proj;
    vec4 camPos_t;
} gUbo;

layout(set = 0, binding = 1) uniform ubObj {
    mat4 model;					// mat4
    mat4 normalMatrix;			// mat3
} ubo[1];						// [i]: array of descriptors

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUVs;
//normal: layout(location = 3) in vec2 inTan;

layout(location = 0) out vec3 outPos;						// world space vertex position
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outUVs;
//normal: layout(location = 3) out TB outTB;				// Tangents & Bitangents

int i = gl_InstanceIndex;

void main()
{
	vec3 pos = inPos;
	//displace: pos.x += 0.2;
	//waving: pos += vec3(1,0,0) * sin(<speed> * (gUbo.camPos_t.w + ubo[i].model[0][0])) * (<amplitude> * inPos.z);	// move axis (0,0,1)
	
	gl_Position = gUbo.proj * gUbo.view * ubo[i].model * vec4(pos, 1.0);
	outPos = (ubo[i].model * vec4(pos, 1.0)).xyz;
	outNormal = mat3(ubo[i].normalMatrix) * inNormal;
	//verticalNormals: outNormal = mat3(ubo[i].normalMatrix) * vec3(0,0,1);
	outUVs = inUVs;
		
	//backfaceNormals: if(dot(outNormal, normalize(gUbo.camPos_t.xyz - outPos)) < 0) outNormal *= -1;
	//sunfaceNormals: if(dot(outNormal, ubo[0].light[0].direction.xyz) > 0) outNormal *= -1;
	
	//normal: outTB = getTB(inNormal, inTan);
}