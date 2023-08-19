#version 450
#extension GL_ARB_separate_shader_objects : enable
#pragma shader_stage(fragment)

#include "..\..\..\projects\Terrain\shaders\GLSL\fragTools.vert"


layout(set = 0, binding = 1) uniform ubobject		// https://www.reddit.com/r/vulkan/comments/7te7ac/question_uniforms_in_glsl_under_vulkan_semantics/
{
	LightProps light[NUMLIGHTS];
} ubo;

layout(set = 0, binding  = 2) uniform sampler2D texSampler[4];		// sampler1D, sampler2D, sampler3D

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUVs;
layout(location = 3) flat in vec3 inCamPos;
layout(location = 4) flat in vec3 inModelPos;
layout(location = 5) in float inSqrDist;
layout(location = 6) flat in LightPD inLight[NUMLIGHTS];			// light positions & directions

layout(location = 0) out vec4 outColor;								// layout(location=0) specifies the index of the framebuffer (usually, there's only one).


void main()
{
	savePrecalcLightValues(inPos, inCamPos, ubo.light, inLight);
	
	//if(applyOrderedDithering(inSqrDist, 2, 0)) { discard; return; }
	
	// Choose texture
	vec4 albedo;
	vec4 noise = texture(texSampler[3], inModelPos.xy/100.f);
	if(noise.x < 0.2)       albedo = texture(texSampler[0], unpackUVmirror(inUVs, 1));		// vec4(1, 0, 0, 1);
	else if (noise.x > 0.4) albedo = texture(texSampler[1], unpackUVmirror(inUVs, 1));		// vec4(0, 1, 0, 1);
	else                    albedo = texture(texSampler[2], unpackUVmirror(inUVs, 1));		// vec4(0, 0, 1, 1);
	
	if(albedo.a < 0.2) { discard; return; }		// discard transparent fragments

	// Transparency (distance to camPos)
	//float dist = getDist(inPos, inCamPos);
	//if(dist < 1) albedo.a *= getRatio(dist, 0.4, 1);
	//if(dist > 14) albedo.a *= 1.f - getRatio(dist, 14, 17);
	
	// Transparency (roots)
	albedo.a *= getRatio(inUVs.y, 0.1, 0.2);
	
	// Color filter
	albedo.xyz *= vec3(0.4, 0.4, 0.4);
	
	// Light
	albedo.xyz = getFragColor(albedo.xyz, inNormal, vec3(0.1, 0.1, 0.1), 0.8);
	
	//outColor.xyz = getFragColor(albedo.xyz, inNormal, vec3(0, 0, 0), 0.f);
	//outColor.w = albedo.w;
	outColor = albedo;
}
