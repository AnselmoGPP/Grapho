#version 450
#extension GL_ARB_separate_shader_objects : enable
#pragma shader_stage(fragment)

#include "..\..\..\projects\Terrain\shaders\GLSL\fragTools.vert"

#define RADIUS 2000
#define SEALEVEL 2000
#define DIST_1 5
#define DIST_2 8

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
float getBlackRatio(float min, float max);
void getTexture_Sand(inout vec3 result);
vec3 getTexture_GrassRock();
vec3 heatMap(float seaLevel, float maxHeight);
vec3 naturalMap(float seaLevel, float maxHeight);

// Definitions:

void main()
{
	//float blackRatio = 0;
	float blackRatio = getBlackRatio(1990, 2000);
	if(blackRatio == 1) { outColor = vec4(0,0,0,1); return; }
	
	savePrecalcLightValues(inPos, inCamPos, ubo.light, inLight);
	savePNT(inPos, inNormal, inTB3);
	
	vec3 color = mix(getTexture_GrassRock(), vec3(0,0,0), blackRatio);
	//vec3 color = naturalMap(RADIUS, RADIUS + 150);
	//vec3 color = heatMap(RADIUS, RADIUS + 150);
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

	result = mix(dunes, plains, ratio);
}

// Get ratio of snow given a slope threshold mixing range (mixRange), and a snow height range at equator (minHeight, maxHeight).
float getSnowRatio_Poles(float mixRange, float minHeight, float maxHeight)
{
	float lat    = atan(abs(inPos.z) / sqrt(inPos.x * inPos.x + inPos.y * inPos.y));
	float decrement = maxHeight * (lat / (PI/2.f));				// height range decreases with latitude
	float slopeThreshold = ((inGroundHeight-RADIUS) - (minHeight-decrement)) / ((maxHeight-decrement) - (minHeight-decrement));	// Height ratio == Slope threshold
	
	return getRatio(inSlope, slopeThreshold + mixRange, slopeThreshold - mixRange);
}

float getSnowRatio_Height(float mixRange, float minHeight, float maxHeight)
{
	float slopeRatio = getRatio(inGroundHeight - RADIUS, maxHeight, minHeight);
	return getRatio(inSlope, slopeRatio - mixRange, slopeRatio + mixRange);
}

// Get ratio of rock given a slope threshold (slopeThreshold) and a slope mixing range (mixRange).
float getRockRatio(float slopeThreshold, float mixRange)
{
	return clamp((inSlope - (slopeThreshold - mixRange)) / (2 * mixRange), 0.f, 1.f);
}

vec3 getTexture_GrassRock()
{	
	// Texture resolution and Ratios.
	float tf[2];														// texture factors
	float ratioMix  = getTexScaling(inDist, 10, 40, 0.2, tf[0], tf[1]);	// params: fragDist, initialTexFactor, baseDist, mixRange, texFactor1, texFactor2

	// Get textures
	vec3 grassPar[2];
	vec3 rockPar [2];
	vec3 snow1Par[2];
	vec3 snow2Par[2];
	vec3 sandPar [2];

	float lowResDist = getLowResDist(inCamSqrHeight, RADIUS, 50);
	
	vec3 dryColor = getDryColor(vec3(0.9, 0.6, 0), RADIUS + 15, RADIUS + 70);
	
	if(true || inDist > lowResDist)	// Low resolution distance (far)
	{
		for(int i = 0; i < 2; i++)
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
	else									// High resolution distance (close)
	{
		for(int i = 0; i < 2; i++)
		{
				grassPar[i]  = getFragColor(
					triplanarTexture(texSampler[0],  tf[i]).rgb * dryColor,
					triplanarNormal (texSampler[16], tf[i] * 1.1),
					triplanarNoColor(texSampler[2],  tf[i]).rgb,
					triplanarNoColor(texSampler[3],  tf[i]).r * 255 );

				rockPar[i] = getFragColor(
					triplanarTexture(texSampler[5],  tf[i]).rgb,
					triplanarNormal (texSampler[6],  tf[i]),
					triplanarNoColor(texSampler[7],  tf[i]).rgb,
					triplanarNoColor(texSampler[8],  tf[i]).r * 255 );

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

	// Grass:
	if(inDist < DIST_2)		// Close grass use different normals (snow normals Vs grass normals)
	{	
		float closeRatio = getRatio(inDist, 0.2 * DIST_2, DIST_2);
		
		grass = mix( getFragColor(
							triplanarTexture(texSampler[0], tf[0]).rgb * dryColor,
							triplanarNormal (texSampler[1], tf[0]),
							triplanarNoColor(texSampler[2], tf[0]).rgb,
							triplanarNoColor(texSampler[3], tf[0]).r * 255 ),
					 grass,
					 closeRatio );
	}

	// Grass + Rock:
	vec3 result;
	float ratioRock = getRockRatio(0.22, 0.02);					// params: slopeThreshold, mixRange
	float ratioCoast = 1.f - getRatio(inGroundHeight, SEALEVEL, SEALEVEL + 1);

	if(inDist < DIST_1)		// Very close range
	{		
		// Rock & grass
		float grassHeight = triplanarNoColor(texSampler[4], tf[0]).r;
		float rockHeight  = triplanarNoColor(texSampler[9], tf[0]).r;
		if(inDist > DIST_1 - 2)
			result  = mix(mixByHeight(grass, rock, grassHeight, rockHeight, ratioRock, 0.1),
						  mix(grass, rock, ratioRock),
						  getRatio(inDist, DIST_1 - 1, DIST_1));
		else result = mixByHeight(grass, rock, grassHeight, rockHeight, ratioRock, 0.1);
		
		// Rocky coast
		if(inGroundHeight < (SEALEVEL + 2))	// Rocky coast
			result = mixByHeight(result, rock, grassHeight, rockHeight, ratioCoast, 0.1);
	}
	else
	{
		result = mix(grass, rock, ratioRock);
		if(inGroundHeight < (SEALEVEL + 1)) result = mix(result, rock, ratioCoast);
	}
	
	// Sand:
	float maxSlope = 1.f - getRatio(inGroundHeight, SEALEVEL - 60, SEALEVEL + 5);
	float sandRatio = 1.f - getRatio(inSlope, maxSlope - 0.05, maxSlope);
	result = mix(result, sand, sandRatio);
	
	// Snow:
	//float ratioSnow = getSnowRatio_Poles(0.1, 100, 140);				// params: mixRange, minHeight, maxHeight
	float ratioSnow = getSnowRatio_Height(0.1, 90, 120);
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

float getBlackRatio(float min, float max)
{
	return 1.f - getRatio(inGroundHeight, min, max);	
}

vec3 heatMap(float seaLevel, float maxHeight)
{
	// Fill arrays (heights & colors)
	const int size = 6;

	float heights[size] = { -0.1, -0.05, 0.05, 0.15, 0.5, 0.9 };
	for(int i = 0; i < size; i++) heights[i] = seaLevel + heights[i] * (maxHeight - seaLevel);
	
	vec3 colors[size] = { vec3(0.1,0.1,0.5), vec3(0.25,0.25,1), vec3(0.25,1,0.25), vec3(1,1,0.25), vec3(1,0.25,0.25), vec3(1,1,1) };
	
	// Colorize
	if(inGroundHeight < heights[0]) return colors[0];
	
	for(int i = 1; i < size; i++)
		if(inGroundHeight < heights[i])
			return lerp(colors[i-1], colors[i],	
						(inGroundHeight - heights[i-1]) / (heights[i] - heights[i-1]) );
	
	return colors[size -1];
}

vec3 naturalMap(float seaLevel, float maxHeight)
{
	// Fill arrays (heights & colors)
	const int size = 8;
	
	float heights[size] = { -0.1, -0.05, 0.05, 0.15, 0.3, 0.5, 0.7, 0.9 };
	for(int i = 0; i < size; i++) heights[i] = seaLevel + heights[i] * (maxHeight - seaLevel);
	
	vec3 colors[size] = { vec3(0.23, 0.43, 0.79), vec3(0.25, 0.44, 0.80), vec3(0.85, 0.84, 0.53), vec3(0.38, 0.64, 0.090), vec3(0.28, 0.47, 0.06), vec3(0.39, 0.31, 0.27), vec3(0.34, 0.27, 0.24), vec3(1, 1, 1) };
	
	// Colorize
	if(inGroundHeight < heights[0]) return colors[0];
	
	for(int i = 1; i < size; i++)
		if(inGroundHeight < heights[i])
			return lerp(colors[i-1], colors[i],	
						(inGroundHeight - heights[i-1]) / (heights[i] - heights[i-1]) );
	
	return colors[size -1];
}