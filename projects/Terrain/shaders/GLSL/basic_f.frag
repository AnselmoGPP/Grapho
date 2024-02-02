#version 450
#extension GL_ARB_separate_shader_objects : enable
#pragma shader_stage(fragment)

#include "..\..\..\projects\Terrain\shaders\GLSL\fragTools.vert"

//earlyDepthTest: layout(early_fragment_tests) in;

layout(set = 0, binding = 1) uniform ubobject		// https://www.reddit.com/r/vulkan/comments/7te7ac/question_uniforms_in_glsl_under_vulkan_semantics/
{
	Light light[NUMLIGHTS];
} ubo;

layout(set = 0, binding  = 2) uniform sampler2D texSampler[1];		// sampler1D, sampler2D, sampler3D

layout(location = 0) in vec3 inPos;									// world space vertex position
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUVs;
layout(location = 3) flat in vec3 inCamPos;
//normal: layout(location = 4) in TB inTB;

//layout(location = 0) out vec4 outColor;							// layout(location=0) specifies the index of the framebuffer (usually, there's only one).
layout (location = 0) out vec4 gPos;
layout (location = 1) out vec4 gAlbedo;
layout (location = 2) out vec4 gNormal;
layout (location = 3) out vec4 gSpecRoug;

vec3 getDryColor(vec3 color, float minHeight, float maxHeight)
{
	vec3 increment = vec3(1-color.x, 1-color.y, 1-color.z);
	float ratio = 1 - clamp((length(inPos) - minHeight) / (maxHeight - minHeight), 0, 1);
	return color + increment * ratio;
}

void main()
{	
	vec4 albedo = vec4(0.5, 0.5, 0.5, 1);
	//discardAlpha: if(albedo.a < 0.5) { discard; return; }														// Discard non-visible fragments
	//distDithering: if(applyOrderedDithering(getDist(inCamPos, inPos), near, far)) { discard; return; }		// Apply dithering to distant fragments
	//dryColor: albedo = vec4(albedo.xyz * getDryColor(vec3(0.9, 0.6, 0), 2000 + 15, 2000 + 70), albedo.w);		// Apply dry color to upper fragments
	vec3 normal = normalize(inNormal);
	vec3 specular = vec3(0, 0, 0);
	float roughness = 0;	
	
	savePrecalcLightValues(inPos, inCamPos, ubo.light);
	//reduceNightLight: modifySavedSunLight(inPos);
	
	gPos      = vec4(inPos, 1.0);
	gAlbedo   = albedo;
	gNormal   = vec4(normalize(inNormal), 1.0);
	gSpecRoug = vec4(specular, roughness);
}