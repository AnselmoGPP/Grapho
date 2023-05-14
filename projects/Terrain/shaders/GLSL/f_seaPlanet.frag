#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "..\..\..\projects\Terrain\shaders\GLSL\fragTools.vert"

#define RADIUS 2020


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
layout(location = 8)  		in TB3	 	inTB3;
layout(location = 14) flat	in LightPD	inLight[NUMLIGHTS];

layout(location = 0) out vec4 outColor;					// layout(location=0) specifies the index of the framebuffer (usually, there's only one).


// Declarations:

vec3  getDryColor	  (vec3 color, float minHeight, float maxHeight);

vec3 getTex_Sea();
vec3 triplanarNormal_Sea(sampler2D tex, float texFactor, float speed);
float getTransparency(float minTransp, float minDist, float maxDist);
float getRatio(float value, float min, float max);

// Definitions:

void main()
{	
	savePrecalcLightValues(inPos, inCamPos, ubo.light, inLight);
	savePNT(inPos, inNormal, inTB3);
	
	outColor = vec4(getTex_Sea(), 1);
	//outColor = vec4(color, getTransparency(0.1, 20, 40));
}

vec3 getTex_Sea()
{	
	// Colors: https://colorswall.com/palette/63192
	// Colors: https://www.color-hex.com/color-palette/101255

	vec3 waterColor  = vec3(36, 76, 92) / 255.f;	// https://colorswall.com/palette/63192
	vec3 specularity = vec3(0.5, 0.5, 0.5);
	float roughness  = 10;
	float scale      = 150;
	float speed      = 4;

	// Green and foam
	if(inDist < 200)
	{
		vec3 finalColor = waterColor;
		
		// Green water
		vec3 depth = triplanarTexture_sea(texSampler[32], scale, speed, inTime).rgb;
		if(depth.x > 0.1)
		{
			float ratio = getRatio(depth.x, 0.1, 0.5);
			finalColor = finalColor * (1 - ratio) + vec3(0.17, 0.71, 0.61) * ratio;
			specularity *= ratio * 1.1 + 1;// <<<<<<<<<<<<<<<<<<<<
			roughness   *= ratio / 1.5 + 1;// <<<<<<<<<<<<<<<<<<<<
		}
	
		// Foam
		vec3 foam = triplanarTexture_sea(texSampler[33], scale, speed, inTime).rgb;
		if(foam.x > 0.18) 
		{
			float ratio = getRatio(foam.x, 0.18, 0.25);
			finalColor = finalColor * (1 - ratio) + vec3(0.95, 0.95, 0.95) * ratio;
			specularity *= ratio * 1.5 + 1;// <<<<<<<<<<<<<<<<<<<<
			roughness   *= ratio / 5.0 + 1;// <<<<<<<<<<<<<<<<<<<<
		}
		
		if(inDist > 100)
		{
			float ratio = getRatio(inDist, 100, 200);
			finalColor = finalColor * (1 - ratio) + waterColor * ratio;
		}
		
		waterColor = finalColor;
	}

	// Fragment color with light
	if(inDist < 150)	// Close waves
		return getFragColor( 
			waterColor,
			triplanarNormal_Sea(texSampler[31], scale, speed, inTime),
			specularity,
			roughness );
	
	if(inDist < 250)	// Mix close and far waves
	{
		vec3 normal  = triplanarNormal_Sea(texSampler[31], scale, speed, inTime);
		vec3 normal2 = triplanarNormal_Sea(texSampler[31], scale * 5, speed, inTime);
		float ratio  = getRatio(inDist, 150, 250);
		normal       = normal * (1 - ratio) + normal2 * ratio;
		
		return getFragColor( 
			waterColor,
			normal,
			specularity,
			roughness );
	}
	
	return getFragColor( 	// Far waves
			waterColor,
			triplanarNormal_Sea(texSampler[31], scale * 5, speed, inTime),
			specularity,
			roughness );
}

float getTransparency(float minTransp, float minDist, float maxDist) 
{
	return 1;
	float ratio_1;		// The closer, the more transparent
	float ratio_2;		// The lower the camera-normal angle is, the more transparent
	float ratio_3;		// The larger distance, the less transparent
	
	ratio_1 = getRatio(inDist, minDist, maxDist);				// [0,1]
	ratio_1 = getScaleRatio(ratio_1, minTransp, 1);				// [minTransp, 1]
	
	ratio_2 = getAngle(inNormal, getDirection(inPos, inCamPos));// angle normal-vertex-camera
	ratio_2 = getRatio(ratio_2, 0, PI/2);						// [0,1]
	ratio_2 = getScaleRatio(ratio_2, minTransp, 1);				// [minTransp, 1]
	
	return ratio_2;
}

vec3 getDryColor(vec3 color, float minHeight, float maxHeight)
{
	vec3 increment = vec3(1-color.x, 1-color.y, 1-color.z);
	float ratio = 1 - clamp((inGroundHeight - minHeight) / (maxHeight - minHeight), 0, 1);
	return color + increment * ratio;
}

