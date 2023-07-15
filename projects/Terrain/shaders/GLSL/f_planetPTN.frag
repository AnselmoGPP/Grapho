#version 450
#extension GL_ARB_separate_shader_objects : enable
#pragma shader_stage(fragment)

#include "..\..\..\projects\Terrain\shaders\GLSL\fragTools.vert"

#define RADIUS 2000

layout(set = 0, binding = 1) uniform ubobject		// https://www.reddit.com/r/vulkan/comments/7te7ac/question_uniforms_in_glsl_under_vulkan_semantics/
{
	LightProps light[NUMLIGHTS];
} ubo;

layout(set = 0, binding  = 2) uniform sampler2D texSampler[27];		// sampler1D, sampler2D, sampler3D

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

vec3  getDryColor	  (vec3 color, float minHeight, float maxHeight);	

void getTexture_Sand(inout vec3 result);
vec3 getTexture_GrassRock();

// Definitions:

void main()
{
	savePrecalcLightValues(inPos, inCamPos, ubo.light, inLight);
	savePNT(inPos, inNormal, inTB3);
	
	//getTexture_Sand(color);
	vec3 color = getTexture_GrassRock();
	//color = applyParabolicFog(color, vec3(0,0,0), 50, 200, inPos, inCamPos);
	
	//outColor = vec4(inColor * texture(texSampler, inTexCoord).rgb, 1.0);
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
	float ratioRock = getRockRatio(0.22, 0.02);					// params: slopeThreshold, mixRange
	float ratioSnow = getSnowRatio(0.1, 100, 140);				// params: mixRange, minHeight, maxHeight

	// Get textures
	vec3 grassPar[2];
	vec3 rockPar [2];
	vec3 snow1Par[2];
	vec3 snow2Par[2];
	
	bool getGrass = (ratioRock < 1.f || ratioSnow < 1.f);
	bool getRock  = (ratioRock > 0.f);
	bool getSnow  = (ratioSnow > 0.f);

	float lowResDist = sqrt(inCamSqrHeight - RADIUS * RADIUS);	// Distance from where low resolution starts
	if(lowResDist < 700) lowResDist = 700;						// Minimum low res. distance
	
	vec3 dryColor = getDryColor(vec3(0.9, 0.6, 0), RADIUS + 15, RADIUS + 70);

	int i;
	if(inDist > lowResDist * 1.0)
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
		
			if(ratioMix == 1.f)
			{
				grassPar[1] = grassPar[0];
				rockPar [1] = rockPar [0];
				snow1Par [1] = snow1Par [0];
				snow2Par [1] = snow2Par [0];
				break;
			}
		}
	}
	else
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
					
			if(ratioMix == 1.f)
			{
				grassPar[1] = grassPar[0];
				rockPar [1] = rockPar [0];
				snow1Par [1] = snow1Par [0];
				snow2Par [1] = snow2Par [0];
				break;
			}
		}
	}
	
	// Get material colors depending upon distance mixing resolution.
	vec3 grass = mix(grassPar [1], grassPar [0], ratioMix);
	vec3 rock  = mix(rockPar  [1], rockPar  [0], ratioMix);
	vec3 snowP = mix(snow1Par [1], snow1Par [0], ratioMix);		// plain snow
	vec3 snowR = mix(snow2Par [1], snow2Par [0], ratioMix);		// rough snow

	// Grass + Rock:

	vec3 result;

	if(inDist < 5) 
	{
		float grassHeight  = triplanarNoColor(texSampler[ 4], tf[0]).r;
		//float grass2Height = triplanarTexture(texSampler[ 4], tf[1]).r;
		float rockHeight   = triplanarNoColor(texSampler[ 9], tf[0]).r;
		//float rock2Height  = triplanarTexture(texSampler[ 9], tf[1]).r;
		//float snowHeight   = triplanarTexture(texSampler[14], tf[0]).r;
		//float snow2Height  = triplanarTexture(texSampler[14], tf[1]).r;
		
		float depth = 0.1;
		float ma = max(rockHeight  + ratioRock, grassHeight + (1-ratioRock)) - depth;
		float b1 = max(rockHeight  + ratioRock     - ma, 0);
		float b2 = max(grassHeight + (1-ratioRock) - ma, 0);
		result = (rock * b1 + grass * b2) / (b1 + b2);
	}
	else result = mix(grass, rock, ratioRock);
	
	// Snow:
	
	vec3 snow = mix(snowP, snowR, ratioRock);
	return mix(result, snow, ratioSnow);
}

vec3 getTexture_GrassRock2()
{
	// Texture resolution and mixing ratio (mixes of same texture between distances).
	float tf[2];	// texture factors
	float ratio = getTexScaling(inDist, 15, 40, 0.2, tf[0], tf[1]);	// initialTexFactor, baseDist, mixRange, minHeight, maxHeight
		
	// Get textures
	vec3 grassPar[2];
	vec3 rockPar [2];
	vec3 snowPar [2];

	float lowResDist = sqrt(inCamSqrHeight - RADIUS * RADIUS);	// Distance from where low resolution starts
	if(lowResDist < 750) lowResDist = 750;						// Minimum low res. distance

	if(inDist > lowResDist * 1.0)
		for(int i = 0; i < 2; i++)
		{
			grassPar[i]  = getFragColor(
				triplanarTexture(texSampler[0], tf[i]).rgb,
				inNormal,
				vec3(0.06, 0.06, 0.06),
				200 );
		
			rockPar[i] = getFragColor(
				triplanarTexture(texSampler[5], tf[i]).rgb,
				inNormal,
				vec3(0.1, 0.1, 0.1),
				125 );
			
			snowPar[i] = getFragColor(
				triplanarTexture(texSampler[10], tf[i]).rgb,
				inNormal,
				vec3(0.2, 0.2, 0.2),
				125 );
		}
	else
		for(int i = 0; i < 2; i++)
		{
			grassPar[i]  = getFragColor(
				triplanarTexture(texSampler[0], tf[i]).rgb,
				triplanarNormal (texSampler[1], tf[i]),
				triplanarNoColor(texSampler[2], tf[i]).rgb,
				triplanarNoColor(texSampler[3], tf[i]).r * 255 );
		
			rockPar[i] = getFragColor(
				triplanarTexture(texSampler[5], tf[i]).rgb,
				triplanarNormal (texSampler[6], tf[i]),
				triplanarNoColor(texSampler[7], tf[i]).rgb,
				triplanarNoColor(texSampler[8], tf[i]).r * 255 );
			
			snowPar[i] = getFragColor(
				triplanarTexture(texSampler[10], tf[i]).rgb,
				triplanarNormal (texSampler[11], tf[i]),
				triplanarNoColor(texSampler[12], tf[i]).rgb,
				triplanarNoColor(texSampler[13], tf[i]).r * 255 );
		}
	
	// Get material colors depending upon distance mixing resolution.
	vec3 grass = grassPar[0] * ratio + grassPar[1] * (1 - ratio);
	vec3 rock  = rockPar [0] * ratio + rockPar [1] * (1 - ratio);
	vec3 snow  = snowPar [0] * ratio + snowPar [1] * (1 - ratio);

	// Grass + Rock:
	ratio = getRockRatio(0.22, 0.02);	// params: slopeThreshold, mixRange
	vec3 result;

	if(inDist < 5) 
	{
		float grassHeight  = triplanarNoColor(texSampler[ 4], tf[0]).r;
		float grass2Height = triplanarNoColor(texSampler[ 4], tf[1]).r;
		float rockHeight   = triplanarNoColor(texSampler[ 9], tf[0]).r;
		float rock2Height  = triplanarNoColor(texSampler[ 9], tf[1]).r;
		float snowHeight   = triplanarNoColor(texSampler[14], tf[0]).r;
		float snow2Height  = triplanarNoColor(texSampler[14], tf[1]).r;
		
		float ma = max(rockHeight  + ratio, grassHeight + (1-ratio)) - 0.1;		// 0.1 = depth
		float b1 = max(rockHeight  + ratio     - ma, 0);
		float b2 = max(grassHeight + (1-ratio) - ma, 0);
		result = (rock * b1 + grass * b2) / (b1 + b2);
	}
	else result = rock * (ratio) + grass * (1-ratio);
	
	// Snow:
	ratio = getSnowRatio(0.1, 100, 140);		// params: mixRange, minHeight, maxHeight
	return result * (1 - ratio) + snow * (ratio);
}

vec3 getDryColor(vec3 color, float minHeight, float maxHeight)
{
	vec3 increment = vec3(1-color.x, 1-color.y, 1-color.z);
	float ratio = 1 - clamp((inGroundHeight - minHeight) / (maxHeight - minHeight), 0, 1);
	return color + increment * ratio;
}


