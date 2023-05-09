#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "..\..\..\projects\Terrain\shaders\GLSL\fragTools.vert"

#define RADIUS 2020


layout(set = 0, binding = 1) uniform ubobject		// https://www.reddit.com/r/vulkan/comments/7te7ac/question_uniforms_in_glsl_under_vulkan_semantics/
{
	LightProps light[NUMLIGHTS];
} ubo;

layout(set = 0, binding  = 2) uniform sampler2D texSampler[32];		// sampler1D, sampler2D, sampler3D

layout(location = 0)  		in vec3 	inPos;
layout(location = 1)  flat	in vec3 	inCamPos;
layout(location = 2)  		in vec3 	inNormal;
layout(location = 3)  		in float	inSlope;
layout(location = 4)  		in float	inDist;
layout(location = 5)  flat	in float	inCamSqrHeight;
layout(location = 6)		in float	inGroundHeight;
layout(location = 7) flat   in float    inTime;
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
	
	vec3 color = getTex_Sea();
	outColor = vec4(color, getTransparency(0.1, 20, 40));
}

vec3 getTex_Sea()
{	
	// Colors: https://colorswall.com/palette/63192
	// Colors: https://www.color-hex.com/color-palette/101255

	return getFragColor( 
		vec3(36, 76, 92) / 255.f,
		triplanarNormal_Sea(texSampler[31], 10, 2, inTime),
		vec3(1, 1, 1),
		30 );
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

