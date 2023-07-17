#version 450
#extension GL_ARB_separate_shader_objects : enable
#pragma shader_stage(fragment)

#include "..\..\..\projects\Terrain\shaders\GLSL\fragTools.vert"

#define RADIUS 2000

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
layout(location = 7)  		in TB3	 	inTB3;
layout(location = 13) flat	in LightPD	inLight[NUMLIGHTS];

layout(location = 0) out vec4 outColor;					// layout(location=0) specifies the index of the framebuffer (usually, there's only one).

// Declarations:

vec3  getDryColor (vec3 color, float minHeight, float maxHeight);	
vec3  getBlack(vec3 color, float min, float max);
void getTexture_Sand(inout vec3 result);
vec3 getTexture_GrassRock();

// Definitions:

void main()
{
	savePrecalcLightValues(inPos, inCamPos, ubo.light, inLight);
	savePNT(inPos, inNormal, inTB3);

	vec3 color = getTexture_GrassRock();
	color = getBlack(color, 2015, 2020);
	outColor = vec4(color, 1.0);
}


void getTexture_Sand(inout vec3 result)
{
    float slopeThreshold = 0.04;          // sand-plainSand slope threshold
    float mixRange       = 0.02;          // threshold mixing range (slope range)
    float tf             = 50;            // texture factor
	
	float ratio = clamp((inSlope - slopeThreshold) / (2 * mixRange), 0.f, 1.f);
		
	vec3 dunes  = getFragColor(
						triplanarTexture(texSampler[17], tf).rgb,
						triplanarNormal(texSampler[18], tf).rgb,
						triplanarNoColor(texSampler[19], tf).rgb,
						triplanarNoColor(texSampler[20], tf).r * 255 );
						
	vec3 plains = getFragColor(
						triplanarTexture(texSampler[21], tf).rgb,
						triplanarNormal(texSampler[22], tf).rgb,
						triplanarNoColor(texSampler[23], tf).rgb,
						triplanarNoColor(texSampler[24], tf).r * 255 );

	result = (ratio) * plains + (1-ratio) * dunes;
}

// Get ratio of snow given a slope threshold mixing range (mixRange), and a snow height range at equator (minHeight, maxHeight).
float getSnowRatio(float mixRange, float minHeight, float maxHeight)
{
	float lat    = atan(abs(inPos.z) / sqrt(inPos.x * inPos.x + inPos.y * inPos.y));
	float height = inGroundHeight - RADIUS;
	float decrement = maxHeight * (lat / (PI/2.f));				// height range decreases with latitude
	minHeight -= decrement;
	maxHeight -= decrement;
	float slopeThreshold = (height - minHeight) / (maxHeight - minHeight);	// Height ratio == Slope threshold
	
	return 1 - clamp((inSlope - (slopeThreshold - mixRange)) / (2 * mixRange), 0.f, 1.f);
}

// Get ratio of rock given a slope threshold (slopeThreshold) and a slope mixing range (mixRange).
float getRockRatio(float slopeThreshold, float mixRange)
{
	return clamp((inSlope - (slopeThreshold - mixRange)) / (2 * mixRange), 0.f, 1.f);
}

vec3 getTexture_GrassRock()
{	
	// Texture resolution and Ratios.
	float tf[2];												// texture factors
	float ratioMix  = getTexScaling(inDist, 15, 40, 0.2, tf[0], tf[1]);	// params: initialTexFactor, baseDist, mixRange, minHeight, maxHeight

	// Get textures
	vec3 grassPar[2];
	vec3 rockPar [2];
	vec3 snow1Par[2];
	vec3 snow2Par[2];
	vec3 sandPar [2];

	float lowResDist = sqrt(inCamSqrHeight - RADIUS * RADIUS);	// Distance from where low resolution starts
	if(lowResDist < 700) lowResDist = 700;						// Minimum low res. distance
	
	vec3 dryColor = getDryColor(vec3(0.9, 0.6, 0), RADIUS + 15, RADIUS + 70);

	int i;
	if(inDist > lowResDist * 1.0)	// Low resolution distance (far)
	{
		for(i = 0; i < 2; i++)
		{
			grassPar[i]  = getFragColor( 
				triplanarTexture(texSampler[0], tf[i]).rgb * dryColor,
				inNormal,
				vec3(0.06, 0.06, 0.06),
				200 );
				
			rockPar[i] = getFragColor(
				triplanarTexture(texSampler[5], tf[i]).rgb,
				inNormal,
				vec3(0.1, 0.1, 0.1),
				125 );
		
			snow1Par[i] = getFragColor(
				triplanarTexture(texSampler[15], tf[i]).rgb,
				inNormal,
				vec3(0.2, 0.2, 0.2),
				125 );
			
			snow2Par[i] = getFragColor(
				triplanarTexture(texSampler[10], tf[i]).rgb,
				inNormal,
				vec3(0.2, 0.2, 0.2),
				125 );
				
			sandPar[i] = getFragColor(
					triplanarTexture(texSampler[25], tf[i]).rgb,
					inNormal,
					vec3(0.2, 0.2, 0.2),
					125);
		
			if(ratioMix == 1.f)
			{
				grassPar[1] = grassPar[0];
				rockPar [1] = rockPar [0];
				snow1Par[1] = snow1Par[0];
				snow2Par[1] = snow2Par[0];
				sandPar [1] = sandPar [0];
				break;
			}
		}
	}
	else							// High resolution distance (close)
	{
		for(i = 0; i < 2; i++)
		{
				grassPar[i]  = getFragColor(
					triplanarTexture(texSampler[0], tf[i]).rgb * dryColor,
					triplanarNormal (texSampler[1], tf[i]),
					triplanarNoColor(texSampler[2], tf[i]).rgb,
					triplanarNoColor(texSampler[3], tf[i]).r * 255 );

				rockPar[i] = getFragColor(
					triplanarTexture(texSampler[5], tf[i]).rgb,
					triplanarNormal (texSampler[6], tf[i]),
					triplanarNoColor(texSampler[7], tf[i]).rgb,
					triplanarNoColor(texSampler[8], tf[i]).r * 255 );

				snow1Par[i] = getFragColor(
					triplanarTexture(texSampler[15], tf[i]).rgb,
					triplanarNormal (texSampler[16], tf[i]),
					triplanarNoColor(texSampler[17], tf[i]).rgb,
					triplanarNoColor(texSampler[18], tf[i]).r * 255 );
					
				snow2Par[i] = getFragColor(
					triplanarTexture(texSampler[10], tf[i]).rgb,
					triplanarNormal (texSampler[11], tf[i]),
					triplanarNoColor(texSampler[12], tf[i]).rgb,
					triplanarNoColor(texSampler[13], tf[i]).r * 255 );
					
				sandPar[i] = getFragColor(
					triplanarTexture(texSampler[25], tf[i]).rgb,
					triplanarNormal (texSampler[26], tf[i]),
					triplanarNoColor(texSampler[27], tf[i]).rgb,
					triplanarNoColor(texSampler[28], tf[i]).r * 255 );	
					
			if(ratioMix == 1.f)
			{
				grassPar[1] = grassPar[0];
				rockPar [1] = rockPar [0];
				snow1Par[1] = snow1Par[0];
				snow2Par[1] = snow2Par[0];
				sandPar [1] = sandPar [0];
				break;
			}
		}
	}
	
	// Get material colors depending upon distance mixing resolution.
	vec3 grass = mix(grassPar [1], grassPar [0], ratioMix);
	vec3 rock  = mix(rockPar  [1], rockPar  [0], ratioMix);
	vec3 snowP = mix(snow1Par [1], snow1Par [0], ratioMix);		// plain snow
	vec3 snowR = mix(snow2Par [1], snow2Par [0], ratioMix);		// rough snow
	vec3 sand  = mix(sandPar  [1], sandPar  [0], ratioMix);

	// Grass + Rock:

	vec3 result;
	float ratioRock = getRockRatio(0.22, 0.02);					// params: slopeThreshold, mixRange
	float ratioSnow = getSnowRatio(0.1, 100, 140);				// params: mixRange, minHeight, maxHeight
	float ratioCoast = 1.f - getRatio(inGroundHeight, 2020, 2021);

	if(inDist < 5) 
	{
		// Rock & grass
		float grassHeight  = triplanarNoColor(texSampler[ 4], tf[0]).r;
		float rockHeight   = triplanarNoColor(texSampler[ 9], tf[0]).r;
		//float sandHeight  = triplanarTexture(texSampler[29], tf[1]).r;
		result = mixByHeight(grass, rock, grassHeight, rockHeight, ratioRock, 0.1);
		
		// Rocky coast
		if(inGroundHeight < 2022)	// Rocky coast
			result = mixByHeight(result, rock, grassHeight, rockHeight, ratioCoast, 0.1);
	}
	else
	{
		result = mix(grass, rock, ratioRock);
		if(inGroundHeight < 2021) result = mix(result, rock, ratioCoast);
	}
	
	// Sand:
	float maxSlope = 1.f - getRatio(inGroundHeight, 1990, 2025);
	float sandRatio = 1.f - getRatio(inSlope, maxSlope - 0.05, maxSlope);
	result = mix(result, sand, sandRatio);
	
	// Snow:
	
	vec3 snow = mix(snowP, snowR, ratioRock);
	result = mix(result, snow, ratioSnow);
	
	return result;
}

vec3 getDryColor(vec3 color, float minHeight, float maxHeight)
{
	vec3 increment = vec3(1-color.x, 1-color.y, 1-color.z);
	float ratio = 1 - clamp((inGroundHeight - minHeight) / (maxHeight - minHeight), 0, 1);
	return color + increment * ratio;
}

vec3 getBlack(vec3 color, float min, float max)
{
	float ratio = getRatio(inGroundHeight, min, max);	
	return vec3(0,0,0) * (1.f - ratio) + color * ratio;
}

