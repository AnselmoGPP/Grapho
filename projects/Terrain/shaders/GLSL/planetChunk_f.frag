#version 450
#extension GL_ARB_separate_shader_objects : enable
#pragma shader_stage(fragment)

#include "..\..\..\projects\Terrain\shaders\GLSL\fragTools.vert"

#define RADIUS 2000
#define SEALEVEL 2000
#define DIST_1 5
#define DIST_2 8

layout(early_fragment_tests) in;

layout(set = 0, binding = 2) uniform globalUbo {
    vec4 camPos_t;
    Light light[NUMLIGHTS];
} gUbo;

layout(set = 0, binding = 3) uniform ubobject		// https://www.reddit.com/r/vulkan/comments/7te7ac/question_uniforms_in_glsl_under_vulkan_semantics/
{
	vec4 test;
} ubo;

layout(set = 0, binding  = 4) uniform sampler2D texSampler[34];		// sampler1D, sampler2D, sampler3D

layout(location = 0)  		in vec3 	inPos;
layout(location = 1)  		in vec3 	inNormal;
layout(location = 2)  		in float	inSlope;
layout(location = 3)  		in float	inDist;
layout(location = 4)  flat	in float	inCamSqrHeight;
layout(location = 5)		in float	inGroundHeight;
layout(location = 6)  		in TB3	 	inTB3;

//layout(location = 0) out vec4 outColor;					// layout(location=0) specifies the index of the framebuffer (usually, there's only one).
layout (location = 0) out vec4 gPos;
layout (location = 1) out vec4 gAlbedo;
layout (location = 2) out vec4 gNormal;
layout (location = 3) out vec4 gSpecRoug;

// Declarations:

vec3  getDryColor (vec3 color, float minHeight, float maxHeight);	
float getBlackRatio(float min, float max);
void getTexture_Sand(inout vec3 result);
vec3 getTexture_GrassRock();
void setData_grassRock();
vec3 heatMap(float seaLevel, float maxHeight);
vec3 naturalMap(float seaLevel, float maxHeight);

// Definitions:

const int iGrass = 0, iRock = 1, iPSnow = 2, iRSnow = 3, iSand = 4;		// indices

void main()
{
	setData_grassRock();
	
	//gPos.xyz = inPos;
	//gAlbedo = vec4(0.2, 0.8, 0.2, 1.f);
	//gNormal.xyz = normalize(inNormal);
	//gSpecRoug.xyz = vec3(0.5, 0.5, 0.5);
	//gSpecRoug.w = 100;
	
	////float blackRatio = 0;
	//float blackRatio = getBlackRatio(1990, 2000);
	//if(blackRatio == 1) { outColor = vec4(0,0,0,1); return; }
	
	//vec3 color = mix(getTexture_GrassRock(), vec3(0,0,0), blackRatio);
	////vec3 color = naturalMap(RADIUS, RADIUS + 150);
	////vec3 color = heatMap(RADIUS, RADIUS + 150);
	//outColor = vec4(color, 1.0);
}


// Get ratio of rock given a slope threshold (slopeThreshold) and a slope mixing range (mixRange).
float getRockRatio(float slopeThreshold, float mixRange)
{
	return clamp((inSlope - (slopeThreshold - mixRange)) / (2 * mixRange), 0.f, 1.f);
}

// Get ratio of snow given a slope threshold mixing range (mixRange), and a snow height range at equator (minHeight, maxHeight).
float getSnowRatio_Poles(float mixRange, float minHeight, float maxHeight)
{
	float lat    = atan(abs(inPos.z) / sqrt(inPos.x * inPos.x + inPos.y * inPos.y));
	float decrement = maxHeight * (lat / (PI/2.f));				// height range decreases with latitude
	float slopeThreshold = ((inGroundHeight-RADIUS) - (minHeight-decrement)) / ((maxHeight-decrement) - (minHeight-decrement));	// Height ratio == Slope threshold
	
	return getRatio(inSlope, slopeThreshold + mixRange, slopeThreshold - mixRange);
}

float getSnowRatio_Height(float mixRange, float minHeight, float maxHeight, float slope)
{
	float slopeRatio = getRatio(inGroundHeight - RADIUS, minHeight, maxHeight);
	return getRatio(slope, slopeRatio, slopeRatio - mixRange);
}


void setData_grassRock()
{
	vec3 baseNormal = normalize(inNormal);
	
	// Texture resolution and Ratios.
	float tf[2];														// texture factors
	float ratioMix  = getTexScaling(inDist, 10, 40, 0.2, tf[0], tf[1]);	// params: fragDist, initialTexFactor, baseDist, mixRange, texFactor1, texFactor2

	// Get data
	vec3 albedo[5][2];	// grass, rock, plainSnow, roughSnow, sand
	vec3 normal[5][2];
	vec3 specul[5][2];
	float rough[5][2];
	
	vec3 grassPar[2];
	vec3 rockPar [2];
	vec3 snow1Par[2];
	vec3 snow2Par[2];
	vec3 sandPar [2];
	
	vec3 dryColor = getDryColor(vec3(0.9, 0.6, 0), RADIUS + 15, RADIUS + 70);
	
	for(int i = 0; i < 2; i++)
	{
		albedo[0][i] = triplanarTexture(texSampler[0],  tf[i], inPos, baseNormal).rgb * dryColor;
		albedo[1][i] = triplanarTexture(texSampler[5],  tf[i], inPos, baseNormal).rgb;
		albedo[2][i] = triplanarTexture(texSampler[15], tf[i], inPos, baseNormal).rgb;
		albedo[3][i] = triplanarTexture(texSampler[10], tf[i], inPos, baseNormal).rgb;
		albedo[4][i] = triplanarTexture(texSampler[25], tf[i], inPos, baseNormal).rgb;
		
		normal[0][i] = triplanarNormal (texSampler[16], tf[i] * 1.1, inPos, baseNormal, inTB3);
		normal[1][i] = triplanarNormal (texSampler[6],  tf[i], inPos, baseNormal, inTB3);
		normal[2][i] = triplanarNormal (texSampler[16], tf[i], inPos, baseNormal, inTB3);
		normal[3][i] = triplanarNormal (texSampler[11], tf[i], inPos, baseNormal, inTB3);
		normal[4][i] = triplanarNormal (texSampler[26], tf[i], inPos, baseNormal, inTB3);
		
		specul[0][i] = triplanarNoColor(texSampler[2],  tf[i], inPos, baseNormal).rgb;
        specul[1][i] = triplanarNoColor(texSampler[7],  tf[i], inPos, baseNormal).rgb;
        specul[2][i] = triplanarNoColor(texSampler[17], tf[i], inPos, baseNormal).rgb;
        specul[3][i] = triplanarNoColor(texSampler[12], tf[i], inPos, baseNormal).rgb;
        specul[4][i] = triplanarNoColor(texSampler[27], tf[i], inPos, baseNormal).rgb;
		
		rough [0][i] = triplanarNoColor(texSampler[3],  tf[i], inPos, baseNormal).r;
        rough [1][i] = triplanarNoColor(texSampler[8],  tf[i], inPos, baseNormal).r;
        rough [2][i] = triplanarNoColor(texSampler[18], tf[i], inPos, baseNormal).r;
        rough [3][i] = triplanarNoColor(texSampler[13], tf[i], inPos, baseNormal).r;
        rough [4][i] = triplanarNoColor(texSampler[28], tf[i], inPos, baseNormal).r;
		
		if(false || ratioMix == 1.f) {
			albedo[0][1] = albedo[0][0]; 
			albedo[1][1] = albedo[1][0]; 			
			albedo[2][1] = albedo[2][0]; 			
			albedo[3][1] = albedo[3][0]; 			
			albedo[4][1] = albedo[4][0]; 			
			
			normal[0][1] = normal[0][0]; 			
			normal[1][1] = normal[1][0]; 			
			normal[2][1] = normal[2][0]; 			
			normal[3][1] = normal[3][0]; 			
			normal[4][1] = normal[4][0]; 			
			
			specul[0][1] = specul[0][0]; 			
			specul[1][1] = specul[1][0]; 			
			specul[2][1] = specul[2][0]; 			
			specul[3][1] = specul[3][0]; 			
			specul[4][1] = specul[4][0]; 			
			
			rough[0][1] = rough[0][0]; 			
			rough[1][1] = rough[1][0]; 			
			rough[2][1] = rough[2][0]; 			
			rough[3][1] = rough[3][0]; 			
			rough[4][1] = rough[4][0]; 			

			break;
		}
	}
	
	// Get material colors depending upon distance mixing resolution.
	vec3 alb[5];
	vec3 nor[5];
	vec3 spe[5];
	float rou[5];
	
	{
		alb[0] = mix(albedo[0][1], albedo[0][0], ratioMix);
		alb[1] = mix(albedo[1][1], albedo[1][0], ratioMix);
		alb[2] = mix(albedo[2][1], albedo[2][0], ratioMix);
		alb[3] = mix(albedo[3][1], albedo[3][0], ratioMix);
		alb[4] = mix(albedo[4][1], albedo[4][0], ratioMix);
	
		
		nor[0] = mix(normal[0][1], normal[0][0], ratioMix);
		nor[1] = mix(normal[1][1], normal[1][0], ratioMix);
		nor[2] = mix(normal[2][1], normal[2][0], ratioMix);
		nor[3] = mix(normal[3][1], normal[3][0], ratioMix);
		nor[4] = mix(normal[4][1], normal[4][0], ratioMix);
	
		
		spe[0] = mix(specul[0][1], specul[0][0], ratioMix);
		spe[1] = mix(specul[1][1], specul[1][0], ratioMix);
		spe[2] = mix(specul[2][1], specul[2][0], ratioMix);
		spe[3] = mix(specul[3][1], specul[3][0], ratioMix);
		spe[4] = mix(specul[4][1], specul[4][0], ratioMix);
	
		
		rou[0] = mix(rough[0][1], rough[0][0], ratioMix);
		rou[1] = mix(rough[1][1], rough[1][0], ratioMix);
		rou[2] = mix(rough[2][1], rough[2][0], ratioMix);
		rou[3] = mix(rough[3][1], rough[3][0], ratioMix);
		rou[4] = mix(rough[4][1], rough[4][0], ratioMix);
	}
	
	// Grass:
	if(inDist < DIST_2)		// Close grass use different normals (snow normals Vs grass normals)
	{	
		float closeRatio = getRatio(inDist, 0.2 * DIST_2, DIST_2);
		
		alb[0] = mix(triplanarTexture(texSampler[0], tf[0], inPos, baseNormal).rgb * dryColor, albedo[0][0], closeRatio);
		nor[0] = mix(triplanarNormal (texSampler[1], tf[0], inPos, baseNormal, inTB3),         normal[0][0], closeRatio);
		spe[0] = mix(triplanarNoColor(texSampler[2], tf[0], inPos, baseNormal).rgb,            specul[0][0], closeRatio);
		rou[0] = mix(triplanarNoColor(texSampler[3], tf[0], inPos, baseNormal).r,              rough [0][0], closeRatio);
	}

	// Grass + Rock:
	float ratioRock = getRockRatio(0.22, 0.02);					// params: slopeThreshold, mixRange
	float ratioCoast = 1.f - getRatio(inGroundHeight, SEALEVEL, SEALEVEL + 1);

	if(inDist < DIST_1)		// Very close range
	{		
		// Rock & grass
		float grassHeight = triplanarNoColor(texSampler[4], tf[0], inPos, baseNormal).r;
		float rockHeight  = triplanarNoColor(texSampler[9], tf[0], inPos, baseNormal).r;

		if(inDist > (DIST_1 - 2))
		{
			alb[0]  = mix(mixByHeight(alb[0], alb[1], grassHeight, rockHeight, ratioRock, 0.1),
						mix(alb[0], alb[1], ratioRock),
						getRatio(inDist, DIST_1 - 1, DIST_1));
			nor[0]  = mix(mixByHeight(nor[0], nor[1], grassHeight, rockHeight, ratioRock, 0.1),
						mix(nor[0], nor[1], ratioRock),
						getRatio(inDist, DIST_1 - 1, DIST_1));
			spe[0]  = mix(mixByHeight(spe[0], spe[1], grassHeight, rockHeight, ratioRock, 0.1),
						mix(spe[0], spe[1], ratioRock),
						getRatio(inDist, DIST_1 - 1, DIST_1));
			rou[0]  = mix(mixByHeight(rou[0], rou[1], grassHeight, rockHeight, ratioRock, 0.1),
						mix(rou[0], rou[1], ratioRock),
						getRatio(inDist, DIST_1 - 1, DIST_1));
		}
		else
		{
			alb[0] = mixByHeight(alb[0], alb[1], grassHeight, rockHeight, ratioRock, 0.1);
			nor[0] = mixByHeight(nor[0], nor[1], grassHeight, rockHeight, ratioRock, 0.1);
			spe[0] = mixByHeight(spe[0], spe[1], grassHeight, rockHeight, ratioRock, 0.1);
			rou[0] = mixByHeight(rou[0], rou[1], grassHeight, rockHeight, ratioRock, 0.1);
		}
		
		// Rocky coast
		if(inGroundHeight < (SEALEVEL + 2))	// Rocky coast
		{
			alb[0] = mixByHeight(alb[0], alb[1], grassHeight, rockHeight, ratioCoast, 0.1);
			nor[0] = mixByHeight(nor[0], nor[1], grassHeight, rockHeight, ratioCoast, 0.1);
			spe[0] = mixByHeight(spe[0], spe[1], grassHeight, rockHeight, ratioCoast, 0.1);
			rou[0] = mixByHeight(rou[0], rou[1], grassHeight, rockHeight, ratioCoast, 0.1);
		}
	}
	else
	{
		alb[0] = mix(alb[0], alb[1], ratioRock);
		nor[0] = mix(nor[0], nor[1], ratioRock);
		spe[0] = mix(spe[0], spe[1], ratioRock);
		rou[0] = mix(rou[0], rou[1], ratioRock);
		
		if(inGroundHeight < (SEALEVEL + 1))
		{
			alb[0] = mix(alb[0], alb[1], ratioCoast);
			nor[0] = mix(nor[0], nor[1], ratioCoast);
			spe[0] = mix(spe[0], spe[1], ratioCoast);
			rou[0] = mix(rou[0], rou[1], ratioCoast);
		}
	}
	
	// Sand:
	float maxSlope = 1.f - getRatio(inGroundHeight, SEALEVEL - 60, SEALEVEL + 5);
	float sandRatio = 1.f - getRatio(inSlope, maxSlope - 0.05, maxSlope);
	alb[0] = mix(alb[0], alb[4], sandRatio);
	nor[0] = mix(nor[0], nor[4], sandRatio);
	spe[0] = mix(spe[0], spe[4], sandRatio);
	rou[0] = mix(rou[0], rou[4], sandRatio);
	
	// Snow:
	//float ratioSnow = getSnowRatio_Poles(0.1, 100, 140);				// params: mixRange, minHeight, maxHeight
	//float slope = dot(baseNormal, nor[0]); 
	float ratioSnow = getSnowRatio_Height(0.1, 100, 120, inSlope);
	alb[2] = mix(alb[2], alb[3], ratioRock);	// mix plain snow and rough snow
	nor[2] = mix(nor[2], nor[3], ratioRock);
	spe[2] = mix(spe[2], spe[3], ratioRock);
	rou[2] = mix(rou[2], rou[3], ratioRock);
	
	alb[0] = mix(alb[0], alb[2], ratioSnow);	// mix snow with soil
	nor[0] = mix(nor[0], nor[2], ratioSnow);
	spe[0] = mix(spe[0], spe[2], ratioSnow);
	rou[0] = mix(rou[0], rou[2], ratioSnow);
	
	// Set g-buffer
	gPos = vec4(inPos, 1.0);
	gAlbedo = vec4(alb[0], 1.0);
	gNormal = vec4(normalize(nor[0]), 1.0);
	gSpecRoug = vec4(spe[0], rou[0]);
}

void getTexture_Sand(inout vec3 result)
{
    float slopeThreshold = 0.04;          // sand-plainSand slope threshold
    float mixRange       = 0.02;          // threshold mixing range (slope range)
    float tf             = 50;            // texture factor
	vec3 baseNormal      = normalize(inNormal);
	
	float ratio = clamp((inSlope - slopeThreshold) / (2 * mixRange), 0.f, 1.f);
		
	vec3 dunes  = getFragColor(
						triplanarTexture(texSampler[17], tf, inPos, baseNormal).rgb,
						triplanarNormal(texSampler[18], tf, inPos, baseNormal, inTB3).rgb,
						triplanarNoColor(texSampler[19], tf, inPos, baseNormal).rgb,
						triplanarNoColor(texSampler[20], tf, inPos, baseNormal).r * 255 );
						
	vec3 plains = getFragColor(
						triplanarTexture(texSampler[21], tf, inPos, baseNormal).rgb,
						triplanarNormal(texSampler[22], tf, inPos, baseNormal, inTB3).rgb,
						triplanarNoColor(texSampler[23], tf, inPos, baseNormal).rgb,
						triplanarNoColor(texSampler[24], tf, inPos, baseNormal).r * 255 );

	result = mix(dunes, plains, ratio);
}


vec3 getTexture_GrassRock()
{	
	// Texture resolution and Ratios.
	float tf[2];														// texture factors
	float ratioMix  = getTexScaling(inDist, 10, 40, 0.2, tf[0], tf[1]);	// params: fragDist, initialTexFactor, baseDist, mixRange, texFactor1, texFactor2
	vec3 baseNormal = normalize(inNormal);

	// Get textures
	vec3 grassPar[2];
	vec3 rockPar [2];
	vec3 snow1Par[2];
	vec3 snow2Par[2];
	vec3 sandPar [2];

	float lowResDist = getLowResDist(inCamSqrHeight, RADIUS, 50);
	
	vec3 dryColor = getDryColor(vec3(0.9, 0.6, 0), RADIUS + 15, RADIUS + 70);
	
	if(inDist > lowResDist)	// Low resolution distance (far)
	{
		for(int i = 0; i < 2; i++)
		{
			grassPar[i]  = getFragColor( 
				triplanarTexture(texSampler[0], tf[i], inPos, baseNormal).rgb * dryColor,
				normalize(inNormal),
				vec3(0.06, 0.06, 0.06),
				200 );
				
			rockPar[i] = getFragColor(
				triplanarTexture(texSampler[5], tf[i], inPos, baseNormal).rgb,
				normalize(inNormal),
				vec3(0.1, 0.1, 0.1),
				125 );
		
			snow1Par[i] = getFragColor(
				triplanarTexture(texSampler[15], tf[i], inPos, baseNormal).rgb,
				normalize(inNormal),
				vec3(0.2, 0.2, 0.2),
				125 );
			
			snow2Par[i] = getFragColor(
				triplanarTexture(texSampler[10], tf[i], inPos, baseNormal).rgb,
				normalize(inNormal),
				vec3(0.2, 0.2, 0.2),
				125 );
				
			sandPar[i] = getFragColor(
				triplanarTexture(texSampler[25], tf[i], inPos, baseNormal).rgb,
				normalize(inNormal),
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
					triplanarTexture(texSampler[0],  tf[i], inPos, baseNormal).rgb * dryColor,
					triplanarNormal (texSampler[16], tf[i] * 1.1, inPos, baseNormal, inTB3),
					triplanarNoColor(texSampler[2],  tf[i], inPos, baseNormal).rgb,
					triplanarNoColor(texSampler[3],  tf[i], inPos, baseNormal).r * 255 );

				rockPar[i] = getFragColor(
					triplanarTexture(texSampler[5],  tf[i], inPos, baseNormal).rgb,
					triplanarNormal (texSampler[6],  tf[i], inPos, baseNormal, inTB3),
					triplanarNoColor(texSampler[7],  tf[i], inPos, baseNormal).rgb,
					triplanarNoColor(texSampler[8],  tf[i], inPos, baseNormal).r * 255 );

				snow1Par[i] = getFragColor(
					triplanarTexture(texSampler[15], tf[i], inPos, baseNormal).rgb,
					triplanarNormal (texSampler[16], tf[i], inPos, baseNormal, inTB3),
					triplanarNoColor(texSampler[17], tf[i], inPos, baseNormal).rgb,
					triplanarNoColor(texSampler[18], tf[i], inPos, baseNormal).r * 255 );
					
				snow2Par[i] = getFragColor(
					triplanarTexture(texSampler[10], tf[i], inPos, baseNormal).rgb,
					triplanarNormal (texSampler[11], tf[i], inPos, baseNormal, inTB3),
					triplanarNoColor(texSampler[12], tf[i], inPos, baseNormal).rgb,
					triplanarNoColor(texSampler[13], tf[i], inPos, baseNormal).r * 255 );
					
				sandPar[i] = getFragColor(
					triplanarTexture(texSampler[25], tf[i], inPos, baseNormal).rgb,
					triplanarNormal (texSampler[26], tf[i], inPos, baseNormal, inTB3),
					triplanarNoColor(texSampler[27], tf[i], inPos, baseNormal).rgb,
					triplanarNoColor(texSampler[28], tf[i], inPos, baseNormal).r * 255 );	
					
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
							triplanarTexture(texSampler[0], tf[0], inPos, baseNormal).rgb * dryColor,
							triplanarNormal (texSampler[1], tf[0], inPos, baseNormal, inTB3),
							triplanarNoColor(texSampler[2], tf[0], inPos, baseNormal).rgb,
							triplanarNoColor(texSampler[3], tf[0], inPos, baseNormal).r * 255 ),
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
		float grassHeight = triplanarNoColor(texSampler[4], tf[0], inPos, baseNormal).r;
		float rockHeight  = triplanarNoColor(texSampler[9], tf[0], inPos, baseNormal).r;
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
	float ratioSnow = getSnowRatio_Height(0.1, 90, 120, inSlope);
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