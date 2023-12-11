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
//layout(location = 4) flat in vec3 inModelPos;
//layout(location = 5) in float inSqrDist;
layout(location = 4) flat in LightPD inLight[NUMLIGHTS];			// light positions & directions

layout(location = 0) out vec4 outColor;								// layout(location=0) specifies the index of the framebuffer (usually, there's only one).


void main()
{	
	//vec4 albedo = texture(texSampler[0], unpackUVmirror(inUVs, 1));
	vec4 albedo = texture(texSampler[0], inUVs);
	if(albedo.a < 0.5) { discard; return; }		// Discard transparent fragments
	
	//if(applyOrderedDithering(inSqrDist, 2, 0)) { discard; return; }

	savePrecalcLightValues(inPos, inCamPos, ubo.light, inLight);
	
	//outColor = vec4(0, 1, 0, 1);
	outColor.w = 1;
	outColor.xyz = getFragColor(albedo.xyz, inNormal, vec3(0.05, 0.05, 0.05), 0.5);
}
