#version 450
#extension GL_ARB_separate_shader_objects : enable
#pragma shader_stage(fragment)

#include "..\..\..\projects\Terrain\shaders\GLSL\fragTools.vert"


layout(set = 0, binding = 1) uniform ubobject		// https://www.reddit.com/r/vulkan/comments/7te7ac/question_uniforms_in_glsl_under_vulkan_semantics/
{
	LightProps light[NUMLIGHTS];
} ubo;

layout(set = 0, binding  = 2) uniform sampler2D texSampler[1];		// sampler1D, sampler2D, sampler3D

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUVs;
layout(location = 3) flat in vec3 inCamPos;
//normal: layout(location = 4) in TB inTB;
layout(location = 4) flat in LightPD inLight[NUMLIGHTS];			// light positions & directions

layout(location = 0) out vec4 outColor;								// layout(location=0) specifies the index of the framebuffer (usually, there's only one).


void main()
{	
	vec4 albedo = vec4(0.5, 0.5, 0.5, 1);
	//discardAlpha: if(albedo.a < 0.6) { discard; return; }			// Discard non-visible fragments
	//distDithering: if(applyOrderedDithering(getDist(inCamPos, inPos), 40, 50)) { discard; return; }
	vec3 normal = normalize(inNormal);	
	vec3 specular = vec3(0, 0, 0);
	float roughness = 0;	
	
	savePrecalcLightValues(inPos, inCamPos, ubo.light, inLight);
	//reduceNightLight: modifySavedSunLight(inPos);
	
	outColor.w = 1;
	outColor.xyz = getFragColor(albedo.xyz, normal, specular, roughness );
}