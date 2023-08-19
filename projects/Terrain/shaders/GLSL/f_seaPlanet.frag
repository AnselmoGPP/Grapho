#version 450
#extension GL_ARB_separate_shader_objects : enable
#pragma shader_stage(fragment)

#include "..\..\..\projects\Terrain\shaders\GLSL\fragTools.vert"

#define RADIUS 2010


layout(set = 0, binding = 1) uniform ubobject		// https://www.reddit.com/r/vulkan/comments/7te7ac/question_uniforms_in_glsl_under_vulkan_semantics/
{
	LightProps light[NUMLIGHTS];
} ubo;

layout(set = 0, binding  = 2) uniform sampler2D texSampler[34];		// sampler1D, sampler2D, sampler3D

layout(location = 0)  		in vec3 	inPos;
layout(location = 1)  flat	in vec3 	inCamPos;
layout(location = 2)  		in vec3 	inNormal;
layout(location = 3)  		in float	inSlope;
layout(location = 4)  		in float	inDist;
layout(location = 5)  flat	in float	inCamSqrHeight;
layout(location = 6)		in float	inGroundHeight;
layout(location = 7)  flat  in float    inTime;
layout(location = 8)  flat  in float    inSoilHeight;
layout(location = 9)  		in TB3	 	inTB3;
layout(location = 15) flat	in LightPD	inLight[NUMLIGHTS];

layout(location = 0) out vec4 outColor;					// layout(location=0) specifies the index of the framebuffer (usually, there's only one).


// Declarations:

vec3  getDryColor (vec3 color, float minHeight, float maxHeight);
vec3 getTex_Sea();
float getTransparency(float minAlpha, float maxDist, float minHeight, float maxHeight);

// Definitions:

void main()
{	
	savePrecalcLightValues(inPos, inCamPos, ubo.light, inLight);
	savePNT(inPos, inNormal, inTB3);
	
	outColor = vec4(getTex_Sea(), getTransparency(0.3, 50, RADIUS - 10, RADIUS - 3));
}

vec3 getTex_Sea()
{	
	// Colors: https://colorswall.com/palette/63192
	// Colors: https://www.color-hex.com/color-palette/101255

	vec3 waterColor  = vec3(0.14, 0.30, 0.36);	// https://colorswall.com/palette/63192
	vec3 specularity = vec3(0.6, 0.6, 0.6);
	float roughness  = 10;
	float scale      = 150;
	float speed      = 2;

	// Green and foam colors
	if(inDist < 200)
	{
		vec3 finalColor = waterColor;
		
		// Green water (depth map)
		vec3 depth = triplanarNoColor_Sea(texSampler[32], scale, speed, inTime).rgb;
		if(depth.x > 0.32)
		{
			float ratio = getRatio(depth.x, 0.32, 0.70);	// Mix ratio
			finalColor = mix(finalColor, vec3(0.17, 0.71, 0.61), ratio);
		}
		
		// Green water (height from nucleus)
		if(inGroundHeight > 2021) 
		{
			float ratio = getRatio(inGroundHeight, 2021, 2027);	// Mix ratio
			finalColor = mix(finalColor, vec3(0.17, 0.71, 0.61), ratio);
		}
	
		// Foam
		vec3 foam = triplanarTexture_Sea(texSampler[33], scale, speed, inTime).rgb;
		if(foam.x > 0.17) 
		{
			float ratio = getRatio(foam.x, 0.17, 0.25);	// Mix ratio
			finalColor = mix(finalColor, vec3(0.98, 0.98, 0.98), ratio);
			specularity *= ratio * 1.5 + 1;// <<<<<<<<<<<<<<<<<<<<
			roughness   *= ratio / 5.0 + 1;// <<<<<<<<<<<<<<<<<<<<
		}
		
		if(inDist > 100)	// Mix area (
		{
			float ratio = getRatio(inDist, 100, 200);
			finalColor = mix(finalColor, waterColor, ratio);
		}
		
		waterColor = finalColor;
	}

	// Normals and light
	if(inDist < 150)	// Close normals
		return getFragColor( 
			waterColor,
			triplanarNormal_Sea(texSampler[31], scale, speed, inTime),
			specularity,
			roughness );
	
	if(inDist < 300)	// Mix area (close and far normals)
	{
		vec3 normal  = triplanarNormal_Sea(texSampler[31], scale, speed, inTime);
		vec3 normal2 = triplanarNormal_Sea(texSampler[31], scale * 5, speed * 1.5, inTime);
		float ratio  = getRatio(inDist, 150, 200);
		normal       = mix(normal, normal2, ratio);
		
		return getFragColor( 
				waterColor,
				normal,
				specularity,
				roughness );
	}
	
	return getFragColor( 	// Far normals
			waterColor,
			triplanarNormal_Sea(texSampler[31], scale * 5, speed * 1.5, inTime),
			specularity,
			roughness );
}

vec3 getDryColor(vec3 color, float minHeight, float maxHeight)
{
	vec3 increment = vec3(1-color.x, 1-color.y, 1-color.z);
	float ratio = 1 - clamp((inGroundHeight - minHeight) / (maxHeight - minHeight), 0, 1);
	return color + increment * ratio;
}

float getTransparency(float minAlpha, float maxDist, float minHeight, float maxHeight)
{
	float ratio = getRatio(inDist, 0, maxDist);
	ratio = getScaleRatio(
				1.f - getRatio(inSoilHeight, minHeight, maxHeight), 
				ratio, 1.f);
	return ratio + minAlpha;
}